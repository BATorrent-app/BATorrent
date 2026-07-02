// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — torrent-info slice. The per-torrent projections the detail
// panel reads (peers, files, trackers, pieces) and the file/tracker mutations
// it triggers (priority, sequential, rename, move-storage, replace-trackers).
// Split out of sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/peer_info.hpp>
#include <libtorrent/announce_entry.hpp>
#include <QFileInfo>
#include <QDir>
#include <sstream>

std::vector<PeerInfo> SessionManager::peersAt(int index, int maxPeers) const
{
    std::vector<PeerInfo> result;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return result;
    if (!m_torrents[index].is_valid())
        return result;

    try {
        std::vector<lt::peer_info> peers;
        m_torrents[index].get_peer_info(peers);

        // Cap huge swarms (9k+) to the most active peers before building the
        // QString-heavy PeerInfo — the long tail isn't worth the work/UI cost.
        if (maxPeers > 0 && peers.size() > static_cast<std::size_t>(maxPeers)) {
            std::partial_sort(peers.begin(), peers.begin() + maxPeers, peers.end(),
                [](const lt::peer_info &a, const lt::peer_info &b) {
                    return (a.down_speed + a.up_speed) > (b.down_speed + b.up_speed);
                });
            peers.resize(maxPeers);
        }

        for (const auto &p : peers) {
            PeerInfo pi;
            try {
                pi.ip = QString::fromStdString(p.ip.address().to_string());
            } catch (...) { continue; }
            pi.port = p.ip.port();
            pi.downloadRate = p.down_speed;
            pi.uploadRate = p.up_speed;
            pi.progress = p.progress;
            // Guard against corrupt/oversized client strings
            if (p.client.size() < 256)
                pi.client = QString::fromStdString(p.client);
            result.push_back(pi);
        }
    } catch (const std::exception &e) {
        qWarning() << "peersAt:" << e.what();
    } catch (...) {
        qWarning() << "peersAt: unknown exception";
    }
    return result;
}

std::vector<FileInfo> SessionManager::filesAt(int index) const
{
    std::vector<FileInfo> result;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return result;
    if (!m_torrents[index].is_valid())
        return result;

    auto ti = m_torrents[index].torrent_file();
    if (!ti) return result;

    std::vector<std::int64_t> fileProgress;
    m_torrents[index].file_progress(fileProgress);

    const auto &fs = ti->files();
    for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
        int idx = static_cast<int>(i);
        FileInfo fi;
        fi.path = QString::fromStdString(fs.file_path(i));
        fi.size = fs.file_size(i);
        fi.progress = (fi.size > 0 && idx < static_cast<int>(fileProgress.size()))
            ? static_cast<float>(fileProgress[idx]) / fi.size : 0.0f;
        fi.priority = static_cast<std::uint8_t>(m_torrents[index].file_priority(i));
        result.push_back(fi);
    }
    return result;
}

std::vector<TrackerInfo> SessionManager::trackersAt(int index) const
{
    std::vector<TrackerInfo> result;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return result;
    if (!m_torrents[index].is_valid())
        return result;

    try {
        auto trackers = m_torrents[index].trackers();
        for (const auto &t : trackers) {
            TrackerInfo ti;
            ti.url = QString::fromStdString(t.url);
            ti.tier = t.tier;
            ti.status = "OK";
            if (!t.endpoints.empty()) {
                const auto &msg = t.endpoints[0].info_hashes[lt::protocol_version::V1].message;
                if (!msg.empty())
                    ti.status = QString::fromStdString(msg);
            }
            result.push_back(ti);
        }
    } catch (const std::exception &e) {
        // Torrent may not be valid yet (metadata still downloading) — that's
        // expected, but log everything else so unexpected libtorrent
        // exceptions don't disappear.
        qWarning() << "trackersAt:" << e.what();
    } catch (...) {
        qWarning() << "trackersAt: unknown exception";
    }
    return result;
}

void SessionManager::addTracker(int index, const QString &url)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[index].is_valid()) return;   // add_tracker throws on an invalid handle
    lt::announce_entry ae(url.toStdString());
    m_torrents[index].add_tracker(ae);
}

void SessionManager::setFilePriority(int torrentIndex, int fileIndex, int priority)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[torrentIndex].is_valid()) return;   // file_priority throws on an invalid handle
    m_torrents[torrentIndex].file_priority(
        lt::file_index_t(fileIndex),
        static_cast<lt::download_priority_t>(static_cast<std::uint8_t>(priority)));
}

void SessionManager::setSequentialDownload(int index, bool sequential)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (sequential)
        m_torrents[index].set_flags(lt::torrent_flags::sequential_download);
    else
        m_torrents[index].unset_flags(lt::torrent_flags::sequential_download);
}

bool SessionManager::isSequentialDownload(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()) || !m_torrents[index].is_valid())
        return false;
    return (m_torrents[index].flags() & lt::torrent_flags::sequential_download)
           != lt::torrent_flags_t{};
}

// The streaming slice (prioritizeFilePieceBoundaries + stream* queries) lives in
// sessionmanager_streaming.cpp. torrentIndexByInfoHash stays here as a general
// index/hash helper used across the engine, not just by streaming.
int SessionManager::torrentIndexByInfoHash(const QString &infoHash) const
{
    if (infoHash.isEmpty()) return -1;
    for (int i = 0; i < static_cast<int>(m_torrents.size()); ++i)
        if (torrentHash(i).compare(infoHash, Qt::CaseInsensitive) == 0)
            return i;
    return -1;
}

void SessionManager::renameFile(int torrentIndex, int fileIndex,
                                 const QString &newRelativePath)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[torrentIndex].is_valid()) return;
    m_torrents[torrentIndex].rename_file(
        lt::file_index_t(fileIndex), newRelativePath.toStdString());
}

void SessionManager::moveStorage(int torrentIndex, const QString &newSavePath)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[torrentIndex].is_valid()) return;
    m_torrents[torrentIndex].move_storage(newSavePath.toStdString());
    // A move can leave the filesystem and libtorrent's piece map out of
    // sync if any underlying file failed to move; re-checking is the safe
    // default so the torrent doesn't keep re-downloading good pieces.
}

void SessionManager::replaceTrackers(int torrentIndex, const QStringList &urls)
{
    if (torrentIndex < 0 || torrentIndex >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[torrentIndex].is_valid()) return;
    std::vector<lt::announce_entry> entries;
    entries.reserve(urls.size());
    for (const QString &u : urls) {
        if (u.trimmed().isEmpty()) continue;
        entries.emplace_back(u.toStdString());
    }
    m_torrents[torrentIndex].replace_trackers(entries);
}

// Category assignment + per-torrent tags live in sessionmanager_categories.cpp.

std::vector<bool> SessionManager::piecesAt(int index) const
{
    std::vector<bool> result;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return result;
    if (!m_torrents[index].is_valid())
        return result;

    lt::torrent_status st = m_torrents[index].status(lt::torrent_handle::query_pieces);
    int numPieces = st.pieces.size();
    result.resize(numPieces, false);
    for (int i = 0; i < numPieces; ++i)
        result[i] = st.pieces[lt::piece_index_t(i)];

    return result;
}
