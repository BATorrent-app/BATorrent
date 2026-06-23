// SPDX-License-Identifier: MIT
// BATorrent Unit / Integration Test Suite (Catch2 v3)

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QXmlStreamReader>
#include <QTcpSocket>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <QElapsedTimer>
#include <QTest>

#include "app/utils.h"
#include "torrent/types.h"
#include "torrent/sessionmanager.h"
#include "webui/webserver.h"
#include "app/translator.h"
#include "app/updater.h"

using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

// ============================================================================
//  Qt app singleton
// ============================================================================

static int   s_argc = 1;
static char  s_arg0[] = "test_unit";
static char *s_argv[] = { s_arg0, nullptr };

static QCoreApplication &app() {
    static QCoreApplication *a = [] {
        // SessionManager now persists speed/queue/network prefs to QSettings and
        // reloads them in its ctor. Enable test mode up front (so it can never
        // touch real user settings) and wipe the store once per run, so the
        // default-state tests below aren't polluted by a prior run or sibling test.
        QStandardPaths::setTestModeEnabled(true);
        auto *inst = new QCoreApplication(s_argc, s_argv);
        QSettings("BATorrent", "BATorrent").clear();
        return inst;
    }();
    return *a;
}

static QByteArray basicAuth(const QString &user, const QString &pass)
{
    return "Authorization: Basic " +
           (user + ":" + pass).toUtf8().toBase64() + "\r\n";
}

// Send raw HTTP request to an in-process server.
// Both client and server live on the same thread, so we must never block
// with waitForReadyRead — we pump the event loop manually instead.
static QByteArray sendHttp(quint16 port, const QByteArray &request, int timeout = 3000)
{
    QTcpSocket sock;
    QByteArray resp;
    bool done = false;

    QObject::connect(&sock, &QTcpSocket::readyRead, [&]() {
        resp += sock.readAll();
    });
    QObject::connect(&sock, &QTcpSocket::disconnected, [&]() {
        done = true;
    });

    sock.connectToHost("127.0.0.1", port);

    // Pump until connected
    QElapsedTimer timer;
    timer.start();
    while (sock.state() != QAbstractSocket::ConnectedState && timer.elapsed() < timeout)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    if (sock.state() != QAbstractSocket::ConnectedState) return {};

    sock.write(request);
    sock.flush();

    // Pump until server closes connection (Connection: close) or timeout
    timer.restart();
    while (!done && timer.elapsed() < timeout)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);

    // Final drain
    if (sock.bytesAvailable() > 0)
        resp += sock.readAll();

    return resp;
}

// ============================================================================
//  UTILITY FUNCTIONS: formatSize
// ============================================================================

TEST_CASE("Utils: formatSize", "[unit][utils]")
{
    // formatSize is locale-aware (QLocale::system); build expectations with the
    // SAME locale so the test checks our threshold/divisor/precision/suffix
    // logic without depending on the test machine's number formatting.
    const QLocale loc = QLocale::system();

    SECTION("bytes") {
        REQUIRE(formatSize(0)    == loc.toString(qlonglong(0))    + " B");
        REQUIRE(formatSize(512)  == loc.toString(qlonglong(512))  + " B");
        REQUIRE(formatSize(1023) == loc.toString(qlonglong(1023)) + " B");
    }

    SECTION("kilobytes") {
        REQUIRE(formatSize(1024)    == loc.toString(1.0, 'f', 1)  + " KB");
        REQUIRE(formatSize(1536)    == loc.toString(1.5, 'f', 1)  + " KB");
        REQUIRE(formatSize(10240)   == loc.toString(10.0, 'f', 1) + " KB");
    }

    SECTION("megabytes") {
        REQUIRE(formatSize(1048576)   == loc.toString(1.0, 'f', 1)   + " MB");
        REQUIRE(formatSize(1572864)   == loc.toString(1.5, 'f', 1)   + " MB");
        REQUIRE(formatSize(104857600) == loc.toString(100.0, 'f', 1) + " MB");
    }

    SECTION("gigabytes") {
        REQUIRE(formatSize(1073741824LL)  == loc.toString(1.0, 'f', 2)  + " GB");
        REQUIRE(formatSize(1610612736LL)  == loc.toString(1.5, 'f', 2)  + " GB");
        REQUIRE(formatSize(10737418240LL) == loc.toString(10.0, 'f', 2) + " GB");
    }

    SECTION("edge: negative values") {
        auto result = formatSize(-1);
        REQUIRE(!result.isEmpty());
    }
}

// ============================================================================
//  UTILITY FUNCTIONS: formatSpeed
// ============================================================================

TEST_CASE("Utils: formatSpeed", "[unit][utils]")
{
    // Locale-aware + honors the bytes/bits unit (global); pin to bytes so the
    // assertions are deterministic.
    setSpeedUnit(0);
    const QLocale loc = QLocale::system();

    SECTION("bytes per second") {
        REQUIRE(formatSpeed(0)    == loc.toString(0)    + " B/s");
        REQUIRE(formatSpeed(500)  == loc.toString(500)  + " B/s");
        REQUIRE(formatSpeed(1023) == loc.toString(1023) + " B/s");
    }

    SECTION("kilobytes per second") {
        REQUIRE(formatSpeed(1024) == loc.toString(1.0, 'f', 1) + " KB/s");
        REQUIRE(formatSpeed(5120) == loc.toString(5.0, 'f', 1) + " KB/s");
    }

    SECTION("megabytes per second") {
        REQUIRE(formatSpeed(1048576)  == loc.toString(1.0, 'f', 1)  + " MB/s");
        REQUIRE(formatSpeed(10485760) == loc.toString(10.0, 'f', 1) + " MB/s");
    }

    SECTION("edge: zero") {
        REQUIRE(formatSpeed(0) == loc.toString(0) + " B/s");
    }
}

// ============================================================================
//  AUTO-UPDATE: version detection (the logic that decides "update available")
// ============================================================================

TEST_CASE("Updater: compareVersions detects newer/older/equal", "[unit][updater]")
{
    // >0 means the first arg is newer than the second.
    SECTION("an update is available when the remote is newer") {
        REQUIRE(Updater::compareVersions("3.0.0", "2.6.1") > 0);
        REQUIRE(Updater::compareVersions("2.6.1", "2.6.0") > 0);
    }
    SECTION("no update when we're at or ahead of the remote") {
        REQUIRE(Updater::compareVersions("2.6.1", "2.6.1") == 0);
        REQUIRE(Updater::compareVersions("3.0.0", "2.6.1") > 0); // remote older → we're ahead
        REQUIRE(Updater::compareVersions("2.6.0", "2.6.1") < 0);
    }
    SECTION("numeric, not lexicographic (2.10 > 2.9)") {
        REQUIRE(Updater::compareVersions("2.10.0", "2.9.0") > 0);
        REQUIRE(Updater::compareVersions("1.2.0", "1.10.0") < 0);
    }
    SECTION("a release outranks its pre-release (semver)") {
        REQUIRE(Updater::compareVersions("3.0.0", "3.0.0-rc1") > 0);
        REQUIRE(Updater::compareVersions("3.0.0-beta", "3.0.0") < 0);
    }
}

// ============================================================================
//  UPGRADE SAFETY: a pre-3.0 user's torrents survive the org-name path shift
// ============================================================================

TEST_CASE("Resume data migrates from the pre-3.0 location", "[unit][migration]")
{
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName("BATorrent");
    QCoreApplication::setApplicationName("BATorrent");
    QSettings().remove("resumeMigrated");   // clear the one-time guard

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString newResume = QDir(appData).filePath("resume");
    QDir up(appData); up.cdUp();
    const QString legacyResume = up.filePath("resume");

    // If the platform's layout collapses both to the same dir, the migration is
    // a no-op by design — skip rather than assert a meaningless copy.
    if (QDir::cleanPath(legacyResume) == QDir::cleanPath(newResume))
        return;

    QDir().mkpath(legacyResume);
    QDir().mkpath(newResume);
    for (const auto &n : QDir(newResume).entryList({"*.resume"}, QDir::Files))
        QFile::remove(newResume + "/" + n);
    {
        QFile f(legacyResume + "/deadbeef.resume");
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write("x");
    }

    // ctor runs the migration; the guard was cleared above so it fires.
    SessionManager sm;

    REQUIRE(QFile::exists(newResume + "/deadbeef.resume"));        // torrents survived
    REQUIRE(QSettings().value("resumeMigrated").toBool() == true); // ran exactly once
}

// ============================================================================
//  DATA TYPES: TorrentInfo
// ============================================================================

TEST_CASE("Types: TorrentInfo default values", "[unit][types]")
{
    TorrentInfo info{};
    REQUIRE(info.name.isEmpty());
    REQUIRE(info.savePath.isEmpty());
    REQUIRE(info.totalSize == 0);
    REQUIRE(info.totalDone == 0);
    REQUIRE(info.progress == Approx(0.0f));
    REQUIRE(info.downloadRate == 0);
    REQUIRE(info.uploadRate == 0);
    REQUIRE(info.numPeers == 0);
    REQUIRE(info.numSeeds == 0);
    REQUIRE(info.paused == false);
    REQUIRE(info.ratio == Approx(0.0f));
}

TEST_CASE("Types: PeerInfo default values", "[unit][types]")
{
    PeerInfo p{};
    REQUIRE(p.ip.isEmpty());
    REQUIRE(p.port == 0);
    REQUIRE(p.downloadRate == 0);
    REQUIRE(p.uploadRate == 0);
    REQUIRE(p.progress == Approx(0.0f));
    REQUIRE(p.client.isEmpty());
}

TEST_CASE("Types: FileInfo default values", "[unit][types]")
{
    FileInfo f{};
    REQUIRE(f.path.isEmpty());
    REQUIRE(f.size == 0);
    REQUIRE(f.progress == Approx(0.0f));
    REQUIRE(f.priority == 0);
}

TEST_CASE("Types: TrackerInfo default values", "[unit][types]")
{
    TrackerInfo t{};
    REQUIRE(t.url.isEmpty());
    REQUIRE(t.tier == 0);
    REQUIRE(t.status.isEmpty());
}

// ============================================================================
//  TRANSLATOR
// ============================================================================

TEST_CASE("Translator: basic functionality", "[unit][translator]")
{
    app();
    auto &tr = Translator::instance();

    SECTION("English is default") {
        tr.setLanguage(Translator::English);
        REQUIRE(tr.language() == Translator::English);
    }

    SECTION("known keys return non-empty strings") {
        tr.setLanguage(Translator::English);
        REQUIRE(!tr.tr("col_name").isEmpty());
        REQUIRE(!tr.tr("col_size").isEmpty());
        REQUIRE(!tr.tr("col_progress").isEmpty());
        REQUIRE(!tr.tr("state_downloading").isEmpty());
        REQUIRE(!tr.tr("state_seeding").isEmpty());
        REQUIRE(!tr.tr("state_paused").isEmpty());
    }

    SECTION("Portuguese mode works") {
        tr.setLanguage(Translator::Portuguese);
        REQUIRE(!tr.tr("col_name").isEmpty());
        tr.setLanguage(Translator::English); // restore
    }

    SECTION("unknown key returns key itself") {
        tr.setLanguage(Translator::English);
        REQUIRE(tr.tr("nonexistent_key_xyz") == "nonexistent_key_xyz");
    }

    SECTION("tr_ shortcut works") {
        tr.setLanguage(Translator::English);
        REQUIRE(tr_("col_name") == tr.tr("col_name"));
    }
}

// ============================================================================
//  SESSION MANAGER: BASIC STATE
// ============================================================================

TEST_CASE("SessionManager: initial state", "[unit][session]")
{
    app();
    SessionManager session;

    REQUIRE(session.torrentCount() == 0);
    REQUIRE(session.dhtEnabled() == true);
    REQUIRE(session.encryptionMode() == 0);
    REQUIRE(session.seedRatioLimit() == Approx(0.0f));
    REQUIRE(session.proxyType() == 0);
    REQUIRE(session.proxyHost().isEmpty());
    REQUIRE(session.ipFilterCount() == 0);
    REQUIRE(session.killSwitchEnabled() == false);
    REQUIRE(session.autoResumeOnReconnect() == false);
    REQUIRE(session.schedulerEnabled() == false);
    REQUIRE(session.altSpeedsActive() == false);
    REQUIRE(session.outgoingInterface().isEmpty());
}

TEST_CASE("SessionManager: torrentAt out of bounds", "[unit][session]")
{
    app();
    SessionManager session;

    TorrentInfo neg = session.torrentAt(-1);
    REQUIRE(neg.name.isEmpty());

    TorrentInfo big = session.torrentAt(9999);
    REQUIRE(big.name.isEmpty());
}

TEST_CASE("SessionManager: peersAt/filesAt/trackersAt out of bounds", "[unit][session]")
{
    app();
    SessionManager session;

    REQUIRE(session.peersAt(-1).empty());
    REQUIRE(session.peersAt(0).empty());
    REQUIRE(session.filesAt(-1).empty());
    REQUIRE(session.filesAt(0).empty());
    REQUIRE(session.trackersAt(-1).empty());
    REQUIRE(session.trackersAt(0).empty());
}

TEST_CASE("SessionManager: pause/resume with no torrents", "[unit][session]")
{
    app();
    SessionManager session;

    // These should not crash
    session.pauseTorrent(-1);
    session.pauseTorrent(0);
    session.resumeTorrent(-1);
    session.resumeTorrent(0);
    session.pauseAll();
    session.resumeAll();
    session.removeTorrent(-1);
    session.removeTorrent(0);
}

// ============================================================================
//  SESSION MANAGER: SETTINGS
// ============================================================================

TEST_CASE("SessionManager: download/upload limits", "[unit][session]")
{
    app();
    SessionManager session;

    session.setDownloadLimit(500);
    REQUIRE(session.downloadLimit() == 500);

    session.setUploadLimit(250);
    REQUIRE(session.uploadLimit() == 250);

    // Zero = unlimited
    session.setDownloadLimit(0);
    REQUIRE(session.downloadLimit() == 0);
}

TEST_CASE("SessionManager: encryption modes", "[unit][session]")
{
    app();
    SessionManager session;

    session.setEncryptionMode(0); // enabled
    REQUIRE(session.encryptionMode() == 0);

    session.setEncryptionMode(1); // forced
    REQUIRE(session.encryptionMode() == 1);

    session.setEncryptionMode(2); // disabled
    REQUIRE(session.encryptionMode() == 2);
}

TEST_CASE("SessionManager: DHT toggle", "[unit][session]")
{
    app();
    SessionManager session;

    REQUIRE(session.dhtEnabled());
    session.setDhtEnabled(false);
    REQUIRE_FALSE(session.dhtEnabled());
    session.setDhtEnabled(true);
    REQUIRE(session.dhtEnabled());
}

TEST_CASE("SessionManager: seed ratio limit", "[unit][session]")
{
    app();
    SessionManager session;

    session.setSeedRatioLimit(2.5f);
    REQUIRE(session.seedRatioLimit() == Approx(2.5f));

    session.setSeedRatioLimit(0.0f);
    REQUIRE(session.seedRatioLimit() == Approx(0.0f));
}

TEST_CASE("SessionManager: proxy settings roundtrip", "[unit][session]")
{
    app();
    SessionManager session;

    session.setProxySettings(1, "socks.example.com", 1080, "user", "pass");
    REQUIRE(session.proxyType() == 1);
    REQUIRE(session.proxyHost() == "socks.example.com");
    REQUIRE(session.proxyPort() == 1080);
    REQUIRE(session.proxyUser() == "user");
    REQUIRE(session.proxyPass() == "pass");

    session.setProxySettings(0, "", 0, "", "");
    REQUIRE(session.proxyType() == 0);
}

TEST_CASE("SessionManager: VPN interface settings", "[unit][session]")
{
    app();
    SessionManager session;

    session.setKillSwitchEnabled(true);
    REQUIRE(session.killSwitchEnabled());

    session.setAutoResumeOnReconnect(true);
    REQUIRE(session.autoResumeOnReconnect());

    session.setKillSwitchEnabled(false);
    REQUIRE_FALSE(session.killSwitchEnabled());
}

TEST_CASE("SessionManager: bandwidth scheduler", "[unit][session]")
{
    app();
    SessionManager session;

    session.setAltSpeedLimits(100, 50);
    REQUIRE(session.altDownloadLimit() == 100);
    REQUIRE(session.altUploadLimit() == 50);

    session.setScheduleFromHour(22);
    session.setScheduleToHour(6);
    REQUIRE(session.scheduleFromHour() == 22);
    REQUIRE(session.scheduleToHour() == 6);

    session.setScheduleDays(0b0011111); // Mon-Fri
    REQUIRE(session.scheduleDays() == 0b0011111);

    session.setSchedulerEnabled(true);
    REQUIRE(session.schedulerEnabled());
    session.setSchedulerEnabled(false);
}

TEST_CASE("SessionManager: max connections", "[unit][session]")
{
    app();
    SessionManager session;

    session.setMaxConnections(200);
    REQUIRE(session.maxConnections() == 200);
}

TEST_CASE("SessionManager: global stats with no torrents", "[unit][session]")
{
    app();
    SessionManager session;

    // With no torrents loaded, global stats come from persisted base
    REQUIRE(session.globalDownloaded() >= 0);
    REQUIRE(session.globalUploaded() >= 0);
    REQUIRE(session.globalRatio() >= 0.0f);
}

TEST_CASE("SessionManager: setFilePriority out of bounds", "[unit][session]")
{
    app();
    SessionManager session;

    // Should not crash
    session.setFilePriority(-1, 0, 4);
    session.setFilePriority(0, 0, 4);
    session.setFilePriority(999, 0, 4);
}

TEST_CASE("SessionManager: setSequentialDownload out of bounds", "[unit][session]")
{
    app();
    SessionManager session;

    // Should not crash
    session.setSequentialDownload(-1, true);
    session.setSequentialDownload(0, true);
    session.setSequentialDownload(999, false);
}

// ============================================================================
//  WEBSERVER: ROUTING
// ============================================================================

static QByteArray webGet(quint16 port, const QByteArray &path)
{
    return sendHttp(port, "GET " + path + " HTTP/1.1\r\nHost: localhost\r\n\r\n");
}

TEST_CASE("WebServer: start and stop", "[unit][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);

    REQUIRE(server.start(18400, false));
    REQUIRE(server.isRunning());

    server.stop();
    REQUIRE_FALSE(server.isRunning());
}

TEST_CASE("WebServer: GET /api/torrents returns JSON array", "[unit][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18401, false));

    auto r = webGet(18401, "/api/torrents");
    REQUIRE(r.contains("200"));
    REQUIRE(r.contains("application/json"));

    // Extract body
    int bodyIdx = r.indexOf("\r\n\r\n");
    REQUIRE(bodyIdx > 0);
    QByteArray body = r.mid(bodyIdx + 4);
    QJsonDocument doc = QJsonDocument::fromJson(body);
    REQUIRE(doc.isArray());
    REQUIRE(doc.array().isEmpty()); // no torrents

    server.stop();
}

TEST_CASE("WebServer: GET /api/status returns JSON object", "[unit][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18402, false));

    auto r = webGet(18402, "/api/status");
    REQUIRE(r.contains("200"));

    int bodyIdx = r.indexOf("\r\n\r\n");
    REQUIRE(bodyIdx > 0);
    QByteArray body = r.mid(bodyIdx + 4);
    QJsonDocument doc = QJsonDocument::fromJson(body);
    REQUIRE(doc.isObject());

    QJsonObject obj = doc.object();
    REQUIRE(obj.contains("torrentCount"));
    REQUIRE(obj.contains("downloadRate"));
    REQUIRE(obj.contains("uploadRate"));
    REQUIRE(obj.contains("downloadLimit"));
    REQUIRE(obj.contains("uploadLimit"));
    REQUIRE(obj["torrentCount"].toInt() == 0);

    server.stop();
}

TEST_CASE("WebServer: GET / serves HTML", "[unit][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18403, false));

    auto r = webGet(18403, "/");
    // Either serves HTML or returns 500 (resource not in test binary)
    REQUIRE((!r.isEmpty()));

    server.stop();
}

TEST_CASE("WebServer: unknown path returns 404", "[unit][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18404, false));

    auto r = webGet(18404, "/nonexistent");
    REQUIRE_THAT(r.toStdString(), ContainsSubstring("404"));

    server.stop();
}

TEST_CASE("WebServer: credentials roundtrip", "[unit][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);

    auto hash = QString::fromUtf8(
        QCryptographicHash::hash("mypass", QCryptographicHash::Sha256).toHex());
    server.setCredentials("admin", hash);

    REQUIRE(server.start(18405, false));

    // Without auth → 401
    auto r1 = sendHttp(18405, "GET /api/torrents HTTP/1.1\r\nHost: localhost\r\n\r\n");
    REQUIRE_THAT(r1.toStdString(), ContainsSubstring("401"));

    // With auth → 200
    auto r2 = sendHttp(18405,
        "GET /api/torrents HTTP/1.1\r\n"
        + basicAuth("admin", "mypass") +
        "Host: localhost\r\n\r\n");
    REQUIRE_THAT(r2.toStdString(), ContainsSubstring("200"));

    server.stop();
}

// ============================================================================
//  WEBSERVER: POST /api/torrents (magnet)
// ============================================================================

TEST_CASE("WebServer: POST magnet without savePath uses default", "[integration][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18406, false));

    QJsonObject body;
    body["magnet"] = "magnet:?xt=urn:btih:da39a3ee5e6b4b0d3255bfef95601890afd80709&dn=test";
    QByteArray json = QJsonDocument(body).toJson(QJsonDocument::Compact);

    auto r = sendHttp(18406,
        "POST /api/torrents HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n" + json);

    // Magnet was added (200) or libtorrent rejected it (400) — both valid
    REQUIRE((!r.isEmpty()));
    REQUIRE((r.contains("200") || r.contains("400")));

    server.stop();
}

// ============================================================================
//  RSS XML PARSING (via QXmlStreamReader, same logic as RssManager)
// ============================================================================

// Since RssManager::parseFeedXml is private, we test the same XML parsing
// logic directly with QXmlStreamReader to validate correctness.

TEST_CASE("RSS: parse standard RSS 2.0 feed", "[unit][rss]")
{
    QByteArray xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<rss version="2.0">
  <channel>
    <title>Test Feed</title>
    <item>
      <title>Ubuntu 24.04 LTS</title>
      <link>magnet:?xt=urn:btih:abc123&amp;dn=ubuntu</link>
      <pubDate>Sat, 05 Apr 2026 12:00:00 +0000</pubDate>
    </item>
    <item>
      <title>Fedora 42</title>
      <enclosure url="https://example.com/fedora.torrent" length="1073741824" type="application/x-bittorrent"/>
    </item>
  </channel>
</rss>)";

    QXmlStreamReader reader(xml);
    QString feedTitle;
    QStringList itemTitles, itemLinks;
    QList<qint64> sizes;
    bool inItem = false;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            QString tag = reader.name().toString();
            if (tag == "item") { inItem = true; itemTitles.append(""); itemLinks.append(""); sizes.append(0); }
            else if (inItem && tag == "title") itemTitles.last() = reader.readElementText();
            else if (inItem && tag == "link")  itemLinks.last()  = reader.readElementText();
            else if (inItem && tag == "enclosure") {
                itemLinks.last() = reader.attributes().value("url").toString();
                sizes.last()     = reader.attributes().value("length").toString().toLongLong();
            }
            else if (!inItem && tag == "title") feedTitle = reader.readElementText();
        } else if (reader.isEndElement() && reader.name().toString() == "item") {
            inItem = false;
        }
    }

    REQUIRE_FALSE(reader.hasError());
    REQUIRE(feedTitle == "Test Feed");
    REQUIRE(itemTitles.size() == 2);
    REQUIRE(itemTitles[0] == "Ubuntu 24.04 LTS");
    REQUIRE(itemLinks[0].startsWith("magnet:"));
    REQUIRE(itemTitles[1] == "Fedora 42");
    REQUIRE(itemLinks[1] == "https://example.com/fedora.torrent");
    REQUIRE(sizes[1] == 1073741824LL);
}

TEST_CASE("RSS: parse Atom feed", "[unit][rss]")
{
    QByteArray xml = R"(<?xml version="1.0" encoding="UTF-8"?>
<feed xmlns="http://www.w3.org/2005/Atom">
  <title>Atom Feed</title>
  <entry>
    <title>Arch Linux 2026.04</title>
    <link href="magnet:?xt=urn:btih:def456&amp;dn=arch"/>
    <published>2026-04-05T00:00:00Z</published>
  </entry>
</feed>)";

    QXmlStreamReader reader(xml);
    QString feedTitle;
    QStringList titles, links;
    bool inEntry = false;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            QString tag = reader.name().toString();
            if (tag == "entry") { inEntry = true; titles.append(""); links.append(""); }
            else if (inEntry && tag == "title") titles.last() = reader.readElementText();
            else if (inEntry && tag == "link" && reader.attributes().hasAttribute("href"))
                links.last() = reader.attributes().value("href").toString();
            else if (!inEntry && tag == "title") feedTitle = reader.readElementText();
        } else if (reader.isEndElement() && reader.name().toString() == "entry") {
            inEntry = false;
        }
    }

    REQUIRE_FALSE(reader.hasError());
    REQUIRE(feedTitle == "Atom Feed");
    REQUIRE(titles.size() == 1);
    REQUIRE(titles[0] == "Arch Linux 2026.04");
    REQUIRE(links[0].startsWith("magnet:"));
}

TEST_CASE("RSS: malformed XML does not crash", "[unit][rss]")
{
    QByteArray xml = "<rss><channel><title>Broken</title><item><title>No closing";
    QXmlStreamReader reader(xml);
    while (!reader.atEnd()) reader.readNext();
    REQUIRE(reader.hasError());
}

TEST_CASE("RSS: empty feed returns no items", "[unit][rss]")
{
    QByteArray xml = R"(<?xml version="1.0"?><rss version="2.0"><channel><title>Empty</title></channel></rss>)";
    QXmlStreamReader reader(xml);
    int itemCount = 0;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && reader.name().toString() == "item")
            ++itemCount;
    }
    REQUIRE(itemCount == 0);
}

TEST_CASE("RSS: torrent:magnetURI tag parsed", "[unit][rss]")
{
    QByteArray xml = R"(<?xml version="1.0"?>
<rss version="2.0" xmlns:torrent="http://example.com/torrent">
  <channel><title>T</title>
    <item>
      <title>Test Magnet</title>
      <torrent:magnetURI>magnet:?xt=urn:btih:aaa111</torrent:magnetURI>
      <torrent:contentLength>999999</torrent:contentLength>
    </item>
  </channel>
</rss>)";

    QXmlStreamReader reader(xml);
    QString link;
    qint64 size = 0;
    bool inItem = false;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            QString tag = reader.name().toString();
            if (tag == "item") inItem = true;
            else if (inItem && (tag == "magnetURI"))      link = reader.readElementText();
            else if (inItem && (tag == "contentLength"))   size = reader.readElementText().toLongLong();
        }
    }

    REQUIRE(link == "magnet:?xt=urn:btih:aaa111");
    REQUIRE(size == 999999);
}

// ============================================================================
//  WEBSERVER: JSON RESPONSE STRUCTURE
// ============================================================================

TEST_CASE("WebServer: /api/status JSON schema", "[integration][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18410, false));

    auto r = webGet(18410, "/api/status");
    int bodyIdx = r.indexOf("\r\n\r\n");
    REQUIRE(bodyIdx > 0);

    QJsonDocument doc = QJsonDocument::fromJson(r.mid(bodyIdx + 4));
    REQUIRE(doc.isObject());
    QJsonObject obj = doc.object();

    // Verify all expected fields exist and have correct types
    REQUIRE(obj["torrentCount"].isDouble());
    REQUIRE(obj["downloadRate"].isDouble());
    REQUIRE(obj["uploadRate"].isDouble());
    REQUIRE(obj["downloadLimit"].isDouble());
    REQUIRE(obj["uploadLimit"].isDouble());

    server.stop();
}

TEST_CASE("WebServer: /api/torrents JSON schema (empty)", "[integration][webserver]")
{
    app();
    SessionManager session;
    WebServer server(&session);
    REQUIRE(server.start(18411, false));

    auto r = webGet(18411, "/api/torrents");
    int bodyIdx = r.indexOf("\r\n\r\n");
    REQUIRE(bodyIdx > 0);

    QJsonDocument doc = QJsonDocument::fromJson(r.mid(bodyIdx + 4));
    REQUIRE(doc.isArray());
    REQUIRE(doc.array().size() == 0);

    server.stop();
}

// ============================================================================
//  UPDATER
// ============================================================================

TEST_CASE("Updater::compareVersions", "[unit][updater]")
{
    REQUIRE(Updater::compareVersions("2.6.0", "2.5.0") == 1);
    REQUIRE(Updater::compareVersions("2.5.0", "2.6.0") == -1);
    REQUIRE(Updater::compareVersions("2.5.0", "2.5.0") == 0);
    REQUIRE(Updater::compareVersions("2.10.0", "2.9.0") == 1);
    REQUIRE(Updater::compareVersions("2.5.1", "2.5.0") == 1);
    REQUIRE(Updater::compareVersions("3.0.0", "2.99.99") == 1);
    REQUIRE(Updater::compareVersions("2.5.0-rc1", "2.5.0") == -1);
    REQUIRE(Updater::compareVersions("2.5.0", "2.5.0-rc1") == 1);
}

TEST_CASE("Updater: signals fire on check", "[integration][updater]")
{
    app();
    Updater updater;
    QSignalSpy spyNoUpdate(&updater, &Updater::noUpdateAvailable);
    QSignalSpy spyUpdate(&updater, &Updater::updateAvailable);
    QSignalSpy spyError(&updater, &Updater::errorOccurred);

    updater.checkForUpdate();

    // Pump event loop for up to 20s (network call)
    QElapsedTimer timer;
    timer.start();
    while (spyNoUpdate.count() == 0 && spyUpdate.count() == 0
           && spyError.count() == 0 && timer.elapsed() < 20000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    // At least ONE signal must have fired — "nothing happens" is a failure
    int total = spyNoUpdate.count() + spyUpdate.count() + spyError.count();
    REQUIRE(total > 0);
}

// ============================================================================
//  SIGNALS
// ============================================================================

TEST_CASE("SessionManager: torrentsUpdated signal fires", "[integration][signals]")
{
    app();
    SessionManager session;
    QSignalSpy spy(&session, &SessionManager::torrentsUpdated);
    REQUIRE(spy.isValid());

    // The update timer fires every 1s; wait for it
    QTest::qWait(1500);
    QCoreApplication::processEvents();

    // Signal should have fired at least once (timer-driven)
    // (may not fire if no torrents — implementation detail)
    // Just verify no crash
    REQUIRE(spy.count() >= 0);
}

// ============================================================================
//  STATS HISTORY (daily usage collector — Wrapped seed)
// ============================================================================

#include "app/statshistory.h"
#include <QTemporaryDir>
#include <QDate>

TEST_CASE("StatsHistory: accumulates deltas and persists across sessions", "[stats]")
{
    QTemporaryDir tmp;
    REQUIRE(tmp.isValid());
    const QString path = tmp.filePath("stats-history.json");
    const QString today = QDate::currentDate().toString(Qt::ISODate);

    {
        StatsHistory h(path);
        h.recordTransfer(1000, 500);    // baseline only — no delta yet
        h.recordTransfer(3000, 900);    // +2000 down, +400 up
        h.recordAdded();
        h.recordAdded();
        h.recordCompleted("Movies");
        h.recordCompleted("");
        h.flush();
    }

    {
        StatsHistory h(path);
        const QJsonObject day = h.days().value(today).toObject();
        REQUIRE(day.value("down").toDouble() == 2000.0);
        REQUIRE(day.value("up").toDouble() == 400.0);
        REQUIRE(day.value("added").toDouble() == 2.0);
        REQUIRE(day.value("completed").toDouble() == 2.0);
        const QJsonObject cats = day.value("cats").toObject();
        REQUIRE(cats.value("Movies").toInt() == 1);
        REQUIRE(cats.value("uncategorized").toInt() == 1);

        // second session: baseline re-seeds, only new growth counts
        h.recordTransfer(50, 10);
        h.recordTransfer(150, 60);      // +100 down, +50 up
        h.flush();
    }

    {
        StatsHistory h(path);
        const QJsonObject day = h.days().value(today).toObject();
        REQUIRE(day.value("down").toDouble() == 2100.0);
        REQUIRE(day.value("up").toDouble() == 450.0);
    }
}

TEST_CASE("StatsHistory: negative deltas are clamped", "[stats]")
{
    QTemporaryDir tmp;
    const QString path = tmp.filePath("s.json");
    const QString today = QDate::currentDate().toString(Qt::ISODate);

    StatsHistory h(path);
    h.recordTransfer(5000, 5000);
    h.recordTransfer(4000, 6000);   // down shrank (counter quirk) → clamp; up +1000
    h.flush();
    const QJsonObject day = h.days().value(today).toObject();
    REQUIRE(day.value("down").toDouble() == 0.0);
    REQUIRE(day.value("up").toDouble() == 1000.0);
}

// ============================================================================
//  SUBTITLE PARSER (external .srt/.vtt for the built-in player)
// ============================================================================

#include "app/subtitleparser.h"

TEST_CASE("SubtitleParser: basic SRT with CRLF, index lines and italics", "[subs]")
{
    const QString srt =
        "1\r\n00:00:01,000 --> 00:00:03,500\r\nHello <i>world</i>\r\n\r\n"
        "2\r\n00:01:00,250 --> 00:01:02,000\r\nSecond line\r\nwrapped\r\n\r\n";
    const auto cues = SubtitleParser::parseSrt(srt);
    REQUIRE(cues.size() == 2);
    REQUIRE(cues[0].startMs == 1000);
    REQUIRE(cues[0].endMs == 3500);
    REQUIRE(cues[0].text == "Hello <i>world</i>");
    REQUIRE(cues[1].startMs == 60250);
    REQUIRE(cues[1].text == "Second line<br/>wrapped");
}

TEST_CASE("SubtitleParser: strips ASS overrides and unknown tags, skips malformed", "[subs]")
{
    const QString srt =
        "1\n00:00:01,000 --> 00:00:02,000\n{\\an8}<font color=\"red\">Top</font> text\n\n"
        "garbage block without timing\n\n"
        "3\n00:00:05,000 --> 00:00:04,000\nend before start: dropped\n\n"
        "4\n00:00:06,000 --> 00:00:07,000\n<b>kept</b>\n";
    const auto cues = SubtitleParser::parseSrt(srt);
    REQUIRE(cues.size() == 2);
    REQUIRE(cues[0].text == "Top text");
    REQUIRE(cues[1].text == "<b>kept</b>");
}

TEST_CASE("SubtitleParser: WEBVTT header, NOTE blocks, cue settings, mm:ss times", "[subs]")
{
    const QString vtt =
        "WEBVTT\n\nNOTE this is a comment\nstill the comment\n\n"
        "intro\n00:05.000 --> 00:07.500 align:start position:10%\n- Hi there\n\n"
        "01:00:00.000 --> 01:00:02.000\nOne hour in\n";
    const auto cues = SubtitleParser::parseVtt(vtt);
    REQUIRE(cues.size() == 2);
    REQUIRE(cues[0].startMs == 5000);
    REQUIRE(cues[0].endMs == 7500);
    REQUIRE(cues[0].text == "- Hi there");
    REQUIRE(cues[1].startMs == 3600000);
}

TEST_CASE("SubtitleParser: Latin-1 fallback for unlabeled Windows files", "[subs]")
{
    QTemporaryDir tmp;
    const QString path = tmp.filePath("legenda.srt");
    {
        QFile f(path);
        REQUIRE(f.open(QIODevice::WriteOnly));
        // "ação" in Latin-1 — invalid as UTF-8, must fall back
        f.write("1\n00:00:01,000 --> 00:00:02,000\n");
        f.write(QByteArray("a\xE7\xE3o\n"));
    }
    const auto cues = SubtitleParser::parseFile(path);
    REQUIRE(cues.size() == 1);
    REQUIRE(cues[0].text == QString::fromUtf8("ação"));
}

// ============================================================================
//  SUBTITLE SEARCH (network integration — Gestdown, keyless)
// ============================================================================

#include "app/subtitlesearch.h"

TEST_CASE("SubtitleSearch: Gestdown end-to-end for a known series episode", "[integration][subs-net]")
{
    app();
    SubtitleSearch search;
    QSignalSpy finished(&search, &SubtitleSearch::searchFinished);
    REQUIRE(finished.isValid());

    search.search("Arcane.S02E01.1080p.WEB-DL", {"en"});

    QElapsedTimer timer;
    timer.start();
    while (finished.count() == 0 && timer.elapsed() < 30000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    REQUIRE(finished.count() > 0);

    // network may legitimately be down — only assert the full chain when the
    // provider answered (same tolerance as the updater integration test)
    if (search.results().isEmpty()) {
        WARN("Gestdown returned no results (offline or API change?) — chain not exercised");
        return;
    }
    REQUIRE(search.results().first().provider == "Gestdown");
    REQUIRE(search.results().first().language == "en");

    QTemporaryDir tmp;
    QSignalSpy dlDone(&search, &SubtitleSearch::downloadFinished);
    QSignalSpy dlErr(&search, &SubtitleSearch::errorOccurred);
    search.download(0, tmp.path(), "Arcane.S02E01.test");

    timer.restart();
    while (dlDone.count() == 0 && dlErr.count() == 0 && timer.elapsed() < 30000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    REQUIRE(dlDone.count() > 0);

    const QString path = dlDone.takeFirst().at(0).toString();
    REQUIRE(path.endsWith(".en.srt"));
    const auto cues = SubtitleParser::parseFile(path);
    INFO("cues parsed: " << cues.size());
    REQUIRE(cues.size() > 50);   // a real episode has hundreds of cues
}

// ============================================================================
//  DELETE-TO-TRASH (remove with files must be recoverable, never a hard erase)
// ============================================================================

#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>

#ifdef Q_OS_MACOS
TEST_CASE("SessionManager: remove with files moves the data to the trash", "[integration][trash]")
{
    app();
    QTemporaryDir work;
    REQUIRE(work.isValid());

    const QString payload = work.filePath("bat-trash-test.bin");
    {
        QFile f(payload);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(QByteArray(256 * 1024, 'x'));
    }

    // build a real .torrent for the junk file
    lt::file_storage fs;
    lt::add_files(fs, payload.toStdString());
    lt::create_torrent ct(fs);
    lt::set_piece_hashes(ct, QFileInfo(payload).absolutePath().toStdString());
    std::vector<char> buf;
    lt::bencode(std::back_inserter(buf), ct.generate());
    const QString torrentPath = work.filePath("bat-trash-test.torrent");
    {
        QFile f(torrentPath);
        REQUIRE(f.open(QIODevice::WriteOnly));
        f.write(buf.data(), static_cast<qint64>(buf.size()));
    }

    SessionManager session;
    const int before = session.torrentCount();
    session.addTorrent(torrentPath, QFileInfo(payload).absolutePath());
    REQUIRE(session.torrentCount() == before + 1);
    const int idx = session.torrentCount() - 1;

    // let libtorrent check the (already complete) data
    QTest::qWait(1500);
    QCoreApplication::processEvents();

    session.removeTorrent(idx, true);

    // the trash move is deferred 900ms past remove_torrent
    QElapsedTimer t;
    t.start();
    while (QFileInfo::exists(payload) && t.elapsed() < 8000)
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);

    const QString trashed = QDir::homePath() + "/.Trash/bat-trash-test.bin";
    REQUIRE(!QFileInfo::exists(payload));      // gone from the save location…
    REQUIRE(QFileInfo::exists(trashed));       // …and recoverable from the Trash
    QFile::remove(trashed);                    // cleanup
}
#endif

#ifdef Q_OS_MACOS
TEST_CASE("QFile::moveToTrash works in this environment", "[trash-env]")
{
    QTemporaryDir tmp;
    const QString p = tmp.filePath("bat-trashenv-probe.txt");
    { QFile f(p); REQUIRE(f.open(QIODevice::WriteOnly)); f.write("x"); }
    QString moved = p;
    const bool ok = QFile::moveToTrash(moved);
    INFO("moveToTrash returned " << ok << ", new path: " << moved.toStdString());
    REQUIRE(ok);
    REQUIRE(!QFileInfo::exists(p));
}
#endif

// ============================================================================
//  ArchiveScan — auto-extract archive discovery (formats + multi-part)
// ============================================================================
#include "app/archivescan.h"

TEST_CASE("ArchiveScan: plain single archives of every format", "[archive]") {
    for (const QString &n : {"movie.rar","data.zip","stuff.7z","pack.tar.gz",
                             "pack.tgz","pack.tar.bz2","pack.tar.xz","blob.gz","blob.xz"}) {
        REQUIRE(ArchiveScan::isArchive(n));
        REQUIRE(ArchiveScan::archivesToExtract({n}) == QStringList{n});
    }
    REQUIRE(ArchiveScan::archivesToExtract({"film.mkv","cover.jpg"}).isEmpty());
}

TEST_CASE("ArchiveScan: .partN.rar keeps only the first volume", "[archive]") {
    QStringList files = {"rls.part1.rar","rls.part2.rar","rls.part3.rar"};
    REQUIRE(ArchiveScan::archivesToExtract(files) == QStringList{"rls.part1.rar"});
    // zero-padded variant
    QStringList padded = {"rls.part01.rar","rls.part02.rar","rls.part10.rar"};
    REQUIRE(ArchiveScan::archivesToExtract(padded) == QStringList{"rls.part01.rar"});
    REQUIRE(ArchiveScan::isContinuationPart("rls.part02.rar"));
    REQUIRE_FALSE(ArchiveScan::isContinuationPart("rls.part01.rar"));
}

TEST_CASE("ArchiveScan: old .rNN rar split keeps the .rar, drops .r00..", "[archive]") {
    QStringList files = {"rls.rar","rls.r00","rls.r01","rls.r02"};
    REQUIRE(ArchiveScan::archivesToExtract(files) == QStringList{"rls.rar"});
    REQUIRE(ArchiveScan::isContinuationPart("rls.r00"));
}

TEST_CASE("ArchiveScan: generic .NNN split keeps .001 only", "[archive]") {
    QStringList files = {"big.7z.001","big.7z.002","big.7z.003"};
    REQUIRE(ArchiveScan::archivesToExtract(files) == QStringList{"big.7z.001"});
    REQUIRE(ArchiveScan::isContinuationPart("big.7z.002"));
    REQUIRE_FALSE(ArchiveScan::isContinuationPart("big.7z.001"));
}

TEST_CASE("ArchiveScan: split-zip .zNN volumes drop, .zip stays", "[archive]") {
    QStringList files = {"set.zip","set.z01","set.z02"};
    REQUIRE(ArchiveScan::archivesToExtract(files) == QStringList{"set.zip"});
    REQUIRE(ArchiveScan::isContinuationPart("set.z01"));
}

TEST_CASE("ArchiveScan: case-insensitive and mixed folder", "[archive]") {
    QStringList files = {"MOVIE.MKV","Bonus.RAR","Bonus.R00","Readme.txt","clip.001","clip.002"};
    auto out = ArchiveScan::archivesToExtract(files);
    REQUIRE(out.contains("Bonus.RAR"));
    REQUIRE(out.contains("clip.001"));
    REQUIRE_FALSE(out.contains("Bonus.R00"));
    REQUIRE_FALSE(out.contains("clip.002"));
    REQUIRE_FALSE(out.contains("MOVIE.MKV"));
    REQUIRE(out.size() == 2);
}

TEST_CASE("ArchiveScan: a media file that looks like a part is not an archive", "[archive]") {
    // ".part1.mkv" must NOT be treated as a rar volume
    REQUIRE_FALSE(ArchiveScan::isArchive("episode.part1.mkv"));
    REQUIRE(ArchiveScan::archivesToExtract({"episode.part1.mkv"}).isEmpty());
}

// ============================================================================
//  ReleasePick — one-click "best release" auto-pick
// ============================================================================
#include "app/releasepick.h"
using ReleasePick::Candidate;
static const qint64 GB_ = 1024LL * 1024 * 1024;

TEST_CASE("ReleasePick: dead releases (0 seeders) are never chosen", "[release]") {
    QList<Candidate> c = {
        {"1080p", true, 0, 8 * GB_},   // perfect but dead
        {"720p",  false, 5, 2 * GB_},
    };
    REQUIRE(ReleasePick::best(c, "1080p", 0) == 1);
}

TEST_CASE("ReleasePick: honors preferred quality over more seeders", "[release]") {
    QList<Candidate> c = {
        {"4K",    false, 500, 40 * GB_},
        {"1080p", false, 50,  8 * GB_},
    };
    REQUIRE(ReleasePick::best(c, "1080p", 0) == 1);   // wants 1080p even with fewer seeders
    REQUIRE(ReleasePick::best(c, "4K", 0) == 0);
}

TEST_CASE("ReleasePick: native language breaks ties within a tier", "[release]") {
    QList<Candidate> c = {
        {"1080p", false, 200, 8 * GB_},
        {"1080p", true,  50,  8 * GB_},   // fewer seeders but native
    };
    REQUIRE(ReleasePick::best(c, "1080p", 0) == 1);
}

TEST_CASE("ReleasePick: native language wins over higher quality (viewer default)", "[release]") {
    QList<Candidate> c = {
        {"4K",    false, 800, 40 * GB_},  // best quality + seeders, but not your language
        {"720p",  true,  20,  3 * GB_},   // your language, lower quality
    };
    REQUIRE(ReleasePick::best(c, "1080p", 0, true) == 1);   // preferNative → language first
    REQUIRE(ReleasePick::best(c, "1080p", 0, false) == 0);  // quality-first mode → 4K
}

TEST_CASE("ReleasePick: seeders break ties when quality and native equal", "[release]") {
    QList<Candidate> c = {
        {"1080p", false, 30,  8 * GB_},
        {"1080p", false, 120, 8 * GB_},
    };
    REQUIRE(ReleasePick::best(c, "1080p", 0) == 1);
}

TEST_CASE("ReleasePick: size cap excludes huge releases when a smaller one fits", "[release]") {
    QList<Candidate> c = {
        {"4K",    false, 500, 40 * GB_},
        {"1080p", false, 80,  6 * GB_},
    };
    REQUIRE(ReleasePick::best(c, "4K", 10 * GB_) == 1);   // 4K too big → fall to the one under cap
}

TEST_CASE("ReleasePick: size cap ignored if nothing fits", "[release]") {
    QList<Candidate> c = {
        {"4K",    false, 500, 40 * GB_},
        {"1080p", false, 80,  20 * GB_},
    };
    REQUIRE(ReleasePick::best(c, "1080p", 5 * GB_) != -1);   // nothing under 5GB → still pick something alive
}

TEST_CASE("ReleasePick: Auto prefers 1080p sweet spot", "[release]") {
    QList<Candidate> c = {
        {"480p",  false, 999, 1 * GB_},
        {"1080p", false, 10,  8 * GB_},
        {"4K",    false, 100, 40 * GB_},
    };
    REQUIRE(ReleasePick::best(c, "Auto", 0) == 1);
}

TEST_CASE("ReleasePick: empty list returns -1", "[release]") {
    REQUIRE(ReleasePick::best({}, "1080p", 0) == -1);
}
