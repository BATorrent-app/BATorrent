// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — torrent lifecycle slice (add/remove/pause/resume, magnets,
// incomplete-suffix / storage mode). Split out of sessionmanager.cpp verbatim;
// no behaviour change.

#include "torrent/sessionmanager.h"
#include "torrent/magnettrackers.h"
#include "services/platform/logger.h"
#include "services/platform/translator.h"

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/create_torrent.hpp>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QDateTime>
#include <QUrl>
#include <QStorageInfo>
#include <QThread>
#include <sstream>
#include <algorithm>

bool SessionManager::isDuplicate(const lt::info_hash_t &ih) const
{
    for (const auto &h : m_torrents) {
        if (h.is_valid() && h.info_hashes() == ih)
            return true;
    }
    return false;
}

void SessionManager::addTorrent(const QString &filePath, const QString &savePath)
{
    try {
        qDebug() << "[session] addTorrent:" << filePath << "save:" << savePath;
        // Auto-copy .torrent to export directory (backup archive)
        if (!m_torrentExportDir.isEmpty()) {
            QDir exportDir(m_torrentExportDir);
            if (exportDir.exists()) {
                QFileInfo fi(filePath);
                QFile::copy(filePath, exportDir.filePath(fi.fileName()));
            }
        }
        lt::add_torrent_params atp;
        atp.ti = std::make_shared<lt::torrent_info>(filePath.toStdString());
        // Reject duplicates by checking our own list (not m_session.find_torrent,
        // whose handle lingers after the async remove_torrent — that would wrongly
        // block a legit re-add right after removal). m_torrents is erased
        // synchronously on remove, so it reflects the visible state.
        if (atp.ti && isDuplicate(atp.ti->info_hashes())) {
            qDebug() << "[session] addTorrent: duplicate ignored";
            return;
        }
        atp.save_path = savePath.toStdString();
        atp.flags &= ~(lt::torrent_flags::auto_managed
                       | lt::torrent_flags::paused);
        if (atp.ti && atp.ti->priv()) {
            atp.flags |= lt::torrent_flags::disable_dht
                       | lt::torrent_flags::disable_lsd
                       | lt::torrent_flags::disable_pex;
        }
        applyContentLayout(atp);
        applyExcludedPatterns(atp);
        applyIncompleteSuffix(atp);
        applyStorageMode(atp);

        // Temp path: download to temp dir, move to real path on finish
        if (!m_tempPath.isEmpty() && QDir(m_tempPath).exists()) {
            std::string hash = (std::ostringstream() << atp.ti->info_hashes().get_best()).str();
            m_torrentIntendedPath[QString::fromStdString(hash)] = savePath;
            atp.save_path = m_tempPath.toStdString();
        }

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        incrementTorrentCount();
        if (m_autoRecheck && h.is_valid()) h.force_recheck();   // verify pre-existing data on disk
        stageResumeSave(h);   // persist now — an idle 0%/no-peer torrent never

        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
        scanTorrentForThreats(h, QString::fromStdString(h.status().name));   // .torrent: metadata is ready now
        maybeAutoExcludeDefender(QString::fromStdString(atp.save_path));
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}

void SessionManager::addTorrentWithPriorities(const QString &filePath,
                                                const QString &savePath,
                                                const std::vector<int> &filePriorities)
{
    try {
        lt::add_torrent_params atp;
        atp.ti = std::make_shared<lt::torrent_info>(filePath.toStdString());
        if (atp.ti && isDuplicate(atp.ti->info_hashes())) {
            qDebug() << "[session] addTorrentWithPriorities: duplicate ignored";
            return;
        }
        atp.save_path = savePath.toStdString();
        atp.flags &= ~(lt::torrent_flags::auto_managed
                       | lt::torrent_flags::paused);
        if (atp.ti && atp.ti->priv()) {
            atp.flags |= lt::torrent_flags::disable_dht
                       | lt::torrent_flags::disable_lsd
                       | lt::torrent_flags::disable_pex;
        }
        atp.file_priorities.reserve(filePriorities.size());
        for (int p : filePriorities) {
            atp.file_priorities.push_back(
                static_cast<lt::download_priority_t>(static_cast<std::uint8_t>(p)));
        }
        applyContentLayout(atp);
        applyExcludedPatterns(atp);
        applyIncompleteSuffix(atp);
        applyStorageMode(atp);

        if (!m_tempPath.isEmpty() && QDir(m_tempPath).exists()) {
            std::string hash = (std::ostringstream() << atp.ti->info_hashes().get_best()).str();
            m_torrentIntendedPath[QString::fromStdString(hash)] = savePath;
            atp.save_path = m_tempPath.toStdString();
        }

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        incrementTorrentCount();
        if (m_autoRecheck && h.is_valid()) h.force_recheck();   // verify pre-existing data on disk
        stageResumeSave(h);   // persist immediately (see addTorrent)
        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
        scanTorrentForThreats(h, QString::fromStdString(h.status().name));   // .torrent: metadata is ready now
        maybeAutoExcludeDefender(QString::fromStdString(atp.save_path));
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}

void SessionManager::applyIncompleteSuffix(lt::add_torrent_params &atp)
{
    if (!atp.ti) return; // magnet without metadata yet; handled after fetch
    const auto &files = atp.ti->files();
    for (lt::file_index_t i(0); i < files.end_file(); ++i) {
        std::string original = files.file_path(i);
        if (original.size() >= 4
            && original.compare(original.size() - 4, 4, ".!bt") == 0)
            continue; // already suffixed (resume data round-trip)
        atp.renamed_files[i] = original + ".!bt";
    }
}

void SessionManager::addMagnet(const QString &uri, const QString &savePath,
                               const QString &coverHint, int coverType)
{
    try {
        qDebug() << "[session] addMagnet:" << uri.left(80) << "save:" << savePath;
        lt::add_torrent_params atp = lt::parse_magnet_uri(uri.toStdString());
        atp.save_path = savePath.toStdString();
        atp.flags &= ~(lt::torrent_flags::auto_managed
                       | lt::torrent_flags::paused);

        // Magnets often carry few or dead trackers and then depend entirely on
        // DHT for the metadata fetch. Append well-known open trackers;
        // onMetadataReceived strips them again if the torrent turns out private.
        if (QSettings("BATorrent", "BATorrent").value("addPublicTrackers", true).toBool())
            bat::appendPublicTrackers(atp.trackers, atp.tracker_tiers);

        // Real hash from the URI (known even before metadata, unlike a magnet's
        // torrent_status which reports all-zeros until then).
        const QString realHash = QString::fromStdString(
            (std::ostringstream() << atp.info_hashes.get_best()).str());

        if (!coverHint.isEmpty()) {
            atp.name = coverHint.toStdString();        // instant display name
            m_coverHints[realHash] = CoverHint{coverHint, coverType};
        }

        if (!m_tempPath.isEmpty() && QDir(m_tempPath).exists()) {
            m_torrentIntendedPath[realHash] = savePath;
            atp.save_path = m_tempPath.toStdString();
        }
        applyStorageMode(atp);

        lt::torrent_handle h = m_session.add_torrent(atp);
        m_torrents.push_back(h);
        m_magnetAddedAt[h] = QDateTime::currentSecsSinceEpoch();
        m_magnetHashes[h] = realHash;
        persistMagnetParams(atp, realHash, savePath);
        incrementTorrentCount();

        emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
        maybeAutoExcludeDefender(QString::fromStdString(atp.save_path));   // scan happens on metadata_received
    } catch (const std::exception &e) {
        emit torrentError(QString::fromStdString(e.what()));
    }
}

SessionManager::CoverHint SessionManager::takeCoverHint(const QString &hash)
{
    return m_coverHints.take(hash);
}

QStringList SessionManager::torrentFileNames(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return {};
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return {};
    auto ti = h.torrent_file();
    if (!ti) return {};
    QStringList out;
    const auto &fs = ti->files();
    for (lt::file_index_t i(0); i < fs.end_file(); ++i)
        out << QString::fromStdString(fs.file_path(i));
    return out;
}

void SessionManager::checkMagnetTimeouts()
{
    // No longer times anything out — a magnet used to be silently deleted
    // after 5 minutes without metadata, which hit rare-seeder torrents hardest
    // (exactly the ones that legitimately take longer to find a peer). The
    // wait is now explained via stateDetail (torrentAt) instead; this pass
    // just prunes the bookkeeping map so it can't grow unbounded.
    if (m_magnetAddedAt.empty())
        return;
    for (auto it = m_magnetAddedAt.begin(); it != m_magnetAddedAt.end(); ) {
        const lt::torrent_handle &h = it->first;
        if (!h.is_valid() || cachedStatus(h).has_metadata)
            it = m_magnetAddedAt.erase(it);
        else
            ++it;
    }
}

void SessionManager::removeTorrent(int index, bool deleteFiles, bool permanent)
{
    qDebug() << "[session] removeTorrent index:" << index << "deleteFiles:" << deleteFiles;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;

    lt::torrent_handle h = m_torrents[index];
    if (!h.is_valid()) {
        m_torrents.erase(m_torrents.begin() + index);
        emit torrentRemoved(index);
        return;
    }

    try {
        lt::torrent_status st = h.status();
        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        // Pre-metadata magnet: status reports an all-zeros hash, but its
        // .resume file and per-torrent maps are keyed by the URI hash.
        if (!st.has_metadata) {
            auto mit = m_magnetHashes.find(h);
            if (mit != m_magnetHashes.end()) hash = mit->second;
        }
        QDir dir(resumeDataDir());
        QDir removedDir(QFileInfo(dir, "../removed").absoluteFilePath());
        if (!removedDir.exists()) removedDir.mkpath(".");
        QFile::remove(removedDir.filePath(hash + ".resume"));
        QFile::rename(dir.filePath(hash + ".resume"),
                      removedDir.filePath(hash + ".resume"));
        QSettings meta(removedDir.filePath("history.ini"), QSettings::IniFormat);
        meta.beginGroup(hash);
        meta.setValue("name", QString::fromStdString(st.name));
        meta.setValue("size", static_cast<qint64>(st.total_wanted));
        meta.setValue("removedAt", QDateTime::currentSecsSinceEpoch());
        meta.endGroup();
        meta.beginGroup("");
        QStringList groups = meta.childGroups();
        if (groups.size() > 50) {
            QList<QPair<qint64, QString>> sorted;
            for (const QString &g : groups) {
                meta.beginGroup(g);
                sorted.append({meta.value("removedAt").toLongLong(), g});
                meta.endGroup();
            }
            std::sort(sorted.begin(), sorted.end());
            for (int i = 0; i < sorted.size() - 50; ++i) {
                meta.remove(sorted[i].second);
                QFile::remove(removedDir.filePath(sorted[i].second + ".resume"));
            }
        }
        meta.endGroup();
        m_perTorrentStopAfter.remove(hash);
        m_perTorrentMaxSeed.remove(hash);
        if (m_perTorrentDownLimit.remove(hash) || m_perTorrentUpLimit.remove(hash)) {
            QSettings s("BATorrent", "BATorrent");
            s.remove("torrentDownLimit/" + hash);
            s.remove("torrentUpLimit/" + hash);
        }
        if (m_completedTorrents.remove(hash))
            saveCompletedSet();
        if (m_forceStartHashes.remove(hash)) {
            QSettings("BATorrent", "BATorrent").setValue(
                "forceStartHashes", QStringList(m_forceStartHashes.values()));
        }
        if (m_torrentTags.remove(hash))
            QSettings("BATorrent", "BATorrent").remove("torrentTags/" + hash);
        m_removedHashes.insert(hash);
        if (m_removedHashes.size() > 500) m_removedHashes.clear();

        m_globalDownBase += st.total_payload_download;
        m_globalUpBase += st.total_payload_upload;

        m_queuePaused.erase(h);
        m_killSwitchPaused.erase(h);
        m_statusCache.erase(h);
        m_lastResumeSaveAt.erase(h);
        m_lastFastAt.erase(h);
        m_pendingResumeStripCheck.erase(h);
        m_magnetAddedAt.erase(h);
        m_magnetHashes.erase(h);

        // "delete files" sends the data to the OS trash instead of erasing it —
        // recoverable removal is a safety net users expect from a desktop app.
        // The move runs shortly after remove_torrent so libtorrent has released
        // its file handles (Windows can't rename open files). Anything
        // moveToTrash can't handle is left on disk rather than force-deleted.
        QStringList trashTargets;
        if (deleteFiles) {
            const QString savePath = QString::fromStdString(st.save_path);
            QSet<QString> tops;
            if (auto ti = h.torrent_file()) {
                const auto &fs = ti->files();
                for (const auto i : fs.file_range()) {
                    QString p = QString::fromStdString(std::string(fs.file_path(i)));
                    // libtorrent reports backslash paths on Windows; normalize so
                    // multi-file torrents resolve to their top-level folder instead
                    // of being trashed file-by-file (folder left behind).
                    p.replace(QLatin1Char('\\'), QLatin1Char('/'));
                    const int slash = p.indexOf(QLatin1Char('/'));
                    tops.insert(slash > 0 ? p.left(slash) : p);
                }
            } else {
                tops.insert(QString::fromStdString(st.name));
            }
            for (const QString &t : tops) {
                if (t.isEmpty()) continue;
                // the in-memory file_path may already carry the incomplete-file
                // ".!bt" suffix (rename_file persists on the handle) — normalize
                // to the base name and target both on-disk variants
                QString base = t;
                if (base.endsWith(QLatin1String(".!bt"))) base.chop(4);
                trashTargets << QDir(savePath).filePath(base)
                             << QDir(savePath).filePath(base + QStringLiteral(".!bt"));
            }
            // also the hidden partial-pieces sidecar (.{hash}.parts) — otherwise it
            // lingers orphaned in the save folder after a remove-with-files
            trashTargets << QDir(savePath).filePath(QStringLiteral(".") + hash + QStringLiteral(".parts"));
        }

        // An actively-downloading torrent still has its files open when we ask
        // libtorrent to drop it; on Windows moveToTrash then hits a sharing
        // violation and the data is silently left on disk. Pausing first starts
        // the handle release immediately, and we retry on a backoff (~27s total)
        // until the handles are gone instead of giving up after one attempt.
        if (deleteFiles) h.pause();
        m_session.remove_torrent(h, {});
        if (!trashTargets.isEmpty()) {
            if (permanent) scheduleDelete(trashTargets, 0);
            else scheduleTrash(trashTargets, 0);
        }
    } catch (const std::exception &e) {
        qWarning() << "[session] removeTorrent exception:" << e.what();
    }
    m_torrents.erase(m_torrents.begin() + index);
    emit torrentRemoved(index);
}

void SessionManager::pauseTorrent(int index)
{
    qDebug() << "[session] pauseTorrent index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    // is_valid first — pause() on an expired handle throws (std::terminate
    // from the Qt event loop = the whole app closes)
    if (!m_torrents[index].is_valid())
        return;
    m_torrents[index].pause();
    // persist the pause now — periodic/shutdown saves can run before this and
    // otherwise the torrent reloads un-paused (resumes downloading on its own)
    m_torrents[index].save_resume_data(lt::torrent_handle::save_info_dict);
}

void SessionManager::resumeTorrent(int index)
{
    qDebug() << "[session] resumeTorrent index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[index].is_valid())
        return;
    // Resume on a completed torrent un-marks it — the user is explicitly
    // asking it to participate again, so the "frozen" flag has to clear.
    unmarkCompleted(index);
    m_torrents[index].resume();
    m_torrents[index].save_resume_data(lt::torrent_handle::save_info_dict);
}

void SessionManager::pauseAll()
{
    qDebug() << "[session] pauseAll";
    for (auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        h.pause();
        h.save_resume_data(lt::torrent_handle::save_info_dict);
    }
}

void SessionManager::resumeAll()
{
    qDebug() << "[session] resumeAll";
    for (auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        h.resume();
        h.save_resume_data(lt::torrent_handle::save_info_dict);
    }
}

void SessionManager::forceRecheck(int index)
{
    qDebug() << "[session] forceRecheck index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[index].is_valid()) return;
    // Clear the cached status BEFORE calling force_recheck. Without this,
    // the update timer reads stale state (e.g. "downloading") while
    // libtorrent is internally in checking_files. That mismatch caused
    // crashes on large torrents (96GB+) because queue management and
    // auto-pause logic acted on inconsistent state.
    m_statusCache.erase(m_torrents[index]);
    m_torrents[index].force_recheck();
}

void SessionManager::forceReannounce(int index)
{
    qDebug() << "[session] forceReannounce index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (!m_torrents[index].is_valid()) return;
    m_torrents[index].force_reannounce();
    m_torrents[index].force_dht_announce();
}

QString SessionManager::torrentMagnetUri(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return {};
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return {};
    try {
        return QString::fromStdString(lt::make_magnet_uri(h));
    } catch (...) {
        return {};
    }
}

void SessionManager::applyStorageMode(lt::add_torrent_params &atp)
{
    atp.storage_mode = m_preallocate ? lt::storage_mode_allocate
                                     : lt::storage_mode_sparse;
}

