// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmlsessionbridge.h"
#include "torrent/iengine.h"
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
    // A batch fires beginRemoveRows/endRemoveRows back-to-back with no time for
    // the grid/list's remove+displaced Transition to settle between them —
    // flagged so the view can skip animating a batch (see LibraryView.qml).
    if (n > 1) { m_bulkRemoveInProgress = true; emit bulkRemoveInProgressChanged(); }
    for (int r : rows) m_session->removeTorrent(r, deleteFiles, permanent);
    if (n > 1) { m_bulkRemoveInProgress = false; emit bulkRemoveInProgressChanged(); }
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
    // A game that bundles cutscene/intro videos must not offer Play — Install wins.
    return hasSelection() && m_session->torrentHasVideo(m_selectedIndex)
           && !isGameTorrent(m_selectedIndex);
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

// Xunlei thunder:// links: base64 of "AA" + the real URL + "ZZ". Decode, then
// fall through to the normal handling (it usually wraps a magnet). Returns a
// normalized "magnet:..." uri, or an empty string if clip isn't magnet-like.
QString QmlSessionBridge::normalizeClipboardMagnet(const QString &clip)
{
    QString s = clip;
    if (s.startsWith(QStringLiteral("thunder://"), Qt::CaseInsensitive)) {
        QString dec = QString::fromUtf8(
            QByteArray::fromBase64(s.mid(10).toLatin1())).trimmed();
        if (dec.startsWith(QStringLiteral("AA"), Qt::CaseInsensitive)
            && dec.endsWith(QStringLiteral("ZZ"), Qt::CaseInsensitive))
            dec = dec.mid(2, dec.size() - 4);
        s = dec.trimmed();
        if (s.isEmpty()) return QString();
    }
    if (s.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)
        || s.startsWith(QStringLiteral("bittorrent:"), Qt::CaseInsensitive))
        return s;
    static const QRegularExpression hashRe(QStringLiteral("^[0-9a-fA-F]{40}$"));
    if (hashRe.match(s).hasMatch())
        return QStringLiteral("magnet:?xt=urn:btih:") + s;
    return QString();
}

void QmlSessionBridge::smartPaste()
{
    QString clip = QGuiApplication::clipboard()->text().trimmed();
    if (clip.isEmpty()) { emit toast(tr_("toast_paste_none"), QString()); return; }
    QString magnet = normalizeClipboardMagnet(clip);
    if (!magnet.isEmpty()) {
        addMagnetUri(magnet);
        // confirm always: past the active-download limit the torrent is queued
        // (not visibly at the top), so without this the paste looks ignored
        emit toast(tr_("toast_magnet_added"), QString());
        return;
    }
    if (clip.endsWith(QStringLiteral(".torrent"), Qt::CaseInsensitive)
        && (clip.startsWith(QStringLiteral("http"), Qt::CaseInsensitive)
            || clip.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive))) {
        addTorrentFile(clip);
        return;
    }
    emit toast(tr_("toast_paste_none"), QString());
}

QString QmlSessionBridge::clipboardMagnetIfNew()
{
    QString clip = QGuiApplication::clipboard()->text().trimmed();
    if (clip.isEmpty()) return QString();
    QString magnet = normalizeClipboardMagnet(clip);
    if (magnet.isEmpty() || magnet == m_lastClipboardMagnet) return QString();
    m_lastClipboardMagnet = magnet;
    return magnet;
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
    lines << QStringLiteral("    • %1: %2").arg(tr_("detail_kv_seeds"), QString::number(info.numSeeds));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_down"), formatSpeed(info.downloadRate));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_up"), formatSpeed(info.uploadRate));
    lines << QStringLiteral("    • %1: %2").arg(tr_("col_state"), info.stateString);
    return lines.join('\n');
}


// Playback/streaming (stream URLs, subtitles, play-by-hash, next-episode,
// watch-when-ready) lives in qmlsessionbridge_playback.cpp; the movie library +
// watchlist projections in qmlsessionbridge_library.cpp.

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
        // Windows has no color-emoji flag glyphs (regional indicators render as
        // letter pairs/boxes) — the QML side falls back to this bare code there.
        m["cc"]   = cc.toUpper();
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

QVariantMap QmlSessionBridge::selectedPieces() const
{
    // Bounded + downsampled at the source: a large torrent has tens of thousands of
    // pieces; returning one QVariant each (re-marshalled on every stats tick) spiked
    // memory and crashed the Pieces tab on Windows. Cap to MAXCELLS buckets, each
    // holding the fraction of its piece-span that's done; real total/done stay exact.
    QVariantMap m;
    QVariantList cells;
    if (!hasSelection()) { m["total"] = 0; m["done"] = 0; m["cells"] = cells; return m; }
    const auto pieces = m_session->piecesAt(m_selectedIndex);
    const int total = int(pieces.size());
    int done = 0;
    for (bool b : pieces) if (b) ++done;

    constexpr int MAXCELLS = 2000;
    if (total <= MAXCELLS) {
        cells.reserve(total);
        for (bool b : pieces) cells << (b ? 1.0 : 0.0);
    } else {
        cells.reserve(MAXCELLS);
        for (int c = 0; c < MAXCELLS; ++c) {
            const qint64 lo = qint64(c) * total / MAXCELLS;
            const qint64 hi = qint64(c + 1) * total / MAXCELLS;
            int d = 0, cnt = 0;
            for (qint64 i = lo; i < hi; ++i) { ++cnt; if (pieces[i]) ++d; }
            cells << (cnt > 0 ? double(d) / double(cnt) : 0.0);
        }
    }
    m["total"] = total;
    m["done"]  = done;
    m["cells"] = cells;
    return m;
}

// Free-space queries + the disk-volume breakdown and the active-downloads /
// seeding / resume list projections live in qmlsessionbridge_disk.cpp.

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
    out["totalSizeBytes"] = static_cast<qint64>(ti->total_size());
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

