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

// ============================================================================
//  playFile / streamUrl / clearResume — past-regression characterization (P0)
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

TEST_CASE("playFile: emits stream URL and sets sequential + file priorities",
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

    REQUIRE(pumpUntil([&] {
        if (!session.isSequentialDownload(row)) return false;
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

    // Same info-hash, different tracker/display-name — the "same content from
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
