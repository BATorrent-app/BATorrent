// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — alert-handling slice. processAlerts() pumps the libtorrent
// alert queue and dispatches one handler per alert type. This is the engine's
// event core (and historically its crash epicenter — every handler guards its
// libtorrent handle before touching it). Split out of sessionmanager.cpp
// verbatim; no behaviour change. cachedStatus() stays in core (general helper).

#include "torrent/sessionmanager.h"
#include "services/platform/logger.h"
#include "services/platform/translator.h"

#include <libtorrent/torrent_info.hpp>
#include <QString>
#include <QDebug>
#include <sstream>
#ifdef BAT_LIBTORRENT_FORK
#include <libtorrent/aux_/ip_helpers.hpp>   // fork-only geo-locality hook
#endif

void SessionManager::processAlerts()
{
    std::vector<lt::alert *> alerts;
    m_session.pop_alerts(&alerts);

    for (auto *a : alerts) {
      try {
        if (auto *su = lt::alert_cast<lt::state_update_alert>(a)) { onStateUpdate(su); continue; }
        if (auto *fa = lt::alert_cast<lt::torrent_finished_alert>(a)) onTorrentFinished(fa);
        if (auto *ea = lt::alert_cast<lt::torrent_error_alert>(a)) onTorrentError(ea);
        if (auto *fe = lt::alert_cast<lt::file_error_alert>(a)) onFileError(fe);
        if (auto *sm = lt::alert_cast<lt::storage_moved_failed_alert>(a)) onStorageMovedFailed(sm);
        if (auto *lf = lt::alert_cast<lt::listen_failed_alert>(a)) onListenFailed(lf);
        if (lt::alert_cast<lt::listen_succeeded_alert>(a)) onListenSucceeded();
        if (lt::alert_cast<lt::portmap_alert>(a)) onPortmapSucceeded();
        if (lt::alert_cast<lt::portmap_error_alert>(a)) onPortmapFailed();
        if (auto *mf = lt::alert_cast<lt::metadata_failed_alert>(a)) onMetadataFailed(mf);
        if (auto *rd = lt::alert_cast<lt::save_resume_data_alert>(a)) onResumeDataReady(rd);
        if (lt::alert_cast<lt::save_resume_data_failed_alert>(a)) onResumeDataFailed();
        if (auto *pf = lt::alert_cast<lt::piece_finished_alert>(a)) onPieceFinished(pf);
        if (auto *ad = lt::alert_cast<lt::alerts_dropped_alert>(a)) onAlertsDropped(ad);
        if (auto *fr = lt::alert_cast<lt::fastresume_rejected_alert>(a)) onFastresumeRejected(fr);
        if (auto *tc = lt::alert_cast<lt::torrent_checked_alert>(a)) onTorrentChecked(tc);
        if (auto *mr = lt::alert_cast<lt::metadata_received_alert>(a)) onMetadataReceived(mr);
        if (auto *fc = lt::alert_cast<lt::file_completed_alert>(a)) onFileCompleted(fc);
#ifdef BAT_LIBTORRENT_FORK
        if (auto *xi = lt::alert_cast<lt::external_ip_alert>(a)) onExternalIp(xi);
#endif
      } catch (const std::exception &e) {
        qWarning() << "[session] alert processing exception:" << e.what();
      } catch (...) {
        qWarning() << "[session] alert processing: unknown exception caught";
      }
    }
}

void SessionManager::onStateUpdate(const lt::state_update_alert *su)
{
    // Status snapshots arrive in one batch per post_torrent_updates(),
    // refreshing every cache entry at once. This is what makes the UI
    // 1/Nth as expensive as polling each handle individually.
    for (const auto &st : su->status) {
        m_statusCache[st.handle] = st;
        // Deferred .!bt strip for files already 100% on resume.
        // Storage is bound by the time we get here, so file_progress
        // is safe — gate on metadata + has_storage just in case.
        auto it = m_pendingResumeStripCheck.find(st.handle);
        if (it != m_pendingResumeStripCheck.end() && st.has_metadata) {
            try {
                if (auto ti = st.handle.torrent_file()) {
                    std::vector<std::int64_t> progress;
                    st.handle.file_progress(progress,
                        lt::torrent_handle::piece_granularity);
                    const auto &fs = ti->files();
                    const int n = static_cast<int>(progress.size());
                    for (int i = 0; i < n; ++i) {
                        lt::file_index_t idx(i);
                        if (progress[i] != fs.file_size(idx)) continue;
                        std::string cur = fs.file_path(idx);
                        if (cur.size() >= 4
                            && cur.compare(cur.size() - 4, 4, ".!bt") == 0) {
                            st.handle.rename_file(idx,
                                std::string(cur, 0, cur.size() - 4));
                        }
                    }
                }
            } catch (const std::exception &) {
                // Storage still not ready — retry on next tick.
                continue;
            }
            m_pendingResumeStripCheck.erase(it);
        }
    }
}

void SessionManager::onTorrentFinished(const lt::torrent_finished_alert *fa)
{
    if (!fa->handle.is_valid()) return;
    QString name = QString::fromStdString(fa->torrent_name());
    lt::torrent_status st = fa->handle.status();

    // Safety net: strip any remaining ".!bt" suffixes. Normally
    // file_completed_alert handles this per-file as the download
    // progresses, but this catches edge cases — torrents that
    // resume already-complete from a previous session, alerts
    // dropped under load, etc.
    if (auto ti = fa->handle.torrent_file()) {
        const auto &files = ti->files();
        for (lt::file_index_t i(0); i < files.end_file(); ++i) {
            std::string current = files.file_path(i);
            if (current.size() >= 4
                && current.compare(current.size() - 4, 4, ".!bt") == 0) {
                std::string stripped(current, 0, current.size() - 4);
                fa->handle.rename_file(i, stripped);
            }
        }
    }

    // Skip torrents that were already complete when the session
    // started — libtorrent fires one finish alert per torrent during
    // the resume check, even if no bytes were actually downloaded.
    bool downloadedThisSession = (st.total_payload_download > 0);

    if (downloadedThisSession) {
        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());

        // Temp path → move to intended final path first
        if (m_torrentIntendedPath.contains(hash)) {
            QString dest = m_torrentIntendedPath.take(hash);
            fa->handle.move_storage(dest.toStdString());
        } else if (m_autoMoveEnabled && !m_autoMovePath.isEmpty()) {
            fa->handle.move_storage(m_autoMovePath.toStdString());
        }
        if (effectiveStopAfterDownload(hash))
            fa->handle.pause();

        if (m_autoExtract)
            extractArchives(QString::fromStdString(st.save_path), name, QString(), hash);

        // Complete torrents now load in seed_mode (see loadResumeData) so
        // they no longer re-check/re-download and re-fire this alert on
        // launch. The remaining guard covers a torrent already persisted
        // complete that still somehow re-finishes — its storage side
        // effects run, but the user-facing completion (script +
        // notification + media-server webhook) is muted.
        const bool resumeRefinish = m_completedAtStartup.contains(hash);
        if (!resumeRefinish) {
            qDebug() << "[session] torrent finished:" << name << "hash:" << hash.left(16);
            if (m_statsHistory) m_statsHistory->recordCompleted(m_categories.value(hash));
            executeOnComplete(name, QString::fromStdString(st.save_path),
                              hash, st.total_wanted);
            emit torrentFinished(name, hash);
            scanTorrentForThreats(fa->handle, name);
        }
    }

    // Remove from queue-paused set in either case (it's no longer
    // contributing to the active-download count).
    m_queuePaused.erase(fa->handle);
}

void SessionManager::onTorrentError(const lt::torrent_error_alert *ea)
{
    // Per-torrent rate-limit (like qBittorrent's 1s cooldown per
    // torrent). The previous global 30s window silently swallowed
    // errors from torrent B if torrent A errored first.
    static QHash<lt::torrent_handle, qint64> lastErrorAt;
    const qint64 nowTe = QDateTime::currentSecsSinceEpoch();
    auto &last = lastErrorAt[ea->handle];
    if (nowTe - last >= 3) {
        emit torrentError(QString::fromStdString(ea->message()));
        last = nowTe;
    }
    if (lastErrorAt.size() > 500) lastErrorAt.clear();
    noteTorrentFault(ea->handle, {});   // isolate it if this keeps happening
}

void SessionManager::onFileError(const lt::file_error_alert *fe)
{
    // Surface previously-swallowed alert categories so the user actually
    // hears about disk-full, move-storage failures, port collisions, and
    // broken magnets instead of staring at silent empty state.
    // Rate-limit file error emissions to avoid notification storms
    // when disk fills up — libtorrent fires one alert per failed
    // piece write, which at full speed can be hundreds per second.
    // One notification per 30 s is enough to inform without locking
    // the UI or crashing the notification stack.
    static qint64 lastFileErrorEmit = 0;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const bool shouldEmit = (now - lastFileErrorEmit) >= 30;

    // Detect disk-full specifically and auto-pause everything.
    // "No space" (Linux/macOS) / "not enough" (Windows) in the
    // libtorrent error message.
    const QString msg = QString::fromStdString(fe->message());
    const bool diskFull = msg.contains("No space", Qt::CaseInsensitive)
                       || msg.contains("not enough", Qt::CaseInsensitive)
                       || msg.contains("disk full", Qt::CaseInsensitive);
    if (diskFull) {
        // Pause ALL downloading torrents — continuing just wastes
        // CPU re-trying writes that will fail.
        for (auto &h : m_torrents) {
            if (!h.is_valid()) continue;
            auto st = cachedStatus(h);
            if (st.state == lt::torrent_status::downloading
                && !(st.flags & lt::torrent_flags::paused)) {
                h.pause();
            }
        }
        if (shouldEmit) {
            emit torrentError(tr_("error_disk_full"));
            lastFileErrorEmit = now;
        }
    } else {
        if (shouldEmit) {
            emit torrentError(msg);
            lastFileErrorEmit = now;
        }
        // Pause finished torrents that hit file errors (external
        // drive unplugged, files deleted, etc.)
        if (fe->handle.is_valid()) {
            lt::torrent_status st = fe->handle.status();
            const bool wasFinished = st.is_finished
                || st.state == lt::torrent_status::finished
                || st.state == lt::torrent_status::seeding;
            if (wasFinished
                && !(st.flags & lt::torrent_flags::paused)) {
                fe->handle.pause();
            }
        }
        // A still-downloading torrent that keeps hitting file errors
        // (bad sector, permissions) gets isolated after enough of them.
        noteTorrentFault(fe->handle, {});
    }
}

void SessionManager::onStorageMovedFailed(const lt::storage_moved_failed_alert *sm)
{
    emit torrentError(QString::fromStdString(sm->message()));
    // Partial moves leave libtorrent's piece map out of sync with
    // disk; a recheck is the only safe recovery.
    if (sm->handle.is_valid())
        sm->handle.force_recheck();
}

void SessionManager::onListenFailed(const lt::listen_failed_alert *lf)
{
    emit torrentError(QString::fromStdString(lf->message()));
    m_listenOk = false;
    updatePortStatus();
}

void SessionManager::onListenSucceeded()
{
    m_listenOk = true;
    updatePortStatus();
}

void SessionManager::onPortmapSucceeded()
{
    m_portmapOk = true;
    updatePortStatus();
}

void SessionManager::onPortmapFailed()
{
    m_portmapOk = false;
    updatePortStatus();
}

void SessionManager::onMetadataFailed(const lt::metadata_failed_alert *mf)
{
    emit torrentError(QString::fromStdString(mf->message()));
}

void SessionManager::onResumeDataReady(const lt::save_resume_data_alert *rd)
{
    // Async resume-data persistence: write the buffer the moment the
    // alert arrives, decrement the outstanding counter so
    // flushResumeDataBlocking() and other callers can tell when every
    // request has been serviced.
    persistResumeAlert(rd);
    if (m_resumeOutstanding > 0) --m_resumeOutstanding;
}

void SessionManager::onResumeDataFailed()
{
    if (m_resumeOutstanding > 0) --m_resumeOutstanding;
}

void SessionManager::onPieceFinished(const lt::piece_finished_alert *pf)
{
    // Piece just verified — opportunistically save resume data for this
    // torrent so a crash before the next 5-min tick doesn't force a
    // full re-hash on the next launch. Rate-limited per handle (60 s)
    // so a fast torrent doesn't hammer the disk; with the limit, the
    // worst case is ~1 resume write per minute per active download.
    if (!pf->handle.is_valid()) return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    auto it = m_lastResumeSaveAt.find(pf->handle);
    if (it == m_lastResumeSaveAt.end() || now - it->second >= 60) {
        pf->handle.save_resume_data(lt::torrent_handle::save_info_dict);
        m_lastResumeSaveAt[pf->handle] = now;
        ++m_resumeOutstanding;
    }
}

void SessionManager::onAlertsDropped(const lt::alerts_dropped_alert *ad)
{
    // Alert queue overflow — critical: alerts were silently dropped.
    // Means our alert_queue_size was too small or processing too slow.
    qWarning() << "[session] CRITICAL: alerts were dropped! Bitmask:"
               << QString::number(ad->dropped_alerts.to_ulong(), 16);
}

void SessionManager::onFastresumeRejected(const lt::fastresume_rejected_alert *fr)
{
    // Resume data was rejected (corrupt, version mismatch, missing
    // files). qBittorrent logs this and sets a missing-files flag;
    // without handling it we get undefined behavior at startup.
    qWarning() << "[session] fastresume rejected:"
               << QString::fromStdString(fr->message());
    if (fr->handle.is_valid())
        fr->handle.force_recheck();
}

void SessionManager::onTorrentChecked(const lt::torrent_checked_alert *tc)
{
    // Recheck completed — the torrent is back in a consistent state.
    // Re-populate the status cache so queue/auto-pause logic sees the
    // real state instead of the stale pre-recheck snapshot.
    if (tc->handle.is_valid()) {
        m_statusCache[tc->handle] = tc->handle.status();
        qDebug() << "[session] recheck completed for torrent";
    }
}

void SessionManager::onMetadataReceived(const lt::metadata_received_alert *mr)
{
    // Magnet just got its metadata — file list is now known, so apply
    // the .!bt suffix to each file.
    if (!mr->handle.is_valid()) return;
    auto ti = mr->handle.torrent_file();
    if (ti) {
        const auto &files = ti->files();
        for (lt::file_index_t i(0); i < files.end_file(); ++i) {
            std::string original = files.file_path(i);
            if (original.size() >= 4
                && original.compare(original.size() - 4, 4, ".!bt") == 0)
                continue;
            mr->handle.rename_file(i, original + ".!bt");
        }
    }
    stageResumeSave(mr->handle);   // persist the magnet now it has metadata
    scanTorrentForThreats(mr->handle, QString::fromStdString(mr->handle.status().name));
}

void SessionManager::onFileCompleted(const lt::file_completed_alert *fc)
{
    // File done — drop the .!bt suffix so the file appears with its
    // final name in the file manager and media server scans. The
    // path libtorrent returns here is the *current* renamed path
    // (still suffixed), so we have to strip the ".!bt" ourselves
    // before renaming; otherwise the call is a no-op.
    if (!fc->handle.is_valid()) return;
    auto ti = fc->handle.torrent_file();
    if (ti) {
        std::string current = ti->files().file_path(fc->index);
        if (current.size() >= 4
            && current.compare(current.size() - 4, 4, ".!bt") == 0) {
            std::string stripped(current, 0, current.size() - 4);
            fc->handle.rename_file(fc->index, stripped);
        }
    }
}

#ifdef BAT_LIBTORRENT_FORK
void SessionManager::onExternalIp(const lt::external_ip_alert *ea)
{
    // libtorrent learns our public IP from peers/trackers. IPv4 only — the
    // GeoIP DB is IPv4, and a v6 self-address can't seed the v4 classifier.
    const lt::address &a = ea->external_address;
    if (!a.is_v4()) return;
    m_externalIp = a.to_v4().to_uint();
    tryInstallGeoClassifier();
}

void SessionManager::tryInstallGeoClassifier()
{
    if (m_geoInstalled || m_externalIp == 0 || !m_geoIp.ready()) return;

    char cc[2];
    if (!m_geoIp.db()->lookup(m_externalIp, cc)) return;   // our IP not in the DB

    // Capture the DB by shared_ptr so it outlives this provider if need be, and
    // the country as two chars (no per-call allocation). The classifier runs on
    // libtorrent's thread inside compare_peer; the DB is immutable post-load.
    auto db = m_geoIp.db();
    const char c0 = cc[0], c1 = cc[1];
    lt::aux::set_geo_local_fn([db, c0, c1](lt::address const &a) -> bool {
        return a.is_v4() && db->inCountry(a.to_v4().to_uint(), c0, c1);
    });
    m_geoInstalled = true;
    Logger::instance().log(Logger::Info,
        QStringLiteral("GeoIP: same-country peer biasing active (%1)")
            .arg(QString::fromLatin1(cc, 2)));
}
#endif

