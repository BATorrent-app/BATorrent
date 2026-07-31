// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlsessionbridge.h"
#include "torrent/iengine.h"
#include "services/downloads/httpdownloadmanager.h"
#include <QThread>
#include <QStorageInfo>
#include "services/metadata/metadataresolver.h"
#include "services/discovery/discoveryservice.h"
#include "services/security/defender.h"
#include "services/metadata/nameparser.h"
#include "services/integrations/rssmanager.h"
#include "services/discovery/addonmanager.h"
#include "services/platform/utils.h"
#include "services/platform/translator.h"
#include "services/integrations/geoip.h"
#include "webui/webserver.h"
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
#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <csignal>
#include <cerrno>
#endif
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

QmlSessionBridge::QmlSessionBridge(IEngine *session, MetadataResolver *resolver, QObject *parent)
    : QObject(parent), m_session(session), m_resolver(resolver)
{
    m_sampleTimer.setInterval(1000);
    connect(&m_sampleTimer, &QTimer::timeout, this, &QmlSessionBridge::sampleSpeeds);
    m_sampleTimer.start();
    recomputeAggregates();   // so the pills show real counts before the first tick

    m_geoIp = new GeoIpResolver(this);
    // A big swarm resolves hundreds of peer IPs one-by-one; emitting on each one
    // rebuilt the whole peer list every time, and each rebuild re-queued lookups
    // — a feedback storm that lagged forever. Coalesce into ≤1 rebuild/sec.
    m_peerListThrottle.setSingleShot(true);
    m_peerListThrottle.setInterval(1000);
    connect(&m_peerListThrottle, &QTimer::timeout, this, [this]() {
        emit selectionChanged(); emit selectionListsChanged();
    });
    connect(m_geoIp, &GeoIpResolver::resolved, this, [this](const QString &, const QString &) {
        if (!m_peerListThrottle.isActive()) m_peerListThrottle.start();
    });

    if (m_resolver) {
        connect(m_resolver, &MetadataResolver::metadataReady, this,
                [this](const QString &infoHash) {
            if (!m_resolver->hasCached(infoHash)) return;
            auto meta = m_resolver->cached(infoHash);
            if (meta.valid && !meta.posterPath.isEmpty())
                emit previewPosterReady(infoHash, meta.posterPath);
        });
    }

    connect(m_session, &IEngine::altSpeedsActiveChanged,
            this, &QmlSessionBridge::altSpeedsActiveChanged);
    connect(m_session, &IEngine::portStatusChanged,
            this, &QmlSessionBridge::portStatusChanged);
    connect(m_session, &IEngine::torrentsUpdated,
            this, &QmlSessionBridge::onWatchTick);
    connect(m_session, &IEngine::extractionCompleted,
            this, &QmlSessionBridge::onExtractionCompleted);
    connect(m_session, &IEngine::torrentFinished,
            this, &QmlSessionBridge::onGameTorrentFinished);
}

bool QmlSessionBridge::altSpeedsActive() const { return m_session->altSpeedsActive(); }
int QmlSessionBridge::portStatus() const { return m_session->portStatus(); }
void QmlSessionBridge::setAltSpeedsActive(bool active) { m_session->setAltSpeedsActive(active); }




void QmlSessionBridge::setSelectedRows(const QList<int> &rows)
{
    m_selectedRows = rows;
    m_selectedIndex = rows.isEmpty() ? -1 : rows.last();
    emit selectionChanged(); emit selectionListsChanged();
}

bool QmlSessionBridge::selectByInfoHash(const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return false;
    setSelectedRows({row});
    return true;
}

QList<int> QmlSessionBridge::selectedRows() const { return m_selectedRows; }

void QmlSessionBridge::onTorrentRemoved(int index)
{
    QList<int> updated;
    for (int r : m_selectedRows) {
        if (r == index) continue;          // the selected torrent itself is gone
        updated << (r > index ? r - 1 : r); // rows after it shifted down by one
    }
    bool changed = updated != m_selectedRows;
    m_selectedRows = updated;
    if (m_selectedIndex == index)      { m_selectedIndex = -1; changed = true; }
    else if (m_selectedIndex > index)  { --m_selectedIndex;    changed = true; }
    if (changed) { emit selectionChanged(); emit selectionListsChanged(); }
}




// Playback/streaming (stream URLs, subtitles, play-by-hash, next-episode,
// watch-when-ready) lives in qmlsessionbridge_playback.cpp; the movie library +
// watchlist projections in qmlsessionbridge_library.cpp.


QVariantMap QmlSessionBridge::wrapped(int year) const
{
    return m_session->statsWrapped(year);
}

QVariantMap QmlSessionBridge::statistics() const
{
    QVariantMap m;
    const qint64 down = m_session->globalDownloaded();
    const qint64 up = m_session->globalUploaded();
    const qint64 sDown = m_session->sessionDownloaded();
    const qint64 sUp = m_session->sessionUploaded();
    m["totalDownloaded"] = formatSize(down);
    m["totalUploaded"] = formatSize(up);
    m["totalRatio"] = QString::number(m_session->globalRatio(), 'f', 3);
    m["torrentsAdded"] = m_session->totalTorrentsAdded();
    m["sessionDownloaded"] = formatSize(sDown);
    m["sessionUploaded"] = formatSize(sUp);
    m["sessionRatio"] = QString::number(sDown > 0 ? double(sUp) / double(sDown) : 0.0, 'f', 3);
    QSettings s;
    const qint64 startTime = s.value(QStringLiteral("sessionStartTime"), 0).toLongLong();
    const qint64 uptime = startTime > 0 ? QDateTime::currentSecsSinceEpoch() - startTime : 0;
    const int d = int(uptime / 86400), h = int((uptime % 86400) / 3600), mn = int((uptime % 3600) / 60);
    m["uptime"] = d > 0 ? QString("%1d %2h %3m").arg(d).arg(h).arg(mn)
                        : QString("%1h %2m").arg(h).arg(mn);
    return m;
}

QVariantMap QmlSessionBridge::diagnostics() const
{
    QVariantMap m;
    const int port = m_session->listenPort();
    const auto stats = m_session->detailedStats();
    const bool dht = m_session->dhtEnabled();

    m["listenOk"] = port > 0;
    m["listenText"] = port > 0 ? tr_("port_listening_on").arg(port)
                               : tr_("port_not_listening");
    m["dhtOk"] = dht;
    m["dhtText"] = dht ? tr_("port_dht_active").arg(stats.dhtNodes)
                       : tr_("port_dht_disabled");
    m["natOk"] = stats.hasIncomingConnections;
    m["natText"] = stats.hasIncomingConnections ? tr_("port_incoming_ok")
                                                : tr_("port_incoming_none");
    m["portOk"] = stats.peersCount > 0;
    m["portText"] = stats.peersCount > 0 ? tr_("port_peers_connected").arg(stats.peersCount)
                                         : tr_("port_unconfirmed");
    return m;
}

QVariantList QmlSessionBridge::recentlyRemoved() const
{
    QVariantList out;
    const auto entries = m_session->recentlyRemoved();
    for (const auto &e : entries) {
        QVariantMap m;
        m["hash"] = e.hash;
        m["name"] = e.name;
        m["size"] = formatSize(e.totalSize);
        m["when"] = QLocale::system().toString(
            QDateTime::fromSecsSinceEpoch(e.removedAt), QLocale::ShortFormat);
        out << m;
    }
    return out;
}

bool QmlSessionBridge::restoreRemoved(const QString &hash)
{
    if (hash.isEmpty()) return false;
    bool ok = m_session->restoreRemoved(hash);
    if (ok) emit queueRefreshNeeded();
    return ok;
}

void QmlSessionBridge::clearRemovedHistory()
{
    m_session->clearRemovedHistory();
}

QString QmlSessionBridge::suggestTorrentOutput(const QString &source) const
{
    QString s = source;
    if (s.startsWith("file://")) s = QUrl(s).toLocalFile();
    if (s.isEmpty()) return {};
    QFileInfo fi(s);
    return fi.absolutePath() + "/" + fi.fileName() + ".torrent";
}

QString QmlSessionBridge::createTorrent(const QVariantMap &opts)
{
    QString source = opts.value("source").toString();
    QString output = opts.value("output").toString();
    if (source.startsWith("file://")) source = QUrl(source).toLocalFile();
    if (output.startsWith("file://")) output = QUrl(output).toLocalFile();
    if (source.isEmpty() || output.isEmpty())
        return QStringLiteral("Informe origem e arquivo de saída.");

    const QString trackerText = opts.value("trackers").toString();
    const int pieceSize = opts.value("pieceSize").toInt();   // bytes, 0 = auto
    const QString comment = opts.value("comment").toString().trimmed();
    const bool priv = opts.value("priv").toBool();
    const bool startSeeding = opts.value("startSeeding").toBool();

    try {
        lt::file_storage fs;
        QFileInfo srcInfo(source);
        const QString parentDir = srcInfo.absolutePath();
        lt::add_files(fs, source.toStdString());
        if (fs.num_files() == 0)
            return tr_("create_no_files");

        lt::create_torrent ct(fs, pieceSize > 0 ? pieceSize : 0);

        const QStringList trackers = trackerText.split('\n', Qt::SkipEmptyParts);
        for (int i = 0; i < trackers.size(); ++i)
            ct.add_tracker(trackers[i].trimmed().toStdString(), i);

        if (!comment.isEmpty()) ct.set_comment(comment.toStdString().c_str());
        ct.set_creator("BATorrent");
        if (priv) ct.set_priv(true);

        lt::set_piece_hashes(ct, parentDir.toStdString());
        auto entry = ct.generate();

        std::vector<char> buf;
        lt::bencode(std::back_inserter(buf), entry);

        QFile outFile(output);
        if (!outFile.open(QIODevice::WriteOnly))
            return tr_("create_write_failed");
        outFile.write(buf.data(), static_cast<qsizetype>(buf.size()));
        outFile.close();

        if (startSeeding) {
            m_session->addTorrent(output, parentDir);
            emit queueRefreshNeeded();
        }
        return {};
    } catch (const std::exception &e) {
        return QString::fromStdString(e.what());
    }
}


bool QmlSessionBridge::hasSelection() const
{
    return m_selectedIndex >= 0 && m_selectedIndex < m_session->torrentCount();
}


void QmlSessionBridge::recomputeAggregates()
{
    m_activeCount = m_downloadingCount = m_seedingCount = 0;
    m_pausedCount = m_completedCount = m_queuedCount = 0;
    m_totalDownRate = m_totalUpRate = 0;
    m_anyDownloading = false;
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i) {
        const auto info = m_session->torrentAt(i);
        m_totalDownRate += info.downloadRate;
        m_totalUpRate   += info.uploadRate;
        if (info.downloadRate > 0 || info.uploadRate > 0) ++m_activeCount;
        // queued (paused by the download-queue cap) is its own bucket, so the
        // Paused pill/count and the Queued pill don't double-count each other
        if (info.queued) ++m_queuedCount;
        else if (info.paused) ++m_pausedCount;
        if (!info.paused && info.progress < 1.0f) { ++m_downloadingCount; m_anyDownloading = true; }
        if (!info.paused && info.progress >= 1.0f && !info.completed) ++m_seedingCount;
        if (info.completed) ++m_completedCount;
    }
}

void QmlSessionBridge::emitStats()
{
    recomputeAggregates();   // one library pass feeds every count/speed getter
    emit statsChanged();
    // NOT selectionListsChanged: this fires every ~1s and must not rebuild the
    // heavy per-selection lists (peers/files/trackers/pieces). The live scalar
    // selected* props still refresh via selectionChanged.
    emit selectionChanged();
    // Exception: keep the peer list live (speeds/progress) only while the Peers
    // tab is actually open — gated so it costs nothing the rest of the time.
    if (m_detailPeersActive) rebuildPeerCache();

    // Post-download-action arming: when a non-"do nothing" action is chosen and
    // at least one torrent exists, fire once the moment nothing is downloading
    // anymore. Re-arms when a new download starts, so each "drain" triggers at
    // most one countdown.
    if (QSettings().value(QStringLiteral("postDownloadAction"), 0).toInt() != 0) {
        if (m_anyDownloading) m_shutdownArmed = true;
        else if (m_shutdownArmed && m_session->torrentCount() > 0) { m_shutdownArmed = false; emit allDownloadsComplete(); }
    } else {
        m_shutdownArmed = false;
    }
}

// postDownloadAction indices — keep in sync with SettingsSchema.qml's
// "postDownloadAction" options list.
namespace {
enum PostDownloadAction {
    ActionNone = 0, ActionCloseApp, ActionLock, ActionSleep,
    ActionHibernate, ActionSignOut, ActionShutdown, ActionRestart
};
}

void QmlSessionBridge::performPostDownloadAction()
{
    const int action = QSettings().value(QStringLiteral("postDownloadAction"), ActionNone).toInt();
    if (action == ActionNone) return;
    m_session->saveResumeData();
    switch (action) {
    case ActionCloseApp:
        QCoreApplication::quit();
        return;
    case ActionLock:
#ifdef Q_OS_WIN
        QProcess::startDetached("rundll32.exe", {"user32.dll,LockWorkStation"});
#elif defined(Q_OS_MACOS)
        QProcess::startDetached("/System/Library/CoreServices/Menu Extras/User.menu/Contents/Resources/CGSession", {"-suspend"});
#else
        QProcess::startDetached("loginctl", {"lock-session"});
#endif
        return;
    case ActionSleep:
#ifdef Q_OS_WIN
        QProcess::startDetached("rundll32.exe", {"powrprof.dll,SetSuspendState", "0,1,0"});
#elif defined(Q_OS_MACOS)
        QProcess::startDetached("pmset", {"sleepnow"});
#else
        QProcess::startDetached("systemctl", {"suspend"});
#endif
        return;
    case ActionHibernate:
#ifdef Q_OS_WIN
        QProcess::startDetached("shutdown", {"/h"});
#elif defined(Q_OS_MACOS)
        QProcess::startDetached("pmset", {"sleepnow"});   // macOS has no separate user-triggered hibernate
#else
        QProcess::startDetached("systemctl", {"hibernate"});
#endif
        return;
    case ActionSignOut:
#ifdef Q_OS_WIN
        QProcess::startDetached("shutdown", {"/l"});
#elif defined(Q_OS_MACOS)
        QProcess::startDetached("osascript", {"-e", "tell application \"System Events\" to log out"});
#else
        QProcess::startDetached("loginctl", {"terminate-user", qgetenv("USER")});
#endif
        return;
    case ActionRestart:
#ifdef Q_OS_WIN
        QProcess::startDetached("shutdown", {"/r", "/t", "0"});
#elif defined(Q_OS_MACOS)
        QProcess::startDetached("osascript", {"-e", "tell app \"System Events\" to restart"});
#else
        QProcess::startDetached("shutdown", {"-r", "now"});
#endif
        QCoreApplication::quit();
        return;
    case ActionShutdown:
    default:
#ifdef Q_OS_WIN
        QProcess::startDetached("shutdown", {"/s", "/t", "0"});
#elif defined(Q_OS_MACOS)
        QProcess::startDetached("osascript", {"-e", "tell app \"System Events\" to shut down"});
#else
        QProcess::startDetached("shutdown", {"-h", "now"});
#endif
        QCoreApplication::quit();
        return;
    }
}

// Theme bridge

