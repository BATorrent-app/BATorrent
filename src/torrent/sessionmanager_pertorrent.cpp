// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — per-torrent slice. Per-torrent overrides (stop-after-download,
// max-seed, rate caps), force-start / super-seeding, the user-marked "completed"
// state + auto-complete, and the effective-rule resolvers that fold per-torrent
// overrides over the globals. Split out of sessionmanager.cpp verbatim.

#include "torrent/sessionmanager.h"

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <QSettings>
#include <QStringList>
#include <QDebug>
#include <sstream>
#include <chrono>
#include <vector>

void SessionManager::setTorrentStopAfterDownload(int index, int value)
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return;
    if (value < 0)
        m_perTorrentStopAfter.remove(hash);
    else
        m_perTorrentStopAfter[hash] = value ? 1 : 0;
}

int SessionManager::torrentStopAfterDownload(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return -1;
    return m_perTorrentStopAfter.value(hash, -1);
}

void SessionManager::setTorrentMaxSeedSeconds(int index, qint64 seconds)
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return;
    if (seconds < 0)
        m_perTorrentMaxSeed.remove(hash);
    else
        m_perTorrentMaxSeed[hash] = seconds;
}

qint64 SessionManager::torrentMaxSeedSeconds(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return -1;
    return m_perTorrentMaxSeed.value(hash, -1);
}

void SessionManager::stopSeedingTorrent(int index)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    m_torrents[index].pause();
}

void SessionManager::setSuperSeeding(int index, bool on)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return;
    if (!m_torrents[index].is_valid()) return;
    qDebug() << "[session] superSeeding index:" << index << "on:" << on;
    if (on)
        m_torrents[index].set_flags(lt::torrent_flags::super_seeding);
    else
        m_torrents[index].unset_flags(lt::torrent_flags::super_seeding);
}

bool SessionManager::isSuperSeeding(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return false;
    if (!m_torrents[index].is_valid()) return false;
    return (m_torrents[index].flags() & lt::torrent_flags::super_seeding) != lt::torrent_flags_t{};
}

void SessionManager::setForceStart(int index, bool on)
{
    qDebug() << "[session] forceStart index:" << index << "on:" << on;
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return;
    if (on) {
        m_forceStartHashes.insert(hash);
        // Resume so the user sees immediate effect — force-start that
        // remains paused would be confusing.
        if (m_torrents[index].is_valid())
            m_torrents[index].resume();
        // Drop from queue-paused set so enforceDownloadQueue doesn't keep
        // pausing it.
        m_queuePaused.erase(m_torrents[index]);
    } else {
        m_forceStartHashes.remove(hash);
    }
    QSettings("BATorrent", "BATorrent").setValue(
        "forceStartHashes", QStringList(m_forceStartHashes.values()));
    emit torrentsUpdated();
}

bool SessionManager::isForceStart(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return false;
    return m_forceStartHashes.contains(hash);
}

void SessionManager::setTorrentDownloadLimit(int index, int kbps)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return;
    if (!m_torrents[index].is_valid()) return;
    if (kbps < 0) kbps = 0;
    QString hash = torrentHash(index);
    QSettings settings("BATorrent", "BATorrent");
    if (kbps == 0) {
        m_perTorrentDownLimit.remove(hash);
        settings.remove("torrentDownLimit/" + hash);
    } else {
        m_perTorrentDownLimit[hash] = kbps;
        settings.setValue("torrentDownLimit/" + hash, kbps);
    }
    // libtorrent takes bytes/sec; 0 = unlimited at the handle level.
    m_torrents[index].set_download_limit(kbps * 1024);
}

void SessionManager::setTorrentUploadLimit(int index, int kbps)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size())) return;
    if (!m_torrents[index].is_valid()) return;
    if (kbps < 0) kbps = 0;
    QString hash = torrentHash(index);
    QSettings settings("BATorrent", "BATorrent");
    if (kbps == 0) {
        m_perTorrentUpLimit.remove(hash);
        settings.remove("torrentUpLimit/" + hash);
    } else {
        m_perTorrentUpLimit[hash] = kbps;
        settings.setValue("torrentUpLimit/" + hash, kbps);
    }
    m_torrents[index].set_upload_limit(kbps * 1024);
}

int SessionManager::torrentDownloadLimit(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return 0;
    return m_perTorrentDownLimit.value(hash, 0);
}

int SessionManager::torrentUploadLimit(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return 0;
    return m_perTorrentUpLimit.value(hash, 0);
}

void SessionManager::markCompleted(int index)
{
    qDebug() << "[session] markCompleted index:" << index;
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    auto &h = m_torrents[index];
    if (!h.is_valid()) return;
    lt::torrent_status st = cachedStatus(h);
    if (!st.has_metadata) return;
    QString hash = QString::fromStdString(
        (std::ostringstream() << st.info_hashes.get_best()).str());
    if (m_completedTorrents.contains(hash)) return;
    m_completedTorrents.insert(hash);
    saveCompletedSet();
    // Freeze it completely: priority 0 on every file so libtorrent never re-downloads
    // anything — even if the user deletes the extracted archives and a recheck later
    // finds them missing. Pause alone doesn't stop a fastresume-rejected recheck.
    if (auto ti = h.torrent_file())
        h.prioritize_files(std::vector<lt::download_priority_t>(
            static_cast<std::size_t>(ti->num_files()), lt::dont_download));
    h.pause();
    emit torrentsUpdated();
}

void SessionManager::unmarkCompleted(int index)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    auto &h = m_torrents[index];
    if (!h.is_valid()) return;
    lt::torrent_status st = cachedStatus(h);
    if (!st.has_metadata) return;
    QString hash = QString::fromStdString(
        (std::ostringstream() << st.info_hashes.get_best()).str());
    if (m_completedTorrents.remove(hash)) {
        // Un-freeze: restore default priority on every file and resume, so it seeds
        // again (and re-fetches anything the user deleted, since they want it back).
        if (auto ti = h.torrent_file())
            h.prioritize_files(std::vector<lt::download_priority_t>(
                static_cast<std::size_t>(ti->num_files()), lt::default_priority));
        h.resume();
        saveCompletedSet();
        emit torrentsUpdated();
    }
}

bool SessionManager::isTorrentCompleted(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return false;
    const auto &h = m_torrents[index];
    if (!h.is_valid()) return false;
    lt::torrent_status st = cachedStatus(h);
    if (!st.has_metadata) return false;
    QString hash = QString::fromStdString(
        (std::ostringstream() << st.info_hashes.get_best()).str());
    return m_completedTorrents.contains(hash);
}

void SessionManager::saveCompletedSet()
{
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("completedTorrents", QStringList(m_completedTorrents.values()));
}

void SessionManager::setAutoCompleteSeconds(qint64 seconds)
{
    if (seconds < 0) seconds = 0;
    m_autoCompleteSeconds = seconds;
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("autoCompleteSeconds", m_autoCompleteSeconds);
}

qint64 SessionManager::autoCompleteSeconds() const
{
    return m_autoCompleteSeconds;
}

void SessionManager::checkAutoComplete()
{
    if (m_autoCompleteSeconds <= 0) return;
    for (int i = 0; i < static_cast<int>(m_torrents.size()); ++i) {
        auto &h = m_torrents[i];
        if (!h.is_valid()) continue;
        lt::torrent_status st = cachedStatus(h);
        if (st.state != lt::torrent_status::seeding) continue;
        if (st.flags & lt::torrent_flags::paused) continue;
        if (!st.has_metadata) continue;
        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        if (m_completedTorrents.contains(hash)) continue;
        qint64 seeded = std::chrono::duration_cast<std::chrono::seconds>(
                            st.seeding_duration).count();
        if (seeded >= m_autoCompleteSeconds)
            markCompleted(i);
    }
}
bool SessionManager::effectiveStopAfterDownload(const QString &hash) const
{
    int override_ = m_perTorrentStopAfter.value(hash, -1);
    if (override_ >= 0)
        return override_ == 1;
    return m_stopAfterDownload;
}

qint64 SessionManager::effectiveMaxSeedSeconds(const QString &hash) const
{
    qint64 override_ = m_perTorrentMaxSeed.value(hash, -1);
    if (override_ >= 0)
        return override_;
    return m_maxSeedSeconds;
}
