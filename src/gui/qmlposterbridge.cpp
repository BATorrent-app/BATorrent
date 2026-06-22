// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "qmlposterbridge.h"
#include <QSysInfo>
#include <QUrlQuery>
#include "../torrent/sessionmanager.h"
#include "../app/metadataresolver.h"
#include "../app/discoveryservice.h"
#include "../app/nameparser.h"
#include "../app/rssmanager.h"
#include "../app/addonmanager.h"
#include "../app/logger.h"
#include "../app/qrcodegen.h"
#include "../app/utils.h"
#include "../app/translator.h"
#include "../app/subtitlesearch.h"
#include "../app/geoip.h"
#include "../app/discordrpc.h"
#include "../app/updater.h"
#include "../app/notifier.h"
#include "../app/secretstore.h"
#include "../webui/webserver.h"
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDateTime>

#include <QNetworkInterface>

#include <QClipboard>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QApplication>
#include <QWindow>
#include <QEvent>
#ifdef Q_OS_WIN
#  include <windows.h>
#  include <dwmapi.h>
#  ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#    define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#  endif
#endif
#include <QCoreApplication>
#include <QStyleHints>
#include <QPainter>
#include <QSvgRenderer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <cstring>
#include <algorithm>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/version.hpp>
#include <openssl/opensslv.h>
#include <boost/version.hpp>
#include <memory>
#include <sstream>

QmlRssBridge::QmlRssBridge(QObject *parent) : QObject(parent)
{
    auto &rss = RssManager::instance();
    connect(&rss, &RssManager::feedAdded, this, [this]() { emit feedsChanged(); });
    connect(&rss, &RssManager::feedUpdated, this, [this]() { emit feedsChanged(); });
    connect(&rss, &RssManager::feedError, this, &QmlRssBridge::errorOccurred);
    connect(&rss, &RssManager::itemAutoDownloaded, this, &QmlRssBridge::autoDownloaded);
    rss.loadFeeds();
}

QVariantList QmlRssBridge::feeds() const
{
    QVariantList out;
    const auto feeds = RssManager::instance().feeds();
    for (int i = 0; i < feeds.size(); ++i) {
        const RssFeed &f = feeds[i];
        QVariantMap m;
        m["index"] = i;
        m["name"] = f.name.isEmpty() ? f.url : f.name;
        m["url"] = f.url;
        m["enabled"] = f.enabled;
        m["autoDownload"] = f.autoDownload;
        m["filterPattern"] = f.filterPattern;
        m["savePath"] = f.savePath;
        m["checkInterval"] = f.checkIntervalMin;
        m["lastChecked"] = f.lastChecked.isValid()
            ? f.lastChecked.toString("yyyy-MM-dd hh:mm") : QString();
        m["count"] = RssManager::instance().itemsForFeed(i).size();
        out << m;
    }
    return out;
}

QVariantList QmlRssBridge::itemsForFeed(int feedIndex) const
{
    QVariantList out;
    const auto items = RssManager::instance().itemsForFeed(feedIndex);
    for (const RssItem &it : items) {
        QVariantMap m;
        m["title"] = it.title;
        m["link"] = it.link;
        m["size"] = it.size > 0 ? formatSize(it.size) : QString();
        m["date"] = it.pubDate.isValid() ? it.pubDate.toString("yyyy-MM-dd hh:mm") : QString();
        m["downloaded"] = it.downloaded;
        out << m;
    }
    return out;
}

void QmlRssBridge::addFeed(const QString &url)
{
    if (url.trimmed().isEmpty()) return;
    RssManager::instance().addFeed(url.trimmed());
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::removeFeed(int index)
{
    RssManager::instance().removeFeed(index);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::setFeedEnabled(int index, bool enabled)
{
    RssManager::instance().setFeedEnabled(index, enabled);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::setAutoDownload(int index, bool on)
{
    auto feeds = RssManager::instance().feeds();
    if (index < 0 || index >= feeds.size()) return;
    RssFeed copy = feeds[index];
    copy.autoDownload = on;
    RssManager::instance().updateFeed(index, copy);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::checkAllFeeds()
{
    RssManager::instance().checkAllFeeds();
}

void QmlRssBridge::checkFeed(int index)
{
    RssManager::instance().checkFeed(index);
}

void QmlRssBridge::updateFeedSettings(int index, const QString &filterPattern,
                                      const QString &savePath, int checkInterval,
                                      bool enabled, bool autoDownload)
{
    auto feeds = RssManager::instance().feeds();
    if (index < 0 || index >= feeds.size()) return;
    RssFeed f = feeds[index];
    f.filterPattern = filterPattern;
    f.savePath = savePath.startsWith("file://") ? QUrl(savePath).toLocalFile() : savePath;
    f.checkIntervalMin = qBound(5, checkInterval, 1440);
    f.enabled = enabled;
    f.autoDownload = autoDownload;
    RssManager::instance().updateFeed(index, f);
    RssManager::instance().saveFeeds();
    emit feedsChanged();
}

void QmlRssBridge::downloadItem(int feedIndex, int itemIndex)
{
    RssManager::instance().downloadItem(feedIndex, itemIndex);
}

// ===================== QmlSearchBridge =====================

QmlAddonBridge::QmlAddonBridge(QObject *parent) : QObject(parent)
{
    auto &mgr = AddonManager::instance();
    connect(&mgr, &AddonManager::addonAdded, this, [this](const AddonManifest &){ emit changed(); });
    connect(&mgr, &AddonManager::addonError, this, [this](const QString &e){ emit error(e); });
    connect(&mgr, &AddonManager::trackerListUpdated, this, &QmlAddonBridge::changed);
}

QVariantList QmlAddonBridge::installed() const
{
    QVariantList out;
    const auto list = AddonManager::instance().addons();
    for (const auto &a : list) {
        QVariantMap m;
        m["name"] = a.name;
        m["description"] = a.description;
        m["url"] = a.url;
        m["types"] = a.types.join(", ");
        m["enabled"] = a.enabled;
        out << m;
    }
    return out;
}

QVariantList QmlAddonBridge::suggested() const
{
    struct S { const char *nm; const char *d; const char *url; };
    static const S items[] = {
        { "Cinemeta", "Catálogos oficiais de filmes e séries", "https://v3-cinemeta.strem.io" },
        { "Torrentio", "Streams de torrent para filmes e séries", "https://torrentio.strem.fun" },
    };
    QVariantList out;
    for (const auto &s : items) {
        QVariantMap m;
        m["name"] = QString::fromUtf8(s.nm);
        m["description"] = QString::fromUtf8(s.d);
        m["url"] = QString::fromUtf8(s.url);
        m["installed"] = isInstalled(QString::fromUtf8(s.url));
        out << m;
    }
    return out;
}

bool QmlAddonBridge::autoTrackers() const { return AddonManager::instance().autoTrackersEnabled(); }
void QmlAddonBridge::setAutoTrackers(bool on) { AddonManager::instance().setAutoTrackersEnabled(on); emit changed(); }
int QmlAddonBridge::trackerCount() const { return AddonManager::instance().trackerList().size(); }
bool QmlAddonBridge::torrentSearchEnabled() const { return AddonManager::instance().torrentSearchEnabled(); }
void QmlAddonBridge::setTorrentSearchEnabled(bool on) { AddonManager::instance().setTorrentSearchEnabled(on); emit changed(); }
QString QmlAddonBridge::torrentSearchUrl() const { return AddonManager::instance().torrentSearchUrl(); }
void QmlAddonBridge::setTorrentSearchUrl(const QString &url) { AddonManager::instance().setTorrentSearchUrl(url); emit changed(); }

void QmlAddonBridge::addAddon(const QString &url) { if (!url.isEmpty()) AddonManager::instance().addAddon(url); }
void QmlAddonBridge::removeAddon(int index) { AddonManager::instance().removeAddon(index); emit changed(); }
void QmlAddonBridge::setEnabled(int index, bool on) { AddonManager::instance().setAddonEnabled(index, on); emit changed(); }
void QmlAddonBridge::refreshTrackers() { AddonManager::instance().fetchTrackerList(); }

bool QmlAddonBridge::isInstalled(const QString &url) const
{
    const auto list = AddonManager::instance().addons();
    for (const auto &a : list)
        if (a.url == url) return true;
    return false;
}

// ===================== QmlPairingBridge =====================

QString QmlPairingBridge::detectLanIp()
{
    // Score IPv4 addresses by how "real LAN" they look. The old version keyed
    // off interface *names* ("en"/"wlan"), which don't exist on Windows — there
    // it fell through to the first non-primary interface, often a Radmin/Hamachi
    // VPN adapter (26.x / 25.x), so the QR pointed at the wrong network.
    QString best;
    int bestScore = -1;
    for (const auto &iface : QNetworkInterface::allInterfaces()) {
        const auto flags = iface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp)) continue;
        if (!flags.testFlag(QNetworkInterface::IsRunning)) continue;
        if (flags.testFlag(QNetworkInterface::IsLoopBack)) continue;
        if (flags.testFlag(QNetworkInterface::IsPointToPoint)) continue;   // most VPN tunnels
        for (const auto &entry : iface.addressEntries()) {
            if (entry.ip().protocol() != QAbstractSocket::IPv4Protocol) continue;
            const QString ip = entry.ip().toString();
            if (ip.startsWith("169.254.")) continue;                 // link-local
            if (ip.startsWith("25.") || ip.startsWith("26.")) continue; // Hamachi / Radmin VPN
            int score = 1;
            if      (ip.startsWith("192.168.")) score = 4;           // typical home LAN
            else if (ip.startsWith("10."))      score = 3;
            else if (ip.startsWith("172."))     score = 3;
            if (flags.testFlag(QNetworkInterface::CanBroadcast)) score += 1; // real NIC, not a tunnel
            if (score > bestScore) { bestScore = score; best = ip; }
        }
    }
    return best;
}

QmlPairingBridge::QmlPairingBridge(QObject *parent) : QObject(parent)
{
    refresh();
}

void QmlPairingBridge::refresh()
{
    const int port = QSettings().value(QStringLiteral("webUiPort"), 8080).toInt();   // same key the server binds
    const QString ip = detectLanIp();
    m_url = ip.isEmpty() ? QString() : QStringLiteral("http://%1:%2/").arg(ip).arg(port);
    emit changed();
}

void QmlPairingBridge::copyUrl()
{
    if (!m_url.isEmpty()) QGuiApplication::clipboard()->setText(m_url);
}

void QmlPairingBridge::copyText(const QString &t)
{
    if (!t.isEmpty()) QGuiApplication::clipboard()->setText(t);
}

void QmlPairingBridge::openBrowser()
{
    if (!m_url.isEmpty()) QDesktopServices::openUrl(QUrl(m_url));
}

QStringList QmlPairingBridge::qrRows() const { return qrRowsForUrl(m_url); }

// QR matrix for an arbitrary URL — lets the dialog encode the URL *with* the
// pairing credentials so the phone signs in by scanning, while the on-screen
// URL stays clean (no password shown).
QStringList QmlPairingBridge::qrRowsForUrl(const QString &url) const
{
    QStringList rows;
    if (url.isEmpty()) return rows;
    const qrgen::Matrix m = qrgen::encode(url);
    if (m.size == 0) return rows;
    for (int y = 0; y < m.size; ++y) {
        QString s;
        s.reserve(m.size);
        for (int x = 0; x < m.size; ++x)
            s += m.at(x, y) ? QLatin1Char('1') : QLatin1Char('0');
        rows << s;
    }
    return rows;
}

// ===================== QmlLogBridge =====================

// ===================== QmlSubtitleBridge =====================

QmlSubtitleBridge::QmlSubtitleBridge(SessionManager *session, QObject *parent)
    : QObject(parent), m_session(session), m_search(new SubtitleSearch(this))
{
    connect(m_search, &SubtitleSearch::resultsChanged, this, &QmlSubtitleBridge::resultsChanged);
    connect(m_search, &SubtitleSearch::searchFinished, this, [this]() {
        m_searching = false;
        emit searchingChanged();
    });
    connect(m_search, &SubtitleSearch::downloadFinished, this, &QmlSubtitleBridge::subtitleReady);
    connect(m_search, &SubtitleSearch::errorOccurred, this, &QmlSubtitleBridge::searchError);
}

bool QmlSubtitleBridge::hasOpenSubtitlesKey() const
{
    bool has = !QSettings("BATorrent", "BATorrent").value("osApiKey").toString().trimmed().isEmpty();
#ifdef BAT_OS_KEY
    has = true;
#endif
    return has;
}

QVariantList QmlSubtitleBridge::results() const
{
    QVariantList out;
    const auto rs = m_search->results();
    out.reserve(rs.size());
    for (const auto &r : rs) {
        QVariantMap m;
        m["provider"] = r.provider;
        m["name"] = r.name;
        m["language"] = r.language;
        out << m;
    }
    return out;
}

void QmlSubtitleBridge::searchFor(const QString &infoHash, int fileIndex, const QString &mediaTitle)
{
    QString video = m_session->streamFilePath(m_session->torrentIndexByInfoHash(infoHash), fileIndex);
    if (video.endsWith(QLatin1String(".!bt"))) video.chop(4);
    const QFileInfo vi(video);
    if (video.isEmpty()) {
        m_targetDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        m_baseName = mediaTitle;
    } else {
        m_targetDir = vi.dir().absolutePath();
        m_baseName = vi.completeBaseName();
    }
    // the filename carries S/E + release tags the parser feeds on; the resolved
    // display title doesn't
    const QString queryName = vi.fileName().isEmpty() ? mediaTitle : vi.fileName();
    static const char *codes[] = {"en", "pt", "zh", "ja", "ru", "es", "de", "uk"};
    QStringList langs{QString::fromLatin1(codes[static_cast<int>(Translator::instance().language())])};
    if (!langs.contains(QStringLiteral("en"))) langs << QStringLiteral("en");
    m_searching = true;
    emit searchingChanged();
    m_search->search(queryName, langs);
}

void QmlSubtitleBridge::download(int index)
{
    if (m_targetDir.isEmpty()) return;
    m_search->download(index, m_targetDir, m_baseName);
}

// ===================== QmlLogBridge =====================

QmlLogBridge::QmlLogBridge(QObject *parent) : QObject(parent)
{
    m_pollTimer.setInterval(1000);
    connect(&m_pollTimer, &QTimer::timeout, this, &QmlLogBridge::poll);
}

int QmlLogBridge::level() const { return int(Logger::instance().level()); }

void QmlLogBridge::setLevel(int l)
{
    Logger::instance().setLevel(Logger::Level(l));
    m_lastSize = 0;
    poll();
    emit levelChanged();
}

QStringList QmlLogBridge::levelNames() const
{
    // Critical omitted on purpose (matches the old combo's 5 entries).
    return { "Trace", "Debug", "Info", "Warning", "Error" };
}

QString QmlLogBridge::logsDir() const { return Logger::instance().logsDir(); }

void QmlLogBridge::start() { poll(); m_pollTimer.start(); }
void QmlLogBridge::stop() { m_pollTimer.stop(); }

void QmlLogBridge::poll()
{
    QFile f(Logger::instance().currentLogPath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    const qint64 size = f.size();
    if (size == m_lastSize) return;
    if (size < m_lastSize) { m_text.clear(); m_lastSize = 0; }  // rotated/cleared
    f.seek(m_lastSize);
    m_text += QString::fromUtf8(f.readAll());
    m_lastSize = size;
    // cap to ~20000 lines (replaces QPlainTextEdit::maximumBlockCount)
    int nl = m_text.count('\n');
    if (nl > 20000) {
        int drop = m_text.indexOf('\n', 0);
        while (nl-- > 20000 && drop >= 0) drop = m_text.indexOf('\n', drop + 1);
        if (drop > 0) m_text = m_text.mid(drop + 1);
    }
    emit textChanged();
}

void QmlLogBridge::clearLog()
{
    Logger::instance().clear();
    m_text.clear();
    m_lastSize = 0;
    emit textChanged();
}

void QmlLogBridge::openLogsFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(Logger::instance().logsDir()));
}

bool QmlLogBridge::exportLogs(const QString &filePath)
{
    QString path = filePath;
    if (path.startsWith("file://")) path = QUrl(path).toLocalFile();
    QFile out(path);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    out.write(Logger::instance().readAllLogs().toUtf8());
    out.close();
    return true;
}

bool QmlLogBridge::previousSessionCrashed() const
{
    return Logger::instance().previousSessionCrashed();
}

// Pre-filled GitHub issue (user reviews in the browser before sending —
// nothing is transmitted by the app itself)
QString QmlLogBridge::crashReportUrl() const
{
    const QString version = QCoreApplication::applicationVersion();
    const QString body = QStringLiteral(
        "**Version:** %1\n**OS:** %2\n\n"
        "**What were you doing when it happened?**\n\n(describe here)\n\n"
        "<details><summary>Log tail (auto-captured from the previous run)</summary>\n\n"
        "```\n%3\n```\n</details>\n")
        .arg(version, QSysInfo::prettyProductName(), Logger::instance().crashTail());
    QUrl url(QStringLiteral("https://github.com/BATorrent-app/BATorrent/issues/new"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("title"),
                   QStringLiteral("[crash] BATorrent %1 ended unexpectedly").arg(version));
    q.addQueryItem(QStringLiteral("labels"), QStringLiteral("bug"));
    q.addQueryItem(QStringLiteral("body"), body);
    url.setQuery(q);
    return url.toString(QUrl::FullyEncoded);
}

QString QmlLogBridge::defaultExportName() const
{
    const QString desktop = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    return desktop + "/batorrent-logs-"
        + QDateTime::currentDateTime().toString("yyyy-MM-dd-HHmmss") + ".txt";
}

// ===================== QmlSettingsBridge =====================

static void maybeBeep()
{
    if (QSettings().value(QStringLiteral("notifSound"), true).toBool())
        QApplication::beep();
}

void QmlNotificationBridge::onTorrentAdded(const QString &name)
{
    emit notify(tr_("notif_torrent_added"), name, 0);   // 0 = info
}

void QmlNotificationBridge::onTorrentFinished(const QString &name, const QString &)
{
    emit notify(tr_("dlg_download_complete"), name, 3);   // 3 = success (green)
    maybeBeep();
}

void QmlNotificationBridge::onTorrentError(const QString &message)
{
    emit notify(tr_("dlg_error"), message, 2);
}

void QmlNotificationBridge::onKillSwitchTriggered()
{
    emit notify(tr_("killswitch_title"), tr_("killswitch_triggered"), 1);
    maybeBeep();
}

void QmlNotificationBridge::onRssAutoDownloaded(const QString &feedName, const QString &itemTitle)
{
    emit notify(feedName, itemTitle, 0);
}

void QmlNotificationBridge::onSuspiciousFilesDetected(const QString &name, const QStringList &files)
{
    // level 1 = warning (amber), deliberately NOT level 2/error red — this is a
    // heads-up, not a verdict. Body: "<torrent> · file1, file2".
    emit notify(tr_("warn_suspicious_title"),
                tr_("warn_suspicious_body").arg(name, files.join(QStringLiteral(", "))), 1);
    maybeBeep();
}

// --- DiscordRpcBridge ---

DiscordRpcBridge::DiscordRpcBridge(SessionManager *session, QObject *parent)
    : QObject(parent), m_session(session)
{
    m_rpc = new DiscordRPC(this);
    m_sessionStart = QDateTime::currentSecsSinceEpoch();
    if (QSettings().value("discordRichPresence", true).toBool())
        m_rpc->setClientId(QStringLiteral("1508208411282640956"));

    connect(&m_timer, &QTimer::timeout, this, &DiscordRpcBridge::refresh);
    m_timer.start(15000);
    refresh();
}

void DiscordRpcBridge::refresh()
{
    if (!m_rpc || m_rpc->clientId().isEmpty()) return;
    if (!QSettings().value("discordEnabled", true).toBool()) {
        if (!m_lastActivityKey.isEmpty()) { m_rpc->clearActivity(); m_lastActivityKey.clear(); }
        return;
    }
    // Mirror MainWindow::refreshDiscordPresence: feature the fastest active
    // download; otherwise report seeding count; otherwise idle.
    int seeding = 0, featured = -1, featuredRate = 0;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        TorrentInfo info = m_session->torrentAt(i);
        if (info.paused || info.completed) continue;
        if (info.progress >= 1.0f) { ++seeding; continue; }
        if (info.downloadRate > featuredRate) {
            featuredRate = info.downloadRate;
            featured = i;
        }
    }
    QString details, state;
    if (featured >= 0) {
        TorrentInfo info = m_session->torrentAt(featured);
        details = info.name.left(64);
        state = QStringLiteral("%1% · ↓ %2")
            .arg(static_cast<int>(info.progress * 100))
            .arg(formatSpeed(info.downloadRate));
    } else if (seeding > 0) {
        details = tr_("discord_seeding").arg(seeding);
        state = tr_("discord_seeding_state");
    } else {
        details = tr_("discord_idle");
    }
    // Connected to torrentsUpdated (~1s); only push to Discord when the visible
    // presence actually changed — re-sending the same payload every second is
    // wasted JSON + socket writes (and Discord rate-limits presence anyway).
    const QString key = details + QLatin1Char('\x1f') + state;
    if (key == m_lastActivityKey) return;
    m_lastActivityKey = key;
    m_rpc->setActivity(details, state, m_sessionStart);
}

// --- QmlUpdaterBridge ---

QmlUpdaterBridge::QmlUpdaterBridge(QObject *parent)
    : QObject(parent), m_updater(new Updater(this))
{
    connect(m_updater, &Updater::updateAvailable, this,
            [this](const QString &v, const QString &url, const QString &asset) {
        QSettings s;
        if (s.value("skippedUpdateVersion").toString() == v && m_silent)
            return;   // user chose to skip this version on a silent check
        emit updateFound(v, url, asset);
    });
    connect(m_updater, &Updater::noUpdateAvailable, this,
            [this]() { emit noUpdate(m_silent); });
    connect(m_updater, &Updater::downloadProgress, this, [this](qint64 r, qint64 t) {
        emit progress(t > 0 ? static_cast<int>((r * 100) / t) : 0);
    });
    connect(m_updater, &Updater::updateReady, this, &QmlUpdaterBridge::ready);
    connect(m_updater, &Updater::errorOccurred, this,
            [this](const QString &msg) { emit failed(msg, m_silent); });
}

void QmlUpdaterBridge::check(bool silent)
{
    m_silent = silent;
    m_updater->checkForUpdate();
}

void QmlUpdaterBridge::downloadAndInstall(const QString &url, const QString &assetName)
{
    m_updater->downloadAndInstall(url, assetName);
}

void QmlUpdaterBridge::skipVersion(const QString &version)
{
    QSettings().setValue("skippedUpdateVersion", version);
}
