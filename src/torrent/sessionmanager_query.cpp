// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — torrent list queries (count/at/hash/root/state string).

#include "torrent/sessionmanager.h"
#include "services/platform/translator.h"

#include <libtorrent/torrent_status.hpp>
#include <libtorrent/torrent_info.hpp>
#include <boost/system/error_code.hpp>
#include <sstream>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QDebug>

int SessionManager::torrentCount() const
{
    return static_cast<int>(m_torrents.size());
}

TorrentInfo SessionManager::torrentAt(int index) const
{
    TorrentInfo info{};
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return info;
    if (!m_torrents[index].is_valid())
        return info;

    try {
    lt::torrent_status st = cachedStatus(m_torrents[index]);
    info.handle = m_torrents[index];
    info.name = QString::fromStdString(st.name);
    info.savePath = QString::fromStdString(st.save_path);
    info.totalSize = st.total_wanted;
    info.totalDone = st.total_wanted_done;
    info.progress = st.progress;
    // Qualified, never raw: without metadata total_wanted is 0 and libtorrent
    // reports both of these true for a magnet that has done nothing.
    const bool hasWork = torrentHasWork(st.has_metadata, static_cast<long long>(st.total_wanted));
    info.hasError = bool(st.errc);
    info.finished = hasWork && st.is_finished;
    info.seeding = hasWork && st.is_seeding;
    info.numPeers = st.num_peers;
    info.numSeeds = st.num_seeds;
    info.stateString = stateToString(st.state);
    info.paused = (st.flags & lt::torrent_flags::paused) != lt::torrent_flags_t{};
    if (info.paused && m_queuePaused.count(m_torrents[index])) {
        info.queued = true;
        info.queuePos = 1;
        for (int i = 0; i < index; ++i)
            if (m_queuePaused.count(m_torrents[i])) ++info.queuePos;
    }
    QString hash;
    if (st.has_metadata)
        hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());

    if (!hash.isEmpty())
        info.completed = m_completedTorrents.contains(hash);

    if (info.completed) {
        info.stateString = tr_("state_completed");
        info.downloadRate = 0;
        info.uploadRate = 0;
    } else if (info.paused) {
        // "Stop seeding after download" pauses the handle directly (see
        // onTorrentFinished) without going through markCompleted(), so a
        // finished torrent otherwise reads as bare "Paused" — ambiguous
        // about whether the download itself is done (reported by a user).
        info.stateString = info.queued
            ? tr_("state_queued").arg(info.queuePos)
            : (info.finished) ? tr_("state_paused_done") : tr_("state_paused");
        info.downloadRate = 0;
        info.uploadRate = 0;
    } else {
        // Payload, not the total: download_rate counts protocol chatter
        // (handshakes, HAVE, bitfields, keepalives, incoming requests), so a
        // torrent sitting at 100% reported a permanent trickle of "download"
        // and never looked done. It also made every connected torrent count as
        // active, and skewed ETA — which libtorrent's own docs call out.
        info.downloadRate = st.download_payload_rate;
        info.uploadRate = st.upload_payload_rate;
    }

    // Data deleted/moved out from under a live torrent: libtorrent errors on the
    // missing path (ENOENT). Surface it as a distinct, actionable state instead of
    // a torrent that silently reads as complete/seeding while the files are gone
    // (tester: a movie sat inactive with no explanation after a manual delete). The
    // errc == enum compare is category-aware, so it matches on every platform.
    if (st.errc == boost::system::errc::no_such_file_or_directory) {
        info.filesMissing = true;
        info.stateString = tr_("state_files_missing");
        info.stateDetail = tr_("state_files_missing");
        info.downloadRate = 0;
        info.uploadRate = 0;
    } else if (info.hasError) {
        // Any other storage failure: disk full, permissions, a read-only volume.
        info.stateString = tr_("state_error");
        info.stateDetail = QString::fromStdString(st.errc.message());
    }

    // qBittorrent's most-repeated complaint is a silent "stalled" — name the
    // actual blocker so the state cell can explain itself on hover
    if (!info.completed && !info.paused && !info.finished
            && st.state == lt::torrent_status::downloading
            && info.downloadRate < 1024) {
        if (st.errc)
            info.stateDetail = QString::fromStdString(st.errc.message());
        else if (info.numPeers == 0)
            // candidates known but none connected yet = the "connecting" phase;
            // nothing found at all = still searching the swarm
            info.stateDetail = st.connect_candidates > 0
                ? tr_("state_connecting")
                : (m_dhtEnabled ? tr_("state_no_peers_dht") : tr_("state_no_peers"));
        else if (info.numSeeds == 0)
            info.stateDetail = tr_("state_no_seeds");
        else
            info.stateDetail = tr_("state_choked");
    } else if (!info.paused && st.state == lt::torrent_status::downloading_metadata) {
        // A rare-seeder magnet can take a long time to find a peer that'll
        // hand over metadata — we used to give up and silently delete it
        // after 5 minutes (issue reported by a user: "deleted without
        // warning"). Explain the wait instead; the user decides when to quit.
        auto it = m_magnetAddedAt.find(m_torrents[index]);
        if (it != m_magnetAddedAt.end()) {
            const qint64 mins = (QDateTime::currentSecsSinceEpoch() - it->second) / 60;
            info.stateDetail = tr_("state_fetching_metadata").arg(mins);
        }
    } else if (!info.completed && !info.paused && !info.finished
               && st.state == lt::torrent_status::downloading
               && info.downloadRate >= 1024 && info.numPeers > 0
               && st.distributed_copies >= 0.0f && st.distributed_copies < 1.0f) {
        // distributed_copies < 1 means some piece of this torrent isn't held by
        // anyone currently in the swarm — the transfer can look healthy (decent
        // rate, progress moving) right up until it needs that missing piece and
        // stalls for good. Surface it early instead of only once it's stuck.
        info.stateDetail = tr_("state_missing_pieces");
    }

    qint64 uploaded = st.total_payload_upload;
    qint64 downloaded = st.total_payload_download;
    info.ratio = downloaded > 0 ? static_cast<float>(uploaded) / static_cast<float>(downloaded) : 0.0f;
    info.totalUploaded = st.all_time_upload;
    info.availability = st.distributed_copies < 0 ? 0.0f : st.distributed_copies;
    info.addedTime = static_cast<qint64>(st.added_time);

    if (!hash.isEmpty()) {
        info.category = m_categories.value(hash);
        info.tags = m_torrentTags.value(hash);
    }

    return info;
    } catch (const std::exception &e) {
        qWarning() << "[session] torrentAt exception:" << e.what();
        return TorrentInfo{};
    } catch (...) {
        qWarning() << "[session] torrentAt: unknown exception";
        return TorrentInfo{};
    }
}


QString SessionManager::torrentHash(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return {};
    if (!m_torrents[index].is_valid()) return {};
    lt::torrent_status st = cachedStatus(m_torrents[index]);
    // Magnet links report an all-zeros hash from get_best() until metadata
    // is downloaded. Returning that string would cause every still-resolving
    // magnet to share the same key — categories and per-torrent seeding
    // overrides set on one would silently apply to all others. Use the real
    // per-magnet hash captured from the URI at add time instead, so the row has
    // a stable unique key (cover/name resolve without waiting for metadata).
    if (!st.has_metadata) {
        auto it = m_magnetHashes.find(m_torrents[index]);
        return it != m_magnetHashes.end() ? it->second : QString();
    }
    return QString::fromStdString(
        (std::ostringstream() << st.info_hashes.get_best()).str());
}



QString SessionManager::torrentHashAt(int index) const
{
    return torrentHash(index);
}

QString SessionManager::torrentRootPath(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return {};
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return {};
    lt::torrent_status st = cachedStatus(h);
    QString save = QString::fromStdString(st.save_path);

    // Existence checker that also tries the ".!bt" suffix (in-progress
    // rename) AND native separators (Windows backslash → forward-slash
    // mismatch from libtorrent can cause QFileInfo::exists to fail on
    // some configs).
    auto existsOnDisk = [](const QString &path) -> QString {
        if (QFileInfo::exists(path)) return path;
        const QString native = QDir::toNativeSeparators(path);
        if (native != path && QFileInfo::exists(native)) return native;
        if (QFileInfo::exists(path + ".!bt")) return path + ".!bt";
        return {};
    };

    // Strategy 1: file_path(0) — the most reliable source since it comes
    // from the torrent metadata and matches what libtorrent wrote to disk.
    auto ti = h.torrent_file();
    if (ti && ti->num_files() > 0) {
        // libtorrent's file_path uses the native separator ('\' on Windows),
        // so normalize to '/' before stripping — otherwise indexOf('/') misses
        // the folder boundary on Windows and we resolve to file index 0 (an
        // arbitrary .dll / .rNN), selecting it instead of opening the folder.
        QString rel = QDir::fromNativeSeparators(QString::fromStdString(
            ti->files().file_path(lt::file_index_t(0))));
        if (ti->num_files() > 1) {
            const int slash = rel.indexOf(QLatin1Char('/'));
            // Strip to the top-level folder. A multi-file torrent with no
            // common folder (files written straight into save_path) has no
            // slash — leave rel empty so we fall through to save_path rather
            // than selecting an arbitrary first file.
            rel = slash > 0 ? rel.left(slash) : QString();
        }
        if (!rel.isEmpty()) {
            QString found = existsOnDisk(save + QLatin1Char('/') + rel);
            if (!found.isEmpty()) {
                qInfo().noquote() << "[reveal] root via file_path:" << found;
                return found;
            }
        }
    }

    // Strategy 2: torrent display name (st.name). May differ from
    // file_path(0) after renames, sanitization, or cross-platform transfer
    // (Windows strips characters that macOS/Linux kept). Covers the common
    // case where file_path was sanitized but name wasn't, or vice-versa.
    QString name = QString::fromStdString(st.name);
    if (!name.isEmpty()) {
        QString found = existsOnDisk(save + QLatin1Char('/') + name);
        if (!found.isEmpty()) {
            qInfo().noquote() << "[reveal] root via name:" << found;
            return found;
        }
    }

    // Strategy 3: scan save_path for a directory or file whose name
    // case-insensitively matches the torrent display name. Handles partial
    // renames and encoding mismatches (e.g. ñ vs n, full-width chars).
    if (!name.isEmpty()) {
        const QDir dir(save);
        const auto entries = dir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
        for (const QString &entry : entries) {
            if (entry.compare(name, Qt::CaseInsensitive) == 0) {
                qInfo().noquote() << "[reveal] root via dir scan:" << (save + QLatin1Char('/') + entry);
                return save + QLatin1Char('/') + entry;
            }
        }
    }

    // All strategies exhausted — fall back to the save directory itself. If
    // this fires, the torrent's folder/file wasn't found on disk under
    // save_path — the reveal lands in the (possibly huge) save folder.
    qWarning().noquote() << "[reveal] FELL BACK to save_path:" << save
                         << "| torrent name=" << name;
    return save;
}


lt::torrent_status SessionManager::cachedStatus(const lt::torrent_handle &h) const
{
    auto it = m_statusCache.find(h);
    if (it != m_statusCache.end())
        return it->second;
    // Cache miss: brand-new torrent before the first state_update_alert
    // landed. Fall back to a live call (and warm the cache) so the first
    // refresh after add doesn't show "-" everywhere.
    if (h.is_valid()) {
        lt::torrent_status st = h.status();
        m_statusCache[h] = st;
        return st;
    }
    return {};
}

// processAlerts() + every on*() alert handler live in sessionmanager_alerts.cpp.


QString SessionManager::stateToString(lt::torrent_status::state_t state)
{
    switch (state) {
    case lt::torrent_status::checking_files: return tr_("state_checking");
    case lt::torrent_status::downloading_metadata: return tr_("state_metadata");
    case lt::torrent_status::downloading: return tr_("state_downloading");
    case lt::torrent_status::finished: return tr_("state_finished");
    case lt::torrent_status::seeding: return tr_("state_seeding");
    case lt::torrent_status::checking_resume_data: return tr_("state_checking");
    default: return tr_("state_unknown");
    }
}
