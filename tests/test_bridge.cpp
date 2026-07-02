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

#include <libtorrent/file_storage.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>

#include "torrent/sessionmanager.h"
#include "services/metadata/metadataresolver.h"
#include "services/discovery/gamesourcemanager.h"
#include "bridges/qmlposterbridge.h"

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

// Build a real, private (offline — no DHT/LSD/PEX) multi-file .torrent under `dir`
// and return its path. Mirrors the create flow in qmlposterbridge.cpp.
static QString makeFixtureTorrent(const QString &dir)
{
    const QString content = dir + "/bat_fixture";
    QDir().mkpath(content + "/sub");
    auto writeN = [](const QString &p, int n) {
        QFile f(p); f.open(QIODevice::WriteOnly); f.write(QByteArray(n, 'x')); f.close();
    };
    writeN(content + "/a.txt", 100);
    writeN(content + "/sub/b.txt", 200);

    lt::file_storage fs;
    lt::add_files(fs, content.toStdString());
    lt::create_torrent ct(fs, 16384, lt::create_torrent::v1_only);   // classic v1 → no v2 pad files
    ct.add_tracker("udp://tracker.test:6969/announce", 0);
    ct.set_priv(true);                                  // private → fully offline
    lt::set_piece_hashes(ct, dir.toStdString());        // parent of "bat_fixture"

    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    const QString out = dir + "/sample.torrent";
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
        // binaries — so wiping our own data dir can never touch real user data or
        // another suite's fixtures.
        QCoreApplication::setOrganizationName("BATorrentBridgeTest");
        QCoreApplication::setApplicationName("BATorrentBridgeTest");
        // Clear only OUR own leaf dirs so every run starts with an empty session
        // and isolated settings (never the parent — that's another suite's space).
        for (auto loc : { QStandardPaths::AppDataLocation, QStandardPaths::AppConfigLocation }) {
            const QString p = QStandardPaths::writableLocation(loc);
            if (!p.isEmpty()) QDir(p).removeRecursively();
        }
        // SessionManager persists to its own explicit org ("BATorrent"), not the
        // test org above — wipe it too so persisted prefs don't leak across runs.
        QSettings("BATorrent", "BATorrent").clear();
        return new QApplication(s_argc, s_argv);
    }();
    return *a;
}

// ============================================================================
//  QmlPairingBridge — the pairing QR (pure: qrcodegen, no session/GUI)
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
//  QmlThemeBridge — theme name persistence
// ============================================================================
TEST_CASE("Theme: themeName round-trips through settings", "[bridge][theme]")
{
    app();
    QmlThemeBridge t;
    t.setThemeName(QStringLiteral("midnight"));
    REQUIRE(t.themeName() == QStringLiteral("midnight"));

    QmlThemeBridge t2;                                // a fresh bridge reads the persisted value
    REQUIRE(t2.themeName() == QStringLiteral("midnight"));

    t.setThemeName(QStringLiteral("dark"));           // restore default-ish
}

// ============================================================================
//  QmlSessionBridge — selection state with no torrents
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

// ============================================================================
//  QmlSearchBridge — "Tudo" aggregates every source into one flat result list
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

// ============================================================================
//  QmlSettingsBridge — pairing flag derives from QSettings (post-keychain move)
// ============================================================================
TEST_CASE("Settings bridge: pairingActive reflects settings, not the keychain", "[bridge][settings]")
{
    app();
    QSettings st;
    st.setValue("webUiEnabled", false);              // keep the ctor from starting a server
    SessionManager session;
    QmlSettingsBridge bridge(&session, nullptr);     // no engine — applyWebUi() early-returns

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
//  QmlPosterModel / QmlTorrentFilterProxy — empty-model contract
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
//  QmlSessionBridge with a REAL loaded torrent — the methods that need content
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
//  SessionManager — speed/queue/network prefs persist across a "restart"
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
//  QmlSettingsBridge — UI bool toggles coerce to a real bool on read.
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
    REQUIRE(v.typeId() == QMetaType::Bool);          // coerced — not the raw int 0
    REQUIRE(v.toBool() == false);
    REQUIRE_FALSE(v.toBool() != false);              // the exact compare QML relies on

    sb.set(QStringLiteral("showSplash"), true);
    REQUIRE(sb.get(QStringLiteral("showSplash")).toBool() == true);

    // An unset toggle stays invalid so QML's own `on:` default still applies.
    QSettings().remove(QStringLiteral("randomPort"));
    QSettings().sync();
    REQUIRE_FALSE(sb.get(QStringLiteral("randomPort")).isValid());
}
