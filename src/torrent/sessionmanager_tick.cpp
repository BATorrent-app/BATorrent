// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager: periodic tick slice (updateStats, seed limits, disk probe,
// download queue). Split out of sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"
#include "torrent/contentprobe.h"
#include "services/platform/translator.h"

#include <libtorrent/torrent_info.hpp>

#include <QDebug>
#include <QDateTime>
#include <QSettings>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QThread>
#include <sstream>

void SessionManager::refreshDiskFreeAsync(const QString &savePath)
{
    if (savePath.isEmpty()) return;
    static qint64 lastKick = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    // Probe at most every 5s, or immediately when the path changes.
    if (savePath == m_cachedDiskPath && now - lastKick < 5) return;
    lastKick = now;
    const QString path = savePath;
    auto *thread = QThread::create([this, path]() {
        QStorageInfo storage(path);
        const qint64 freeB = storage.isValid() ? storage.bytesAvailable() : -1;
        QMetaObject::invokeMethod(this, [this, path, freeB]() {
            m_cachedDiskPath = path;
            m_cachedDiskFree = freeB;
        }, Qt::QueuedConnection);
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void SessionManager::checkMissingFiles()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (m_missingProbeRunning || now - m_lastMissingProbe < m_missingProbeMs) return;
    m_lastMissingProbe = now;

    // Handles and torrent_info must be read here: only this thread may touch
    // them. The probe thread gets plain strings.
    struct Probe { QString hash; QStringList candidates; };
    std::vector<Probe> probes;
    probes.reserve(m_torrents.size());

    for (const auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        const lt::torrent_status st = cachedStatus(h);
        if (!st.has_metadata) continue;
        // Nothing written yet is nothing to miss. total_wanted_done is the only
        // honest signal: progress is a float, and is_finished lies while
        // total_wanted is 0 (see torrentHasWork).
        if (st.total_wanted_done <= 0) continue;

        QString rel;
        bool multiFile = false;
        if (auto ti = h.torrent_file(); ti && ti->num_files() > 0) {
            multiFile = ti->num_files() > 1;
            rel = QString::fromStdString(ti->files().file_path(lt::file_index_t(0)));
        }

        const QStringList candidates = bat::contentRootCandidates(
            QString::fromStdString(st.save_path), rel,
            QString::fromStdString(st.name), multiFile);
        if (candidates.isEmpty()) continue;

        probes.push_back({QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str()), candidates});
    }

    if (probes.empty()) {
        m_missingHashes.clear();
        m_missingSeen.clear();
        return;
    }

    m_missingProbeRunning = true;
    auto *thread = QThread::create([this, probes]() {
        QSet<QString> missing;
        for (const auto &p : probes)
            if (!bat::contentPresent(p.candidates, bat::defaultExists))
                missing.insert(p.hash);
        QMetaObject::invokeMethod(this, [this, missing]() {
            // Two probes in a row before we believe it. A network share that
            // drops for a moment, or an external disk still spinning up, would
            // otherwise flip a healthy torrent to "missing" and pause it.
            const QSet<QString> confirmed = missing & m_missingSeen;
            m_missingSeen = missing;

            // Whole-set replacement, not a merge: a torrent that was probed and
            // found is absent from `confirmed` and must stop being flagged.
            if (confirmed != m_missingHashes) {
                for (const QString &h : confirmed)
                    if (!m_missingHashes.contains(h))
                        qWarning().noquote() << "[session] data gone from disk:" << h;
                m_missingHashes = confirmed;
            }
            // Forget torrents that came back, so a later disappearance pauses again.
            m_missingPaused &= confirmed;
            pauseMissing(confirmed);
            m_missingProbeRunning = false;
        }, Qt::QueuedConnection);
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
}

void SessionManager::pauseMissing(const QSet<QString> &confirmed)
{
    if (!m_pauseOnMissingData || confirmed.isEmpty()) return;

    int paused = 0;
    QString lastName;
    for (int i = 0; i < static_cast<int>(m_torrents.size()); ++i) {
        const auto &h = m_torrents[i];
        if (!h.is_valid()) continue;
        const lt::torrent_status st = cachedStatus(h);
        if (!st.has_metadata) continue;
        if (st.flags & lt::torrent_flags::paused) continue;

        const QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        if (!confirmed.contains(hash)) continue;
        // Already acted on this one; the user is free to resume it and we do
        // not fight them on the next probe.
        if (m_missingPaused.contains(hash)) continue;

        h.pause();
        m_missingPaused.insert(hash);
        lastName = QString::fromStdString(st.name);
        ++paused;
    }

    if (paused > 0) {
        qWarning() << "[session] paused" << paused << "torrent(s) with no data on disk";
        emit torrentError(paused == 1 ? tr_("warn_missing_paused_one").arg(lastName)
                                      : tr_("warn_missing_paused").arg(paused));
    }
}

void SessionManager::updateStats()
{
    // Ask libtorrent to deliver a state_update_alert with fresh statuses for
    // every torrent. The alert lands inside processAlerts() below; until it
    // arrives, m_statusCache may be one tick stale: acceptable trade-off
    // for getting rid of dozens of synchronous status() calls per second.
    m_session.post_torrent_updates();

    processAlerts();
    // daily usage history sample (cheap: cachedStatus sums), every 5s
    {
        static int statsTick = 0;
        if (++statsTick >= 5 && m_statsHistory) {
            statsTick = 0;
            m_statsHistory->recordTransfer(globalDownloaded(), globalUploaded());
        }
    }
    checkSeedRatios();
    checkSeedingLimits();
    checkAutoComplete();
    checkAndBlockLeechers();
    // Disk pressure: warn under 1 GB, and *act* under 512 MB by pausing active
    // downloads (with hysteresis) so we don't fill the disk or corrupt data.
    {
        static qint64 lastDiskWarn = 0;
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        QSettings s("BATorrent", "BATorrent");
        const QString savePath = s.value("lastSavePath",
            QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).toString();
        refreshDiskFreeAsync(savePath);
        const qint64 freeB = m_cachedDiskFree;

        if (freeB >= 0 && freeB < 512LL * 1024 * 1024 && !m_diskAutoPaused) {
            int paused = 0;
            for (auto &h : m_torrents) {
                if (!h.is_valid()) continue;
                const lt::torrent_status st = cachedStatus(h);
                const bool active = st.state == lt::torrent_status::downloading
                                 || st.state == lt::torrent_status::downloading_metadata;
                if (active && !(st.flags & lt::torrent_flags::paused)) { h.pause(); ++paused; }
            }
            m_diskAutoPaused = true;   // runtime-only pause (not persisted): recovers on disk free
            qWarning() << "[session] CRITICAL DISK:" << freeB / (1024*1024) << "MB — paused" << paused << "downloads";
            if (paused > 0) emit torrentError(tr_("warn_disk_autopause").arg(paused).arg(freeB / (1024*1024)));
            lastDiskWarn = now;
        } else if (freeB >= 2LL * 1024 * 1024 * 1024) {
            m_diskAutoPaused = false;   // recovered → re-arm (user decides whether to resume)
        } else if (freeB >= 0 && freeB < 1024LL * 1024 * 1024 && now - lastDiskWarn >= 300) {
            qWarning() << "[session] LOW DISK SPACE:" << freeB / (1024*1024) << "MB remaining on" << savePath;
            emit torrentError(tr_("warn_low_disk").arg(freeB / (1024*1024)));
            lastDiskWarn = now;
        }
    }
    checkInterfaceStatus();
    checkBandwidthSchedule();
    checkMagnetTimeouts();
    checkMissingFiles();
    checkMemoryGuard();
    enforceDownloadQueue();
    if (!m_torrents.empty())
        emit torrentsUpdated();
}

void SessionManager::checkSeedRatios()
{
    if (m_seedRatioLimit <= 0.0f) return;

    for (int i = 0; i < static_cast<int>(m_torrents.size()); ++i) {
        auto &h = m_torrents[i];
        lt::torrent_status st = cachedStatus(h);
        if (st.state != lt::torrent_status::seeding) continue;
        if (st.flags & lt::torrent_flags::paused) continue;

        // Use payload counters so the pause-at-ratio threshold lines up with
        // the ratio shown to the user and what trackers report.
        float ratio = st.total_payload_download > 0
            ? static_cast<float>(st.total_payload_upload)
              / static_cast<float>(st.total_payload_download)
            : 0.0f;

        // reaching the limit is the natural end of the torrent's life:
        // mark it completed (freeze + persist), not merely paused
        if (ratio >= m_seedRatioLimit)
            markCompleted(i);
    }
}

void SessionManager::checkSeedingLimits()
{
    for (int i = 0; i < static_cast<int>(m_torrents.size()); ++i) {
        auto &h = m_torrents[i];
        if (!h.is_valid()) continue;
        lt::torrent_status st = cachedStatus(h);
        if (st.state != lt::torrent_status::seeding) continue;
        if (st.flags & lt::torrent_flags::paused) continue;

        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        qint64 maxSec = effectiveMaxSeedSeconds(hash);
        if (maxSec <= 0) continue;

        // st.seeding_duration is a std::chrono::seconds duration tracking
        // total time the torrent has been in the seeding state.
        qint64 seeded = std::chrono::duration_cast<std::chrono::seconds>(
                            st.seeding_duration).count();
        if (seeded >= maxSec)
            markCompleted(i);
    }
}

void SessionManager::setMaxActiveDownloads(int max)
{
    m_maxActiveDownloads = max;
    QSettings("BATorrent", "BATorrent").setValue("maxActiveDownloads", max);
}

int SessionManager::maxActiveDownloads() const
{
    return m_maxActiveDownloads;
}

void SessionManager::setTorrentQueuePosition(int index, int position)
{
    if (index < 0 || index >= static_cast<int>(m_torrents.size()))
        return;
    if (position < 0 || position >= static_cast<int>(m_torrents.size()))
        return;
    if (index == position)
        return;

    lt::torrent_handle h = m_torrents[index];
    m_torrents.erase(m_torrents.begin() + index);
    m_torrents.insert(m_torrents.begin() + position, h);
}

void SessionManager::enforceDownloadQueue()
{
    if (m_maxActiveDownloads <= 0)
        return;

    // Constants for slow-torrent detection. A torrent counts as "fast" if
    // it's actually transferring above this rate; stalled torrents fall
    // off the active list after the timeout so a stuck download can't
    // permanently consume a queue slot.
    constexpr int kSlowTorrentThresholdBps = 10 * 1024; // 10 KB/s
    constexpr qint64 kSlowTorrentTimeoutSec = 60;
    const qint64 now = QDateTime::currentSecsSinceEpoch();

    // Count active (non-paused) downloading torrents
    std::vector<int> activeIndices;
    std::vector<int> queuedIndices;

    for (int i = 0; i < static_cast<int>(m_torrents.size()); ++i) {
        if (!m_torrents[i].is_valid()) continue;
        lt::torrent_status st = cachedStatus(m_torrents[i]);

        bool isPaused = (st.flags & lt::torrent_flags::paused) != lt::torrent_flags_t{};
        bool isDownloading = (st.state == lt::torrent_status::downloading
                              || st.state == lt::torrent_status::downloading_metadata);

        // Force-start torrents are exempt from the queue entirely: they
        // neither count against the cap nor get auto-paused. Resume them
        // here if they're paused for any reason.
        if (st.has_metadata) {
            QString hash = QString::fromStdString(
                (std::ostringstream() << st.info_hashes.get_best()).str());
            if (m_forceStartHashes.contains(hash)) {
                if (isPaused) m_torrents[i].resume();
                continue;
            }
        }

        if (isDownloading && !isPaused) {
            // Stamp the "last fast" timestamp on tick ticks where the
            // torrent is moving; fresh adds also get an initial stamp so
            // they're given a grace period before being demoted.
            auto it = m_lastFastAt.find(m_torrents[i]);
            if (it == m_lastFastAt.end()) {
                m_lastFastAt[m_torrents[i]] = now;
            } else if (st.download_rate >= kSlowTorrentThresholdBps) {
                it->second = now;
            }

            const qint64 lastFast = m_lastFastAt[m_torrents[i]];
            const bool isStalled = (now - lastFast) > kSlowTorrentTimeoutSec;
            // Stalled torrents stay running (we don't pause them: the
            // user can do that), they just don't count against the active
            // limit. New downloads can therefore start in their place.
            if (!isStalled)
                activeIndices.push_back(i);
        } else if (isDownloading && isPaused && m_queuePaused.count(m_torrents[i])) {
            // This torrent was paused by queue logic -- it's waiting
            queuedIndices.push_back(i);
        }
    }

    int activeCount = static_cast<int>(activeIndices.size());

    // If over limit, pause lowest priority (highest index) torrents
    if (activeCount > m_maxActiveDownloads) {
        for (int i = activeCount - 1; i >= m_maxActiveDownloads; --i) {
            int idx = activeIndices[i];
            m_torrents[idx].pause();
            m_queuePaused.insert(m_torrents[idx]);
        }
    }
    // If under limit, resume queued torrents
    else if (activeCount < m_maxActiveDownloads && !queuedIndices.empty()) {
        int toResume = m_maxActiveDownloads - activeCount;
        for (int i = 0; i < toResume && i < static_cast<int>(queuedIndices.size()); ++i) {
            int idx = queuedIndices[i];
            m_torrents[idx].resume();
            m_queuePaused.erase(m_torrents[idx]);
        }
    }
}

