// SPDX-License-Identifier: MIT
// BATorrent C++<->QML bridge tests (Catch2 v3)
//
// Covers the qmlposterbridge layer that the QML UI depends on but no other test
// exercised. Runs headless via the "offscreen" platform so the GUI-adjacent
// bridges (clipboard, painter, style hints) construct without a display.

#include <catch2/catch_test_macros.hpp>

#include <QApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QVariantMap>
#include <QMetaType>
#include <QSignalSpy>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>

#include <libtorrent/file_storage.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>

#include "torrent/sessionmanager.h"
#include "bridges/qmlpostermodel.h"
#include "services/metadata/metadataresolver.h"
#include "services/games/playlog.h"
#include "services/discovery/gamesourcemanager.h"
#include "services/platform/translator.h"
#include "torrent/types.h"
#include "bridges/qmlposterbridge.h"
#include "bridges/search/qmlsearchbridge_util.h"
#include "nointernet.h"
#include "httptestserver.h"
#include "bridges/qmlupdaterbridge.h"
#include "services/downloads/httpdownloadmanager.h"
#include "services/downloads/httpdownload.h"
#include "services/downloads/httpmergeengine.h"
#include <QClipboard>
#include <QGuiApplication>

namespace lt = libtorrent;

// Pump the Qt event loop until pred() holds or the timeout elapses; SessionManager
// adds/renames torrents asynchronously (libtorrent alerts), so reads need to settle.
template <typename Pred>
static bool pumpUntil(Pred pred, int timeoutMs = 8000)
{
    QElapsedTimer t; t.start();
    while (!pred() && t.elapsed() < timeoutMs)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 30);
    return pred();
}

// Build a real, private (offline: no DHT/LSD/PEX) multi-file .torrent under `dir`
// and return its path. Mirrors the create flow in qmlposterbridge.cpp. `tag`
// varies the file content (and thus the info hash) so a test can create several
// distinct fixture torrents without them colliding as duplicates.
static QString makeFixtureTorrent(const QString &dir, const QString &tag = QString())
{
    const QString name = tag.isEmpty() ? QStringLiteral("bat_fixture") : QStringLiteral("bat_fixture_") + tag;
    const QString content = dir + "/" + name;
    QDir().mkpath(content + "/sub");
    auto writeN = [](const QString &p, int n, char c) {
        QFile f(p); f.open(QIODevice::WriteOnly); f.write(QByteArray(n, c)); f.close();
    };
    const char filler = tag.isEmpty() ? 'x' : tag.at(0).toLatin1();
    writeN(content + "/a.txt", 100, filler);
    writeN(content + "/sub/b.txt", 200, filler);

    lt::file_storage fs;
    lt::add_files(fs, content.toStdString());
    lt::create_torrent ct(fs, 16384, lt::create_torrent::v1_only);   // classic v1 → no v2 pad files
    ct.add_tracker("udp://tracker.test:6969/announce", 0);
    ct.set_priv(true);                                  // private → fully offline
    lt::set_piece_hashes(ct, dir.toStdString());        // parent of `name`

    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    const QString out = dir + "/" + name + ".torrent";
    QFile f(out); f.open(QIODevice::WriteOnly);
    f.write(buf.data(), static_cast<qsizetype>(buf.size()));
    f.close();
    return out;
}

// Private multi-file torrent with a video leaf (larger) + a sample text file.
// Used to pin streamUrl / playFile URL shape and priority side-effects.
static QString makeVideoFixtureTorrent(const QString &dir, const QString &tag = QString())
{
    const QString name = tag.isEmpty() ? QStringLiteral("bat_video")
                                       : QStringLiteral("bat_video_") + tag;
    const QString content = dir + "/" + name;
    QDir().mkpath(content);
    auto writeN = [](const QString &p, int n, char c) {
        QFile f(p); f.open(QIODevice::WriteOnly); f.write(QByteArray(n, c)); f.close();
    };
    const char filler = tag.isEmpty() ? 'v' : tag.at(0).toLatin1();
    writeN(content + "/sample.txt", 50, filler);
    writeN(content + "/Show.S01E01.mkv", 400, filler);

    lt::file_storage fs;
    lt::add_files(fs, content.toStdString());
    lt::create_torrent ct(fs, 16384, lt::create_torrent::v1_only);
    ct.add_tracker("udp://tracker.test:6969/announce", 0);
    ct.set_priv(true);
    lt::set_piece_hashes(ct, dir.toStdString());

    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    const QString out = dir + "/" + name + ".torrent";
    QFile f(out); f.open(QIODevice::WriteOnly);
    f.write(buf.data(), static_cast<qsizetype>(buf.size()));
    f.close();
    return out;
}

// ============================================================================
//  Headless Qt app singleton (offscreen) + isolated settings store
// ============================================================================
static int   s_argc = 1;
static char  s_arg0[] = "test_bridge";
static char *s_argv[] = { s_arg0, nullptr };

static QApplication &app()
{
    static QApplication *a = [] {
        qputenv("QT_QPA_PLATFORM", "offscreen");
        // Sandbox all paths + settings: empty session, isolated QSettings, never
        // touches (or migrates from) the user's real BATorrent data.
        QStandardPaths::setTestModeEnabled(true);
        // A dedicated org that shares NOTHING with the real app or other test
        // binaries: so wiping our own data dir can never touch real user data or
        // another suite's fixtures.
        QCoreApplication::setOrganizationName("BATorrentBridgeTest");
        QCoreApplication::setApplicationName("BATorrentBridgeTest");
        // Clear only OUR own leaf dirs so every run starts with an empty session
        // and isolated settings (never the parent: that's another suite's space).
        for (auto loc : { QStandardPaths::AppDataLocation, QStandardPaths::AppConfigLocation }) {
            const QString p = QStandardPaths::writableLocation(loc);
            if (!p.isEmpty()) QDir(p).removeRecursively();
        }
        // SessionManager persists to its own explicit org ("BATorrent"), not the
        // test org above: wipe it too so persisted prefs don't leak across runs.
        QSettings("BATorrent", "BATorrent").clear();
        auto *inst = new QApplication(s_argc, s_argv);
        // "all" searches also query the torrent providers over the network.
        nointernet::install();
        return inst;
    }();
    return *a;
}

// ============================================================================
//  QmlPairingBridge: the pairing QR (pure: qrcodegen, no session/GUI)
// ============================================================================
TEST_CASE("Pairing: qrRowsForUrl encodes a square binary matrix", "[bridge][pairing]")
{
    app();
    QmlPairingBridge p;

    const QStringList rows = p.qrRowsForUrl(QStringLiteral("http://192.168.0.10:8080/?pair=YWRtaW46cHc"));
    REQUIRE_FALSE(rows.isEmpty());

    const int n = rows.size();
    for (const QString &r : rows) {
        REQUIRE(r.size() == n);                       // square
        for (const QChar c : r)
            REQUIRE((c == QLatin1Char('0') || c == QLatin1Char('1')));   // binary only
    }
}

TEST_CASE("Pairing: encoding is deterministic", "[bridge][pairing]")
{
    app();
    QmlPairingBridge p;
    const QString url = QStringLiteral("http://10.0.0.5:8080/?pair=dXNlcjpzZWNyZXQ");
    REQUIRE(p.qrRowsForUrl(url) == p.qrRowsForUrl(url));
}

TEST_CASE("Pairing: empty URL yields no QR rows", "[bridge][pairing]")
{
    app();
    QmlPairingBridge p;
    REQUIRE(p.qrRowsForUrl(QString()).isEmpty());     // empty/garbage URL → no matrix
}

// ============================================================================
//  QmlThemeBridge: theme name persistence
// ============================================================================
TEST_CASE("Theme: themeName round-trips through settings", "[bridge][theme]")
{
    app();
    QmlThemeBridge t;
    t.setThemeName(QStringLiteral("midnight"));
    REQUIRE(t.themeName() == QStringLiteral("midnight"));

    QmlThemeBridge t2;                                // a fresh bridge reads the persisted value
    REQUIRE(t2.themeName() == QStringLiteral("midnight"));

    t.setThemeName(QStringLiteral("default"));        // restore
}

TEST_CASE("Theme: the older names and the anime switch migrate once", "[bridge][theme]")
{
    app();
    QSettings s;
    s.remove(QStringLiteral("qmlThemesV2"));
    s.remove(QStringLiteral("qmlBackgroundMode"));
    s.remove(QStringLiteral("qmlCustomProfiles"));
    s.setValue(QStringLiteral("qmlThemeName"), QStringLiteral("dark"));
    s.setValue(QStringLiteral("qmlAnime"), true);
    {
        QmlThemeBridge t;
        REQUIRE(t.themeName() == QStringLiteral("default"));   // "dark" was the app as it is
        REQUIRE(t.backgroundMode() == QStringLiteral("theme"));
        REQUIRE(!QSettings().contains(QStringLiteral("qmlAnime")));
    }
    // an existing install with the art off keeps a clean background
    s.remove(QStringLiteral("qmlThemesV2"));
    s.remove(QStringLiteral("qmlBackgroundMode"));
    s.setValue(QStringLiteral("qmlThemeName"), QStringLiteral("midnight"));
    s.setValue(QStringLiteral("qmlAnime"), false);
    {
        QmlThemeBridge t;
        REQUIRE(t.themeName() == QStringLiteral("midnight"));
        REQUIRE(t.backgroundMode() == QStringLiteral("clean"));
    }
    // the old Custom theme becomes a theme of the user's own
    s.remove(QStringLiteral("qmlThemesV2"));
    s.setValue(QStringLiteral("qmlThemeName"), QStringLiteral("custom"));
    s.setValue(QStringLiteral("qmlCustomProfiles"),
               QByteArray(R"([{"name":"Mine","bg":"#101010","opacity":70}])"));
    {
        QmlThemeBridge t;
        REQUIRE(t.themeName() == QStringLiteral("user"));
        REQUIRE(t.customProfiles().size() == 1);
        const QVariantMap p = t.customProfiles().first().toMap();
        REQUIRE(p.value("dim").toInt() == 30);                 // opacity 70 -> dim 30
        REQUIRE(p.value("base").toString() == QStringLiteral("dark"));
        t.removeProfile(0);                                     // deleting it goes back to Default
        REQUIRE(t.themeName() == QStringLiteral("default"));
    }
    s.remove(QStringLiteral("qmlCustomProfiles"));
}

TEST_CASE("Theme: a theme of one's own is light or dark by its base", "[bridge][theme]")
{
    app();
    QSettings().remove(QStringLiteral("qmlCustomProfiles"));
    QmlThemeBridge t;
    const int i = t.addProfile();
    t.selectProfile(i);
    REQUIRE(t.themeName() == QStringLiteral("user"));
    REQUIRE(!t.isLightTheme(QStringLiteral("user")));
    t.setProfileValue(i, QStringLiteral("base"), QStringLiteral("light"));
    REQUIRE(t.isLightTheme(QStringLiteral("user")));
    REQUIRE(t.duplicateProfile(i) == i + 1);
    REQUIRE(t.customProfiles().size() == 2);
    t.removeProfile(i + 1);
    t.removeProfile(i);
    REQUIRE(t.themeName() == QStringLiteral("default"));
}

// ============================================================================
//  QmlSessionBridge: selection state with no torrents
// ============================================================================
TEST_CASE("Session bridge: empty session has no selection", "[bridge][session]")
{
    app();
    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    REQUIRE(bridge.torrentCount() == 0);
    REQUIRE_FALSE(bridge.hasSelection());
    REQUIRE(bridge.selectedName().isEmpty());
    REQUIRE(bridge.selectedFiles().isEmpty());
    REQUIRE(bridge.selectedPeerList().isEmpty());
    REQUIRE(bridge.selectedTrackers().isEmpty());
}

TEST_CASE("Session bridge: a torrent with no page says so, so the file opens instead", "[bridge][session]")
{
    app();
    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);
    REQUIRE(bridge.torrentPage(QStringLiteral("0000000000000000000000000000000000000000")).isEmpty());
    REQUIRE(bridge.torrentPage(QString()).isEmpty());
}

TEST_CASE("Session bridge: playtime kept in settings moves into the play log", "[bridge][session][games]")
{
    app();
    const QString logPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                                .filePath(QStringLiteral("playlog.json"));
    QFile::remove(logPath);
    {
        QSettings s;
        s.setValue(QStringLiteral("gameSeconds/abc"), 5400);
        s.setValue(QStringLiteral("gamePlayed/abc"), 1700000000000LL);
        s.setValue(QStringLiteral("gameLaunches/abc"), 4);
        s.setValue(QStringLiteral("gameLaunches/onlylaunched"), 1);
    }
    // Twice: the second start must not add the old totals on top again.
    for (int run = 0; run < 2; ++run) {
        SessionManager session;
        MetadataResolver resolver;
        QmlSessionBridge bridge(&session, &resolver);
    }
    // Kept for a build that predates the log.
    REQUIRE(QSettings().contains(QStringLiteral("gameSeconds/abc")));

    const PlayLog log(logPath);
    REQUIRE(log.summary(QStringLiteral("abc")).totalSecs == 5400);
    REQUIRE(log.summary(QStringLiteral("abc")).lastPlayedMs == 1700000000000LL);
    REQUIRE(log.summary(QStringLiteral("abc")).launches == 4);
    REQUIRE(log.summary(QStringLiteral("onlylaunched")).launches == 1);
}

TEST_CASE("Session bridge: an installed game stays in the library after its torrent goes", "[bridge][session][games]")
{
    app();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString exe = dir.filePath(QStringLiteral("007FirstLight.exe"));
    { QFile f(exe); REQUIRE(f.open(QIODevice::WriteOnly)); f.write("MZ"); }
    const QString hash = QStringLiteral("5ea0f2bb95a2f71f1f7b433045bcb43d3ef7cd4b");
    {
        QSettings s;
        s.setValue(QStringLiteral("keptGames/") + hash + QStringLiteral("/name"),
                   QStringLiteral("007 First Light [FitGirl Repack]"));
        s.setValue(QStringLiteral("keptGames/") + hash + QStringLiteral("/size"), 38151060554LL);
        s.setValue(QStringLiteral("gameExe/") + hash, exe);
    }
    SessionManager session;   // no torrents: the repack was removed
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    auto find = [&]() {
        for (const QVariant &v : bridge.gameLibrary())
            if (v.toMap().value(QStringLiteral("infoHash")).toString() == hash) return v.toMap();
        return QVariantMap();
    };
    const QVariantMap g = find();
    REQUIRE(g.value(QStringLiteral("torrentless")).toBool());
    REQUIRE(g.value(QStringLiteral("title")).toString() == QLatin1String("007 First Light"));
    REQUIRE(g.value(QStringLiteral("group")).toString() == QLatin1String("FitGirl"));
    REQUIRE(g.value(QStringLiteral("installState")).toInt() == 4);   // Ready: Play
    REQUIRE(bridge.gameTorrentless(hash));
    REQUIRE(QDir(bridge.gameFolder(hash)) == QDir(dir.path()));

    // Uninstalled: it drops out on its own.
    QFile::remove(exe);
    REQUIRE(find().isEmpty());
    { QFile f(exe); REQUIRE(f.open(QIODevice::WriteOnly)); f.write("MZ"); }
    REQUIRE_FALSE(find().isEmpty());

    // Removed from the library by the user.
    bridge.forgetGame(hash);
    REQUIRE(find().isEmpty());
    REQUIRE_FALSE(QSettings().contains(QStringLiteral("gameExe/") + hash));
}

// ============================================================================
//  QmlSearchBridge: "Tudo" aggregates every source into one flat result list
// ============================================================================
TEST_CASE("Search bridge: 'Tudo' merges loaded game catalog synchronously", "[bridge][search]")
{
    app();
    SessionManager session;
    QmlSearchBridge bridge(&session);

    // Pre-load a catalog so the aggregate path resolves without the network.
    const QByteArray cat = R"({"name":"Test","downloads":[
        {"title":"Cyberpunk 2077 [FitGirl Repack]","uris":["magnet:?xt=urn:btih:aaaa"],"fileSize":"58 GB"},
        {"title":"Elden Ring [DODI]","uris":["magnet:?xt=urn:btih:bbbb"],"fileSize":"45 GB"}]})";
    REQUIRE(GameSourceManager::instance().indexCatalog("Test", cat) == 2);

    bridge.search("all", "cyberpunk", 0);

    // Games append synchronously; any torrent providers would arrive later (async),
    // so right after the call only the matching game is present.
    REQUIRE(bridge.mode() == "all");
    const QVariantList results = bridge.results();
    REQUIRE(results.size() == 1);
    CHECK(results[0].toMap().value("name").toString() == "Cyberpunk 2077");
    CHECK(results[0].toMap().value("sub").toString() == "Test");
}

TEST_CASE("Search bridge: addAndWatch watches only what it was asked to", "[bridge][search]")
{
    app();
    // The disk-fit check needs a real save volume; the sandbox does not create one.
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    SessionManager session;
    QmlSearchBridge bridge(&session);
    const QByteArray cat = R"({"name":"W","downloads":[
        {"title":"Watchgame Small","uris":["magnet:?xt=urn:btih:1111111111111111111111111111111111111111"],"fileSize":"1 MB"},
        {"title":"Watchgame Other","uris":["magnet:?xt=urn:btih:2222222222222222222222222222222222222222"],"fileSize":"1 MB"},
        {"title":"Watchgame Huge","uris":["magnet:?xt=urn:btih:3333333333333333333333333333333333333333"],"fileSize":"900000 GB"}]})";
    REQUIRE(GameSourceManager::instance().indexCatalog("W", cat) == 3);
    bridge.search("all", "watchgame", 0);
    const QVariantList rows = bridge.results();
    REQUIRE(rows.size() == 3);
    auto at = [&](const char *name) {
        for (int i = 0; i < rows.size(); ++i)
            if (rows[i].toMap().value("name").toString().endsWith(name)) return i;
        return -1;
    };
    const int small = at("Small"), other = at("Other"), huge = at("Huge");

    QSignalSpy watched(&bridge, &QmlSearchBridge::prepareAndWatch);
    QSignalSpy wontFit(&bridge, &QmlSearchBridge::addWontFit);

    bridge.addAndWatch(small);
    CHECK(watched.count() == 1);
    bridge.activateResult(other);
    CHECK(watched.count() == 1);               // a plain add stays a plain add

    bridge.addAndWatch(huge);
    CHECK(wontFit.count() == 1);
    CHECK(watched.count() == 1);
    bridge.activateResult(huge, true);         // the disk prompt's "add anyway"
    CHECK(watched.count() == 2);

    bridge.addAndWatch(huge);                  // prompt shown, then dismissed...
    bridge.activateResult(other);              // ...and something else added instead
    bridge.activateResult(huge, true);
    CHECK(watched.count() == 2);

    // The adds are real: leave the sandbox session empty for the suites after.
    for (int i = session.torrentCount() - 1; i >= 0; --i)
        session.removeTorrent(i, false, true);
    REQUIRE(pumpUntil([&] { return session.torrentCount() == 0; }));
}

TEST_CASE("Search bridge: openTitle goes straight to one work's releases", "[bridge][search]")
{
    app();
    SessionManager session;
    QmlSearchBridge bridge(&session);

    bridge.openTitle({ { "title", "Dune: Part Two" }, { "type", "movie" }, { "year", "2024" },
                       { "tmdbId", 693134 }, { "backdrop", "b.jpg" } });
    CHECK(bridge.workTitle() == "Dune: Part Two");
    CHECK(bridge.workType() == "movie");
    CHECK(bridge.workBackdrop() == "b.jpg");
    CHECK(bridge.singleTitleView());
    CHECK(bridge.canGoBack());

    // Back has no grid to show: an empty titles stage, and no work left.
    bridge.back();
    CHECK(bridge.mode() == "titles");
    CHECK_FALSE(bridge.singleTitleView());      // the grid is many titles again
    CHECK(bridge.results().isEmpty());
    CHECK(bridge.workTitle().isEmpty());

    bridge.openTitle({ { "type", "movie" } });     // nothing to search for
    CHECK(bridge.workTitle().isEmpty());
}

TEST_CASE("Search bridge: a title row carries everything the hero reads", "[bridge][search]")
{
    const QVariantMap work{
        { "title", "Resident Evil Requiem" }, { "type", "game" }, { "year", "2026" },
        { "poster", "p.jpg" }, { "backdrop", "b.jpg" }, { "tmdbId", 0 },
        { "genres", QStringList{ "Shooter" } }, { "maker", "Capcom" } };
    const QVariantMap row = SearchBridgeUtil::titleRowFromWork(work);
    // The backdrop was once dropped here, so every picked title opened on its
    // poster stretched across the hero.
    CHECK(row.value("backdrop").toString() == "b.jpg");
    CHECK(row.value("genres").toStringList() == QStringList{ "Shooter" });
    CHECK(row.value("maker").toString() == "Capcom");
    CHECK(row.value("name").toString() == "Resident Evil Requiem");
    CHECK(row.value("isTitle").toBool());
}

// ============================================================================
//  QmlSettingsBridge: pairing flag derives from QSettings (post-keychain move)
// ============================================================================
TEST_CASE("Settings bridge: pairingActive reflects settings, not the keychain", "[bridge][settings]")
{
    app();
    QSettings st;
    st.setValue("webUiEnabled", false);              // keep the ctor from starting a server
    SessionManager session;
    QmlSettingsBridge bridge(&session, nullptr);     // no engine: applyWebUi() early-returns

    REQUIRE(bridge.webUiUser() == QStringLiteral("admin"));
    REQUIRE_FALSE(bridge.pairingActive());

    st.setValue("webUiEnabled", true);
    st.setValue("webUiRemoteAccess", true);
    st.setValue("webUiPasswordHash", QStringLiteral("deadbeef"));
    REQUIRE(bridge.pairingActive());

    st.setValue("webUiPasswordHash", QString());     // no credential → not paired
    REQUIRE_FALSE(bridge.pairingActive());

    st.setValue("webUiEnabled", false);
    st.setValue("webUiRemoteAccess", false);
}

// ============================================================================
//  QmlPosterModel / QmlTorrentFilterProxy: empty-model contract
// ============================================================================
TEST_CASE("Poster model: empty model, roles defined", "[bridge][model]")
{
    app();
    SessionManager session;
    MetadataResolver resolver;
    QmlPosterModel model(&session, &resolver);

    REQUIRE(model.rowCount() == 0);
    REQUIRE_FALSE(model.roleNames().isEmpty());

    QmlTorrentFilterProxy proxy;
    proxy.setSourceModel(&model);
    REQUIRE(proxy.rowCount() == 0);
}

// ============================================================================
//  QmlSessionBridge with a REAL loaded torrent: the methods that need content
// ============================================================================
TEST_CASE("Session bridge: a loaded torrent exposes and mutates files/trackers", "[bridge][session][torrent]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString torrentPath = makeFixtureTorrent(tmp.path());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    session.addTorrent(torrentPath, tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == 1; }));

    bridge.setSelectedRows({0});
    REQUIRE(bridge.hasSelection());

    // --- read side (from the torrent metadata, deterministic once loaded) ---
    REQUIRE_FALSE(bridge.selectedName().isEmpty());
    REQUIRE_FALSE(bridge.selectedHash().isEmpty());
    REQUIRE_FALSE(bridge.selectedSize().isEmpty());

    QVariantList files = bridge.selectedFiles();
    REQUIRE(files.size() == 2);

    const int trackers0 = bridge.selectedTrackers().size();
    REQUIRE(trackers0 >= 1);                              // the tracker baked in at create

    // --- per-file priority ---
    bridge.setSelectedFilePriority(0, 0);                 // Skip file 0
    REQUIRE(pumpUntil([&] {
        return bridge.selectedFiles().value(0).toMap().value("priority").toInt() == 0;
    }));

    // --- add / remove tracker ---
    bridge.addTrackerToSelected("udp://added.test:80/announce");
    REQUIRE(pumpUntil([&] { return bridge.selectedTrackers().size() > trackers0; }));
    bridge.removeTrackerFromSelected("udp://added.test:80/announce");
    REQUIRE(pumpUntil([&] { return bridge.selectedTrackers().size() == trackers0; }));

    // --- rename a file (async libtorrent file_renamed alert) ---
    bridge.renameSelectedFile(0, "renamed_fixture.txt");
    REQUIRE(pumpUntil([&] {
        const QVariantList fl = bridge.selectedFiles();
        for (const QVariant &v : fl)
            if (v.toMap().value("path").toString().contains("renamed_fixture"))
                return true;
        return false;
    }));
}

// ============================================================================
//  Session bridge: removing a multi-row selection removes every row, not just
//  one (reported by a user: "deleting several selected items only removes one
//  at a time").
// ============================================================================
TEST_CASE("Session bridge: removeSelected removes every row in a multi-selection", "[bridge][session][remove]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    // other test cases in this binary share the same sandboxed resume-data dir
    // and may leave torrents behind, so compare against a baseline instead of
    // an absolute count.
    const int base = session.torrentCount();

    for (const QString &tag : {QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c")})
        session.addTorrent(makeFixtureTorrent(tmp.path(), tag), tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == base + 3; }));

    QList<int> rows;
    for (int i = base; i < base + 3; ++i) rows << i;
    bridge.setSelectedRows(rows);
    REQUIRE(bridge.hasSelection());
    bridge.removeSelected();

    REQUIRE(pumpUntil([&] { return session.torrentCount() == base; }));
}

// ============================================================================
//  SessionManager: speed/queue/network prefs persist across a "restart"
//  Regression: the QWidget→QML migration left these setters writing only to the
//  live libtorrent session (never QSettings) and the ctor never reloaded them,
//  so every limit reset to 0/default on relaunch ("settings don't save").
// ============================================================================
TEST_CASE("Session: speed/queue/network prefs survive a restart", "[bridge][session][persist]")
{
    app();
    QSettings("BATorrent", "BATorrent").clear();   // self-contained: don't inherit or leak state
    {
        SessionManager s;                       // user changes settings...
        s.setUploadLimit(123);
        s.setDownloadLimit(456);
        s.setMaxActiveDownloads(7);
        s.setSeedRatioLimit(2.5f);
        s.setMaxConnections(321);
        s.setDhtEnabled(false);
        s.setEncryptionMode(2);
        s.setPreallocate(true);
        s.setAutoRecheck(true);
        QSettings("BATorrent", "BATorrent").sync();   // flush before the "restart"
    }
    {
        SessionManager s2;                      // ...fresh instance = app relaunch
        // member-backed getters: deterministic right after the ctor's reload
        REQUIRE(s2.uploadLimit() == 123);
        REQUIRE(s2.downloadLimit() == 456);
        REQUIRE(s2.maxActiveDownloads() == 7);
        REQUIRE(s2.seedRatioLimit() == 2.5f);
        REQUIRE(s2.dhtEnabled() == false);
        REQUIRE(s2.encryptionMode() == 2);
        REQUIRE(s2.preallocate() == true);
        REQUIRE(s2.autoRecheck() == true);
    }
    // The keys that drive live libtorrent state are verified at the storage layer
    // (the getter reads the async session, which needn't have settled yet).
    QSettings st("BATorrent", "BATorrent");
    REQUIRE(st.value("maxConnections").toInt() == 321);
    REQUIRE(st.value("uploadLimit").toInt() == 123);
    st.clear();   // leave the store clean for sibling tests / the next run
}

// ============================================================================
//  QmlSettingsBridge: UI bool toggles coerce to a real bool on read.
//  Regression: on the Windows registry a bool round-trips as an int (DWORD), so
//  QML's `settings.get(key) !== false` saw `0 !== false` → true and the splash /
//  close-to-tray toggles ignored being switched off.
// ============================================================================
TEST_CASE("Settings bridge: UI bool toggles read back as real bool", "[bridge][settings][persist]")
{
    app();
    SessionManager s;
    QmlSettingsBridge sb(&s, nullptr);

    // Mimic the Windows DWORD readback: store the toggle as an int, not a bool.
    QSettings().setValue(QStringLiteral("showSplash"), 0);
    QSettings().sync();

    const QVariant v = sb.get(QStringLiteral("showSplash"));
    REQUIRE(v.typeId() == QMetaType::Bool);          // coerced: not the raw int 0
    REQUIRE(v.toBool() == false);
    REQUIRE_FALSE(v.toBool() != false);              // the exact compare QML relies on

    sb.set(QStringLiteral("showSplash"), true);
    REQUIRE(sb.get(QStringLiteral("showSplash")).toBool() == true);

    // An unset toggle stays invalid so QML's own `on:` default still applies.
    QSettings().remove(QStringLiteral("randomPort"));
    QSettings().sync();
    REQUIRE_FALSE(sb.get(QStringLiteral("randomPort")).isValid());
}

// Regression: with the engine in a child process the bridge has no session and
// returned every key raw. The registry hands a bool back as the string "true",
// so `welcomeShown !== true` held and the setup wizard opened on every start.
TEST_CASE("Settings bridge: split mode coerces UI bools too", "[bridge][settings][persist]")
{
    app();
    QmlSettingsBridge sb(nullptr, nullptr);   // split engine: no in-process session

    QSettings().setValue(QStringLiteral("welcomeShown"), QStringLiteral("true"));
    QSettings().setValue(QStringLiteral("showSplash"), QStringLiteral("false"));
    QSettings().sync();

    const QVariant w = sb.get(QStringLiteral("welcomeShown"));
    REQUIRE(w.typeId() == QMetaType::Bool);
    REQUIRE(w.toBool() == true);
    const QVariant sp = sb.get(QStringLiteral("showSplash"));
    REQUIRE(sp.typeId() == QMetaType::Bool);
    REQUIRE(sp.toBool() == false);

    QSettings().remove(QStringLiteral("welcomeShown"));
    QSettings().remove(QStringLiteral("showSplash"));
    QSettings().sync();
}

// ============================================================================
//  playFile / streamUrl / clearResume: behaviour that broke before
// ============================================================================
static QString stripIncompleteSuffix(QString path)
{
    if (path.endsWith(QStringLiteral(".!bt"))) path.chop(4);
    return path;
}

static int findVideoFileIndex(const std::vector<FileInfo> &files)
{
    for (int i = 0; i < int(files.size()); ++i) {
        const QString mp = stripIncompleteSuffix(files[size_t(i)].path);
        if (mp.endsWith(QStringLiteral(".mkv"), Qt::CaseInsensitive)
            || mp.endsWith(QStringLiteral(".mp4"), Qt::CaseInsensitive))
            return i;
    }
    return -1;
}

TEST_CASE("playFile: no-op when port unset, hash missing, or index OOB",
          "[bridge][playback][playFile]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    const int base = session.torrentCount();
    const QString torrentPath = makeVideoFixtureTorrent(tmp.path(), QStringLiteral("noop"));
    session.addTorrent(torrentPath, tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == base + 1; }));

    const int row = base;
    const QString hash = session.torrentHashAt(row);
    REQUIRE_FALSE(hash.isEmpty());
    REQUIRE(pumpUntil([&] { return int(session.filesAt(row).size()) >= 2; }));
    const int nFiles = int(session.filesAt(row).size());

    QSignalSpy spy(&bridge, &QmlSessionBridge::openPlayer);

    bridge.playFile(hash, 0);                                 // port still 0
    bridge.playFile(QStringLiteral("deadbeef"), 0);           // unknown hash
    bridge.setStreamPort(18765);
    bridge.playFile(hash, -1);
    bridge.playFile(hash, nFiles);
    REQUIRE(spy.count() == 0);
}

TEST_CASE("playFile: emits stream URL, only that file wanted, not sequential",
          "[bridge][playback][playFile]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    const int base = session.torrentCount();
    const QString torrentPath = makeVideoFixtureTorrent(tmp.path(), QStringLiteral("play"));
    session.addTorrent(torrentPath, tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == base + 1; }));

    const int row = base;
    const QString hash = session.torrentHashAt(row);
    REQUIRE_FALSE(hash.isEmpty());
    REQUIRE(pumpUntil([&] { return findVideoFileIndex(session.filesAt(row)) >= 0; }));
    const int videoIdx = findVideoFileIndex(session.filesAt(row));
    REQUIRE(videoIdx >= 0);

    constexpr quint16 port = 18766;
    bridge.setStreamPort(port);
    QSignalSpy spy(&bridge, &QmlSessionBridge::openPlayer);
    bridge.playFile(hash, videoIdx);
    REQUIRE(spy.count() == 1);

    const QString url = spy.at(0).at(0).toString();
    const QString emittedHash = spy.at(0).at(2).toString();
    const int emittedIdx = spy.at(0).at(3).toInt();
    REQUIRE(emittedHash == hash);
    REQUIRE(emittedIdx == videoIdx);
    REQUIRE(url == QStringLiteral("http://127.0.0.1:%1/stream/%2/%3")
                       .arg(port).arg(hash).arg(videoIdx));
    static const QRegularExpression re(
        QStringLiteral("^http://127\\.0\\.0\\.1:\\d+/stream/[0-9a-fA-F]+/\\d+$"));
    REQUIRE(re.match(url).hasMatch());

    // Streaming leaves strict order off: the engine's deadline window and
    // position-based piece priorities feed the player, and outside them
    // rarest-first gets more out of the swarm.
    REQUIRE(pumpUntil([&] {
        if (session.isSequentialDownload(row)) return false;
        const auto files = session.filesAt(row);
        if (videoIdx >= int(files.size())) return false;
        if (files[size_t(videoIdx)].priority != 7) return false;
        for (int i = 0; i < int(files.size()); ++i) {
            if (i == videoIdx) continue;
            if (files[size_t(i)].priority != 0) return false;
        }
        return true;
    }));
}

TEST_CASE("streamUrl: empty without port or video; shape picks largest video",
          "[bridge][playback][streamUrl]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    // Text-only fixture → no video → empty even with a port.
    {
        const int base = session.torrentCount();
        const QString path = makeFixtureTorrent(tmp.path(), QStringLiteral("txt"));
        session.addTorrent(path, tmp.path() + "/dl");
        REQUIRE(pumpUntil([&] { return session.torrentCount() == base + 1; }));
        const int row = base;
        REQUIRE(bridge.streamUrl(row).isEmpty());           // port 0
        bridge.setStreamPort(18767);
        REQUIRE(bridge.streamUrl(row).isEmpty());           // no video leaf
        REQUIRE(bridge.streamUrl(-1).isEmpty());
        REQUIRE(bridge.streamUrl(session.torrentCount()).isEmpty());
    }

    const int base = session.torrentCount();
    const QString vpath = makeVideoFixtureTorrent(tmp.path(), QStringLiteral("url"));
    session.addTorrent(vpath, tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == base + 1; }));
    const int row = base;
    const QString hash = session.torrentHashAt(row);
    REQUIRE(pumpUntil([&] { return findVideoFileIndex(session.filesAt(row)) >= 0; }));
    const int videoIdx = findVideoFileIndex(session.filesAt(row));
    REQUIRE(videoIdx >= 0);

    constexpr quint16 port = 18767;
    bridge.setStreamPort(port);
    const QString url = bridge.streamUrl(row);
    REQUIRE(url == QStringLiteral("http://127.0.0.1:%1/stream/%2/%3")
                       .arg(port).arg(hash).arg(videoIdx));
}

TEST_CASE("clearResume: removes resume_ hash_index and _dur/_at sidecars",
          "[bridge][playback][clearResume]")
{
    app();
    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    const QString hash = QStringLiteral("abcdef0123456789abcdef0123456789abcdef01");
    constexpr int fileIndex = 3;
    const QString rk = QStringLiteral("resume_%1_%2").arg(hash).arg(fileIndex);
    QSettings s;
    s.setValue(rk, 42.5);
    s.setValue(rk + QStringLiteral("_dur"), 1200.0);
    s.setValue(rk + QStringLiteral("_at"), qint64(1700000000));
    s.sync();
    REQUIRE(s.contains(rk));
    REQUIRE(s.contains(rk + QStringLiteral("_dur")));
    REQUIRE(s.contains(rk + QStringLiteral("_at")));

    bridge.clearResume(hash, fileIndex);
    s.sync();
    REQUIRE_FALSE(s.contains(rk));
    REQUIRE_FALSE(s.contains(rk + QStringLiteral("_dur")));
    REQUIRE_FALSE(s.contains(rk + QStringLiteral("_at")));
}

// A magnet carries its info-hash in the URI, so a re-add is knowable before any
// metadata arrives. Without the guard, add_torrent hands back the EXISTING
// handle and it lands in m_torrents twice: two rows reading one handle (same
// size/speed/progress), and removing either strands the other.
TEST_CASE("addMagnet rejects a duplicate info-hash", "[session][add][magnet]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    const int base = session.torrentCount();

    // Same info-hash, different tracker/display-name: the "same content from
    // two sources" case: one torrent, two magnet links.
    const QString hash = QStringLiteral("85360c42e678d8f814c54b448f9e49b5db93db8f");
    const QString first  = QStringLiteral("magnet:?xt=urn:btih:%1&dn=Source+A").arg(hash);
    const QString second = QStringLiteral("magnet:?xt=urn:btih:%1&dn=Source+B"
                                          "&tr=udp%3A%2F%2Ftracker.example%3A80").arg(hash);

    session.addMagnet(first, tmp.path() + "/dl", QString(), 0);
    REQUIRE(session.torrentCount() == base + 1);

    session.addMagnet(second, tmp.path() + "/dl", QString(), 0);
    REQUIRE(session.torrentCount() == base + 1);

    // A genuinely different hash still gets through.
    const QString other = QStringLiteral("magnet:?xt=urn:btih:"
        "1111111111111111111111111111111111111111&dn=Other");
    session.addMagnet(other, tmp.path() + "/dl", QString(), 0);
    REQUIRE(session.torrentCount() == base + 2);

    QDir(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
             .filePath("resume")).removeRecursively();
}

// ============================================================================
//  QmlPosterModel: the grid and the list read the same row
// ============================================================================

// Reported on 4.8.0: the same torrent showed "Seeding" in grid view and
// "Downloading" in classic view. Both views are fed by this one model: the
// grid off StateKeyRole, the list off StateStringRole: and the two roles were
// produced by independent code paths that could disagree.
//
// This is the layer the earlier fix was missing. torrentHasWork() had a unit
// test and it passed while the symptom stayed alive on screen, because nothing
// exercised the roles the UI actually binds to.
TEST_CASE("Poster model: the state key and the state label agree on every row",
          "[bridge][model][regression][state]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    // Tagged: an untagged fixture has the same info-hash as the one the
    // selection test adds, and the second add is rejected as a duplicate.
    const QString torrentPath = makeFixtureTorrent(tmp.path(), QStringLiteral("state"));
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    MetadataResolver resolver;
    QmlPosterModel model(&session, &resolver);

    // Earlier cases leave resume data behind, so this session may already hold
    // torrents. That is useful here: more rows, more states to check.
    const int before = session.torrentCount();
    session.addTorrent(torrentPath, tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() > before; }));
    model.refreshFull();
    REQUIRE(model.rowCount() == session.torrentCount());

    const QString downloading = tr_(QStringLiteral("state_downloading"));
    const QString seeding     = tr_(QStringLiteral("state_seeding"));

    for (int r = 0; r < model.rowCount(); ++r) {
        INFO("row " << r);
        const QModelIndex row = model.index(r, 0);
        const QString key   = model.data(row, QmlPosterModel::StateKeyRole).toString();
        const QString label = model.data(row, QmlPosterModel::StateStringRole).toString();
        REQUIRE_FALSE(key.isEmpty());
        REQUIRE_FALSE(label.isEmpty());

        // The contradiction itself: the grid must not be able to call a row
        // "seeding" while the list prints the downloading label for it.
        if (key == QStringLiteral("seeding"))
            CHECK(label != downloading);
        if (label == seeding)
            CHECK(key == QStringLiteral("seeding"));

        // And the general invariant: when the key carries a label of its own,
        // that is the label on the row. "queued" is excluded only because its
        // label takes the position as an argument.
        const QString labelKey = torrentStateLabelKey(session.torrentAt(r));
        if (!labelKey.isEmpty() && labelKey != QStringLiteral("state_queued")) {
            INFO("state key: " << key.toStdString());
            CHECK(label == tr_(labelKey));
        }
    }

    // Drive one row to a state the classification owns and libtorrent's enum
    // knows nothing about. If the label wiring is removed from torrentAt(),
    // stateString is left empty and the REQUIRE above fails on every row.
    const int added = session.torrentCount() - 1;
    session.markCompleted(added);
    model.refreshFull();

    const QModelIndex doneRow = model.index(added, 0);
    CHECK(model.data(doneRow, QmlPosterModel::StateKeyRole).toString()
          == QStringLiteral("completed"));
    CHECK(model.data(doneRow, QmlPosterModel::StateStringRole).toString()
          == tr_(QStringLiteral("state_completed")));
    CHECK(model.data(doneRow, QmlPosterModel::StateStringRole).toString() != seeding);
}

// A file that is not a title still has a place: Sherwan's installers, disc
// images and archives sat in no category, so only "Movies" ever showed.
TEST_CASE("categoryForExtension sorts files that are not titles", "[bridge][category]")
{
    CHECK(QmlPosterModel::categoryForExtension("iso") == "Compressed");   // with the archives, as asked
    CHECK(QmlPosterModel::categoryForExtension("ZIP") == "Compressed");
    CHECK(QmlPosterModel::categoryForExtension("exe") == "Apps");
    CHECK(QmlPosterModel::categoryForExtension("flac") == "Music");
    CHECK(QmlPosterModel::categoryForExtension("png") == "Images");
    CHECK(QmlPosterModel::categoryForExtension("pdf") == "Documents");
    CHECK(QmlPosterModel::categoryForExtension("torrent") == "Torrent");
    // a video is named by what it is (a movie, an episode), not by its type
    CHECK(QmlPosterModel::categoryForExtension("mkv").isEmpty());
    CHECK(QmlPosterModel::categoryForExtension("").isEmpty());
    CHECK(QmlPosterModel::categoryForExtension("nfo").isEmpty());
}

// A tester's filter counts stayed at 1 after the last item went: the engine sent
// no update while the library was empty, and the app (in split mode) kept the
// old list. One more update goes out as it empties, then it is quiet again.
TEST_CASE("Session: one update goes out as the library empties", "[session][tick]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    while (session.torrentCount() > 0) session.removeTorrent(0, false, true);
    session.addTorrent(makeFixtureTorrent(tmp.path(), QStringLiteral("empties")), tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == 1; }));

    QSignalSpy updates(&session, &SessionManager::torrentsUpdated);
    session.removeTorrent(0, false, true);
    REQUIRE(session.torrentCount() == 0);
    // the tick is once a second
    REQUIRE(pumpUntil([&] { return updates.count() > 0; }, 3000));
    const int afterEmpty = updates.count();
    pumpUntil([] { return false; }, 2500);
    CHECK(updates.count() == afterEmpty);
}

// Several items selected, all paused: the toolbar reads Resume. It only did for one.
TEST_CASE("Session bridge: allSelectedPaused covers a multi-selection", "[bridge][session]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);
    const int base = session.torrentCount();
    for (const QString &tag : {QStringLiteral("p1"), QStringLiteral("p2")})
        session.addTorrent(makeFixtureTorrent(tmp.path(), tag), tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == base + 2; }));

    bridge.setSelectedRows({base, base + 1});
    session.pauseTorrent(base);
    session.pauseTorrent(base + 1);
    REQUIRE(pumpUntil([&] { return session.torrentAt(base).paused && session.torrentAt(base + 1).paused; }));
    CHECK(bridge.allSelectedPaused());

    session.resumeTorrent(base + 1);
    REQUIRE(pumpUntil([&] { return !session.torrentAt(base + 1).paused; }));
    CHECK_FALSE(bridge.allSelectedPaused());
}

// The carousel's resume list asks the engine for the libraries over a call that
// runs the event loop. Built inside the getter, a tick arriving meanwhile
// re-ran the bindings reading it, and QML logged a binding loop on every start.
// The getter hands back the last list; the rebuild runs afterwards.
TEST_CASE("Session bridge: resumeItems is built outside the getter", "[bridge][session][games]")
{
    app();
    QTemporaryDir dir;
    REQUIRE(dir.isValid());
    const QString exe = dir.filePath(QStringLiteral("Played.exe"));
    { QFile f(exe); REQUIRE(f.open(QIODevice::WriteOnly)); f.write("MZ"); }
    const QString hash = QStringLiteral("6fb1a3cc06b3f82f2f8c544156cdc54e4f08de5c");
    QFile::remove(QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                      .filePath(QStringLiteral("playlog.json")));
    {
        QSettings s;
        s.setValue(QStringLiteral("keptGames/") + hash + QStringLiteral("/name"), QStringLiteral("Played Game"));
        s.setValue(QStringLiteral("gameExe/") + hash, exe);
        s.setValue(QStringLiteral("gamePlayed/") + hash, 1700000000000LL);
    }
    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    CHECK(bridge.resumeItems().isEmpty());   // nothing built yet, and nothing built here
    QSignalSpy changed(&bridge, &QmlSessionBridge::statsChanged);
    REQUIRE(pumpUntil([&] { return !bridge.resumeItems().isEmpty(); }, 3000));
    CHECK(changed.count() > 0);
    CHECK(bridge.resumeItems().first().toMap().value(QStringLiteral("infoHash")).toString() == hash);
    bridge.forgetGame(hash);
}

// The detail panel's Mark button and the menu's Sequential tick read these from
// bindings. In split mode the answer is a call to the engine that runs the
// event loop, and a tick arriving meanwhile re-ran the binding: QML logged a
// loop. The getters hand back the last answer and ask outside the binding.
TEST_CASE("Session bridge: selectedHasVideo is asked of the engine outside the getter", "[bridge][session]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    SessionManager session;
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);
    const int base = session.torrentCount();
    session.addTorrent(makeVideoFixtureTorrent(tmp.path(), QStringLiteral("flags")), tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == base + 1; }));
    REQUIRE(pumpUntil([&] { return session.torrentHasVideo(base); }));

    bridge.setSelectedRows({base});
    CHECK_FALSE(bridge.selectedHasVideo());   // not asked yet, and not asked here
    QSignalSpy changed(&bridge, &QmlSessionBridge::selectionChanged);
    REQUIRE(pumpUntil([&] { return bridge.selectedHasVideo(); }, 3000));
    CHECK(changed.count() > 0);

    CHECK_FALSE(bridge.selectedSequential());
    bridge.setSelectedSequential(true);
    CHECK(bridge.selectedSequential());       // the setter keeps the answer current
    // the menu's Force start tick reads the same way
    CHECK_FALSE(bridge.selectedForceStart());
    bridge.setSelectedForceStart(true);
    CHECK(bridge.selectedForceStart());
}

// Copy hands out what gets the item again: a magnet for a torrent, the address
// for a direct download (it copied nothing for those), one per line for several.
// The toolbar and the menu name it by selectedCopyKind.
TEST_CASE("Session bridge: Copy gives a direct download's address and names it", "[bridge][session][copy]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");

    httptest::RangeServer server(httptest::makePayload(5000));
    REQUIRE(server.listen(QHostAddress::LocalHost));
    HttpDownloadManager http;
    SessionManager inner;
    while (inner.torrentCount() > 0) inner.removeTorrent(0, false, true);
    inner.addTorrent(makeFixtureTorrent(tmp.path(), QStringLiteral("copy")), tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return inner.torrentCount() == 1; }));
    while (http.count() > 0) http.remove(http.at(0)->id(), false);
    http.add(server.url(), tmp.path());
    HttpMergeEngine merged(&inner, &http);
    MetadataResolver resolver;
    QmlSessionBridge bridge(&merged, &resolver);
    REQUIRE(merged.torrentCount() == 2);   // the torrent, then the direct download

    bridge.setSelectedRows({1});
    CHECK(bridge.selectedCopyKind() == QLatin1String("link"));
    bridge.copyMagnetLink();
    CHECK(QGuiApplication::clipboard()->text() == server.url().toString());

    bridge.setSelectedRows({0});
    CHECK(bridge.selectedCopyKind() == QLatin1String("magnet"));

    bridge.setSelectedRows({0, 1});
    CHECK(bridge.selectedCopyKind() == QLatin1String("mixed"));
    bridge.copyMagnetLink();
    const QStringList lines = QGuiApplication::clipboard()->text().split(QLatin1Char('\n'));
    REQUIRE(lines.size() == 2);
    CHECK(lines.at(0).startsWith(QLatin1String("magnet:?")));
    CHECK(lines.at(1) == server.url().toString());
}

// After a film, the player offers the next one of its TMDB collection: the
// library's copy when there is one, else what TMDB says comes next.
TEST_CASE("Search util: filmAfter picks the next film of a collection", "[bridge][search][collection]")
{
    const QVariantList films = { QVariantMap{{"tmdbId", 671}, {"title", "Stone"}},
                                 QVariantMap{{"tmdbId", 672}, {"title", "Chamber"}},
                                 QVariantMap{{"tmdbId", 673}, {"title", "Azkaban"}} };
    CHECK(SearchBridgeUtil::filmAfter(films, 671).value("tmdbId").toInt() == 672);
    CHECK(SearchBridgeUtil::filmAfter(films, 672).value("title").toString() == QStringLiteral("Azkaban"));
    CHECK(SearchBridgeUtil::filmAfter(films, 673).isEmpty());   // the last
    CHECK(SearchBridgeUtil::filmAfter(films, 9).isEmpty());     // not in it
}

TEST_CASE("Session bridge: nextFilmInLibrary finds the library's next film of the collection", "[bridge][session][collection]")
{
    app();
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    QDir().mkpath(tmp.path() + "/dl");
    SessionManager session;
    while (session.torrentCount() > 0) session.removeTorrent(0, false, true);
    for (const QString &tag : {QStringLiteral("hp1"), QStringLiteral("hp3"), QStringLiteral("hp2")})
        session.addTorrent(makeFixtureTorrent(tmp.path(), tag), tmp.path() + "/dl");
    REQUIRE(pumpUntil([&] { return session.torrentCount() == 3; }));

    // what the resolver would have cached for them: three films of one collection
    const QString metaDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/metadata";
    QDir().mkpath(metaDir);
    const int ids[] = {671, 673, 672};
    const int years[] = {2001, 2004, 2002};
    for (int i = 0; i < 3; ++i) {
        QFile f(metaDir + "/" + session.torrentHashAt(i).toLower() + ".json");
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QJsonDocument(QJsonObject{{"title", QStringLiteral("HP %1").arg(i)}, {"year", years[i]},
                                          {"tmdbId", ids[i]}, {"collectionId", 1241},
                                          {"collectionName", "Harry Potter Collection"},
                                          {"collectionChecked", true}, {"contentType", "movie"}}).toJson());
    }
    MetadataResolver resolver;
    QmlSessionBridge bridge(&session, &resolver);

    const QVariantMap afterFirst = bridge.nextFilmInLibrary(session.torrentHashAt(0));
    CHECK(afterFirst.value("infoHash").toString() == session.torrentHashAt(2));   // 2002, not 2004
    CHECK(afterFirst.value("year").toInt() == 2002);
    const QVariantMap afterLast = bridge.nextFilmInLibrary(session.torrentHashAt(1));
    CHECK(afterLast.value("infoHash").toString().isEmpty());   // nothing later in the library
    CHECK(afterLast.value("collectionId").toInt() == 1241);    // so TMDB is asked
    CHECK(afterLast.value("tmdbId").toInt() == 673);
}

// A tester asked for Settings to say where updates stand: the installed version,
// whether it is current, what is out, when it was last checked, and why a check
// failed. The status survives a restart; a failure keeps the last good time.
TEST_CASE("Updater bridge: the last check's outcome is kept and said", "[bridge][updates]")
{
    app();
    {
        QSettings s;
        for (const char *k : {"lastUpdateStatus", "lastUpdateLatest", "lastUpdateFailure", "lastUpdateCheck"})
            s.remove(QLatin1String(k));
    }
    {
        QmlUpdaterBridge up;
        CHECK(up.status().isEmpty());
        CHECK(up.lastChecked() == 0);
        QSignalSpy changed(&up, &QmlUpdaterBridge::statusChanged);

        up.noteNone();
        CHECK(up.status() == QStringLiteral("current"));
        CHECK(up.latestVersion() == QStringLiteral(APP_VERSION));
        CHECK(up.lastChecked() > 0);
        CHECK(changed.count() > 0);

        up.noteFound(QStringLiteral("9.9.9"));
        CHECK(up.status() == QStringLiteral("available"));
        CHECK(up.latestVersion() == QStringLiteral("9.9.9"));

        const qint64 goodAt = up.lastChecked();
        QSettings().setValue("lastUpdateCheck", goodAt - 100);   // as if the good one were older
        up.noteFailed(QStringLiteral("offline"));
        CHECK(up.status() == QStringLiteral("failed"));
        CHECK(up.failure() == QStringLiteral("offline"));
        CHECK(up.latestVersion() == QStringLiteral("9.9.9"));
        CHECK(up.lastChecked() == goodAt - 100);   // a failure is not a check that answered
    }
    QmlUpdaterBridge restarted;   // what Settings shows after a restart
    CHECK(restarted.status() == QStringLiteral("failed"));
    CHECK(restarted.failure() == QStringLiteral("offline"));
}
