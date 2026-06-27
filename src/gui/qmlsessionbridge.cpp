// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "qmlposterbridge.h"
#include "../app/subtitleparser.h"
#include <QStorageInfo>
#include "../torrent/sessionmanager.h"
#include "../app/metadataresolver.h"
#include "../app/discoveryservice.h"
#include "../app/defender.h"
#include "../app/nameparser.h"
#include "../app/installerprofile.h"
#include "../app/rssmanager.h"
#include "../app/addonmanager.h"
#include "../app/logger.h"
#include "../app/qrcodegen.h"
#include "../app/utils.h"
#include "../app/translator.h"
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

QmlSessionBridge::QmlSessionBridge(SessionManager *session, MetadataResolver *resolver, QObject *parent)
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

    connect(m_session, &SessionManager::altSpeedsActiveChanged,
            this, &QmlSessionBridge::altSpeedsActiveChanged);
    connect(m_session, &SessionManager::portStatusChanged,
            this, &QmlSessionBridge::portStatusChanged);
    connect(m_session, &SessionManager::torrentsUpdated,
            this, &QmlSessionBridge::onWatchTick);
    connect(m_session, &SessionManager::extractionCompleted,
            this, &QmlSessionBridge::onExtractionCompleted);
    connect(m_session, &SessionManager::torrentFinished,
            this, &QmlSessionBridge::onGameTorrentFinished);
}

bool QmlSessionBridge::altSpeedsActive() const { return m_session->altSpeedsActive(); }
int QmlSessionBridge::portStatus() const { return m_session->portStatus(); }
void QmlSessionBridge::setAltSpeedsActive(bool active) { m_session->setAltSpeedsActive(active); }

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

static QVariantList intsToVariant(const QVector<int> &v)
{
    QVariantList out;
    out.reserve(v.size());
    for (int x : v) out << x;
    return out;
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
QString QmlSessionBridge::totalDownSpeed() const { return formatSpeed(m_totalDownRate); }
QString QmlSessionBridge::totalUpSpeed() const   { return formatSpeed(m_totalUpRate); }

QString QmlSessionBridge::totalDownloaded() const { return formatSize(m_session->globalDownloaded()); }
QString QmlSessionBridge::totalUploaded() const { return formatSize(m_session->globalUploaded()); }
QString QmlSessionBridge::globalRatio() const { return QString::number(m_session->globalRatio(), 'f', 2); }

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

void QmlSessionBridge::pauseSelected()
{
    if (m_selectedRows.isEmpty()) {
        if (m_selectedIndex >= 0) m_session->pauseTorrent(m_selectedIndex);
        return;
    }
    for (int r : m_selectedRows) m_session->pauseTorrent(r);
}

void QmlSessionBridge::resumeSelected()
{
    if (m_selectedRows.isEmpty()) {
        if (m_selectedIndex >= 0) m_session->resumeTorrent(m_selectedIndex);
        return;
    }
    for (int r : m_selectedRows) m_session->resumeTorrent(r);
}

// Highest index first, so erasing earlier rows doesn't shift the ones we
// haven't removed yet. Both remove paths share this so they can't diverge.
void QmlSessionBridge::removeSelectedRows(bool deleteFiles, bool permanent)
{
    QList<int> rows = m_selectedRows.isEmpty()
        ? (m_selectedIndex >= 0 ? QList<int>{m_selectedIndex} : QList<int>{})
        : m_selectedRows;
    if (rows.isEmpty()) return;
    const int n = rows.size();
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int r : rows) m_session->removeTorrent(r, deleteFiles, permanent);
    m_selectedRows.clear();
    m_selectedIndex = -1;
    emit selectionChanged(); emit selectionListsChanged();
    // set the expectation: trashed files still occupy disk until the Trash is emptied
    if (deleteFiles)
        emit toast(permanent ? tr_("remove_deleted") : tr_("remove_trashed"),
                   n > 1 ? QString::number(n) : QString());
}
void QmlSessionBridge::removeSelectedWithFilesPermanent() { removeSelectedRows(true, true); }

void QmlSessionBridge::removeSelected()          { removeSelectedRows(false); }
void QmlSessionBridge::removeSelectedWithFiles() { removeSelectedRows(true); }

void QmlSessionBridge::pauseAll() { m_session->pauseAll(); }
void QmlSessionBridge::resumeAll() { m_session->resumeAll(); }

void QmlSessionBridge::copyMagnetLink()
{
    if (!hasSelection()) return;
    QString uri = m_session->torrentMagnetUri(m_selectedIndex);
    if (!uri.isEmpty()) QGuiApplication::clipboard()->setText(uri);
}

void QmlSessionBridge::copyInfoHash()
{
    if (!hasSelection()) return;
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (!hash.isEmpty()) QGuiApplication::clipboard()->setText(hash);
}

void QmlSessionBridge::openSaveFolder()
{
    // Same behavior as a double-click: reveal the torrent's own folder/file,
    // not the bare save_path. Was opening save_path directly (e.g. Downloads),
    // which is why right-click "open folder" diverged from double-click.
    openSelectedFile();
}

bool QmlSessionBridge::excludeTorrentFromDefender(int row)
{
    if (row < 0) return false;
    auto info = m_session->torrentAt(row);
    // Prefer the torrent's own content folder; fall back to the save root.
    QString dir = info.savePath;
    const QString contentDir = QDir(info.savePath).filePath(info.name);
    if (QDir(contentDir).exists()) dir = contentDir;
    const bool ok = Defender::addExclusion(dir);
    emit toast(ok ? tr_("defender_excluded_ok") : tr_("defender_excluded_fail"), info.name);
    return ok;
}

bool QmlSessionBridge::selectedHasArchives() const
{
    return hasSelection() && m_session->torrentHasArchives(m_selectedIndex);
}

bool QmlSessionBridge::selectedHasVideo() const
{
    return hasSelection() && m_session->torrentHasVideo(m_selectedIndex);
}

void QmlSessionBridge::extractSelected(const QString &password)
{
    if (!hasSelection()) return;
    m_session->extractTorrent(m_selectedIndex, password);
    emit toast(tr_("extract_started"), m_session->torrentAt(m_selectedIndex).name);
}

bool QmlSessionBridge::selectedForceStart() const
{
    return hasSelection() && m_session->isForceStart(m_selectedIndex);
}

bool QmlSessionBridge::selectedSuperSeeding() const
{
    return hasSelection() && m_session->isSuperSeeding(m_selectedIndex);
}

bool QmlSessionBridge::selectedCompleted() const
{
    return hasSelection() && m_session->torrentAt(m_selectedIndex).completed;
}

bool QmlSessionBridge::selectedPaused() const
{
    return hasSelection() && m_session->torrentAt(m_selectedIndex).paused;
}

void QmlSessionBridge::smartPaste()
{
    QString clip = QGuiApplication::clipboard()->text().trimmed();
    if (clip.isEmpty()) return;
    // Xunlei thunder:// links: base64 of "AA" + the real URL + "ZZ". Decode, then
    // fall through to the normal handling (it usually wraps a magnet or .torrent).
    if (clip.startsWith(QStringLiteral("thunder://"), Qt::CaseInsensitive)) {
        QString dec = QString::fromUtf8(
            QByteArray::fromBase64(clip.mid(10).toLatin1())).trimmed();
        if (dec.startsWith(QStringLiteral("AA"), Qt::CaseInsensitive)
            && dec.endsWith(QStringLiteral("ZZ"), Qt::CaseInsensitive))
            dec = dec.mid(2, dec.size() - 4);
        clip = dec.trimmed();
        if (clip.isEmpty()) return;
    }
    if (clip.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) {
        addMagnetUri(clip);
        return;
    }
    static const QRegularExpression hashRe(QStringLiteral("^[0-9a-fA-F]{40}$"));
    if (hashRe.match(clip).hasMatch()) {
        addMagnetUri(QStringLiteral("magnet:?xt=urn:btih:") + clip);
        return;
    }
    if (clip.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive)
        && (clip.startsWith(QStringLiteral("http"), Qt::CaseInsensitive)
            || clip.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))) {
        addTorrentFile(clip);
    }
}

void QmlSessionBridge::setSelectedForceStart(bool on)
{
    if (hasSelection()) m_session->setForceStart(m_selectedIndex, on);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::setSelectedSuperSeeding(bool on)
{
    if (hasSelection()) m_session->setSuperSeeding(m_selectedIndex, on);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::markSelectedCompleted()
{
    if (hasSelection()) m_session->markCompleted(m_selectedIndex);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::unmarkSelectedCompleted()
{
    if (hasSelection()) m_session->unmarkCompleted(m_selectedIndex);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::forceRecheckSelected()
{
    if (hasSelection()) m_session->forceRecheck(m_selectedIndex);
}

bool QmlSessionBridge::exportSelectedTorrent(const QString &destPath)
{
    return hasSelection() && m_session->exportTorrent(m_selectedIndex, destPath);
}

void QmlSessionBridge::forceReannounceSelected()
{
    if (hasSelection()) m_session->forceReannounce(m_selectedIndex);
}

void QmlSessionBridge::queueUpSelected()
{
    QList<int> rows = m_selectedRows.isEmpty()
        ? (m_selectedIndex >= 0 ? QList<int>{m_selectedIndex} : QList<int>{})
        : m_selectedRows;
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end());
    QList<int> newRows;
    for (int r : rows) {
        if (r > 0 && !newRows.contains(r - 1)) {
            m_session->setTorrentQueuePosition(r, r - 1);
            emit queueMoved(r, r - 1);
            newRows << (r - 1);
        } else {
            newRows << r;
        }
    }
    m_selectedRows = newRows;
    m_selectedIndex = newRows.isEmpty() ? -1 : newRows.last();
    // queueMoved already drives QmlPosterModel::moveRow (a real beginMoveRows),
    // so the rows slide smoothly. A queueRefreshNeeded here would re-emit
    // dataChanged for the whole list and reload every cover → full-screen flash.
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::queueDownSelected()
{
    QList<int> rows = m_selectedRows.isEmpty()
        ? (m_selectedIndex >= 0 ? QList<int>{m_selectedIndex} : QList<int>{})
        : m_selectedRows;
    if (rows.isEmpty()) return;
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    int lastIdx = m_session->torrentCount() - 1;
    QList<int> newRows;
    for (int r : rows) {
        if (r < lastIdx && !newRows.contains(r + 1)) {
            m_session->setTorrentQueuePosition(r, r + 1);
            emit queueMoved(r, r + 1);
            newRows << (r + 1);
        } else {
            newRows << r;
        }
    }
    m_selectedRows = newRows;
    m_selectedIndex = newRows.isEmpty() ? -1 : newRows.first();
    // see queueUpSelected: moveRow handles the visual move; no full refresh.
    emit selectionChanged(); emit selectionListsChanged();
}

// resolve the active rows (multi-select, falling back to the focus index)
static QList<int> resolveRows(const QList<int> &rows, int idx)
{
    if (!rows.isEmpty()) return rows;
    return idx >= 0 ? QList<int>{idx} : QList<int>{};
}

void QmlSessionBridge::queueTopSelected()
{
    QList<int> rows = resolveRows(m_selectedRows, m_selectedIndex);
    if (rows.size() == 1 && rows.first() > 0) {
        int r = rows.first();
        m_session->setTorrentQueuePosition(r, 0);
        emit queueMoved(r, 0);                 // smooth move, no flash
        m_selectedRows = {0};
        m_selectedIndex = 0;
    } else {
        for (int r : rows) m_session->setTorrentQueuePosition(r, 0);
        emit queueRefreshNeeded();
    }
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::queueBottomSelected()
{
    const int last = m_session->torrentCount() - 1;
    QList<int> rows = resolveRows(m_selectedRows, m_selectedIndex);
    if (rows.size() == 1 && rows.first() < last) {
        int r = rows.first();
        m_session->setTorrentQueuePosition(r, last);
        emit queueMoved(r, last);              // smooth move, no flash
        m_selectedRows = {last};
        m_selectedIndex = last;
    } else {
        for (int r : rows) m_session->setTorrentQueuePosition(r, last);
        emit queueRefreshNeeded();
    }
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::stopSeedingSelected()
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->stopSeedingTorrent(r);
    emit selectionChanged(); emit selectionListsChanged();
}

QString QmlSessionBridge::urlToLocalPath(const QString &url) const
{
    if (url.startsWith(QStringLiteral("file:")))
        return QUrl(url).toLocalFile();
    return url;
}

void QmlSessionBridge::moveSelectedStorage(const QString &path)
{
    if (path.isEmpty()) return;
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->moveStorage(r, path);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::setSelectedDownloadLimit(int kbps)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentDownloadLimit(r, kbps);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::setSelectedUploadLimit(int kbps)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentUploadLimit(r, kbps);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::setSelectedSequential(bool on)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setSequentialDownload(r, on);
    emit selectionChanged(); emit selectionListsChanged();
}

bool QmlSessionBridge::selectedSequential() const
{
    return hasSelection() && m_session->isSequentialDownload(m_selectedIndex);
}

void QmlSessionBridge::setSelectedStopAfter(int mode)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentStopAfterDownload(r, mode);
    emit selectionChanged(); emit selectionListsChanged();
}

int QmlSessionBridge::selectedStopAfter() const
{
    return hasSelection() ? m_session->torrentStopAfterDownload(m_selectedIndex) : -1;
}

void QmlSessionBridge::setSelectedMaxSeedDays(int days)
{
    const qint64 secs = days < 0 ? -1 : qint64(days) * 86400;
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentMaxSeedSeconds(r, secs);
    emit selectionChanged(); emit selectionListsChanged();
}

int QmlSessionBridge::selectedMaxSeedDays() const
{
    if (!hasSelection()) return -1;
    const qint64 s = m_session->torrentMaxSeedSeconds(m_selectedIndex);
    return s < 0 ? -1 : int(s / 86400);
}

void QmlSessionBridge::renameSelected(const QString &name)
{
    if (hasSelection() && !name.trimmed().isEmpty())
        m_session->renameFile(m_selectedIndex, 0, name.trimmed());
    emit selectionChanged(); emit selectionListsChanged();
}

QString QmlSessionBridge::diagnoseSelectedSlow() const
{
    if (!hasSelection()) return QString();
    TorrentInfo info = m_session->torrentAt(m_selectedIndex);
    QStringList lines;
    if (info.paused) lines << "★ " + tr_("diag_paused");
    else if (info.completed) lines << "★ " + tr_("diag_completed");
    else if (info.progress >= 1.0f)
        lines << "★ " + tr_(info.uploadRate == 0 ? "diag_seeding_no_uploaders" : "diag_seeding_ok");
    else if (info.numPeers == 0) lines << "★ " + tr_("diag_no_peers");
    else if (info.numSeeds == 0) lines << "★ " + tr_("diag_no_seeds");
    else if (info.downloadRate == 0) lines << "★ " + tr_("diag_choked");
    else {
        const int dlimit = m_session->torrentDownloadLimit(m_selectedIndex);
        if (dlimit > 0 && info.downloadRate >= dlimit * 1024 * 0.9)
            lines << "★ " + tr_("diag_at_local_limit").arg(dlimit);
        else
            lines << "★ " + tr_("diag_throughput_normal").arg(formatSpeed(info.downloadRate));
    }
    lines << "" << tr_("diag_facts");
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_peers"), QString::number(info.numPeers));
    lines << QStringLiteral("    • %1: %2").arg(tr_("detail_seeds"), QString::number(info.numSeeds));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_down"), formatSpeed(info.downloadRate));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_up"), formatSpeed(info.uploadRate));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_state"), info.stateString);
    return lines.join('\n');
}

// Prefer a content-sniffing player (VLC/mpv/IINA) — they play a still-downloading
// or ".!bt"-suffixed file that the OS default handler would refuse — then fall back.
static bool launchMediaPlayer(const QString &path)
{
#if defined(Q_OS_MACOS)
    for (const QString &app : {QStringLiteral("VLC"), QStringLiteral("IINA"),
                               QStringLiteral("mpv"), QStringLiteral("QuickTime Player")})
        if (QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-a"), app, path}))
            return true;
    return QProcess::startDetached(QStringLiteral("open"), {path});
#elif defined(Q_OS_WIN)
    static const QStringList exes = {
        QStringLiteral("C:/Program Files/VideoLAN/VLC/vlc.exe"),
        QStringLiteral("C:/Program Files (x86)/VideoLAN/VLC/vlc.exe"),
        QStringLiteral("C:/Program Files/mpv/mpv.exe")};
    for (const QString &exe : exes)
        if (QFile::exists(exe) && QProcess::startDetached(exe, {path}))
            return true;
    for (const QString &cmd : {QStringLiteral("vlc"), QStringLiteral("mpv")})
        if (QProcess::startDetached(cmd, {path})) return true;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#else
    for (const QString &cmd : {QStringLiteral("vlc"), QStringLiteral("mpv"),
                               QStringLiteral("celluloid")})
        if (QProcess::startDetached(cmd, {path})) return true;
    return QDesktopServices::openUrl(QUrl::fromLocalFile(path));
#endif
}

void QmlSessionBridge::streamSelected()
{
    if (!hasSelection()) return;
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    const int row = m_selectedIndex;
    auto files = m_session->filesAt(row);
    TorrentInfo info = m_session->torrentAt(row);
    int bestIdx = -1; qint64 bestSize = 0;
    auto stripBt = [](const QString &p){ return p.endsWith(QStringLiteral(".!bt")) ? p.chopped(4) : p; };
    for (int i = 0; i < int(files.size()); ++i) {
        const QString mp = stripBt(files[i].path);
        for (const auto &ext : videoExts)
            if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                if (files[i].size > bestSize) { bestSize = files[i].size; bestIdx = i; }
                break;
            }
    }
    if (bestIdx < 0) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }

    m_session->resumeTorrent(row);   // a paused torrent would never buffer
    m_session->setSequentialDownload(row, true);
    for (int i = 0; i < int(files.size()); ++i)
        if (i != bestIdx) m_session->setFilePriority(row, i, 0);
    m_session->setFilePriority(row, bestIdx, 7);
    m_session->prioritizeFilePieceBoundaries(row, bestIdx);

    m_streamIndex = row; m_streamFileIdx = bestIdx;
    m_streamFilePath = stripBt(info.savePath + "/" + files[bestIdx].path);   // nice name, no ".!bt"
    m_streamTries = 0;

    if (!m_streamTimer) { m_streamTimer = new QTimer(this); m_streamTimer->setInterval(2000); }
    QObject::disconnect(m_streamTimer, nullptr, nullptr, nullptr);
    connect(m_streamTimer, &QTimer::timeout, this, [this, stripBt]() {
        if (m_streamIndex < 0 || m_streamIndex >= m_session->torrentCount()) { m_streamTimer->stop(); return; }
        auto files = m_session->filesAt(m_streamIndex);
        if (m_streamFileIdx >= int(files.size())) { m_streamTimer->stop(); return; }
        TorrentInfo cur = m_session->torrentAt(m_streamIndex);
        // Row may have shifted to a different torrent (list reordered/removed) — bail.
        if (stripBt(cur.savePath + "/" + files[m_streamFileIdx].path) != m_streamFilePath) {
            m_streamTimer->stop(); return;
        }
        // Open the finished file, or the in-progress ".!bt" if that's what's on disk.
        QString actual = m_streamFilePath;
        if (!QFile::exists(actual) && QFile::exists(actual + QStringLiteral(".!bt")))
            actual += QStringLiteral(".!bt");
        const float prog = files[m_streamFileIdx].progress;
        const qint64 done = qint64(prog * files[m_streamFileIdx].size);
        if (QFile::exists(actual) && (prog >= 0.02f || done > 5*1024*1024)) {
            m_streamTimer->stop();
            const bool opened = launchMediaPlayer(actual);
            emit toast(tr_("ctx_stream"), opened ? tr_("stream_started").arg(cur.name)
                                                 : tr_("stream_no_player"));
        } else if (++m_streamTries > 300) {   // ~10 min with no buffer (dead torrent) — give up
            m_streamTimer->stop();
        }
    });
    m_streamTimer->start();
    emit toast(tr_("ctx_stream"), tr_("stream_started").arg(info.name));
}

QString QmlSessionBridge::streamUrl(int row)
{
    if (m_streamPort == 0) return {};
    if (row < 0 || row >= m_session->torrentCount()) return {};
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    auto files = m_session->filesAt(row);
    auto stripBt = [](const QString &p){ return p.endsWith(QStringLiteral(".!bt")) ? p.chopped(4) : p; };
    int bestIdx = -1; qint64 bestSize = 0;
    for (int i = 0; i < int(files.size()); ++i) {
        const QString mp = stripBt(files[i].path);
        for (const auto &ext : videoExts)
            if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                if (files[i].size > bestSize) { bestSize = files[i].size; bestIdx = i; }
                break;
            }
    }
    if (bestIdx < 0) return {};
    const QString hash = m_session->torrentHashAt(row);
    if (hash.isEmpty()) return {};

    // same prep as the external stream: resume, sequential, only the video
    // file at full priority, and boost the header/index pieces.
    m_session->resumeTorrent(row);
    m_session->setSequentialDownload(row, true);
    for (int i = 0; i < int(files.size()); ++i)
        if (i != bestIdx) m_session->setFilePriority(row, i, 0);
    m_session->setFilePriority(row, bestIdx, 7);
    m_session->prioritizeFilePieceBoundaries(row, bestIdx);

    return QStringLiteral("http://127.0.0.1:%1/stream/%2/%3").arg(m_streamPort).arg(hash).arg(bestIdx);
}

void QmlSessionBridge::openExternalForHash(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const QString path = m_session->streamFilePath(row, fileIndex);
    if (path.isEmpty()) return;
    if (!launchMediaPlayer(path))
        emit toast(tr_("ctx_stream"), tr_("stream_no_player"));
}

// External subtitles for the built-in player: parse to plain cue maps the
// QML overlay can binary-search.
QVariantList QmlSessionBridge::loadSubtitleFile(const QString &path)
{
    QString p = path;
    if (p.startsWith(QLatin1String("file://"))) p = QUrl(p).toLocalFile();
    QVariantList out;
    const auto cues = SubtitleParser::parseFile(p);
    out.reserve(cues.size());
    for (const auto &c : cues) {
        QVariantMap m;
        m["start"] = c.startMs;
        m["end"] = c.endMs;
        m["text"] = c.text;
        out << m;
    }
    return out;
}

QString QmlSessionBridge::findSidecarSubtitle(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    QString video = m_session->streamFilePath(row, fileIndex);
    if (video.isEmpty()) return {};
    if (video.endsWith(QLatin1String(".!bt"))) video.chop(4);
    const QFileInfo vi(video);
    const QString base = vi.completeBaseName();
    QDir dir = vi.dir();
    for (const char *ext : {"srt", "vtt"}) {
        const QString exact = dir.filePath(base + QLatin1Char('.') + QLatin1String(ext));
        if (QFileInfo::exists(exact)) return exact;
    }
    // language-suffixed sidecars ("Movie.pt-BR.srt") sort first by name
    const QStringList matches = dir.entryList({base + QStringLiteral("*.srt"), base + QStringLiteral("*.vtt")},
                                              QDir::Files, QDir::Name);
    return matches.isEmpty() ? QString() : dir.filePath(matches.first());
}

void QmlSessionBridge::playSelected()
{
    if (!hasSelection()) return;
    const int row = m_selectedIndex;
    const QString url = streamUrl(row);
    if (url.isEmpty()) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }
    const TorrentInfo info = m_session->torrentAt(row);
    const QString hash = m_session->torrentHashAt(row);
    const int fileIdx = url.section('/', -1).toInt();
    emit openPlayer(url, info.name, hash, fileIdx);
}

void QmlSessionBridge::playFile(const QString &infoHash, int fileIndex)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0 || m_streamPort == 0) return;
    auto files = m_session->filesAt(row);
    if (fileIndex < 0 || fileIndex >= int(files.size())) return;
    m_session->resumeTorrent(row);
    m_session->setSequentialDownload(row, true);
    for (int i = 0; i < int(files.size()); ++i)
        m_session->setFilePriority(row, i, i == fileIndex ? 7 : 0);
    m_session->prioritizeFilePieceBoundaries(row, fileIndex);
    const QString hash = m_session->torrentHashAt(row);
    const TorrentInfo info = m_session->torrentAt(row);
    emit openPlayer(QStringLiteral("http://127.0.0.1:%1/stream/%2/%3").arg(m_streamPort).arg(hash).arg(fileIndex),
                    info.name, hash, fileIndex);
}

QVariantList QmlSessionBridge::movieLibrary() const
{
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    auto stripBt = [](const QString &p){ return p.endsWith(QStringLiteral(".!bt")) ? p.chopped(4) : p; };
    QSettings s;
    QVariantList out;
    const int n = m_session->torrentCount();
    for (int row = 0; row < n; ++row) {
        auto files = m_session->filesAt(row);
        int bestIdx = -1; qint64 bestSize = 0;
        struct Vid { int idx; QString name; int season; int episode; };
        std::vector<Vid> vids;                           // all video files (for episode picking)
        for (int i = 0; i < int(files.size()); ++i) {
            const QString mp = stripBt(files[i].path);
            for (const auto &ext : videoExts)
                if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                    if (files[i].size > bestSize) { bestSize = files[i].size; bestIdx = i; }
                    const QString fname = QFileInfo(mp).fileName();
                    const ParsedName pn = NameParser::parse(fname);
                    vids.push_back({ i, fname, pn.season, pn.episode });
                    break;
                }
        }
        if (bestIdx < 0) continue;                       // no video file

        const TorrentInfo info = m_session->torrentAt(row);
        const float fprog = files[bestIdx].progress;
        const qint64 done = qint64(double(fprog) * files[bestIdx].size);
        const bool watchable = info.completed || fprog >= 0.02f || done > 5 * 1024 * 1024;
        if (!watchable) continue;                        // not enough buffered to start

        const QString hash = m_session->torrentHashAt(row);
        if (hash.isEmpty()) continue;

        // Order episodes by season then episode (loose files first stay by index),
        // and flag each as watched from its resume record.
        std::sort(vids.begin(), vids.end(), [](const Vid &a, const Vid &b) {
            if (a.season != b.season) return a.season < b.season;
            if (a.episode != b.episode) return a.episode < b.episode;
            return a.idx < b.idx;
        });
        QVariantList videos;
        bool isSeries = false;
        for (const Vid &v : vids) {
            const QString vrk = QStringLiteral("resume_%1_%2").arg(hash).arg(v.idx);
            QVariantMap vf;
            vf["idx"] = v.idx;
            vf["name"] = v.name;
            vf["season"] = v.season;
            vf["episode"] = v.episode;
            vf["watched"] = s.value(vrk + QStringLiteral("_watched"), false).toBool();
            videos << vf;
            if (v.season >= 0 && v.episode >= 0) isSeries = true;
        }

        QString poster, title = info.name;
        int year = 0, tmdbId = 0;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (meta.valid) {
                if (!meta.posterPath.isEmpty()) poster = QUrl::fromLocalFile(meta.posterPath).toString();
                if (!meta.title.isEmpty()) title = meta.title;
                year = meta.year;
                tmdbId = meta.tmdbId;
            }
        }

        const QString rk = QStringLiteral("resume_%1_%2").arg(hash).arg(bestIdx);
        const qint64 resumeMs = s.value(rk, 0).toLongLong();
        const qint64 durMs    = s.value(rk + QStringLiteral("_dur"), 0).toLongLong();
        const qint64 resumeAt = s.value(rk + QStringLiteral("_at"), 0).toLongLong();

        QVariantMap m;
        m["infoHash"]   = hash;
        m["title"]      = title;
        m["year"]       = year > 0 ? QString::number(year) : QString();
        m["poster"]     = poster;
        m["fileIndex"]  = bestIdx;
        m["videos"]     = videos;                         // [{idx,name,season,episode,watched}] sorted
        m["isSeries"]   = isSeries;                       // has SxxExx markers → group by season
        m["tmdbId"]     = tmdbId;                          // for TMDB episode-title lookup
        m["progress"]   = double(fprog);                 // download progress 0..1
        m["completed"]  = info.completed;
        m["size"]       = info.totalSize;
        m["resumeMs"]   = resumeMs;
        m["durMs"]      = durMs;
        m["resumeAt"]   = resumeAt;                       // last-watched timestamp (ms)
        m["watchedPct"] = (durMs > 0 && resumeMs > 0) ? double(resumeMs) / double(durMs) : 0.0;
        out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::watchlist() const
{
    const QByteArray raw = QSettings().value(QStringLiteral("watchlist")).toString().toUtf8();
    return QJsonDocument::fromJson(raw).array().toVariantList();
}

bool QmlSessionBridge::inWatchlist(const QString &title, const QString &type) const
{
    const QVariantList list = watchlist();
    for (const QVariant &v : list) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("title")).toString() == title
            && m.value(QStringLiteral("type")).toString() == type)
            return true;
    }
    return false;
}

void QmlSessionBridge::toggleWatchlist(const QVariantMap &item)
{
    const QString title = item.value(QStringLiteral("title")).toString();
    const QString type = item.value(QStringLiteral("type")).toString();
    if (title.isEmpty()) return;
    QVariantList list = watchlist();
    bool removed = false;
    for (int i = 0; i < list.size(); ++i) {
        const QVariantMap m = list[i].toMap();
        if (m.value(QStringLiteral("title")).toString() == title
            && m.value(QStringLiteral("type")).toString() == type) {
            list.removeAt(i); removed = true; break;
        }
    }
    if (!removed) list.prepend(item);
    QSettings().setValue(QStringLiteral("watchlist"),
                         QString::fromUtf8(QJsonDocument(QJsonArray::fromVariantList(list)).toJson(QJsonDocument::Compact)));
    emit watchlistChanged();
}

void QmlSessionBridge::clearResume(const QString &infoHash, int fileIndex)
{
    const QString rk = QStringLiteral("resume_%1_%2").arg(infoHash).arg(fileIndex);
    QSettings s;
    s.remove(rk);
    s.remove(rk + QStringLiteral("_dur"));
    s.remove(rk + QStringLiteral("_at"));
}

QVariantMap QmlSessionBridge::streamFileStats(const QString &infoHash, int fileIndex) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    return m_session->streamFileStats(row, fileIndex);
}

QString QmlSessionBridge::streamFileName(const QString &infoHash, int fileIndex) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    const QStringList names = m_session->torrentFileNames(row);
    if (fileIndex < 0 || fileIndex >= names.size()) return {};
    return names.at(fileIndex).section('/', -1);   // leaf name
}

void QmlSessionBridge::playByHash(const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const QString url = streamUrl(row);                  // preps priorities + picks the best video
    if (url.isEmpty()) { emit toast(tr_("ctx_stream"), tr_("stream_no_video")); return; }
    const TorrentInfo info = m_session->torrentAt(row);
    const int fileIdx = url.section('/', -1).toInt();
    emit openPlayer(url, info.name, infoHash, fileIdx);
}

int QmlSessionBridge::nextEpisode(const QString &infoHash, int fileIndex) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return -1;
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts"};
    auto files = m_session->filesAt(row);
    struct V { int idx; int season; int episode; };
    std::vector<V> vids;
    for (int i = 0; i < int(files.size()); ++i) {
        QString mp = files[i].path;
        if (mp.endsWith(QStringLiteral(".!bt"))) mp.chop(4);
        for (const auto &ext : videoExts)
            if (mp.endsWith(ext, Qt::CaseInsensitive)) {
                const ParsedName pn = NameParser::parse(QFileInfo(mp).fileName());
                vids.push_back({ i, pn.season, pn.episode });
                break;
            }
    }
    std::sort(vids.begin(), vids.end(), [](const V &a, const V &b) {
        if (a.season != b.season) return a.season < b.season;
        if (a.episode != b.episode) return a.episode < b.episode;
        return a.idx < b.idx;
    });
    for (size_t i = 0; i + 1 < vids.size(); ++i)
        if (vids[i].idx == fileIndex) return vids[i + 1].idx;
    return -1;
}

void QmlSessionBridge::watchWhenReady(const QString &infoHash, const QString &title)
{
    if (infoHash.isEmpty()) return;
    m_pendingWatch.insert(infoHash, qMakePair(title, QDateTime::currentSecsSinceEpoch()));
    emit watchBuffering(title);
}

void QmlSessionBridge::cancelWatch(const QString &infoHash)
{
    m_pendingWatch.remove(infoHash);
}

// Runs each ~1s tick: open the player for any pending Get&Watch hash that has
// become playable; give up after ~2 min of no metadata/seeds.
void QmlSessionBridge::onWatchTick()
{
    pollRunningGames();
    pollInstallWatch();
    if (m_pendingWatch.isEmpty()) return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const QString &hash : m_pendingWatch.keys()) {
        const int idx = m_session->torrentIndexByInfoHash(hash);
        if (idx >= 0) {
            const TorrentInfo info = m_session->torrentAt(idx);
            emit watchProgress(hash, info.progress);
            if (m_session->torrentHasVideo(idx)) {
                const bool ready = info.completed || info.progress >= 0.02f
                                 || info.totalDone > 5LL * 1024 * 1024;
                if (ready) {
                    m_pendingWatch.remove(hash);
                    playByHash(hash);
                    continue;
                }
            }
        }
        if (now - m_pendingWatch.value(hash).second > 120)
            emit watchFailed(m_pendingWatch.take(hash).first);
    }
}

QVariantList QmlSessionBridge::gameLibrary() const
{
    QVariantList out;
    const int n = m_session->torrentCount();
    for (int row = 0; row < n; ++row) {
        const TorrentInfo info = m_session->torrentAt(row);
        const QString hash = m_session->torrentHashAt(row);
        if (hash.isEmpty()) continue;

        bool isGame = false;
        QString poster, title, description;
        QStringList genres;
        double rating = 0; int gyear = 0;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (meta.valid && meta.contentType == ContentType::Game) {
                isGame = true;
                title = meta.title;
                if (!meta.posterPath.isEmpty()) poster = QUrl::fromLocalFile(meta.posterPath).toString();
                description = meta.description;
                genres = meta.genres;
                rating = meta.rating;
                gyear = meta.year;
            }
        }
        if (!isGame) {
            const ParsedName pn = NameParser::parse(info.name);
            if (pn.contentType == ContentType::Game) {
                isGame = true;
                title = pn.cleanTitle.isEmpty() ? info.name : pn.cleanTitle;
            } else {
                bool hasExe = false, hasVideo = false;
                const auto files = m_session->filesAt(row);
                for (const auto &f : files) {
                    const QString p = f.path.toLower();
                    if (p.endsWith(QStringLiteral(".exe"))) hasExe = true;
                    else if (p.endsWith(QStringLiteral(".mkv")) || p.endsWith(QStringLiteral(".mp4"))
                             || p.endsWith(QStringLiteral(".avi"))) hasVideo = true;
                }
                if (hasExe && !hasVideo) { isGame = true; title = pn.cleanTitle.isEmpty() ? info.name : pn.cleanTitle; }
            }
        }
        if (!isGame) continue;

        QVariantMap m;
        m["infoHash"]  = hash;
        m["title"]     = title.isEmpty() ? info.name : title;
        m["poster"]    = poster;
        m["progress"]   = double(info.progress);
        m["completed"]  = info.completed;
        m["size"]       = info.totalSize;
        m["hasExe"]     = !gameExe(hash).isEmpty();
        m["lastPlayed"] = QSettings().value(QStringLiteral("gamePlayed/") + hash, 0).toLongLong();
        m["description"] = description;
        m["genres"]     = genres;
        m["rating"]     = rating;
        m["year"]       = gyear > 0 ? QString::number(gyear) : QString();
        m["playSeconds"] = QSettings().value(QStringLiteral("gameSeconds/") + hash, 0).toLongLong();
        m["launches"]   = QSettings().value(QStringLiteral("gameLaunches/") + hash, 0).toLongLong();
        m["playing"]    = m_runningGames.contains(hash);
        m["installState"] = gameInstallState(hash, info.completed);
        out << m;
    }
    return out;
}

QString QmlSessionBridge::gameExe(const QString &infoHash) const
{
    return QSettings().value(QStringLiteral("gameExe/") + infoHash).toString();
}

void QmlSessionBridge::setGameExe(const QString &infoHash, const QString &fileUrl)
{
    const QString path = fileUrl.startsWith(QStringLiteral("file:")) ? QUrl(fileUrl).toLocalFile() : fileUrl;
    if (path.isEmpty()) return;
    QSettings().setValue(QStringLiteral("gameExe/") + infoHash, path);
}

QString QmlSessionBridge::gameFolder(const QString &infoHash) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    const TorrentInfo info = m_session->torrentAt(row);
    const QString root = info.savePath + QStringLiteral("/") + info.name;
    return QFileInfo(root).isDir() ? root : info.savePath;
}

// Best-guess game executable inside a folder. Skips redists/uninstallers,
// treats setup/installer separately, and prefers the largest .exe near the
// root (where a game's main binary usually lives). A heuristic — the user can
// always override via "Set executable…". Returns "" if nothing runnable.
static QString autodetectGameExe(const QString &folder, bool *isInstaller)
{
    if (isInstaller) *isInstaller = false;
    if (folder.isEmpty()) return {};
    static const QStringList skip = {
        "unins", "redist", "vcredist", "vc_redist", "directx", "dxsetup", "dotnet",
        "dotnetfx", "oalinst", "_commonredist", "prereq", "crashreport", "crashpad",
        "python", "benchmark", "config", "settings", "cleanup" };
    static const QStringList installerHints = { "setup", "install" };

    QString bestGame, installer;
    qint64 bestScore = -1;
    int scanned = 0;
    QDirIterator it(folder, {QStringLiteral("*.exe")}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && scanned < 4000) {
        const QString path = it.next();
        ++scanned;
        const QString lower = QFileInfo(path).fileName().toLower();
        bool skipIt = false;
        for (const auto &s : skip) if (lower.contains(s)) { skipIt = true; break; }
        if (skipIt) continue;
        bool inst = false;
        for (const auto &s : installerHints) if (lower.contains(s)) { inst = true; break; }
        if (inst) { if (installer.isEmpty()) installer = path; continue; }

        const QString rel = path.mid(folder.size());
        const int depth = rel.count(QLatin1Char('/'));
        qint64 score = QFileInfo(path).size();
        if (depth <= 1) score += qint64(800) * 1024 * 1024;            // root binary strongly preferred
        if (lower.contains("launcher")) score -= qint64(300) * 1024 * 1024;
        if (score > bestScore) { bestScore = score; bestGame = path; }
    }
    if (!bestGame.isEmpty()) return bestGame;
    if (!installer.isEmpty()) { if (isInstaller) *isInstaller = true; return installer; }
    return {};
}

static bool pidAlive(qint64 pid)
{
    if (pid <= 0) return false;
#if defined(Q_OS_WIN)
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, DWORD(pid));
    if (!h) return false;
    const DWORD r = WaitForSingleObject(h, 0);
    CloseHandle(h);
    return r == WAIT_TIMEOUT;   // still running
#else
    return ::kill(pid_t(pid), 0) == 0 || errno != ESRCH;
#endif
}

void QmlSessionBridge::launchGame(const QString &infoHash)
{
    QString exe = gameExe(infoHash);              // a manual override always wins
    bool isInstaller = false;
    if (exe.isEmpty() || !QFile::exists(exe))
        exe = autodetectGameExe(gameFolder(infoHash), &isInstaller);

    if (!exe.isEmpty() && QFile::exists(exe)) {
        const QString wd = QFileInfo(exe).absolutePath();
        qint64 pid = 0;
        // Detached so the game survives BATorrent closing; the returned pid lets
        // us poll for exit and credit playtime.
        if (QProcess::startDetached(exe, {}, wd, &pid)) {
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            QSettings s;
            s.setValue(QStringLiteral("gamePlayed/") + infoHash, nowMs);
            s.setValue(QStringLiteral("gameLaunches/") + infoHash,
                       s.value(QStringLiteral("gameLaunches/") + infoHash, 0).toLongLong() + 1);
            if (!isInstaller && pid > 0 && !m_runningGames.contains(infoHash)) {
                m_runningGames.insert(infoHash, pid);
                m_gameStartMs.insert(infoHash, nowMs);
                emit gamesChanged();   // "playing now"
            }
            emit toast(isInstaller ? tr_("hub_game_installing") : tr_("hub_game_launch"),
                       QFileInfo(exe).completeBaseName());
            return;
        }
    }
    // nothing runnable found → open the folder so the user can run/set it
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const TorrentInfo info = m_session->torrentAt(row);
    revealTorrentRoot(info.savePath, info.name);
}

// Poll launched games for exit; credit elapsed time to the per-game total.
void QmlSessionBridge::pollRunningGames()
{
    if (m_runningGames.isEmpty()) return;
    bool changed = false;
    for (const QString &hash : m_runningGames.keys()) {
        if (pidAlive(m_runningGames.value(hash))) continue;
        const qint64 secs = (QDateTime::currentMSecsSinceEpoch() - m_gameStartMs.value(hash)) / 1000;
        if (secs > 30) {                       // ignore quick bounces / launchers handing off
            QSettings s;
            s.setValue(QStringLiteral("gameSeconds/") + hash,
                       s.value(QStringLiteral("gameSeconds/") + hash, 0).toLongLong() + secs);
        }
        m_runningGames.remove(hash);
        m_gameStartMs.remove(hash);
        changed = true;
    }
    if (changed) emit gamesChanged();
}

// ---- Game install orchestrator (the "Steam pirata" one-click chain) --------

int QmlSessionBridge::gameInstallState(const QString &infoHash, bool completed) const
{
    if (m_runningGames.contains(infoHash)) return GIS_Playing;
    if (m_gameInstallState.contains(infoHash)) return m_gameInstallState.value(infoHash);
    if (!completed) return GIS_Downloading;
    if (!gameExe(infoHash).isEmpty()) return GIS_Ready;
    return GIS_ReadyToInstall;
}

void QmlSessionBridge::installGame(const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    if (!m_session->torrentAt(row).completed) return;   // nothing to install yet

    const int st = m_gameInstallState.value(infoHash, -1);
    if (st == GIS_Extracting || st == GIS_Installing) return;   // already in flight

    // torrentHasArchives reads the file list, so it stays true even after a prior
    // extraction — m_extracted guards against unpacking twice.
    if (m_session->torrentHasArchives(row) && !m_extracted.contains(infoHash)) {
        m_gameInstallState.insert(infoHash, GIS_Extracting);
        emit gamesChanged();
        m_session->extractTorrent(row, QString());   // → extractionCompleted → onExtractionCompleted
        return;
    }
    finalizeInstall(infoHash);
}

void QmlSessionBridge::onExtractionCompleted(const QString &infoHash, bool success)
{
    if (success) m_extracted.insert(infoHash);
    if (m_gameInstallState.value(infoHash, -1) != GIS_Extracting) return;   // not our install flow
    if (!success) {
        m_gameInstallState.insert(infoHash, GIS_Failed);
        emit gamesChanged();
        emit toast(tr_("hub_install_failed"), QString());
        return;
    }
    finalizeInstall(infoHash);
}

// Scene "copy crack" step: if the extracted tree carries a group/Crack folder,
// copy its files over the game's directory (overwriting the DRM stub).
static void applyCrackIfPresent(const QString &root, const QString &gameDir)
{
    if (root.isEmpty() || gameDir.isEmpty()) return;
    QStringList subdirs;
    for (const QFileInfo &fi : QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
        subdirs << fi.fileName();
    const QString crack = InstallerProfile::crackDir(subdirs);
    if (crack.isEmpty()) return;
    const QString crackPath = QDir(root).filePath(crack);
    for (const QFileInfo &fi : QDir(crackPath).entryInfoList(QDir::Files)) {
        const QString dest = QDir(gameDir).filePath(fi.fileName());
        QFile::remove(dest);
        QFile::copy(fi.absoluteFilePath(), dest);
    }
}

void QmlSessionBridge::finalizeInstall(const QString &infoHash)
{
    const QString folder = gameFolder(infoHash);
    bool isInstaller = false;
    const QString exe = autodetectGameExe(folder, &isInstaller);

    if (exe.isEmpty()) {
        // Tier D: a scene release whose game sits inside an .iso → can't run without
        // mounting. On Windows, mount it so the disc appears; everywhere, guide the user.
        QString iso;
        for (const QFileInfo &fi : QDir(folder).entryInfoList({QStringLiteral("*.iso")}, QDir::Files)) {
            iso = fi.absoluteFilePath(); break;
        }
#ifdef Q_OS_WIN
        if (!iso.isEmpty()) {
            QProcess::startDetached(QStringLiteral("powershell"), {
                QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
                QStringLiteral("Mount-DiskImage -ImagePath '%1'")
                    .arg(QString(iso).replace(QLatin1Char('\''), QStringLiteral("''"))) });
        }
#endif
        if (!iso.isEmpty()) {
            const int row = m_session->torrentIndexByInfoHash(infoHash);
            if (row >= 0) { const TorrentInfo info = m_session->torrentAt(row);
                            revealTorrentRoot(info.savePath, info.name); }
        }
        m_gameInstallState.insert(infoHash, GIS_NeedsSetup);
        emit gamesChanged();
        emit toast(tr_("hub_install_need_exe"), QString());
        return;
    }
    if (!isInstaller) {                                   // portable/cracked → register & done
        applyCrackIfPresent(folder, QFileInfo(exe).absolutePath());
        QSettings().setValue(QStringLiteral("gameExe/") + infoHash, exe);
        m_gameInstallState.remove(infoHash);             // → derived Ready
        emit gamesChanged();
        emit toast(tr_("hub_install_ready"), QFileInfo(exe).completeBaseName());
        return;
    }
    runInstaller(infoHash, exe, folder);
}

void QmlSessionBridge::runInstaller(const QString &infoHash, const QString &installerExe,
                                    const QString &folder)
{
    QStringList siblings;
    for (const QFileInfo &fi : QDir(QFileInfo(installerExe).absolutePath()).entryInfoList(QDir::Files))
        siblings << fi.fileName();
    const InstallerProfile::Engine engine = InstallerProfile::detectEngine(installerExe);
    const bool repack = InstallerProfile::isLikelyRepack(installerExe, siblings);
    const QString targetDir = QDir(folder).filePath(QStringLiteral("_BATorrent"));

    m_gameInstallState.insert(infoHash, GIS_Installing);
    emit gamesChanged();

#ifdef Q_OS_WIN
    // Tier B: a silenceable generic installer (NOT a repack — those need the user's
    // component/language choices) → drive it unattended into a known dir we can scan.
    const InstallerProfile::SilentInvocation si =
        InstallerProfile::silentInvocation(engine, installerExe, targetDir);
    if (si.supported && !repack) {
        QDir().mkpath(targetDir);
        auto *p = new QProcess(this);
        const QString program = si.program.isEmpty() ? installerExe : si.program;
        if (!si.rawTail.isEmpty())
            p->setNativeArguments(si.rawTail);   // NSIS /D= must stay last and unquoted
        connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                [this, p, infoHash, targetDir](int code, QProcess::ExitStatus) {
            p->deleteLater();
            const bool ok = (code == 0 || code == 3010 || code == 1641);  // MSI reboot codes = success
            bool inst = false;
            const QString exe = ok ? autodetectGameExe(targetDir, &inst) : QString();
            if (!exe.isEmpty() && !inst) {
                QSettings().setValue(QStringLiteral("gameExe/") + infoHash, exe);
                m_gameInstallState.remove(infoHash);
                emit toast(tr_("hub_install_ready"), QFileInfo(exe).completeBaseName());
            } else {
                m_gameInstallState.insert(infoHash, GIS_NeedsSetup);
                emit toast(tr_("hub_install_need_exe"), QString());
            }
            emit gamesChanged();
        });
        p->start(program, si.args);
        return;
    }
#else
    Q_UNUSED(engine);
    Q_UNUSED(repack);
#endif

    // Guided (repacks/unknown/non-Windows): open the installer; pollInstallWatch
    // detects when it exits, then registers the produced exe (or asks the user).
    qint64 pid = 0;
    if (QProcess::startDetached(installerExe, {}, QFileInfo(installerExe).absolutePath(), &pid)
        && pid > 0) {
        m_installWatch.insert(infoHash, pid);
        emit toast(tr_("hub_game_installing"), QFileInfo(installerExe).completeBaseName());
    } else {
        const int row = m_session->torrentIndexByInfoHash(infoHash);
        if (row >= 0) {
            const TorrentInfo info = m_session->torrentAt(row);
            revealTorrentRoot(info.savePath, info.name);
        }
        m_gameInstallState.insert(infoHash, GIS_NeedsSetup);
        emit gamesChanged();
    }
}

void QmlSessionBridge::pollInstallWatch()
{
    if (m_installWatch.isEmpty()) return;
    bool changed = false;
    for (const QString &hash : m_installWatch.keys()) {
        if (pidAlive(m_installWatch.value(hash))) continue;   // installer still open
        m_installWatch.remove(hash);
        bool inst = false;
        const QString folder = gameFolder(hash);
        const QString exe = autodetectGameExe(folder, &inst);
        if (!exe.isEmpty() && !inst) {
            applyCrackIfPresent(folder, QFileInfo(exe).absolutePath());
            QSettings().setValue(QStringLiteral("gameExe/") + hash, exe);
            m_gameInstallState.remove(hash);
            emit toast(tr_("hub_install_ready"), QFileInfo(exe).completeBaseName());
        } else {
            m_gameInstallState.insert(hash, GIS_NeedsSetup);   // user points us at it
            emit toast(tr_("hub_install_need_exe"), QString());
        }
        changed = true;
    }
    if (changed) emit gamesChanged();
}

bool QmlSessionBridge::isGameTorrent(int row) const
{
    if (row < 0) return false;
    const QString hash = m_session->torrentHashAt(row);
    if (!hash.isEmpty() && m_resolver && m_resolver->hasCached(hash)) {
        const auto meta = m_resolver->cached(hash);
        if (meta.valid && meta.contentType == ContentType::Game) return true;
    }
    if (NameParser::parse(m_session->torrentAt(row).name).contentType == ContentType::Game)
        return true;
    bool hasExe = false, hasVideo = false;
    for (const auto &f : m_session->filesAt(row)) {
        const QString p = f.path.toLower();
        if (p.endsWith(QStringLiteral(".exe"))) hasExe = true;
        else if (p.endsWith(QStringLiteral(".mkv")) || p.endsWith(QStringLiteral(".mp4"))
                 || p.endsWith(QStringLiteral(".avi"))) hasVideo = true;
    }
    return hasExe && !hasVideo;
}

void QmlSessionBridge::onGameTorrentFinished(const QString &name, const QString &infoHash)
{
    Q_UNUSED(name);
    if (!QSettings().value(QStringLiteral("gameAutoInstall"), false).toBool()) return;
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (isGameTorrent(row)) installGame(infoHash);
}

bool QmlSessionBridge::selectedIsGame() const
{
    return hasSelection() && isGameTorrent(m_selectedIndex);
}

int QmlSessionBridge::selectedGameState() const
{
    if (!hasSelection() || !isGameTorrent(m_selectedIndex)) return -1;
    const QString hash = m_session->torrentHashAt(m_selectedIndex);
    return gameInstallState(hash, m_session->torrentAt(m_selectedIndex).completed);
}

void QmlSessionBridge::installSelectedGame()
{
    if (hasSelection()) installGame(m_session->torrentHashAt(m_selectedIndex));
}

void QmlSessionBridge::playSelectedGame()
{
    if (hasSelection()) launchGame(m_session->torrentHashAt(m_selectedIndex));
}

void QmlSessionBridge::setSelectedCategory(const QString &category)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentCategory(r, category);
    emit selectionChanged(); emit selectionListsChanged();
    emit queueRefreshNeeded();   // category is a full-role edit → repaint the cards
}

void QmlSessionBridge::setSelectedTags(const QStringList &tags)
{
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->setTorrentTags(r, tags);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::addTrackerToSelected(const QString &url)
{
    if (url.isEmpty()) return;
    for (int r : resolveRows(m_selectedRows, m_selectedIndex))
        m_session->addTracker(r, url);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::removeTrackerFromSelected(const QString &url)
{
    if (url.isEmpty() || !hasSelection()) return;
    QStringList keep;
    for (const auto &t : m_session->trackersAt(m_selectedIndex))
        if (t.url != url) keep << t.url;
    m_session->replaceTrackers(m_selectedIndex, keep);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::renameSelectedFile(int fileIndex, const QString &newName)
{
    if (!hasSelection() || newName.isEmpty()) return;
    m_session->renameFile(m_selectedIndex, fileIndex, newName);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::setSelectedFilePriority(int fileIndex, int priority)
{
    if (!hasSelection()) return;
    m_session->setFilePriority(m_selectedIndex, fileIndex, priority);
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::copySelectedName()
{
    if (!hasSelection()) return;
    const QString n = m_session->torrentAt(m_selectedIndex).name;
    if (!n.isEmpty()) QGuiApplication::clipboard()->setText(n);
}

void QmlSessionBridge::openSelectedFile()
{
    if (!hasSelection()) return;
    const QString path = m_session->torrentRootPath(m_selectedIndex);
    if (!path.isEmpty()) revealInFileManager(path);   // open the folder with the item selected
}

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

void QmlSessionBridge::copySelectedContentPath()
{
    if (!hasSelection()) return;
    const QString path = m_session->torrentRootPath(m_selectedIndex);
    if (path.isEmpty()) return;
    QGuiApplication::clipboard()->setText(QDir::toNativeSeparators(path));
    emit toast(tr_("ctx_path_copied"), QDir::toNativeSeparators(path));
}

void QmlSessionBridge::relinkSelectedCover(const QString &query, const QString &typeStr)
{
    if (!hasSelection() || !m_resolver || query.trimmed().isEmpty()) return;
    const QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (hash.isEmpty()) return;
    const ContentType type = typeStr == QLatin1String("series") ? ContentType::Series
                           : typeStr == QLatin1String("game")   ? ContentType::Game
                                                                : ContentType::Movie;
    m_resolver->resolveManual(hash, query.trimmed(), type);
}

void QmlSessionBridge::clearSelectedCover()
{
    if (!hasSelection() || !m_resolver) return;
    const QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (!hash.isEmpty()) m_resolver->clearMetadata(hash);
}

void QmlSessionBridge::importQbittorrent(const QString &savePath)
{
    m_session->importFromQBittorrent(savePath);
    emit queueRefreshNeeded();
}

void QmlSessionBridge::copyToClipboard(const QString &text)
{
    if (!text.isEmpty()) QGuiApplication::clipboard()->setText(text);
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

int QmlSessionBridge::selectedDownloadLimit() const
{
    return hasSelection() ? m_session->torrentDownloadLimit(m_selectedIndex) : 0;
}

int QmlSessionBridge::selectedUploadLimit() const
{
    return hasSelection() ? m_session->torrentUploadLimit(m_selectedIndex) : 0;
}

QString QmlSessionBridge::selectedCategory() const
{
    return hasSelection() ? m_session->torrentAt(m_selectedIndex).category : QString();
}

QStringList QmlSessionBridge::selectedTagList() const
{
    return hasSelection() ? m_session->torrentTags(m_selectedIndex) : QStringList();
}

QStringList QmlSessionBridge::categories() const { return m_session->categories(); }

void QmlSessionBridge::setDetailPeersActive(bool active)
{
    if (m_detailPeersActive == active) return;
    m_detailPeersActive = active;
    if (active) {
        // Show a placeholder instantly, then build the (heavy) peer list off the
        // click on the next event-loop turn — opening the tab never blocks.
        m_peersLoading = true;
        m_peerCache.clear();
        emit selectionListsChanged();
        QTimer::singleShot(0, this, [this]() { rebuildPeerCache(); });
    } else {
        m_peerCache.clear();
        m_peersLoading = false;
    }
}

// Cheap getter — the QML binding reads the cache, never touches libtorrent.
QVariantList QmlSessionBridge::selectedPeerList() const { return m_peerCache; }

// Heavy build (peersAt pulls every peer from libtorrent). Runs deferred / per
// tick while the Peers tab is open — never on the QML binding path or the click.
void QmlSessionBridge::rebuildPeerCache()
{
    if (!m_detailPeersActive || !hasSelection()) {
        if (!m_peerCache.isEmpty()) { m_peerCache.clear(); emit selectionListsChanged(); }
        m_peersLoading = false;
        return;
    }
    QVariantList out;
    // A big swarm (9k+ peers) froze the UI: a QVariantMap + geo lookup per peer.
    // Cap to the 500 most active (capped inside peersAt, before the QString-heavy
    // conversion); the total count is shown in the panel.
    auto peers = m_session->peersAt(m_selectedIndex, 500);
    out.reserve(peers.size());
    for (const auto &p : peers) {
        QVariantMap m;
        m["ip"]       = p.ip;
        m["port"]     = p.port;
        m["client"]   = p.client;
        m["downSpeed"]= formatSpeed(p.downloadRate);
        m["upSpeed"]  = formatSpeed(p.uploadRate);
        m["progress"] = p.progress;
        const QString cc = m_geoIp->cachedCountry(p.ip);
        if (cc.isEmpty())
            m_geoIp->resolve(p.ip);
        m["flag"] = cc.isEmpty() ? QString() : countryCodeToFlag(cc);
        out << m;
    }
    m_peerCache = out;
    m_peersLoading = false;
    emit selectionListsChanged();
}

QVariantList QmlSessionBridge::selectedFiles() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto files = m_session->filesAt(m_selectedIndex);
    out.reserve(files.size());
    for (const auto &f : files) {
        QVariantMap m;
        m["path"]     = f.path;
        m["size"]     = formatSize(f.size);
        m["progress"] = f.progress;
        m["priority"] = f.priority;
        out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::selectedTrackers() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto trackers = m_session->trackersAt(m_selectedIndex);
    out.reserve(trackers.size());
    for (const auto &t : trackers) {
        QVariantMap m;
        m["url"]    = t.url;
        m["tier"]   = t.tier;
        m["status"] = t.status;
        out << m;
    }
    return out;
}

QVariantList QmlSessionBridge::selectedPieces() const
{
    QVariantList out;
    if (!hasSelection()) return out;
    auto pieces = m_session->piecesAt(m_selectedIndex);
    out.reserve(pieces.size());
    for (bool b : pieces) out << b;
    return out;
}

// Free space on the default save volume, polled at most every 10s — the
// status bar binds to statsChanged which ticks every second.
// Single source of truth for "free disk": one cached read of the default save
// volume, shared by the status bar, search disk-fit, add-guard and auto-pause —
// so every screen agrees instead of each polling at its own moment.
qint64 QmlSessionBridge::freeSaveBytes() const
{
    static qint64 cached = -1;
    static qint64 lastCheck = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (cached < 0 || now - lastCheck >= 5) {
        lastCheck = now;
        const QString path = defaultSavePath();
        QStorageInfo si(path.isEmpty() ? QDir::homePath() : path);
        cached = si.isValid() ? si.bytesAvailable() : -1;
    }
    return cached;
}

QString QmlSessionBridge::freeDiskSpace() const
{
    const qint64 b = freeSaveBytes();
    return b >= 0 ? formatSize(b) : QString();
}

// Fraction of the save volume that's used (0..1) — drives the sidebar disk bar.
double QmlSessionBridge::diskUsedFraction() const
{
    static double cached = 0;
    static qint64 lastCheck = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (lastCheck == 0 || now - lastCheck >= 10) {
        lastCheck = now;
        const QString path = defaultSavePath();
        QStorageInfo si(path.isEmpty() ? QDir::homePath() : path);
        const qint64 total = si.isValid() ? si.bytesTotal() : 0;
        cached = total > 0 ? double(total - si.bytesAvailable()) / double(total) : 0.0;
    }
    return cached;
}

// One entry per distinct volume torrents save to (default + per-category paths).
QVariantList QmlSessionBridge::diskVolumes() const
{
    static QVariantList cached;
    static qint64 lastCheck = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (lastCheck != 0 && now - lastCheck < 10) return cached;
    lastCheck = now;

    QStringList paths;
    paths << defaultSavePath();
    const auto catPaths = m_session->allCategorySavePaths();
    for (auto it = catPaths.constBegin(); it != catPaths.constEnd(); ++it)
        if (!it.value().isEmpty()) paths << it.value();

    QVariantList out;
    QSet<QString> seenRoots;
    for (QString p : paths) {
        if (p.isEmpty()) p = QDir::homePath();
        QStorageInfo si(p);
        if (!si.isValid() || si.bytesTotal() <= 0) continue;
        const QString root = QString::fromUtf8(si.rootPath().toUtf8());
        if (seenRoots.contains(root)) continue;
        seenRoots.insert(root);

        QVariantMap m;
        QString name = si.displayName();
        if (name.isEmpty()) name = root;
        m["name"] = name;
        m["free"] = formatSize(si.bytesAvailable());
        m["freeBytes"] = si.bytesAvailable();
        m["usedFraction"] = double(si.bytesTotal() - si.bytesAvailable()) / double(si.bytesTotal());
        out << m;
    }
    cached = out;
    return out;
}

// Currently-downloading torrents for the nav-rail mini card: cover + name + % +
// down speed. Stable order (torrentAt index) so the QML carousel stays consistent.
QVariantList QmlSessionBridge::activeDownloads() const
{
    QVariantList out;
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i) {   // navigate via hover arrows, so no cap
        const TorrentInfo info = m_session->torrentAt(i);
        if (info.paused || info.completed || info.progress >= 1.0f) continue;
        const QString hash = m_session->torrentHashAt(i);
        QString poster;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (!meta.posterPath.isEmpty())
                poster = QUrl::fromLocalFile(meta.posterPath).toString();
        }
        QVariantMap m;
        m["infoHash"]  = hash;
        m["title"]     = info.name;
        m["progress"]  = double(info.progress);
        m["downSpeed"] = formatSize(info.downloadRate) + QStringLiteral("/s");
        m["poster"]    = poster;
        out << m;
    }
    return out;
}

// Continue watching / playing for the nav-rail slot when you're on the Downloads
// tab (the downloads are already on screen there, so show what's resumable instead).
QVariantList QmlSessionBridge::resumeItems() const
{
    static QVariantList cached; static qint64 last = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    if (last != 0 && now - last < 8) return cached;
    last = now;

    auto fmtPlay = [](qint64 s) {
        const qint64 h = s / 3600, m = (s % 3600) / 60;
        return h > 0 ? QStringLiteral("%1h %2m").arg(h).arg(m) : QStringLiteral("%1m").arg(m);
    };
    QVector<QPair<qint64, QVariantMap>> rows;   // (recency ms, item)
    for (const QVariant &v : movieLibrary()) {
        const QVariantMap mv = v.toMap();
        if (mv.value(QStringLiteral("resumeMs")).toLongLong() <= 0) continue;
        const double pct = mv.value(QStringLiteral("watchedPct")).toDouble();
        QVariantMap o;
        o["kind"] = QStringLiteral("movie");
        o["infoHash"]  = mv.value(QStringLiteral("infoHash"));
        o["fileIndex"] = mv.value(QStringLiteral("fileIndex"));
        o["title"]     = mv.value(QStringLiteral("title"));
        o["poster"]    = mv.value(QStringLiteral("poster"));
        o["progress"]  = pct;
        o["metric"]    = QString::number(int(pct * 100)) + QStringLiteral("%");
        rows.append({ mv.value(QStringLiteral("resumeAt")).toLongLong(), o });
    }
    for (const QVariant &v : gameLibrary()) {
        const QVariantMap g = v.toMap();
        if (g.value(QStringLiteral("lastPlayed")).toLongLong() <= 0) continue;
        QVariantMap o;
        o["kind"] = QStringLiteral("game");
        o["infoHash"] = g.value(QStringLiteral("infoHash"));
        o["title"]    = g.value(QStringLiteral("title"));
        o["poster"]   = g.value(QStringLiteral("poster"));
        o["progress"] = 0.0;
        o["metric"]   = fmtPlay(g.value(QStringLiteral("playSeconds")).toLongLong());
        rows.append({ g.value(QStringLiteral("lastPlayed")).toLongLong(), o });
    }
    std::sort(rows.begin(), rows.end(), [](const auto &a, const auto &b){ return a.first > b.first; });
    QVariantList out;
    for (int i = 0; i < rows.size() && i < 8; ++i) out << rows[i].second;
    cached = out;
    return out;
}

QString QmlSessionBridge::defaultSavePath() const
{
    QSettings s;
    QString p = s.value(QStringLiteral("lastSavePath")).toString();
    if (!p.isEmpty() && QDir(p).exists()) return p;
    return QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
}

void QmlSessionBridge::addTorrentFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    QString local = filePath;
    if (local.startsWith(QStringLiteral("file://")))
        local = QUrl(local).toLocalFile();
    m_session->addTorrent(local, defaultSavePath());
}

void QmlSessionBridge::requestAddTorrentFile(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    emit openTorrentRequested(filePath);
}

void QmlSessionBridge::addMagnetUri(const QString &uri, const QString &savePath)
{
    if (uri.isEmpty()) return;
    m_session->addMagnet(uri, savePath.isEmpty() ? defaultSavePath() : savePath);
}

void QmlSessionBridge::addTorrentUrl(const QString &url)
{
    const QString u = url.trimmed();
    if (u.isEmpty()) return;
    if (u.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) { addMagnetUri(u); return; }

    const QUrl qurl(u);
    if (!qurl.isValid() || !(qurl.scheme() == QLatin1String("http") || qurl.scheme() == QLatin1String("https"))) {
        emit toast(tr_("add_url_failed"), u);
        return;
    }

    auto *nam = new QNetworkAccessManager(this);
    QNetworkRequest req(qurl);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent"));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    auto *reply = nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, nam]() {
        reply->deleteLater();
        nam->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit toast(tr_("add_url_failed"), reply->errorString());
            return;
        }
        const QByteArray data = reply->readAll();
        // A bencoded .torrent always starts with 'd' (a dict). Anything else
        // (an HTML error page, a redirect to a login wall) is rejected here.
        if (data.isEmpty() || data.front() != 'd') {
            emit toast(tr_("add_url_failed"), reply->url().toString());
            return;
        }
        QString name = QFileInfo(reply->url().path()).fileName();
        if (!name.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive))
            name = QStringLiteral("download.torrent");
        const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
            + QStringLiteral("/bat_%1_%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(name);
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            emit toast(tr_("add_url_failed"), path);
            return;
        }
        f.write(data);
        f.close();
        // Route through the same add dialog as a dropped file (user picks
        // save path / files) instead of silently auto-downloading.
        emit openTorrentRequested(path);
    });
}

QVariantMap QmlSessionBridge::previewTorrent(const QString &filePath) const
{
    QString local = filePath;
    if (local.startsWith(QStringLiteral("file://")))
        local = QUrl(local).toLocalFile();

    QVariantMap out;
    std::shared_ptr<lt::torrent_info> ti;
    try {
        ti = std::make_shared<lt::torrent_info>(local.toStdString());
    } catch (const std::exception &e) {
        out["ok"] = false;
        out["error"] = QString::fromUtf8(e.what());
        return out;
    }
    out["ok"] = true;
    out["name"] = QString::fromStdString(ti->name());
    out["totalSize"] = formatSize(ti->total_size());
    out["fileCount"] = ti->num_files();

    // info hash (same convention as SessionManager: info_hashes().get_best())
    QString infoHash = QString::fromStdString(
        (std::ostringstream() << ti->info_hashes().get_best()).str());
    out["infoHash"] = infoHash;

    // poster from metadata cache, if this torrent was seen before
    if (m_resolver && m_resolver->hasCached(infoHash)) {
        auto meta = m_resolver->cached(infoHash);
        if (meta.valid) out["posterPath"] = meta.posterPath;
    }

    QVariantList files;
    const lt::file_storage &fs = ti->files();
    for (int i = 0; i < ti->num_files(); ++i) {
        lt::file_index_t fi(i);
        QVariantMap f;
        f["path"] = QString::fromStdString(fs.file_path(fi));
        f["size"] = formatSize(fs.file_size(fi));
        f["dir"]  = false;
        f["depth"] = 0;
        files << f;
    }
    out["files"] = files;
    return out;
}

void QmlSessionBridge::resolvePreview(const QString &infoHash, const QString &name)
{
    if (m_resolver && !infoHash.isEmpty() && !m_resolver->hasCached(infoHash))
        m_resolver->resolve(infoHash, name);
}

void QmlSessionBridge::addTorrentWithPrefs(const QString &filePath, const QString &savePath,
                                           const QVariantList &priorities)
{
    if (filePath.isEmpty()) return;
    QString local = filePath;
    if (local.startsWith(QStringLiteral("file://")))
        local = QUrl(local).toLocalFile();
    QString dest = savePath;
    if (dest.startsWith(QStringLiteral("file://")))
        dest = QUrl(dest).toLocalFile();
    if (dest.isEmpty()) dest = defaultSavePath();

    if (priorities.isEmpty()) {
        m_session->addTorrent(local, dest);
    } else {
        std::vector<int> prios;
        prios.reserve(priorities.size());
        for (const auto &v : priorities) prios.push_back(v.toInt());
        m_session->addTorrentWithPriorities(local, dest, prios);
    }
}

bool QmlSessionBridge::hasSelection() const
{
    return m_selectedIndex >= 0 && m_selectedIndex < m_session->torrentCount();
}

QString QmlSessionBridge::selectedName() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).name;
}

QString QmlSessionBridge::selectedSize() const
{
    if (!hasSelection()) return {};
    return formatSize(m_session->torrentAt(m_selectedIndex).totalSize);
}

QString QmlSessionBridge::selectedHash() const
{
    if (!hasSelection()) return {};
    QString h = m_session->torrentHashAt(m_selectedIndex);
    if (h.size() > 14) return h.left(8) + QStringLiteral("…") + h.right(4);
    return h;
}

QString QmlSessionBridge::selectedDownloaded() const
{
    if (!hasSelection()) return {};
    auto info = m_session->torrentAt(m_selectedIndex);
    return QString("%1 (%2%)").arg(formatSize(info.totalDone))
                              .arg(info.progress * 100.0, 0, 'f', 1);
}

QString QmlSessionBridge::selectedDownSpeed() const
{
    if (!hasSelection()) return QStringLiteral("—");
    return formatSpeed(m_session->torrentAt(m_selectedIndex).downloadRate);
}

QString QmlSessionBridge::selectedUpSpeed() const
{
    if (!hasSelection()) return QStringLiteral("—");
    return formatSpeed(m_session->torrentAt(m_selectedIndex).uploadRate);
}

QString QmlSessionBridge::selectedEta() const
{
    if (!hasSelection()) return QStringLiteral("—");
    auto info = m_session->torrentAt(m_selectedIndex);
    if (info.downloadRate <= 0 || info.totalSize <= info.totalDone) return QStringLiteral("—");
    qint64 secs = (info.totalSize - info.totalDone) / info.downloadRate;
    if (secs < 60) return QString("%1s").arg(secs);
    if (secs < 3600) return QString("%1m %2s").arg(secs / 60).arg(secs % 60);
    return QString("%1h %2m").arg(secs / 3600).arg((secs % 3600) / 60);
}

int QmlSessionBridge::selectedSeeds() const
{
    return hasSelection() ? m_session->torrentAt(m_selectedIndex).numSeeds : 0;
}

int QmlSessionBridge::selectedPeers() const
{
    return hasSelection() ? m_session->torrentAt(m_selectedIndex).numPeers : 0;
}

QString QmlSessionBridge::selectedRatio() const
{
    if (!hasSelection()) return {};
    return QString::number(m_session->torrentAt(m_selectedIndex).ratio, 'f', 2);
}

double QmlSessionBridge::selectedProgress() const
{
    if (!hasSelection()) return 0.0;
    return m_session->torrentAt(m_selectedIndex).progress;
}

QString QmlSessionBridge::selectedUploaded() const
{
    if (!hasSelection()) return {};
    return formatSize(m_session->torrentAt(m_selectedIndex).totalUploaded);
}

QString QmlSessionBridge::selectedAvailability() const
{
    if (!hasSelection()) return {};
    return QString::number(m_session->torrentAt(m_selectedIndex).availability, 'f', 2);
}

QString QmlSessionBridge::selectedAdded() const
{
    if (!hasSelection()) return {};
    const qint64 t = m_session->torrentAt(m_selectedIndex).addedTime;
    if (t <= 0) return QStringLiteral("—");
    const QDateTime dt = QDateTime::fromSecsSinceEpoch(t);
    if (dt.date() == QDate::currentDate())
        return QStringLiteral("Today ") + dt.toString(QStringLiteral("HH:mm"));
    return dt.toString(QStringLiteral("dd MMM HH:mm"));
}

QString QmlSessionBridge::selectedPath() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).savePath;
}

QString QmlSessionBridge::selectedState() const
{
    if (!hasSelection()) return {};
    return m_session->torrentAt(m_selectedIndex).stateString;
}

QString QmlSessionBridge::selectedPoster() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid && !meta.posterPath.isEmpty()) {
            // mtime cache-bust so "fix cover" refreshes the detail-panel art too
            const qint64 mt = QFileInfo(meta.posterPath).lastModified().toMSecsSinceEpoch();
            return meta.posterPath + QStringLiteral("?v=") + QString::number(mt);
        }
    }
    return {};
}

QString QmlSessionBridge::selectedDescription() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid) return meta.description;
    }
    return {};
}

QString QmlSessionBridge::selectedMetaTitle() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (m_resolver->hasCached(hash)) {
        auto meta = m_resolver->cached(hash);
        if (meta.valid) return meta.title;
    }
    return {};
}

QString QmlSessionBridge::selectedMetaInfo() const
{
    if (!hasSelection() || !m_resolver) return {};
    QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (!m_resolver->hasCached(hash)) return {};
    auto meta = m_resolver->cached(hash);
    if (!meta.valid) return {};
    QStringList parts;
    if (meta.year > 0) parts << QString::number(meta.year);
    if (!meta.genres.isEmpty()) parts << meta.genres.mid(0, 3).join(QStringLiteral(", "));
    if (meta.rating > 0) parts << QStringLiteral("%1/10").arg(meta.rating, 0, 'f', 1);
    return parts.join(QStringLiteral(" · "));
}

void QmlSessionBridge::recomputeAggregates()
{
    m_activeCount = m_downloadingCount = m_seedingCount = 0;
    m_pausedCount = m_completedCount = 0;
    m_totalDownRate = m_totalUpRate = 0;
    m_anyDownloading = false;
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i) {
        const auto info = m_session->torrentAt(i);
        m_totalDownRate += info.downloadRate;
        m_totalUpRate   += info.uploadRate;
        if (info.downloadRate > 0 || info.uploadRate > 0) ++m_activeCount;
        if (info.paused) ++m_pausedCount;
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

    // Auto-shutdown arming: when enabled and at least one torrent exists, fire
    // once the moment nothing is downloading anymore. Re-arms when a new
    // download starts, so each "drain" triggers at most one countdown.
    if (QSettings().value(QStringLiteral("autoShutdown"), false).toBool()) {
        if (m_anyDownloading) m_shutdownArmed = true;
        else if (m_shutdownArmed && m_session->torrentCount() > 0) { m_shutdownArmed = false; emit allDownloadsComplete(); }
    } else {
        m_shutdownArmed = false;
    }
}

void QmlSessionBridge::performShutdown()
{
    m_session->saveResumeData();
#ifdef Q_OS_WIN
    QProcess::startDetached("shutdown", {"/s", "/t", "0"});
#elif defined(Q_OS_MACOS)
    QProcess::startDetached("osascript", {"-e", "tell app \"System Events\" to shut down"});
#else
    QProcess::startDetached("shutdown", {"-h", "now"});
#endif
    QCoreApplication::quit();
}

// Theme bridge

