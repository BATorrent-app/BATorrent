// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — speed sampling / aggregate stats / palette. Split out of
// qmlsessionbridge.cpp verbatim; no behaviour change.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/platform/utils.h"
#include <QCoreApplication>

#include <QVariantList>
#include <QVector>
#include <QSet>
#include <QSettings>
#include <QDateTime>
#include <QLocale>
#include "services/platform/translator.h"
#include <QProcess>
#include <QGuiApplication>
#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <csignal>
#include <cerrno>
#endif

static QVariantList intsToVariant(const QVector<int> &v)
{
    QVariantList out;
    out.reserve(v.size());
    for (int x : v) out << x;
    return out;
}

void QmlSessionBridge::sampleSpeeds()
{
    int dl = 0, ul = 0;
    QSet<QString> live;
    for (int i = 0; i < m_session->torrentCount(); ++i) {
        auto info = m_session->torrentAt(i);
        dl += info.downloadRate;
        ul += info.uploadRate;

        const QString h = m_session->torrentHashAt(i);
        if (h.isEmpty()) continue;
        live.insert(h);
        QVector<int> &dh = m_torrentDownHist[h];
        QVector<int> &uh = m_torrentUpHist[h];
        dh.append(info.downloadRate);
        uh.append(info.uploadRate);
        while (dh.size() > HistoryMaxPoints) dh.removeFirst();
        while (uh.size() > HistoryMaxPoints) uh.removeFirst();
    }
    // forget history for torrents that are gone (no leak across a session)
    for (auto it = m_torrentDownHist.begin(); it != m_torrentDownHist.end(); ) {
        if (!live.contains(it.key())) { m_torrentUpHist.remove(it.key()); it = m_torrentDownHist.erase(it); }
        else ++it;
    }

    m_downloadHistory.append(dl);
    m_uploadHistory.append(ul);
    while (m_downloadHistory.size() > HistoryMaxPoints) m_downloadHistory.removeFirst();
    while (m_uploadHistory.size() > HistoryMaxPoints) m_uploadHistory.removeFirst();
    emit historyChanged();
}

QVariantList QmlSessionBridge::selectedDownHistory() const
{
    if (!hasSelection()) return {};
    const QString h = m_session->torrentHashAt(m_selectedIndex);   // FULL hash (selectedHash() is abbreviated for display)
    const auto it = m_torrentDownHist.constFind(h);
    return it == m_torrentDownHist.constEnd() ? QVariantList() : intsToVariant(*it);
}

QVariantList QmlSessionBridge::selectedUpHistory() const
{
    if (!hasSelection()) return {};
    const QString h = m_session->torrentHashAt(m_selectedIndex);
    const auto it = m_torrentUpHist.constFind(h);
    return it == m_torrentUpHist.constEnd() ? QVariantList() : intsToVariant(*it);
}

QVariantList QmlSessionBridge::downloadHistory() const
{
    QVariantList out;
    out.reserve(m_downloadHistory.size());
    for (int v : m_downloadHistory) out << v;
    return out;
}

QVariantList QmlSessionBridge::uploadHistory() const
{
    QVariantList out;
    out.reserve(m_uploadHistory.size());
    for (int v : m_uploadHistory) out << v;
    return out;
}

int QmlSessionBridge::torrentCount() const { return m_session->torrentCount(); }

int QmlSessionBridge::activeCount() const      { return m_activeCount; }
int QmlSessionBridge::downloadingCount() const { return m_downloadingCount; }
int QmlSessionBridge::seedingCount() const     { return m_seedingCount; }
int QmlSessionBridge::pausedCount() const      { return m_pausedCount; }
int QmlSessionBridge::completedCount() const   { return m_completedCount; }
int QmlSessionBridge::queuedCount() const      { return m_queuedCount; }
QString QmlSessionBridge::totalDownSpeed() const { return formatSpeed(m_totalDownRate); }
QString QmlSessionBridge::totalUpSpeed() const   { return formatSpeed(m_totalUpRate); }

QString QmlSessionBridge::totalDownloaded() const { return formatSize(m_session->globalDownloaded()); }
QString QmlSessionBridge::totalUploaded() const { return formatSize(m_session->globalUploaded()); }
QString QmlSessionBridge::globalRatio() const { return QString::number(m_session->globalRatio(), 'f', 2); }

QVariantList QmlSessionBridge::torrentPalette() const
{
    QVariantList out;
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i) {
        const TorrentInfo info = m_session->torrentAt(i);
        if (info.name.isEmpty()) continue;
        QVariantMap m;
        m["name"] = info.name;
        m["source"] = i;
        out << m;
    }
    return out;
}

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
        // Count off the same key the tiles colour themselves with. These two
        // used to re-derive state from `progress` here, so a torrent could be
        // counted under Downloading while its tile read SEEDING — and a
        // finished one stuck at 0.99999994 landed in Downloading forever.
        const QString state = torrentStateKey(info);
        if (state == QLatin1String("downloading")) { ++m_downloadingCount; m_anyDownloading = true; }
        else if (state == QLatin1String("seeding")) ++m_seedingCount;
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
