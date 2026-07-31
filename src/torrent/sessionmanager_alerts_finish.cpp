// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — finished / error / file-error alert handlers.

#include "torrent/sessionmanager.h"
#include "torrent/sessionconfig.h"
#include "torrent/sessionresume.h"
#include "services/platform/translator.h"
#include "services/platform/logger.h"
#include "services/security/archivescan.h"

#include <libtorrent/alert_types.hpp>
#include <libtorrent/torrent_status.hpp>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QSettings>

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
            if (SessionResume::stripIncompleteSuffix(current))
                fa->handle.rename_file(i, current);
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
        if (SessionResume::shouldEmitTorrentFinished(downloadedThisSession, resumeRefinish)) {
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

