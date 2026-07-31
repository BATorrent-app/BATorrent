// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — archive/video probe + extract orchestration.

#include "torrent/sessionmanager.h"
#include "services/security/archivescan.h"
#include "services/security/archiveextractor.h"

#include <libtorrent/torrent_info.hpp>

void SessionManager::extractArchives(const QString &savePath, const QString &torrentName,
                                     const QString &priorityPassword, const QString &infoHash)
{
    m_extractor->setPasswords(m_extractPasswords);
    m_extractor->setDeleteAfter(m_autoExtractDelete);
    m_extractor->extract(savePath, torrentName, priorityPassword, infoHash);
}

bool SessionManager::torrentHasArchives(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return false;
    if (!m_torrents[index].is_valid()) return false;   // torrent_file() throws on an invalid handle
    auto ti = m_torrents[index].torrent_file();
    if (!ti) return false;
    const auto &fs = ti->files();
    QStringList names;
    for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
        QString p = QString::fromStdString(fs.file_path(i));
        if (p.endsWith(QLatin1String(".!bt"))) p.chop(4);
        names << p;
    }
    return !ArchiveScan::archivesToExtract(names).isEmpty();
}

bool SessionManager::torrentHasVideo(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return false;
    if (!m_torrents[index].is_valid()) return false;   // torrent_file() throws on an invalid handle
    auto ti = m_torrents[index].torrent_file();
    if (!ti) return false;
    static const QStringList videoExts = {".mp4",".mkv",".avi",".mov",".wmv",".flv",".webm",".m4v",".ts",".mpg",".mpeg",".m2ts"};
    const auto &fs = ti->files();
    for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
        QString p = QString::fromStdString(fs.file_path(i)).toLower();
        if (p.endsWith(QLatin1String(".!bt"))) p.chop(4);
        for (const auto &ext : videoExts)
            if (p.endsWith(ext)) return true;
    }
    return false;
}

void SessionManager::extractTorrent(int index, const QString &password)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return;
    const TorrentInfo info = torrentAt(index);
    extractArchives(info.savePath, info.name, password, torrentHashAt(index));
}

