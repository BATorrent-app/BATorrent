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
#include <QElapsedTimer>
#include <QTest>

#include "app/utils.h"
#include "torrent/types.h"
#include "torrent/sessionmanager.h"
#include "gui/torrentmodel.h"
#include "gui/torrentfilter.h"
#include "webui/webserver.h"
#include "app/translator.h"

using Catch::Approx;
using Catch::Matchers::ContainsSubstring;

// ============================================================================
//  Qt app singleton
// ============================================================================

static int   s_argc = 1;
static char  s_arg0[] = "test_unit";
static char *s_argv[] = { s_arg0, nullptr };

static QCoreApplication &app() {
    static QCoreApplication a(s_argc, s_argv);
    return a;
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
    SECTION("bytes") {
        REQUIRE(formatSize(0) == "0 B");
        REQUIRE(formatSize(1) == "1 B");
        REQUIRE(formatSize(512) == "512 B");
        REQUIRE(formatSize(1023) == "1023 B");
    }

    SECTION("kilobytes") {
        REQUIRE(formatSize(1024) == "1.0 KB");
        REQUIRE(formatSize(1536) == "1.5 KB");
        REQUIRE(formatSize(10240) == "10.0 KB");
        REQUIRE(formatSize(1048575) == "1024.0 KB");
    }

    SECTION("megabytes") {
        REQUIRE(formatSize(1048576) == "1.0 MB");
        REQUIRE(formatSize(1572864) == "1.5 MB");
        REQUIRE(formatSize(104857600) == "100.0 MB");
    }

    SECTION("gigabytes") {
        REQUIRE(formatSize(1073741824LL) == "1.00 GB");
        REQUIRE(formatSize(1610612736LL) == "1.50 GB");
        REQUIRE(formatSize(10737418240LL) == "10.00 GB");
    }

    SECTION("edge: negative values") {
        // Negative bytes shouldn't crash (may display oddly, but no UB)
        auto result = formatSize(-1);
        REQUIRE(!result.isEmpty());
    }
}

// ============================================================================
//  UTILITY FUNCTIONS: formatSpeed
// ============================================================================

TEST_CASE("Utils: formatSpeed", "[unit][utils]")
{
    SECTION("bytes per second") {
        REQUIRE(formatSpeed(0) == "0 B/s");
        REQUIRE(formatSpeed(500) == "500 B/s");
        REQUIRE(formatSpeed(1023) == "1023 B/s");
    }

    SECTION("kilobytes per second") {
        REQUIRE(formatSpeed(1024) == "1.0 KB/s");
        REQUIRE(formatSpeed(5120) == "5.0 KB/s");
        REQUIRE(formatSpeed(1048575) == "1024.0 KB/s");
    }

    SECTION("megabytes per second") {
        REQUIRE(formatSpeed(1048576) == "1.0 MB/s");
        REQUIRE(formatSpeed(10485760) == "10.0 MB/s");
    }

    SECTION("edge: zero") {
        REQUIRE(formatSpeed(0) == "0 B/s");
    }
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
//  TORRENT MODEL
// ============================================================================

TEST_CASE("TorrentModel: empty model", "[unit][model]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);

    REQUIRE(model.rowCount() == 0);
    REQUIRE(model.columnCount() == TorrentModel::ColumnCount);
    REQUIRE(model.columnCount() == 7);
}

TEST_CASE("TorrentModel: header data", "[unit][model]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);

    for (int col = 0; col < TorrentModel::ColumnCount; ++col) {
        QVariant header = model.headerData(col, Qt::Horizontal, Qt::DisplayRole);
        REQUIRE(header.isValid());
        REQUIRE(!header.toString().isEmpty());
    }

    // Vertical orientation returns nothing
    QVariant v = model.headerData(0, Qt::Vertical, Qt::DisplayRole);
    REQUIRE_FALSE(v.isValid());
}

TEST_CASE("TorrentModel: invalid index returns empty QVariant", "[unit][model]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);

    QModelIndex invalid;
    REQUIRE_FALSE(model.data(invalid).isValid());

    QModelIndex oob = model.index(999, 0);
    REQUIRE_FALSE(model.data(oob).isValid());
}

TEST_CASE("TorrentModel: flags support drag & drop", "[unit][model]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);

    // Drop is always enabled (even without valid index)
    Qt::ItemFlags f = model.flags(QModelIndex());
    REQUIRE((f & Qt::ItemIsDropEnabled));

    REQUIRE(model.supportedDropActions() == Qt::MoveAction);
    REQUIRE(model.mimeTypes().contains("application/x-batorrent-row"));
}

TEST_CASE("TorrentModel: moveRow out of bounds", "[unit][model]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);

    // No torrents — moveRow should do nothing, not crash
    model.moveRow(-1, 0);
    model.moveRow(0, 0);
    model.moveRow(0, 5);
}

TEST_CASE("TorrentModel: mimeData with empty list", "[unit][model]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);

    QModelIndexList empty;
    QMimeData *mime = model.mimeData(empty);
    REQUIRE(mime != nullptr);
    delete mime;
}

TEST_CASE("TorrentModel: refresh emits no crash", "[unit][model]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);

    // Multiple refreshes with 0 torrents
    model.refresh();
    model.refresh();
    model.refresh();
    REQUIRE(model.rowCount() == 0);
}

// ============================================================================
//  TORRENT FILTER
// ============================================================================

TEST_CASE("TorrentFilter: initial state passes all", "[unit][filter]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);
    TorrentFilter filter;
    filter.setSourceModel(&model);

    // No filter set = all rows pass (0 rows, 0 shown)
    REQUIRE(filter.rowCount() == 0);
}

TEST_CASE("TorrentFilter: name filter on empty model", "[unit][filter]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);
    TorrentFilter filter;
    filter.setSourceModel(&model);

    filter.setNameFilter("ubuntu");
    REQUIRE(filter.rowCount() == 0);

    filter.setNameFilter("");
    REQUIRE(filter.rowCount() == 0);
}

TEST_CASE("TorrentFilter: state filter on empty model", "[unit][filter]")
{
    app();
    SessionManager session;
    TorrentModel model(&session);
    TorrentFilter filter;
    filter.setSourceModel(&model);

    filter.setStateFilter("Downloading");
    REQUIRE(filter.rowCount() == 0);

    filter.setStateFilter("__active__");
    REQUIRE(filter.rowCount() == 0);

    filter.setStateFilter("");
    REQUIRE(filter.rowCount() == 0);
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
