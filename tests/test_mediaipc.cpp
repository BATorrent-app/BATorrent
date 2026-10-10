// SPDX-License-Identifier: MIT
// Isolated decoder: frames through a shared-memory slot, a real MediaHost
// driving an IsolatedMediaPlayer, and a hostile host that must not hurt the UI.

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMediaPlayer>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QUuid>
#include <QVideoFrameFormat>
#include <QVideoSink>
#include <cstring>
#include <limits>

#include "bridges/isolatedmediaplayer.h"
#include "ipc/ipcprotocol.h"
#include "ipc/mediaframe.h"
#include "ipc/mediahost.h"

using namespace media;

namespace {

int   s_argc = 1;
char  s_arg0[] = "test_mediaipc";
char *s_argv[] = { s_arg0, nullptr };

QCoreApplication &app()
{
    static QCoreApplication *a = [] {
        auto *inst = new QCoreApplication(s_argc, s_argv);
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

QString uniqueName() { return QStringLiteral("bat-test-media-") + QUuid::createUuid().toString(QUuid::Id128).left(12); }

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
        QByteArray y(w * h, char(16 + (n * 4) % 200));
        f.write(y);
        f.write(QByteArray(w * h / 2, char(128)));
    }
    return f.fileName();
}

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
    h.pixelFormat = int(QVideoFrameFormat::Format_Jpeg);
    CHECK(refused(h));
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
//  INTEGRATION: a real host decoding a real file
// -----------------------------------------------------------------------------

TEST_CASE("an isolated player shows frames decoded by its host",
          "[integration][slow][media]")
{
    app();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString clip = writeY4m(dir);
    const QString name = uniqueName();

    auto host = std::make_unique<MediaHost>(name);
    REQUIRE(host->listen());
    IsolatedMediaPlayer player;
    QVideoSink sink;
    int frames = 0;
    QObject::connect(&sink, &QVideoSink::videoFrameChanged, [&](const QVideoFrame &f) {
        if (f.isValid()) ++frames;
    });
    player.setVideoOutput(&sink);
    player.attachTo(name);
    player.setSource(QUrl::fromLocalFile(clip));
    player.play();

    pumpUntil([&] { return frames >= 5 || player.error() != QMediaPlayer::NoError; }, 8000);
    if (frames == 0 && player.error() != QMediaPlayer::NoError)
        SKIP("no Qt multimedia backend that reads y4m here: " << player.errorString().toStdString());
    REQUIRE(frames >= 5);
    CHECK(sink.videoFrame().size() == QSize(64, 48));
    CHECK(player.hasVideo());
    CHECK(player.duration() > 0);
    CHECK(player.playbackState() == QMediaPlayer::PlayingState);

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
        CHECK(player.playbackState() == QMediaPlayer::StoppedState);
    }
}

// -----------------------------------------------------------------------------
//  SECURITY: a host that has been taken over
// -----------------------------------------------------------------------------

TEST_CASE("a hostile host cannot push the UI out of bounds", "[integration][media][security]")
{
    app();
    const QString name = uniqueName();
    QLocalServer server;
    REQUIRE(server.listen(name));
    IsolatedMediaPlayer player;
    QVideoSink sink;
    player.setVideoOutput(&sink);
    player.attachTo(name);
    REQUIRE(pumpUntil([&] { return server.hasPendingConnections(); }, 3000));
    QLocalSocket *evil = server.nextPendingConnection();
    ipc::writeFrame(evil, ipc::Kind::Hello, {});

    State s;
    s.playbackState = 77; s.mediaStatus = -5; s.error = 1234;
    s.duration = -1; s.position = -9; s.playbackRate = 1e9;
    s.errorString = QString(100000, QLatin1Char('x'));
    sendEvent(evil, QStringLiteral("state"), encode(s));

    Tracks t;
    for (int i = 0; i < 500; ++i) t.audio.append({ 99999, QString(10000, QLatin1Char('t')) });
    t.activeAudio = 400; t.activeSubtitle = 3;
    sendEvent(evil, QStringLiteral("tracks"), encode(t));

    FrameHeader hdr;
    hdr.slot = 99; hdr.pixelFormat = int(QVideoFrameFormat::Format_NV12); hdr.width = 64; hdr.height = 48;
    sendEvent(evil, QStringLiteral("frame"), encode(qint32(1), hdr));                  // no ring at all
    sendEvent(evil, QStringLiteral("ring"), encode(qint32(1), qint64(1) << 40));       // absurd size
    sendEvent(evil, QStringLiteral("ring"), encode(qint32(2), qint64(4096)));          // no such segment
    sendEvent(evil, QStringLiteral("sub"), encode(QString(1000000, QLatin1Char('s'))));
    sendEvent(evil, QStringLiteral("nonsense"), QByteArray(64, '\xff'));
    ipc::writeFrame(evil, ipc::Kind::Event, QByteArray("\xff\xff\xff\xff garbage", 13));
    evil->flush();

    REQUIRE(pumpUntil([&] { return player.audioTracks().size() > 0; }, 3000));
    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    CHECK(player.playbackState() >= 0);
    CHECK(player.playbackState() <= int(QMediaPlayer::PausedState));
    CHECK(player.mediaStatus() == int(QMediaPlayer::NoMedia));
    CHECK(player.error() <= int(QMediaPlayer::AccessDeniedError));
    CHECK(player.errorString().size() <= 4096);
    CHECK(player.duration() == 0);
    CHECK(player.position() == 0);
    CHECK(player.playbackRate() == 1.0);
    CHECK(player.audioTracks().size() == 64);
    CHECK(player.activeAudioTrack() == -1);
    CHECK(player.activeSubtitleTrack() == -1);
    CHECK(sink.subtitleText().size() <= 4096);
    CHECK_FALSE(sink.videoFrame().isValid());
}
