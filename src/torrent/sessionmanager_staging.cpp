// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager: what the Add download window needs from the engine.
// Adds with the window's choices (files, start or not), and magnets that
// wait in the window: added out of the library in upload mode on a scratch
// folder, so they fetch their metadata and nothing else, then either
// promoted into the library as they are or dropped.

#include "torrent/sessionmanager.h"
#include "torrent/magnettrackers.h"

#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/torrent_info.hpp>
#include <QDebug>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QDateTime>
#include <sstream>

namespace {
QString hashOf(const lt::info_hash_t &ih)
{
    return QString::fromStdString((std::ostringstream() << ih.get_best()).str());
}
// where a waiting magnet "lives": nothing is written in upload mode, but
// libtorrent wants a save path
QString scratchDir()
{
    const QString d = QDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation))
                          .filePath(QStringLiteral("BATorrent-staging"));
    QDir().mkpath(d);
    return d;
}
}

bool SessionManager::isStaged(const lt::torrent_handle &h) const
{
    for (const auto &[hash, st] : m_staged)
        if (st.handle == h) return true;
    return false;
}

void SessionManager::addMagnetWith(const QString &uri, const AddOptions &opts)
{
    addMagnet(uri, opts.savePath);
    if (!opts.paused) return;
    try {
        const QString hash = hashOf(lt::parse_magnet_uri(uri.toStdString()).info_hashes);
        const int i = torrentIndexByInfoHash(hash);
        if (i >= 0) pauseTorrent(i);
    } catch (const std::exception &) {}
}

void SessionManager::stageMagnet(const QString &uri)
{
    try {
        lt::add_torrent_params atp = lt::parse_magnet_uri(uri.toStdString());
        const QString hash = hashOf(atp.info_hashes);
        if (isDuplicate(atp.info_hashes)) return;          // the window says so itself
        if (auto it = m_staged.find(hash); it != m_staged.end()) {
            if (it->second.handle.is_valid() && it->second.handle.status().has_metadata)
                onStagedMetadata(it->second.handle);
            return;
        }
        atp.save_path = scratchDir().toStdString();
        atp.flags &= ~(lt::torrent_flags::auto_managed | lt::torrent_flags::paused);
        atp.flags |= lt::torrent_flags::upload_mode;        // metadata only, no pieces
        if (QSettings("BATorrent", "BATorrent").value("addPublicTrackers", true).toBool())
            bat::appendPublicTrackers(atp.trackers, atp.tracker_tiers);
        lt::torrent_handle h = m_session.add_torrent(atp);
        m_staged[hash] = Staged{ h, uri };
        qDebug() << "[session] staged magnet" << hash.left(8);
    } catch (const std::exception &e) {
        qWarning() << "[session] stageMagnet failed:" << e.what();
    }
}

void SessionManager::onStagedMetadata(const lt::torrent_handle &h)
{
    for (const auto &[hash, st] : m_staged) {
        if (st.handle != h) continue;
        auto ti = h.torrent_file();
        if (!ti) return;
        QStringList paths;
        QList<qint64> sizes;
        const auto &fs = ti->files();
        for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
            paths << QString::fromStdString(fs.file_path(i));
            sizes << fs.file_size(i);
        }
        emit stagedMetadata(hash, QString::fromStdString(ti->name()), ti->total_size(), paths, sizes);
        return;
    }
}

void SessionManager::discardStaged(const QString &hash)
{
    auto it = m_staged.find(hash);
    if (it == m_staged.end()) return;
    try {
        if (it->second.handle.is_valid()) m_session.remove_torrent(it->second.handle);
    } catch (const std::exception &) {}
    m_staged.erase(it);
}

void SessionManager::commitStaged(const QString &hash, const AddOptions &opts)
{
    const QString &savePath = opts.savePath;
    const std::vector<int> &filePriorities = opts.filePriorities;
    const bool paused = opts.paused;
    auto it = m_staged.find(hash);
    if (it == m_staged.end()) {             // never staged (or already gone): a plain add
        return;
    }
    const Staged st = it->second;
    m_staged.erase(it);
    lt::torrent_handle h = st.handle;
    if (!h.is_valid()) { addMagnetWith(st.uri, opts); return; }

    // Promoted as it is rather than removed and added again: libtorrent drops
    // a torrent asynchronously, and the same hash added back at once comes
    // back as the old, dying handle.
    try {
        const bool hasMeta = h.status().has_metadata;
        QString target = savePath;
        if (!m_tempPath.isEmpty() && QDir(m_tempPath).exists()) {
            m_torrentIntendedPath[hash] = savePath;
            target = m_tempPath;
        }
        h.move_storage(target.toStdString());
        if (hasMeta && !filePriorities.empty()) {
            std::vector<lt::download_priority_t> pr;
            pr.reserve(filePriorities.size());
            for (int p : filePriorities)
                pr.push_back(static_cast<lt::download_priority_t>(static_cast<std::uint8_t>(p)));
            h.prioritize_files(pr);
        }
        h.unset_flags(lt::torrent_flags::upload_mode);
        if (paused) h.pause(); else h.resume();

        m_torrents.push_back(h);
        m_magnetHashes[h] = hash;
        incrementTorrentCount();
        if (hasMeta) {
            handleMetadata(h);              // .!bt names, private torrents, resume save, scan
        } else {
            m_magnetAddedAt[h] = QDateTime::currentSecsSinceEpoch();
            lt::add_torrent_params atp = lt::parse_magnet_uri(st.uri.toStdString());
            persistMagnetParams(atp, hash, savePath);
        }
        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
        maybeAutoExcludeDefender(target);
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}
