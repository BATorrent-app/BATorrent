// SPDX-License-Identifier: MIT
// Deleted-data detection: candidate paths, real-filesystem probe, and the
// reported regression (a torrent whose files are gone still read as Seeding).

#include <catch2/catch_test_macros.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>

#include "torrent/contentprobe.h"
#include "torrent/sessionmanager.h"
#include "torrent/types.h"

using namespace bat;

namespace {
// Stands in for the disk so the candidate logic can be tested without one.
ExistsFn only(const QStringList &present)
{
    return [present](const QString &p) { return present.contains(p); };
}

int   s_argc = 1;
char  s_arg0[] = "test_contentprobe";
char *s_argv[] = { s_arg0, nullptr };

QCoreApplication &app()
{
    static QCoreApplication *a = [] {
        // The probe is throttled to 10s and needs two agreeing rounds, so at
        // production timing each assertion below would cost ~25s of wall clock.
        // Same code path, 400x faster.
        qputenv("BAT_MISSING_PROBE_MS", "500");
        QStandardPaths::setTestModeEnabled(true);
        auto *inst = new QCoreApplication(s_argc, s_argv);
        QSettings("BATorrent", "BATorrent").clear();
        // Resume data outlives QSettings. A torrent left behind by an earlier
        // run resurrects pointing at a deleted QTemporaryDir, and then the add
        // below is rejected as a duplicate.
        QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                 .filePath(QStringLiteral("resume"))).removeRecursively();
        return inst;
    }();
    return *a;
}

// Spin the event loop until `done`, or give up. Real time has to pass here:
// the probe is throttled and answers from a worker thread.
bool pumpUntil(const std::function<bool()> &done, int timeoutMs)
{
    QElapsedTimer t;
    t.start();
    while (!done() && t.elapsed() < timeoutMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    return done();
}
} // namespace

// -----------------------------------------------------------------------------
//  UNIT: candidate construction
// -----------------------------------------------------------------------------

TEST_CASE("contentRootCandidates prefers metadata, falls back to the name",
          "[unit][contentprobe]")
{
    SECTION("single file: the file itself is the root") {
        const auto c = contentRootCandidates("/data", "Movie.2026.mkv", "Movie.2026", false);
        CHECK(c.first() == QStringLiteral("/data/Movie.2026.mkv"));
        CHECK(c.contains(QStringLiteral("/data/Movie.2026")));
    }

    SECTION("multi file: strips to the common folder, not to file 0") {
        // Probing file 0 of a scene release would stat an arbitrary .r01 part.
        const auto c = contentRootCandidates("/data", "Show.S01/Show.S01E01.mkv",
                                             "Show.S01", true);
        CHECK(c.first() == QStringLiteral("/data/Show.S01"));
        CHECK_FALSE(c.contains(QStringLiteral("/data/Show.S01/Show.S01E01.mkv")));
    }

    SECTION("multi file with no common folder yields only the name") {
        const auto c = contentRootCandidates("/data", "loose.bin", "Loose Pack", true);
        CHECK(c == QStringList{QStringLiteral("/data/Loose Pack")});
    }

    SECTION("windows separators from libtorrent still find the boundary") {
        const auto c = contentRootCandidates("C:/data", "Show.S01\\ep.mkv", "Show.S01", true);
        CHECK(c.first() == QStringLiteral("C:/data/Show.S01"));
    }

    SECTION("a trailing slash on save_path does not double up") {
        const auto c = contentRootCandidates("/data/", "a.mkv", "a", false);
        CHECK(c.first() == QStringLiteral("/data/a.mkv"));
    }

    SECTION("identical name and file path are not probed twice") {
        const auto c = contentRootCandidates("/data", "a.mkv", "a.mkv", false);
        CHECK(c.size() == 1);
    }

    SECTION("nothing to go on") {
        CHECK(contentRootCandidates("", "a.mkv", "a", false).isEmpty());
        CHECK(contentRootCandidates("/data", "", "", false).isEmpty());
    }
}

TEST_CASE("pathVariants covers the in-progress suffix", "[unit][contentprobe]")
{
    // A download in flight is on disk as <name>.<ext>.!bt: without this every
    // active torrent would probe as missing.
    const auto v = pathVariants("/data/Movie.mkv");
    CHECK(v.contains(QStringLiteral("/data/Movie.mkv")));
    CHECK(v.contains(QStringLiteral("/data/Movie.mkv.!bt")));
    CHECK(pathVariants("").isEmpty());
}

// -----------------------------------------------------------------------------
//  UNIT: presence, with the disk faked out
// -----------------------------------------------------------------------------

TEST_CASE("contentPresent never invents a deletion", "[unit][contentprobe]")
{
    SECTION("no candidates means we cannot tell, which is not missing") {
        // The whole failure mode to avoid: flagging a healthy torrent as gone
        // because metadata gave us nothing to look for.
        CHECK(contentPresent({}, only({})));
    }

    SECTION("no predicate is also not an answer") {
        CHECK(contentPresent({QStringLiteral("/data/x")}, ExistsFn{}));
    }

    SECTION("the second candidate still counts") {
        const QStringList c{QStringLiteral("/data/renamed"), QStringLiteral("/data/original")};
        CHECK(contentPresent(c, only({QStringLiteral("/data/original")})));
    }

    SECTION("found only under the in-progress suffix") {
        CHECK(contentPresent({QStringLiteral("/data/a.mkv")},
                             only({QStringLiteral("/data/a.mkv.!bt")})));
    }

    SECTION("nothing anywhere is missing") {
        CHECK_FALSE(contentPresent({QStringLiteral("/data/a"), QStringLiteral("/data/b")},
                                   only({QStringLiteral("/data/c")})));
    }
}

// -----------------------------------------------------------------------------
//  INTEGRATION: against a real filesystem
// -----------------------------------------------------------------------------

TEST_CASE("contentPresent tracks a real deletion on disk",
          "[integration][contentprobe]")
{
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString save = tmp.path();

    auto write = [](const QString &path) {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write("x");
        f.close();
    };

    SECTION("single file, deleted under a live torrent") {
        const QString file = save + "/Movie.2026.mkv";
        write(file);
        const auto c = contentRootCandidates(save, "Movie.2026.mkv", "Movie.2026", false);
        CHECK(contentPresent(c, defaultExists));

        REQUIRE(QFile::remove(file));
        CHECK_FALSE(contentPresent(c, defaultExists));
    }

    SECTION("multi-file folder, removed whole") {
        REQUIRE(QDir(save).mkpath("Show.S01"));
        write(save + "/Show.S01/ep1.mkv");
        const auto c = contentRootCandidates(save, "Show.S01/ep1.mkv", "Show.S01", true);
        CHECK(contentPresent(c, defaultExists));

        REQUIRE(QDir(save + "/Show.S01").removeRecursively());
        CHECK_FALSE(contentPresent(c, defaultExists));
    }

    SECTION("a download still in flight is present, not missing") {
        write(save + "/Big.mkv.!bt");
        const auto c = contentRootCandidates(save, "Big.mkv", "Big", false);
        CHECK(contentPresent(c, defaultExists));
    }
}

// -----------------------------------------------------------------------------
//  REGRESSION: the 4.8.0 report
// -----------------------------------------------------------------------------

// Reported on 4.8.0: files deleted off disk, yet the torrent kept showing
// "Seeding" and, after a restart, silently started downloading again. The state
// was never wrong on purpose: nothing ever set filesMissing, because
// libtorrent only raises ENOENT once it reads the file, which a seeding torrent
// nobody requests never does. checkMissingFiles() now stats for it.
TEST_CASE("a seeding torrent whose data is gone reports missing, not seeding",
          "[regression][contentprobe][state]")
{
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());

    const QString file = tmp.path() + "/Movie.2026.mkv";
    QFile f(file);
    REQUIRE(f.open(QIODevice::WriteOnly));
    f.write("x");
    f.close();

    const auto candidates = contentRootCandidates(tmp.path(), "Movie.2026.mkv",
                                                  "Movie.2026", false);

    // Everything libtorrent reports about a healthy completed torrent.
    TorrentInfo info{};
    info.progress = 1.0f;
    info.totalDone = 4148528658LL;
    info.finished = true;
    info.seeding = true;

    info.filesMissing = !contentPresent(candidates, defaultExists);
    REQUIRE_FALSE(info.filesMissing);
    CHECK(torrentStateKey(info) == QStringLiteral("seeding"));

    // The user deletes it in Finder/Explorer. libtorrent's flags do not change.
    REQUIRE(QFile::remove(file));

    info.filesMissing = !contentPresent(candidates, defaultExists);
    REQUIRE(info.filesMissing);
    CHECK(torrentStateKey(info) == QStringLiteral("missing"));

    SECTION("and it outranks a completed flag too, not just seeding") {
        // "Completed" is checked before seeding in torrentStateKey, so the
        // regression has to hold for a torrent the user marked done as well.
        info.completed = true;
        CHECK(torrentStateKey(info) == QStringLiteral("missing"));
    }
}

// The whole point of the probe is that it runs without anyone asking, so the
// pure logic passing is not evidence the user ever sees the state change. This
// drives a real SessionManager over a real torrent and waits for the tick.
TEST_CASE("SessionManager flips a live torrent to missing on its own",
          "[integration][slow][contentprobe][session]")
{
    app();
    QTemporaryDir work;
    REQUIRE(work.isValid());

    // Private, v1-only, and a folder: the shape the other session fixtures use.
    // v2 pad files make an already-complete torrent fail its initial check.
    const QString content = work.filePath(QStringLiteral("bat_missing"));
    REQUIRE(QDir().mkpath(content));
    {
        QFile f(content + "/movie.bin");
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(64 * 1024, 'x'));
    }

    lt::file_storage fs;
    lt::add_files(fs, content.toStdString());
    lt::create_torrent ct(fs, 16384, lt::create_torrent::v1_only);
    ct.add_tracker("udp://tracker.test:6969/announce", 0);
    ct.set_priv(true);                                  // never touches the network
    lt::set_piece_hashes(ct, work.path().toStdString());

    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    const QString torrentPath = work.filePath(QStringLiteral("bat_missing.torrent"));
    {
        QFile f(torrentPath);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(buf.data(), static_cast<qsizetype>(buf.size()));
    }

    // addTorrent renames every file to <name>.!bt, so complete data has to be
    // sitting under that name for the initial check to find it: which is
    // exactly the shape a real in-flight download has on disk.
    REQUIRE(QFile::rename(content + "/movie.bin", content + "/movie.bin.!bt"));

    SessionManager session;
    session.addTorrent(torrentPath, work.path());
    const int idx = session.torrentCount() - 1;
    REQUIRE(idx >= 0);

    // The data is there and complete, so the probe must leave it alone.
    REQUIRE(pumpUntil([&] { return session.torrentAt(idx).seeding; }, 20000));
    CHECK_FALSE(session.torrentAt(idx).filesMissing);
    CHECK(torrentStateKey(session.torrentAt(idx)) == QStringLiteral("seeding"));

    // The user deletes it outside the app. Nothing reads the file, so libtorrent
    // never raises ENOENT: before the probe this stayed "seeding" forever and
    // silently re-downloaded on the next launch.
    REQUIRE(QDir(content).removeRecursively());

    // Two probes have to agree before the state changes.
    const bool flipped = pumpUntil(
        [&] { return session.torrentAt(idx).filesMissing; }, 15000);
    INFO("state was: " << torrentStateKey(session.torrentAt(idx)).toStdString());
    REQUIRE(flipped);
    CHECK(torrentStateKey(session.torrentAt(idx)) == QStringLiteral("missing"));

    session.removeTorrent(idx, false);
}

// A single bad probe must never pause anything: an external disk spinning up or
// a network share blinking would otherwise stop a healthy torrent. The state is
// only allowed to change once two consecutive probes agree.
TEST_CASE("a torrent that comes back between probes is never paused",
          "[integration][slow][contentprobe][session]")
{
    app();
    QTemporaryDir work;
    REQUIRE(work.isValid());

    const QString content = work.filePath(QStringLiteral("bat_blink"));
    REQUIRE(QDir().mkpath(content));
    const QString payload = content + "/movie.bin";
    {
        QFile f(payload);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(64 * 1024, 'x'));
    }

    lt::file_storage fs;
    lt::add_files(fs, content.toStdString());
    lt::create_torrent ct(fs, 16384, lt::create_torrent::v1_only);
    ct.add_tracker("udp://tracker.test:6969/announce", 0);
    ct.set_priv(true);
    lt::set_piece_hashes(ct, work.path().toStdString());

    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    const QString torrentPath = work.filePath(QStringLiteral("bat_blink.torrent"));
    {
        QFile f(torrentPath);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(buf.data(), static_cast<qsizetype>(buf.size()));
    }
    REQUIRE(QFile::rename(payload, payload + ".!bt"));

    // This case needs an outage that fits inside ONE probe interval, so it runs
    // at its own slower cadence. The value is read when SessionManager is built.
    qputenv("BAT_MISSING_PROBE_MS", "1500");
    SessionManager session;
    qputenv("BAT_MISSING_PROBE_MS", "500");         // restore for the other cases
    REQUIRE(session.pauseOnMissingData());          // the behaviour under test is on
    session.addTorrent(torrentPath, work.path());
    const int idx = session.torrentCount() - 1;
    REQUIRE(idx >= 0);
    REQUIRE(pumpUntil([&] { return session.torrentAt(idx).seeding; }, 20000));

    // Gone for less than one probe interval, then back: the blink.
    const QString hidden = work.filePath(QStringLiteral("hidden"));
    REQUIRE(QDir().rename(content, hidden));
    pumpUntil([&] { return false; }, 400);          // well under one 1500ms cycle
    REQUIRE(QDir().rename(hidden, content));

    // Ride out several full probe cycles: nothing may have changed.
    pumpUntil([&] { return false; }, 6000);
    const TorrentInfo after = session.torrentAt(idx);
    CHECK_FALSE(after.filesMissing);
    CHECK_FALSE(after.paused);
    CHECK(torrentStateKey(after) == QStringLiteral("seeding"));

    session.removeTorrent(idx, false);
}

// The other half of the 4.8.0 report: after a restart the torrent silently
// started downloading its deleted data all over again. Confirmed-missing data
// now pauses the torrent instead.
TEST_CASE("confirmed-missing data pauses the torrent",
          "[regression][slow][contentprobe][session]")
{
    app();
    QTemporaryDir work;
    REQUIRE(work.isValid());

    const QString content = work.filePath(QStringLiteral("bat_autopause"));
    REQUIRE(QDir().mkpath(content));
    {
        QFile f(content + "/movie.bin");
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(64 * 1024, 'x'));
    }

    lt::file_storage fs;
    lt::add_files(fs, content.toStdString());
    lt::create_torrent ct(fs, 16384, lt::create_torrent::v1_only);
    ct.add_tracker("udp://tracker.test:6969/announce", 0);
    ct.set_priv(true);
    lt::set_piece_hashes(ct, work.path().toStdString());

    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    const QString torrentPath = work.filePath(QStringLiteral("bat_autopause.torrent"));
    {
        QFile f(torrentPath);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(buf.data(), static_cast<qsizetype>(buf.size()));
    }
    REQUIRE(QFile::rename(content + "/movie.bin", content + "/movie.bin.!bt"));

    SessionManager session;
    session.addTorrent(torrentPath, work.path());
    const int idx = session.torrentCount() - 1;
    REQUIRE(idx >= 0);
    REQUIRE(pumpUntil([&] { return session.torrentAt(idx).seeding; }, 20000));
    REQUIRE_FALSE(session.torrentAt(idx).paused);

    REQUIRE(QDir(content).removeRecursively());

    REQUIRE(pumpUntil([&] { return session.torrentAt(idx).paused; }, 15000));
    CHECK(session.torrentAt(idx).filesMissing);

    // A manual resume must stick: the probe keeps seeing the data as gone, and
    // must not re-pause what the user deliberately started again.
    session.resumeTorrent(idx);
    REQUIRE(pumpUntil([&] { return !session.torrentAt(idx).paused; }, 5000));
    pumpUntil([&] { return false; }, 3000);   // several more probe cycles
    CHECK_FALSE(session.torrentAt(idx).paused);

    // Stop it before teardown. Resumed, it announces to the fixture's dead UDP
    // tracker, and libtorrent's shutdown then blocks on that retry for minutes.
    session.pauseTorrent(idx);
    session.removeTorrent(idx, false);
}
