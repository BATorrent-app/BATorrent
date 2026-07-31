// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — "why is this slow" report for the selected torrent.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/metadata/metadataresolver.h"
#include "services/metadata/nameparser.h"
#include <QList>
#include "services/platform/translator.h"
#include "services/platform/utils.h"

#include <QStringList>

static QList<int> resolveRows(const QList<int> &rows, int idx)
{
    if (!rows.isEmpty()) return rows;
    return idx >= 0 ? QList<int>{idx} : QList<int>{};
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

void QmlSessionBridge::openSelectedFile()
{
    if (!hasSelection()) return;
    const QString path = m_session->torrentRootPath(m_selectedIndex);
    if (!path.isEmpty()) revealInFileManager(path);   // open the folder with the item selected
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

