// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: selected-torrent actions (pause/remove/queue/limits/…).
// Split out of qmlsessionbridge.cpp verbatim; no behaviour change.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/security/defender.h"
#include "services/metadata/metadataresolver.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QDateTime>
#include <QTimer>
#include <QSettings>
#include <algorithm>

// Resolve the active rows (multi-select, falling back to the focus index).
static QList<int> resolveRows(const QList<int> &rows, int idx)
{
    if (!rows.isEmpty()) return rows;
    return idx >= 0 ? QList<int>{idx} : QList<int>{};
}

void QmlSessionBridge::pauseSelected()
{
    if (m_selectedRows.isEmpty()) {
        if (m_selectedIndex >= 0) m_session->pauseTorrent(m_selectedIndex);
        return;
    }
    for (int r : m_selectedRows) m_session->pauseTorrent(r);
}

bool QmlSessionBridge::allSelectedPaused() const
{
    if (m_selectedRows.isEmpty()) return selectedPaused();
    const int n = m_session->torrentCount();
    for (int r : m_selectedRows)
        if (r < 0 || r >= n || !m_session->torrentAt(r).paused) return false;
    return true;
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
    // A game installed outside its download stays playable: record it as a
    // kept game now, while its torrent is still there to name it.
    for (int r : rows)
        if (gameOutlivesTorrent(m_session->torrentHashAt(r))) { gameLibrary(); break; }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    // A batch fires beginRemoveRows/endRemoveRows back-to-back with no time for
    // the grid/list's remove+displaced Transition to settle between them:
    // flagged so the view can skip animating a batch (see LibraryView.qml).
    if (n > 1) { m_bulkRemoveInProgress = true; emit bulkRemoveInProgressChanged(); }
    for (int r : rows) m_session->removeTorrent(r, deleteFiles, permanent);
    if (n > 1) { m_bulkRemoveInProgress = false; emit bulkRemoveInProgressChanged(); }
    m_selectedRows.clear();
    m_selectedIndex = -1;
    emit selectionChanged(); emit selectionListsChanged();
    // No toast: the remove dialog already said what happens to the files.
}
void QmlSessionBridge::removeSelectedWithFilesPermanent() { removeSelectedRows(true, true); }

void QmlSessionBridge::removeSelected()          { removeSelectedRows(false); }
void QmlSessionBridge::removeSelectedWithFiles() { removeSelectedRows(true); }

QVariantMap QmlSessionBridge::removalInfo()
{
    const QList<int> rows = resolveRows(m_selectedRows, m_selectedIndex);
    QVariantMap m;
    m[QStringLiteral("count")] = rows.size();
    if (rows.isEmpty()) return m;

    QStringList names;
    QString folder, poster, game, gameSize;
    bool oneFolder = true;
    qint64 bytes = 0;
    int peers = 0;
    for (int r : rows) {
        const TorrentInfo info = m_session->torrentAt(r);
        const QString hash = m_session->torrentHashAt(r);
        QString cover;
        const QString name = shownTitle(hash, info.name, &cover);
        if (poster.isEmpty()) poster = cover;
        names << name;
        bytes += info.totalDone;
        // the people this stops: only what is seeding right now
        if (info.seeding && !info.paused) peers += info.numPeers;
        const QString dir = QDir::toNativeSeparators(info.savePath);
        if (folder.isEmpty()) folder = dir;
        else if (dir.compare(folder, Qt::CaseInsensitive) != 0) oneFolder = false;
        if (game.isEmpty() && gameOutlivesTorrent(hash)) {
            game = name;
            gameSize = formatSize(info.totalDone);
        }
    }
    m[QStringLiteral("names")] = names;
    m[QStringLiteral("poster")] = poster;
    m[QStringLiteral("size")] = formatSize(bytes);
    m[QStringLiteral("folder")] = oneFolder ? folder : QString();
    m[QStringLiteral("peers")] = peers;
    m[QStringLiteral("game")] = game;
    m[QStringLiteral("gameSize")] = gameSize;
    return m;
}

void QmlSessionBridge::pauseAll() { m_session->pauseAll(); }
void QmlSessionBridge::resumeAll() { m_session->resumeAll(); }

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
    const EngineFlags *f = selectedEngineFlags();
    return f && f->playable;
}

// The last answer for the selected torrent, and a fresh question queued when
// there is none or it is old. Never asks the engine itself (see the header).
const QmlSessionBridge::EngineFlags *QmlSessionBridge::selectedEngineFlags() const
{
    if (!hasSelection()) return nullptr;
    const QString hash = m_session->torrentHashAt(m_selectedIndex);
    if (hash.isEmpty()) return nullptr;
    const auto it = m_engineFlags.constFind(hash);
    const bool stale = it == m_engineFlags.constEnd()
                       || QDateTime::currentMSecsSinceEpoch() - it->at > 5000;
    if (stale && !m_engineFlagsQueued.contains(hash)) {
        m_engineFlagsQueued.insert(hash);
        QTimer::singleShot(0, const_cast<QmlSessionBridge *>(this), [this, hash]() {
            const_cast<QmlSessionBridge *>(this)->fetchEngineFlags(hash);
        });
    }
    return it == m_engineFlags.constEnd() ? nullptr : &it.value();
}

void QmlSessionBridge::fetchEngineFlags(const QString &hash)
{
    m_engineFlagsQueued.remove(hash);
    const int row = m_session->torrentIndexByInfoHash(hash);
    if (row < 0) { m_engineFlags.remove(hash); return; }
    EngineFlags f;
    // A game that bundles cutscene/intro videos must not offer Play: Install wins.
    f.isGame = isGameTorrent(row);
    f.playable = m_session->torrentHasVideo(row) && !f.isGame;
    f.sequential = m_session->isSequentialDownload(row);
    f.forceStart = m_session->isForceStart(row);
    f.superSeeding = m_session->isSuperSeeding(row);
    f.at = QDateTime::currentMSecsSinceEpoch();
    const auto old = m_engineFlags.constFind(hash);
    const bool changed = old == m_engineFlags.constEnd()
                         || old->playable != f.playable || old->sequential != f.sequential
                         || old->forceStart != f.forceStart || old->superSeeding != f.superSeeding
                         || old->isGame != f.isGame;
    m_engineFlags.insert(hash, f);
    if (changed) emit selectionChanged();
}

void QmlSessionBridge::extractSelected(const QString &password)
{
    if (!hasSelection()) return;
    m_session->extractTorrent(m_selectedIndex, password);
}

bool QmlSessionBridge::selectedForceStart() const
{
    const EngineFlags *f = selectedEngineFlags();
    return f && f->forceStart;
}

bool QmlSessionBridge::selectedSuperSeeding() const
{
    const EngineFlags *f = selectedEngineFlags();
    return f && f->superSeeding;
}

bool QmlSessionBridge::selectedCompleted() const
{
    return hasSelection() && m_session->torrentAt(m_selectedIndex).completed;
}

bool QmlSessionBridge::selectedPaused() const
{
    return hasSelection() && m_session->torrentAt(m_selectedIndex).paused;
}

void QmlSessionBridge::setSelectedForceStart(bool on)
{
    if (hasSelection()) {
        m_session->setForceStart(m_selectedIndex, on);
        auto it = m_engineFlags.find(m_session->torrentHashAt(m_selectedIndex));
        if (it != m_engineFlags.end()) it->forceStart = on;
    }
    emit selectionChanged(); emit selectionListsChanged();
}

void QmlSessionBridge::setSelectedSuperSeeding(bool on)
{
    if (hasSelection()) {
        m_session->setSuperSeeding(m_selectedIndex, on);
        auto it = m_engineFlags.find(m_session->torrentHashAt(m_selectedIndex));
        if (it != m_engineFlags.end()) it->superSeeding = on;
    }
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

void QmlSessionBridge::refreshAll()
{
    // Manual "Refresh": re-announce every torrent to its trackers (fetch a
    // fresh peer set) and push a stats recompute now. The list already updates
    // live per tick: this is the on-demand kick some users want.
    const int n = m_session->torrentCount();
    for (int i = 0; i < n; ++i)
        m_session->forceReannounce(i);
    emitStats();
    // Re-emit every role now so speed/ETA/status visibly repaint on the spot,
    // instead of the user waiting for the next periodic tick to see any change.
    emit queueRefreshNeeded();
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
    for (int r : resolveRows(m_selectedRows, m_selectedIndex)) {
        m_session->setSequentialDownload(r, on);
        auto it = m_engineFlags.find(m_session->torrentHashAt(r));
        if (it != m_engineFlags.end()) it->sequential = on;
    }
    emit selectionChanged(); emit selectionListsChanged();
}

bool QmlSessionBridge::selectedSequential() const
{
    const EngineFlags *f = selectedEngineFlags();
    return f && f->sequential;
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
