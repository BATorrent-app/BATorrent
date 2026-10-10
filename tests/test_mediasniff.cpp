// SPDX-License-Identifier: MIT
// Content check for files named like video: container sniffing, the ASF
// licence lure, and the quarantine a real session applies on its own.

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QUuid>

#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>

#include "services/security/mediaguard.h"
#include "services/security/mediasniff.h"
#include "torrent/sessionmanager.h"

using namespace MediaSniff;

namespace {

QByteArray padded(QByteArray b, int size = 4096)
{
    if (b.size() < size) b.append(QByteArray(size - b.size(), '\0'));
    return b;
}

void putLe32(QByteArray &b, int off, quint32 v)
{
    for (int i = 0; i < 4; ++i) b[off + i] = char((v >> (8 * i)) & 0xFF);
}

void appendLe64(QByteArray &b, quint64 v)
{
    for (int i = 0; i < 8; ++i) b.append(char((v >> (8 * i)) & 0xFF));
}

QByteArray portableExecutable()
{
    QByteArray b = padded("MZ", 0x200);
    putLe32(b, 0x3C, 0x80);
    b.replace(0x80, 4, QByteArray("PE\0\0", 4));
    return b;
}

const QByteArray kAsfHeaderGuid = QByteArray::fromHex("3026b2758e66cf11a6d900aa0062ce6c");
const QByteArray kFilePropsGuid = QByteArray::fromHex("a1dcab8c47a9cf118ee400c00c205365");
const QByteArray kEncryptGuid   = QByteArray::fromHex("fbb3112223bdd211b4b700a0c955fc6e");
const QByteArray kScriptGuid    = QByteArray::fromHex("301afb1e620bd011a39b00a0c90348f6");

// An ASF header object wrapping the given children; sizes are self-consistent
// unless a test overrides one to be hostile.
QByteArray asf(const QList<QByteArray> &childGuids, quint64 childSize = 40)
{
    QByteArray children;
    for (const QByteArray &g : childGuids) {
        children += g;
        appendLe64(children, childSize);
        children += QByteArray(int(qBound<quint64>(24, childSize, 4096)) - 24, '\0');
    }
    QByteArray b = kAsfHeaderGuid;
    appendLe64(b, quint64(30 + children.size()));
    b += QByteArray("\x02\x00\x00\x00\x01\x02", 6);
    b += children;
    return padded(b);
}

int   s_argc = 1;
char  s_arg0[] = "test_mediasniff";
char *s_argv[] = { s_arg0, nullptr };

QCoreApplication &app()
{
    static QCoreApplication *a = [] {
        // No wipe here: ctest -j runs each case in its own process, and one
        // clearing settings or resume data would pull the rug from a sibling.
        // Every torrent below is unique per run instead (see makeTorrent).
        QStandardPaths::setTestModeEnabled(true);
        return new QCoreApplication(s_argc, s_argv);
    }();
    return *a;
}

bool pumpUntil(const std::function<bool()> &done, int timeoutMs)
{
    QElapsedTimer t;
    t.start();
    while (!done() && t.elapsed() < timeoutMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    return done();
}

// A private v1 torrent of one folder holding `fileName` with `body`, already
// complete on disk under the .!bt name addTorrent expects. The tail carries a
// nonce so the info-hash never collides with a torrent left by an earlier run.
QString makeTorrent(const QTemporaryDir &work, const QString &folder,
                    const QString &fileName, QByteArray body)
{
    body.replace(body.size() - 16, 16, QUuid::createUuid().toRfc4122());
    const QString content = work.filePath(folder);
    REQUIRE(QDir().mkpath(content));
    {
        QFile f(content + '/' + fileName);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(body);
    }
    lt::file_storage fs;
    lt::add_files(fs, content.toStdString());
    lt::create_torrent ct(fs, 16384, lt::create_torrent::v1_only);
    ct.add_tracker("udp://tracker.test:6969/announce", 0);
    ct.set_priv(true);
    lt::set_piece_hashes(ct, work.path().toStdString());
    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    const QString torrentPath = work.filePath(folder + QStringLiteral(".torrent"));
    QFile f(torrentPath);
    REQUIRE(f.open(QIODevice::WriteOnly));
    f.write(buf.data(), static_cast<qsizetype>(buf.size()));
    f.close();
    REQUIRE(QFile::rename(content + '/' + fileName, content + '/' + fileName + ".!bt"));
    return torrentPath;
}

} // namespace

// -----------------------------------------------------------------------------
//  UNIT: what the bytes are
// -----------------------------------------------------------------------------

TEST_CASE("identify recognises the containers torrents actually carry", "[unit][mediasniff]")
{
    CHECK(identify(padded("\x1A\x45\xDF\xA3")) == Kind::Matroska);
    CHECK(identify(padded(QByteArray("\x00\x00\x00\x20" "ftypisom", 12))) == Kind::Mp4);
    CHECK(identify(padded(QByteArray("\x00\x00\x00\x08" "mdat", 8))) == Kind::Mp4);
    CHECK(identify(padded(QByteArray("RIFF\x10\x00\x00\x00" "AVI LIST", 16))) == Kind::Avi);
    CHECK(identify(padded("FLV\x01\x05")) == Kind::Flv);
    CHECK(identify(padded(QByteArray("\x00\x00\x01\xBA", 4))) == Kind::MpegPs);
    CHECK(identify(asf({ kFilePropsGuid })) == Kind::Asf);

    QByteArray ts = padded({}, 188 * 4);
    ts[0] = ts[188] = ts[376] = '\x47';
    CHECK(identify(ts) == Kind::MpegTs);
    QByteArray m2ts = padded({}, 192 * 4);
    m2ts[4] = m2ts[196] = m2ts[388] = '\x47';
    CHECK(identify(m2ts) == Kind::MpegTs);
}

TEST_CASE("identify names what a fake video really is", "[unit][mediasniff]")
{
    CHECK(identify(portableExecutable()) == Kind::Executable);
    CHECK(identify(padded("\x7F" "ELF\x02")) == Kind::Executable);
    CHECK(identify(padded("\xCF\xFA\xED\xFE")) == Kind::Executable);
    CHECK(identify(padded(QByteArray("\x4C\x00\x00\x00\x01\x14\x02\x00", 8))) == Kind::Executable);
    CHECK(identify(padded("#!/bin/sh\n")) == Kind::Executable);
    CHECK(identify(padded("PK\x03\x04")) == Kind::Archive);
    CHECK(identify(padded("Rar!\x1A\x07\x01")) == Kind::Archive);
    CHECK(identify(padded("7z\xBC\xAF\x27\x1C")) == Kind::Archive);
    CHECK(identify(padded("%PDF-1.7")) == Kind::Document);
    CHECK(identify(padded("\xEF\xBB\xBF  \n<!DOCTYPE HTML><html>")) == Kind::WebPage);
    CHECK(identify(padded("<ASX version=\"3.0\">")) == Kind::WebPage);
}

TEST_CASE("identify does not accuse on weak evidence", "[unit][mediasniff]")
{
    SECTION("MZ without a PE header behind it is not a program") {
        CHECK(identify(padded("MZ")) == Kind::Unknown);
        QByteArray b = padded("MZ", 0x200);
        putLe32(b, 0x3C, 0x7FFFFFF0);   // points far past the buffer
        CHECK(identify(b) == Kind::Unknown);
    }
    SECTION("one sync byte is not a transport stream") {
        QByteArray b = padded({}, 188 * 4);
        b[0] = '\x47';
        CHECK(identify(b) == Kind::Unknown);
    }
    SECTION("an empty or tiny head is unknown") {
        CHECK(identify({}) == Kind::Unknown);
        CHECK(identify("M") == Kind::Unknown);
    }
}

// -----------------------------------------------------------------------------
//  UNIT: the verdict
// -----------------------------------------------------------------------------

TEST_CASE("judge waits for the head, then decides", "[unit][mediasniff]")
{
    const QByteArray exe = portableExecutable();

    SECTION("a big file needs the full head first") {
        CHECK(judge(exe, 700LL * 1024 * 1024) == Verdict::NeedMore);
        CHECK(judge(padded(exe, int(kHeadBytes)), 700LL * 1024 * 1024) == Verdict::Disguised);
    }
    SECTION("a file smaller than the head is judged whole") {
        CHECK(judge(exe, exe.size()) == Verdict::Disguised);
    }
    SECTION("an empty or unknown-size file is never judged") {
        CHECK(judge({}, 0) == Verdict::NeedMore);
        CHECK(judge(exe, -1) == Verdict::NeedMore);
    }
    SECTION("every non-media kind is disguised") {
        for (const char *lead : { "PK\x03\x04", "%PDF-1.4", "<html>" })
            CHECK(judge(padded(lead), 4096) == Verdict::Disguised);
    }
    SECTION("real and unrecognised video both pass") {
        CHECK(judge(padded("\x1A\x45\xDF\xA3"), 4096) == Verdict::Ok);
        CHECK(judge(padded("not a known container"), 4096) == Verdict::Ok);
    }
}

TEST_CASE("an ASF that sends the viewer somewhere is a lure", "[unit][mediasniff]")
{
    CHECK(judge(asf({ kFilePropsGuid }), 4096) == Verdict::Ok);
    CHECK(judge(asf({ kFilePropsGuid, kEncryptGuid }), 4096) == Verdict::Lure);
    CHECK(judge(asf({ kScriptGuid }), 4096) == Verdict::Lure);

    SECTION("a lure GUID outside the header object does not count") {
        QByteArray b = asf({ kFilePropsGuid });
        b.replace(2000, 16, kEncryptGuid);
        CHECK_FALSE(asfHasLure(b));
    }
    SECTION("hostile object sizes end the walk instead of looping or overreading") {
        CHECK_FALSE(asfHasLure(asf({ kFilePropsGuid }, 0)));
        CHECK_FALSE(asfHasLure(asf({ kFilePropsGuid }, quint64(-1))));
        QByteArray truncated = asf({ kFilePropsGuid, kEncryptGuid }).left(40);
        CHECK_FALSE(asfHasLure(truncated));
    }
}

TEST_CASE("isVideoName sees through the in-progress suffix", "[unit][mediasniff]")
{
    CHECK(isVideoName("Movie.2026.1080p.mkv"));
    CHECK(isVideoName("Movie.MP4.!bt"));
    CHECK_FALSE(isVideoName("Movie.mkv.exe"));
    CHECK_FALSE(isVideoName("Movie.mkv.quarantine"));
    CHECK_FALSE(isVideoName("subs.srt"));
}

// -----------------------------------------------------------------------------
//  INTEGRATION: a real session quarantines a fake without being asked
// -----------------------------------------------------------------------------

TEST_CASE("a fake movie is quarantined on finish, a real one is left alone",
          "[integration][slow][mediasniff][session]")
{
    app();
    QTemporaryDir work;
    REQUIRE(work.isValid());

    SessionManager session;
    MediaGuard guard(&session);
    QSignalSpy held(&guard, &MediaGuard::quarantined);

    session.addTorrent(makeTorrent(work, "bat_fake", "Movie.2026.1080p.mkv",
                                   padded(portableExecutable(), 96 * 1024)), work.path());
    const QString fakeHash = session.torrentHashAt(session.torrentCount() - 1);
    session.addTorrent(makeTorrent(work, "bat_real", "Movie.2026.720p.mkv",
                                   padded("\x1A\x45\xDF\xA3", 96 * 1024)), work.path());
    const QString realHash = session.torrentHashAt(session.torrentCount() - 1);
    REQUIRE(fakeHash != realHash);

    auto seeding = [&](const QString &hash) {
        const int i = session.torrentIndexByInfoHash(hash);
        return i >= 0 && session.torrentAt(i).seeding;
    };
    REQUIRE(pumpUntil([&] { return seeding(fakeHash) && seeding(realHash); }, 20000));

    // Seeded data never "finishes this session", so stand in for the alert.
    emit session.torrentFinished(QStringLiteral("bat_fake"), fakeHash);
    emit session.torrentFinished(QStringLiteral("bat_real"), realHash);

    REQUIRE(held.count() == 1);
    const QList<QVariant> args = held.takeFirst();
    CHECK(args.at(0).toString() == fakeHash);
    CHECK(args.at(2).toString() == QStringLiteral("Movie.2026.1080p.mkv"));
    CHECK(args.at(3).toString() == QStringLiteral("program"));

    const QString moved = work.filePath("bat_fake/Movie.2026.1080p.mkv.quarantine");
    REQUIRE(pumpUntil([&] { return QFile::exists(moved); }, 10000));
    CHECK_FALSE(QFile::exists(work.filePath("bat_fake/Movie.2026.1080p.mkv")));
    CHECK(pumpUntil([&] {
        return session.torrentAt(session.torrentIndexByInfoHash(fakeHash)).paused;
    }, 5000));
    CHECK(guard.quarantinedFileOf(fakeHash) == 0);
    CHECK(guard.admit(fakeHash, 0) == MediaGuard::Gate::Block);

    CHECK(guard.admit(realHash, 0) == MediaGuard::Gate::Allow);
    CHECK(guard.quarantinedFileOf(realHash) == -1);
    CHECK(QFile::exists(work.filePath("bat_real/Movie.2026.720p.mkv")));
    CHECK(held.isEmpty());

    SECTION("the quarantine survives a restart and explains itself again") {
        MediaGuard later(&session);
        CHECK(later.isQuarantined(fakeHash, 0));
        QSignalSpy reminded(&later, &MediaGuard::quarantined);
        later.remind(fakeHash, 0);
        REQUIRE(reminded.count() == 1);
        CHECK(reminded.first().at(2).toString() == QStringLiteral("Movie.2026.1080p.mkv"));
        CHECK(reminded.first().at(3).toString() == QStringLiteral("program"));
    }

    session.removeTorrent(session.torrentIndexByInfoHash(realHash), false);
    session.removeTorrent(session.torrentIndexByInfoHash(fakeHash), false);
}

// Found driving the real app: after a hard kill the quarantine rename was gone,
// the fast-resume was rejected and libtorrent went looking for the original
// .mkv, so resuming the torrent would have fetched the fake back under its
// video name. A rename has to reach the .resume file without a clean quit.
TEST_CASE("a rename reaches the resume file before any clean shutdown",
          "[regression][slow][mediasniff][session]")
{
    app();
    QTemporaryDir work;
    REQUIRE(work.isValid());

    SessionManager session;
    session.addTorrent(makeTorrent(work, "bat_rename", "Clip.mkv",
                                   padded("\x1A\x45\xDF\xA3", 64 * 1024)), work.path());
    const QString hash = session.torrentHashAt(session.torrentCount() - 1);
    auto row = [&] { return session.torrentIndexByInfoHash(hash); };
    REQUIRE(pumpUntil([&] { return row() >= 0 && session.torrentAt(row()).seeding; }, 20000));

    const QString resume = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                               .filePath(QStringLiteral("resume/") + hash + QStringLiteral(".resume"));
    session.renameFile(row(), 0, QStringLiteral("bat_rename/Clip.mkv.quarantine"));
    REQUIRE(pumpUntil([&] { return QFile::exists(work.filePath("bat_rename/Clip.mkv.quarantine")); }, 10000));

    auto resumeHasNewName = [&] {
        QFile f(resume);
        return f.open(QIODevice::ReadOnly) && f.readAll().contains("Clip.mkv.quarantine");
    };
    CHECK(pumpUntil(resumeHasNewName, 10000));

    session.removeTorrent(row(), false);
}
