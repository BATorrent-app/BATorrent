// SPDX-License-Identifier: MIT
// Isolated decoder: frames through a shared-memory slot, bytes fed to the child
// over the socket, a real MediaHost driving an IsolatedMediaPlayer, and a
// hostile child that must not get the UI to misbehave.

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLocalSocket>
#include <QMediaPlayer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <atomic>
#include <cstring>
#include <limits>

#include "bridges/isolatedmediaplayer.h"
#include "httptestserver.h"
#include "ipc/ipcprotocol.h"
#include "ipc/mediafeed.h"
#include "ipc/mediaframe.h"
#include "ipc/mediahost.h"
#include "ipc/remotesource.h"

using namespace media;

namespace {

QCoreApplication &app()
{
    static QCoreApplication *a = [] {
        QCoreApplication *inst = &httptest::ensureApp();
        // Homebrew Qt ships no ffmpeg backend; the dev build keeps one here.
        const QString dev = QStringLiteral(BAT_SOURCE_DIR "/dev-qt-plugins");
        if (QDir(dev).exists()) QCoreApplication::addLibraryPath(dev);
        return inst;
    }();
    return *a;
}

bool pumpUntil(const std::function<bool()> &done, int timeoutMs)
{
    QElapsedTimer t;
    t.start();
    while (!done() && t.elapsed() < timeoutMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    return done();
}

QVideoFrame patternFrame(QVideoFrameFormat::PixelFormat pf, int w, int h)
{
    QVideoFrame f(QVideoFrameFormat(QSize(w, h), pf));
    REQUIRE(f.map(QVideoFrame::WriteOnly));
    for (int p = 0; p < f.planeCount(); ++p)
        for (int i = 0; i < f.mappedBytes(p); ++i) f.bits(p)[i] = uchar((i * 7 + p * 31) & 0xFF);
    f.unmap();
    return f;
}

// A 64x48, 25 fps, 2 s raw video: decodable by ffmpeg without any codec, and
// small enough to write by hand.
QString writeY4m(const QTemporaryDir &dir)
{
    const int w = 64, h = 48, frames = 50;
    QFile f(dir.filePath(QStringLiteral("clip.y4m")));
    REQUIRE(f.open(QIODevice::WriteOnly));
    f.write("YUV4MPEG2 W64 H48 F25:1 Ip A1:1 C420jpeg\n");
    for (int n = 0; n < frames; ++n) {
        f.write("FRAME\n");
        f.write(QByteArray(w * h, char(16 + (n * 4) % 200)));
        f.write(QByteArray(w * h / 2, char(128)));
    }
    return f.fileName();
}

// What the UI sent a fake child: method names with their args, in order.
struct Inbox {
    QByteArray buf;
    QList<QPair<QString, QByteArray>> requests;
    bool hello = false;
    void drain(QLocalSocket *s)
    {
        buf.append(s->readAll());
        ipc::drainFrames(buf, [this](ipc::Kind k, const QByteArray &p) {
            if (k != ipc::Kind::Request) return;
            QDataStream in(p);
            in.setVersion(ipc::kStreamVersion);
            quint32 id; QString method; QByteArray args;
            in >> id >> method >> args;
            requests.append({ method, args });
        });
    }
    int count(const QString &m) const
    {
        int n = 0;
        for (const auto &r : requests) n += r.first == m;
        return n;
    }
};

void sendEvent(QLocalSocket *s, const QString &name, const QByteArray &args)
{
    ipc::writeFrame(s, ipc::Kind::Event, encode(name, args));
    s->flush();
}

} // namespace

// -----------------------------------------------------------------------------
//  UNIT: the slot
// -----------------------------------------------------------------------------

TEST_CASE("a frame survives the trip through a slot", "[unit][media]")
{
    for (auto pf : { QVideoFrameFormat::Format_NV12, QVideoFrameFormat::Format_YUV420P,
                     QVideoFrameFormat::Format_BGRA8888 }) {
        QVideoFrame src = patternFrame(pf, 64, 48);
        src.setStartTime(40000);
        REQUIRE(src.map(QVideoFrame::ReadOnly));
        QByteArray slot(int(packedSize(src)), '\0');
        FrameHeader hdr;
        REQUIRE(pack(src, reinterpret_cast<uchar *>(slot.data()), slot.size(), hdr));

        QVideoFrame out = unpack(hdr, reinterpret_cast<const uchar *>(slot.constData()), slot.size());
        REQUIRE(out.isValid());
        CHECK(out.size() == QSize(64, 48));
        CHECK(out.pixelFormat() == pf);
        CHECK(out.startTime() == 40000);
        REQUIRE(out.map(QVideoFrame::ReadOnly));
        for (int p = 0; p < src.planeCount(); ++p) {
            const int rowBytes = std::min(src.bytesPerLine(p), out.bytesPerLine(p));
            const int rows = src.mappedBytes(p) / src.bytesPerLine(p);
            for (int r = 0; r < rows; ++r)
                CHECK(std::memcmp(src.bits(p) + r * src.bytesPerLine(p),
                                  out.bits(p) + r * out.bytesPerLine(p), size_t(rowBytes)) == 0);
        }
        out.unmap();
        src.unmap();
    }
}

TEST_CASE("pack refuses a slot too small for the frame", "[unit][media]")
{
    QVideoFrame src = patternFrame(QVideoFrameFormat::Format_NV12, 64, 48);
    REQUIRE(src.map(QVideoFrame::ReadOnly));
    QByteArray slot(int(packedSize(src)) - 1, '\0');
    FrameHeader hdr;
    CHECK_FALSE(pack(src, reinterpret_cast<uchar *>(slot.data()), slot.size(), hdr));
    src.unmap();
}

TEST_CASE("unpack treats the child's header as hostile", "[unit][media][security]")
{
    QVideoFrame src = patternFrame(QVideoFrameFormat::Format_NV12, 64, 48);
    REQUIRE(src.map(QVideoFrame::ReadOnly));
    QByteArray slot(int(packedSize(src)), '\0');
    FrameHeader good;
    REQUIRE(pack(src, reinterpret_cast<uchar *>(slot.data()), slot.size(), good));
    src.unmap();
    const auto *base = reinterpret_cast<const uchar *>(slot.constData());
    auto refused = [&](const FrameHeader &h) { return !unpack(h, base, slot.size()).isValid(); };

    REQUIRE_FALSE(refused(good));

    FrameHeader h = good;
    h.pixelFormat = int(QVideoFrameFormat::Format_Jpeg);  CHECK(refused(h));
    h = good; h.pixelFormat = 9999;                        CHECK(refused(h));
    h = good; h.width = 0;                                 CHECK(refused(h));
    h = good; h.height = kMaxDimension + 1;                CHECK(refused(h));
    h = good; h.planeCount = 0;                            CHECK(refused(h));
    h = good; h.planeCount = kMaxPlanes + 1;               CHECK(refused(h));
    h = good; h.planeCount = 1;                            CHECK(refused(h));   // NV12 has two
    h = good; h.planes[1].offset = slot.size();            CHECK(refused(h));
    h = good; h.planes[1].offset = -64;                    CHECK(refused(h));
    h = good; h.planes[0].bytes = std::numeric_limits<qint64>::max(); CHECK(refused(h));
    h = good; h.planes[0].stride = 0;                      CHECK(refused(h));
    h = good; h.planes[0].bytes = good.planes[0].bytes / 2; CHECK(refused(h));   // rows don't fit
    h = good; h.width = 4096; h.height = 4096;             CHECK(refused(h));    // bigger than its planes
    CHECK_FALSE(unpack(good, nullptr, slot.size()).isValid());
    CHECK_FALSE(unpack(good, base, 0).isValid());
}

// -----------------------------------------------------------------------------
//  UNIT: RemoteSource, the child's only way to read a video
// -----------------------------------------------------------------------------

namespace {
// Answers a RemoteSource's requests from `file`, on the main thread, the way
// the socket would. Readers run on a worker thread like ffmpeg's demuxer.
struct Responder {
    RemoteSource *src;
    QByteArray file;
    std::atomic<int> asked{0};
    void hook()
    {
        QObject::connect(src, &RemoteSource::readRequested, src, [this](quint32 id, qint64 off, qint32 len) {
            ++asked;
            src->deliver(id, off, file.mid(int(off), len));
        }, Qt::QueuedConnection);
    }
};

QByteArray readOnWorker(RemoteSource *src, qint64 at, qint64 n)
{
    QByteArray out;
    std::atomic<bool> done{false};
    QThread *t = QThread::create([&] {
        src->seek(at);
        out = src->read(n);
        done = true;
    });
    t->start();
    pumpUntil([&] { return done.load(); }, 5000);
    t->wait();
    delete t;
    return out;
}
}

TEST_CASE("RemoteSource reads the UI's bytes from any thread", "[unit][media]")
{
    app();
    const QByteArray file = httptest::makePayload(3 * 1024 * 1024 + 123);
    RemoteSource src(file.size());
    Responder r{ &src, file };
    r.hook();

    CHECK(readOnWorker(&src, 0, 4096) == file.left(4096));
    CHECK(readOnWorker(&src, 2 * 1024 * 1024 + 5, 1000) == file.mid(2 * 1024 * 1024 + 5, 1000));
    const int before = r.asked;
    CHECK(readOnWorker(&src, 100, 50) == file.mid(100, 50));   // served from cache
    CHECK(r.asked == before);
    CHECK(readOnWorker(&src, file.size() - 10, 100) == file.right(10));
    CHECK(readOnWorker(&src, file.size(), 100).isEmpty());     // EOF
}

TEST_CASE("RemoteSource ignores answers it never asked for", "[unit][media][security]")
{
    app();
    RemoteSource src(1000, 300);
    src.deliver(1, 0, QByteArray(10, 'x'));        // no such request
    QByteArray got = "untouched";
    QThread *t = QThread::create([&] { got = src.read(10); });
    QSignalSpy asked(&src, &RemoteSource::readRequested);
    t->start();
    REQUIRE(pumpUntil([&] { return asked.count() == 1; }, 2000));
    const quint32 id = asked.first().at(0).toUInt();
    src.deliver(id, 7, QByteArray(10, 'x'));       // wrong offset
    src.deliver(id, 0, QByteArray(5000, 'x'));     // longer than asked and than the file
    t->wait();
    delete t;
    CHECK(got.isEmpty());                          // timed out: nothing bogus was taken
}

TEST_CASE("aborting a RemoteSource frees a blocked reader at once", "[unit][media]")
{
    app();
    RemoteSource src(1000, 60000);
    std::atomic<bool> done{false};
    QThread *t = QThread::create([&] { src.read(10); done = true; });
    t->start();
    QThread::msleep(100);
    REQUIRE_FALSE(done.load());
    src.abort();
    REQUIRE(t->wait(2000));
    delete t;
    CHECK(done.load());
}

// -----------------------------------------------------------------------------
//  UNIT: MediaFeed, the UI answering the child
// -----------------------------------------------------------------------------

TEST_CASE("MediaFeed serves a file and refuses what is out of bounds", "[unit][media][security]")
{
    app();
    QTemporaryDir dir;
    const QByteArray body = httptest::makePayload(200000);
    QFile f(dir.filePath("v.mkv"));
    REQUIRE(f.open(QIODevice::WriteOnly));
    f.write(body);
    f.close();

    MediaFeed feed(QUrl::fromLocalFile(f.fileName()));
    QSignalSpy ready(&feed, &MediaFeed::ready), data(&feed, &MediaFeed::data), refused(&feed, &MediaFeed::refused);
    feed.start();
    REQUIRE(ready.count() == 1);
    CHECK(ready.first().at(0).toLongLong() == body.size());

    feed.request(1, 1000, 500);
    REQUIRE(data.count() == 1);
    CHECK(data.first().at(2).toByteArray() == body.mid(1000, 500));
    feed.request(2, body.size() - 10, 500);
    CHECK(data.last().at(2).toByteArray() == body.right(10));     // clipped to the file

    feed.request(3, -1, 10);
    feed.request(4, body.size(), 10);
    feed.request(5, 0, 0);
    feed.request(6, 0, MediaFeed::kMaxRequest + 1);
    CHECK(refused.count() == 4);
}

TEST_CASE("MediaFeed only talks to the local stream server", "[unit][media][security]")
{
    app();
    for (const char *u : { "http://example.com/stream/x/0", "https://127.0.0.1/stream/x/0", "ftp://127.0.0.1/x" }) {
        MediaFeed feed{ QUrl(QString::fromLatin1(u)) };
        QSignalSpy failed(&feed, &MediaFeed::failed);
        feed.start();
        CHECK(failed.count() == 1);
    }
}

TEST_CASE("MediaFeed streams ranges over HTTP and follows a seek", "[integration][media]")
{
    app();
    const QByteArray body = httptest::makePayload(6 * 1024 * 1024);
    httptest::RangeServer server(body);
    REQUIRE(server.listen(QHostAddress::LocalHost));
    MediaFeed feed(server.url());
    QSignalSpy ready(&feed, &MediaFeed::ready), data(&feed, &MediaFeed::data);
    feed.start();
    REQUIRE(pumpUntil([&] { return ready.count() == 1; }, 5000));
    CHECK(ready.first().at(0).toLongLong() == body.size());

    feed.request(1, 0, 256 * 1024);
    REQUIRE(pumpUntil([&] { return data.count() == 1; }, 5000));
    const QByteArray first = data.first().at(2).toByteArray();
    CHECK(first == body.left(first.size()));

    feed.request(2, 5 * 1024 * 1024, 100 * 1024);   // far ahead: a new range request
    REQUIRE(pumpUntil([&] { return data.count() == 2; }, 5000));
    CHECK(data.last().at(1).toLongLong() == 5 * 1024 * 1024);
    const QByteArray far = data.last().at(2).toByteArray();
    CHECK(far == body.mid(5 * 1024 * 1024, far.size()));
}

TEST_CASE("MediaFeed keeps a sequential reader fed past its read-ahead window", "[unit][media]")
{
    app();
    const QByteArray body = httptest::makePayload(40 * 1024 * 1024);
    httptest::RangeServer server(body);
    REQUIRE(server.listen(QHostAddress::LocalHost));
    MediaFeed feed(server.url());
    QSignalSpy ready(&feed, &MediaFeed::ready);
    feed.start();
    REQUIRE(pumpUntil([&] { return ready.count() == 1; }, 5000));

    // The way a demuxer reads: each request starts where the last answer ended.
    QByteArray got;
    QObject::connect(&feed, &MediaFeed::data, [&](quint32, qint64 offset, const QByteArray &bytes) {
        CHECK(offset == got.size());
        got += bytes;
    });
    quint32 id = 1;
    while (got.size() < body.size()) {
        const int before = got.size();
        feed.request(id++, got.size(), 1024 * 1024);
        if (!pumpUntil([&] { return got.size() > before; }, 5000)) break;
    }
    CHECK(got.size() == body.size());
    CHECK(got == body);
}

TEST_CASE("MediaFeed refuses bytes from a server that ignored the range", "[unit][media][security]")
{
    app();
    const QByteArray body = httptest::makePayload(1024 * 1024);
    httptest::RangeServer server(body);
    server.ignoreRanges = true;
    REQUIRE(server.listen(QHostAddress::LocalHost));
    MediaFeed feed(server.url());
    QSignalSpy ready(&feed, &MediaFeed::ready), data(&feed, &MediaFeed::data), refused(&feed, &MediaFeed::refused);
    feed.start();
    REQUIRE(pumpUntil([&] { return ready.count() == 1; }, 5000));
    feed.request(1, 500000, 1000);
    REQUIRE(pumpUntil([&] { return refused.count() == 1; }, 5000));
    CHECK(data.isEmpty());   // byte 0 offered as byte 500000 would be a lie

    feed.request(2, 0, 1000);   // from the start a plain 200 is the truth
    REQUIRE(pumpUntil([&] { return data.count() == 1; }, 5000));
    CHECK(data.first().at(2).toByteArray() == body.left(1000));
}

// -----------------------------------------------------------------------------
//  INTEGRATION: a real host decoding bytes it never reads itself
// -----------------------------------------------------------------------------

TEST_CASE("an isolated player shows frames decoded from bytes it fed",
          "[integration][slow][media]")
{
    app();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString clip = writeY4m(dir);

    IsolatedMediaPlayer player;
    QVideoSink sink;
    int frames = 0;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, [&](const QVideoFrame &f) {
        if (f.isValid()) ++frames;
    });
    player.setVideoOutput(&sink);
    const QString name = player.listenWithoutChild();
    REQUIRE_FALSE(name.isEmpty());
    auto host = std::make_unique<MediaHost>(name);
    REQUIRE(host->start());
    player.setSource(QUrl::fromLocalFile(clip));
    player.play();

    pumpUntil([&] { return frames >= 5 || player.error() != QMediaPlayer::NoError; }, 8000);
    if (frames == 0 && player.error() != QMediaPlayer::NoError)
        SKIP("no Qt multimedia backend that reads y4m here: " << player.errorString().toStdString());
    REQUIRE(frames >= 5);
    CHECK(sink.videoFrame().size() == QSize(64, 48));
    CHECK(player.hasVideo());
    CHECK(player.duration() > 0);

    SECTION("pause and seek round-trip") {
        player.pause();
        REQUIRE(pumpUntil([&] { return player.playbackState() == QMediaPlayer::PausedState; }, 3000));
        player.setPosition(1000);
        CHECK(pumpUntil([&] { return std::abs(player.position() - 1000) < 200; }, 3000));
    }

    SECTION("a decoder that dies leaves an error, not a crash") {
        QSignalSpy crashed(&player, &IsolatedMediaPlayer::decoderCrashed);
        host.reset();
        REQUIRE(pumpUntil([&] { return crashed.count() == 1; }, 3000));
        CHECK(player.error() == QMediaPlayer::ResourceError);
        CHECK(player.mediaStatus() == QMediaPlayer::InvalidMedia);
    }
}

// Found driving a stream: its size is learned asynchronously, so "play" (and
// a resume seek) left before "open" and the child dropped them on the floor.
TEST_CASE("play and seek asked before a stream opens still reach the child", "[integration][media]")
{
    app();
    httptest::RangeServer server(httptest::makePayload(1024 * 1024));
    REQUIRE(server.listen(QHostAddress::LocalHost));
    IsolatedMediaPlayer player;
    const QString name = player.listenWithoutChild();
    QLocalSocket child;
    child.connectToServer(name);
    REQUIRE(child.waitForConnected(3000));
    ipc::writeFrame(&child, ipc::Kind::Hello, {});
    child.flush();
    Inbox inbox;
    QObject::connect(&child, &QLocalSocket::readyRead, [&] { inbox.drain(&child); });

    player.setSource(server.url());
    player.setPosition(5000);
    player.play();
    REQUIRE(pumpUntil([&] { return inbox.count("play") == 1; }, 5000));

    QStringList order;
    for (const auto &r : inbox.requests)
        if (r.first == "open" || r.first == "seek" || r.first == "play") order << r.first;
    CHECK(order == QStringList{ "open", "seek", "play" });
}

// -----------------------------------------------------------------------------
//  SECURITY: a child that has been taken over
// -----------------------------------------------------------------------------

TEST_CASE("a hostile child cannot push the UI out of bounds", "[integration][media][security]")
{
    app();
    QTemporaryDir dir;
    const QByteArray body = httptest::makePayload(100000);
    QFile f(dir.filePath("v.mkv"));
    REQUIRE(f.open(QIODevice::WriteOnly));
    f.write(body);
    f.close();

    IsolatedMediaPlayer player;
    QVideoSink sink;
    player.setVideoOutput(&sink);
    const QString name = player.listenWithoutChild();
    QLocalSocket evil;
    evil.connectToServer(name);
    REQUIRE(evil.waitForConnected(3000));
    ipc::writeFrame(&evil, ipc::Kind::Hello, {});
    evil.flush();
    Inbox inbox;
    QObject::connect(&evil, &QLocalSocket::readyRead, [&] { inbox.drain(&evil); });

    player.setSource(QUrl::fromLocalFile(f.fileName()));
    REQUIRE(pumpUntil([&] { return inbox.count("open") == 1; }, 3000));

    // Reads the child has no business making.
    sendEvent(&evil, "read", encode(qint32(1), quint32(1), qint64(-5), qint32(100)));
    sendEvent(&evil, "read", encode(qint32(1), quint32(2), qint64(body.size()), qint32(100)));
    sendEvent(&evil, "read", encode(qint32(1), quint32(3), qint64(0), qint32(100 * 1024 * 1024)));
    sendEvent(&evil, "read", encode(qint32(99), quint32(4), qint64(0), qint32(100)));   // stale source
    sendEvent(&evil, "read", encode(qint32(1), quint32(5), qint64(10), qint32(20)));    // fine
    // A ring big enough to pin the machine's memory, and a nonsense one.
    sendEvent(&evil, "needRing", encode(qint64(1) << 40));
    sendEvent(&evil, "needRing", encode(qint64(-1)));

    State s;
    s.playbackState = 77; s.mediaStatus = -5; s.error = 1234;
    s.duration = -1; s.position = -9; s.playbackRate = 1e9;
    s.errorString = QString(100000, QLatin1Char('x'));
    sendEvent(&evil, "state", encode(s));
    Tracks t;
    for (int i = 0; i < 500; ++i) t.audio.append({ 99999, QString(10000, QLatin1Char('t')) });
    t.activeAudio = 400;
    sendEvent(&evil, "tracks", encode(t));
    FrameHeader hdr;
    hdr.slot = 99; hdr.pixelFormat = int(QVideoFrameFormat::Format_NV12); hdr.width = 64; hdr.height = 48;
    sendEvent(&evil, "frame", encode(qint32(1), hdr));                                  // no ring at all
    sendEvent(&evil, "sub", encode(QString(1000000, QLatin1Char('s'))));
    sendEvent(&evil, "nonsense", QByteArray(64, '\xff'));
    ipc::writeFrame(&evil, ipc::Kind::Event, QByteArray("\xff\xff\xff\xff garbage", 13));
    evil.flush();

    REQUIRE(pumpUntil([&] { return inbox.count("data") + inbox.count("dataError") >= 4; }, 3000));
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    CHECK(inbox.count("data") == 1);                  // only the honest read
    CHECK(inbox.count("dataError") == 3);
    CHECK(inbox.count("ring") == 0);
    for (const auto &r : inbox.requests) {
        if (r.first != "data") continue;
        QDataStream in(r.second);
        in.setVersion(ipc::kStreamVersion);
        qint32 gen; quint32 id; qint64 offset; QByteArray bytes;
        in >> gen >> id >> offset >> bytes;
        CHECK(id == 5);
        CHECK(bytes == body.mid(10, 20));
    }
    CHECK(player.playbackState() <= int(QMediaPlayer::PausedState));
    CHECK(player.mediaStatus() == int(QMediaPlayer::NoMedia));
    CHECK(player.errorString().size() <= 4096);
    CHECK(player.duration() == 0);
    CHECK(player.playbackRate() == 1.0);
    CHECK(player.audioTracks().size() == 64);
    CHECK(player.activeAudioTrack() == -1);
    CHECK(sink.subtitleText().size() <= 4096);
    CHECK_FALSE(sink.videoFrame().isValid());
}
