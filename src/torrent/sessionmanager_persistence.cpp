// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — persistence slice. Fast-resume data round-trips (save/load/
// flush/persist), the resume-data directory, and the pre-3.0 legacy-dir
// migration. Split out of sessionmanager.cpp verbatim; no behaviour change.
// The DHT session_params persistence (dhtStatePath/loadStartupParams) stays in
// the core file — it's wired directly into the ctor/dtor.

#include "torrent/sessionmanager.h"

#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/write_resume_data.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <QSettings>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QFileInfo>
#include <QStringList>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <sstream>
#include <chrono>

QByteArray SessionManager::captureResumeData(int index) const
{
    QString hash = torrentHash(index);
    if (hash.isEmpty()) return {};
    QFile f(QDir(resumeDataDir()).filePath(hash + ".resume"));
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}

bool SessionManager::restoreFromResumeData(const QByteArray &data)
{
    if (data.isEmpty()) return false;
    lt::error_code ec;
    lt::add_torrent_params atp = lt::read_resume_data(
        lt::span<const char>(data.data(), data.size()), ec);
    if (ec) return false;
    atp.flags &= ~lt::torrent_flags::auto_managed;
    if (atp.ti && atp.ti->priv()) {
        atp.flags |= lt::torrent_flags::disable_dht
                   | lt::torrent_flags::disable_lsd
                   | lt::torrent_flags::disable_pex;
    }
    lt::torrent_handle h;
    try {
        h = m_session.add_torrent(atp);
    } catch (const std::exception &) {
        return false;
    }
    if (!h.is_valid()) return false;
    m_torrents.push_back(h);
    // Re-stage so the next save_resume_data call writes the .resume file
    // (which removeTorrent had deleted).
    h.save_resume_data(lt::torrent_handle::save_info_dict);
    ++m_resumeOutstanding;
    // Clear the "recently removed" guard so the resulting alert actually
    // persists; otherwise processAlerts drops it on the floor.
    if (auto st = h.status(); st.has_metadata) {
        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        m_removedHashes.remove(hash);
    }
    emit torrentAdded(static_cast<int>(m_torrents.size()) - 1);
    emit torrentsUpdated();
    return true;
}
void SessionManager::persistMagnetParams(lt::add_torrent_params atp,
                                         const QString &hash,
                                         const QString &finalSavePath)
{
    // Written with the *final* save path: if this file is ever loaded the
    // temp-path bookkeeping (m_torrentIntendedPath) is gone with the crash,
    // so downloading straight to the destination is the correct fallback.
    atp.save_path = finalSavePath.toStdString();
    QDir dir(resumeDataDir());
    if (!dir.exists())
        dir.mkpath(".");
    const std::vector<char> buf = lt::write_resume_data_buf(atp);
    QSaveFile file(dir.filePath(hash + ".resume"));
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.write(buf.data(), static_cast<qint64>(buf.size()));
    file.commit();
}

void SessionManager::stageResumeSave(const lt::torrent_handle &h)
{
    if (!h.is_valid()) return;
    h.save_resume_data(lt::torrent_handle::save_info_dict);
    m_lastResumeSaveAt[h] = QDateTime::currentSecsSinceEpoch();
    ++m_resumeOutstanding;
}

void SessionManager::saveResumeData()
{
    // QSettings writes are cheap (memory-backed; flushed on app exit) so we
    // keep them inline here.
    QSettings settings("BATorrent", "BATorrent");
    settings.setValue("globalDownloaded", globalDownloaded());
    settings.setValue("globalUploaded", globalUploaded());

    settings.beginGroup("categories");
    settings.remove("");
    for (auto it = m_categories.cbegin(); it != m_categories.cend(); ++it)
        settings.setValue(it.key(), it.value());
    settings.endGroup();

    settings.setValue("stopAfterDownload", m_stopAfterDownload);
    settings.setValue("maxSeedSeconds", m_maxSeedSeconds);

    settings.beginGroup("torrentStopAfter");
    settings.remove("");
    for (auto it = m_perTorrentStopAfter.cbegin(); it != m_perTorrentStopAfter.cend(); ++it)
        settings.setValue(it.key(), it.value());
    settings.endGroup();

    settings.beginGroup("torrentMaxSeed");
    settings.remove("");
    for (auto it = m_perTorrentMaxSeed.cbegin(); it != m_perTorrentMaxSeed.cend(); ++it)
        settings.setValue(it.key(), it.value());
    settings.endGroup();

    QDir dir(resumeDataDir());
    if (!dir.exists())
        dir.mkpath(".");

    // Kick off async save_resume_data for every torrent with metadata.
    // Counter represents *total* outstanding writes across batches; resetting
    // it here would let prior-batch alerts decrement the shutdown reservation
    // and let flushResumeDataBlocking exit before our writes actually land.
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        lt::torrent_status st = cachedStatus(h);
        if (!st.has_metadata) continue;
        h.save_resume_data(lt::torrent_handle::save_info_dict);
        m_lastResumeSaveAt[h] = now;
        ++m_resumeOutstanding;
    }
}

void SessionManager::flushResumeDataBlocking(int timeoutMs)
{
    // Called from the destructor. Kick off the async requests, then turn
    // the crank on alerts synchronously with a bounded total wait time so
    // app shutdown is durable but never hangs indefinitely.
    saveResumeData();

    const auto deadline = std::chrono::steady_clock::now()
                          + std::chrono::milliseconds(timeoutMs);

    while (m_resumeOutstanding > 0
           && std::chrono::steady_clock::now() < deadline) {
        // Wait for the next alert up to whatever time is left in the
        // deadline window. wait_for_alert returns immediately if alerts
        // are already queued.
        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - std::chrono::steady_clock::now());
        if (remaining.count() <= 0) break;
        lt::alert const *a = m_session.wait_for_alert(remaining);
        if (!a) break;
        processAlerts();
    }
}

bool SessionManager::persistResumeAlert(const lt::save_resume_data_alert *rd)
{
    QDir dir(resumeDataDir());
    if (!dir.exists())
        dir.mkpath(".");
    auto buf = lt::write_resume_data_buf(rd->params);
    // Read the info-hash from the alert's params, NOT from rd->handle.status().
    // If the torrent was removed between the save_resume_data() request and
    // this alert landing, the handle is invalid and .status() throws
    // "invalid torrent handle" — which would terminate the app.
    QString hash = QString::fromStdString(
        (std::ostringstream() << rd->params.info_hashes.get_best()).str());
    if (m_removedHashes.contains(hash))
        return false;
    QString filePath = dir.filePath(hash + ".resume");
    // Atomic: write to a temp file and rename on commit, so a crash mid-write
    // can never leave a torn/corrupt .resume that a later startup has to quarantine.
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(buf.data(), static_cast<qint64>(buf.size()));
    return file.commit();
}

void SessionManager::loadResumeData()
{
    QDir dir(resumeDataDir());
    if (!dir.exists()) return;

    QStringList files = dir.entryList({"*.resume"}, QDir::Files);
    for (const auto &fileName : files) {
        QFile file(dir.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly)) continue;

        QByteArray data = file.readAll();
        lt::error_code ec;
        lt::add_torrent_params atp = lt::read_resume_data(
            lt::span<const char>(data.data(), data.size()), ec);
        if (ec) {
            // Recovery path: the .resume bytes are corrupt (parse failed),
            // but most fields might still be partial. Try to pull the
            // embedded torrent_info out and re-add as a fresh torrent that
            // will force-recheck against whatever's on disk. Better to lose
            // the user's queue position than to silently drop their torrent
            // — the pieces themselves are still on disk under save_path.
            qWarning("loadResumeData: %s corrupt, attempting recovery (%s)",
                     qPrintable(fileName), ec.message().c_str());
            if (!atp.ti) {
                // No torrent_info embedded — nothing we can recover from.
                // Move the corrupt file aside so a future startup doesn't
                // hit the same parse failure forever.
                dir.rename(fileName, fileName + ".corrupt");
                continue;
            }
            // Wipe state that read_resume_data may have partially set; we
            // want libtorrent to start from scratch on this torrent.
            atp.flags &= ~lt::torrent_flags::paused;
            atp.have_pieces.clear();
            atp.verified_pieces.clear();
        }
        const bool recoveredFromCorrupt = static_cast<bool>(ec);

        // Manual queue management — never let libtorrent's auto-manager
        // override user pause state on resumed torrents.
        atp.flags &= ~lt::torrent_flags::auto_managed;
        // BEP-27: enforce private flag even on legacy resume data that
        // predates the addTorrent fix.
        if (atp.ti && atp.ti->priv()) {
            atp.flags |= lt::torrent_flags::disable_dht
                       | lt::torrent_flags::disable_lsd
                       | lt::torrent_flags::disable_pex;
        }

        // Reconcile the ".!bt" incomplete-suffix mapping with what's on disk. A
        // resume/strip race can leave the saved mapping pointing at "X.!bt" while
        // the finished data sits at "X" (or vice-versa) — libtorrent then can't
        // find it and re-downloads a torrent that's already complete. Point each
        // file at whichever variant has full-size data on disk; the recheck still
        // hash-validates, so a wrong guess just re-downloads (no data loss).
        if (atp.ti) {
            const QString savePath = QString::fromStdString(atp.save_path);
            const auto &fs = atp.ti->files();
            const auto &prio = atp.file_priorities;
            bool allWantedFullSize = fs.num_files() > 0;
            for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
                std::string eff = atp.renamed_files.count(i) ? atp.renamed_files[i]
                                                             : fs.file_path(i);
                const bool suffixed = eff.size() >= 4 && eff.compare(eff.size() - 4, 4, ".!bt") == 0;
                const std::string base = suffixed ? eff.substr(0, eff.size() - 4) : eff;
                const std::int64_t wantSize = fs.file_size(i);
                const QString basePath = QDir(savePath).filePath(
                    QDir::fromNativeSeparators(QString::fromStdString(base)));
                const QFileInfo plain(basePath), bt(basePath + QStringLiteral(".!bt"));
                std::string chosen = eff;
                if (plain.isFile() && plain.size() == wantSize)   chosen = base;
                else if (bt.isFile() && bt.size() == wantSize)    chosen = base + ".!bt";
                else {
                    // Only files the user actually wants count toward completeness.
                    // Stream-while-watch sets every non-video file (e.g. YTS's
                    // .txt/.jpg) to priority 0, so they never hit disk — without
                    // this guard the torrent looks "incomplete" and re-finishes
                    // (re-firing "download complete") on every launch.
                    const std::size_t idx = static_cast<std::size_t>(static_cast<int>(i));
                    const bool wanted = idx >= prio.size() || prio[idx] != lt::dont_download;
                    if (wanted) allWantedFullSize = false;
                }
                if (chosen != eff) atp.renamed_files[i] = chosen;
            }
            // All wanted files present at full size → complete for what the user
            // kept. Load in seed_mode so libtorrent trusts it instead of
            // re-checking/re-downloading on launch, and remember it as
            // already-complete so the inevitable finish alert doesn't re-notify.
            if (allWantedFullSize) {
                atp.flags |= lt::torrent_flags::seed_mode;
                const QString hash = QString::fromStdString(
                    (std::ostringstream() << atp.ti->info_hashes().get_best()).str());
                m_completedAtStartup.insert(hash);
            }
        }

        lt::torrent_handle h;
        try {
            h = m_session.add_torrent(atp);
        } catch (const std::exception &e) {
            qWarning("loadResumeData: skipping %s: %s",
                     qPrintable(fileName), e.what());
            continue;
        }
        if (!h.is_valid()) continue;
        m_torrents.push_back(h);
        // A magnet persisted before its metadata arrived: rebuild the URI-hash
        // maps so the UI keys (cover/name) and the "fetching metadata"
        // stateDetail work the same as on a fresh add.
        if (!atp.ti) {
            const QString hash = QString::fromStdString(
                (std::ostringstream() << atp.info_hashes.get_best()).str());
            m_magnetHashes[h] = hash;
            m_magnetAddedAt[h] = QDateTime::currentSecsSinceEpoch();
        }
        if (recoveredFromCorrupt) {
            h.force_recheck();
        }
        // Mark this handle so the first state_update_alert it generates runs
        // a ".!bt" strip pass for any files already complete on resume. We
        // can't call file_progress() inline here — the storage isn't bound
        // yet and libtorrent throws invalid_torrent_handle.
        m_pendingResumeStripCheck.insert(h);
    }

    // Re-apply saved per-torrent rate caps. The .resume data carries the
    // torrent_info synchronously, so the handle has metadata immediately and
    // info_hashes.get_best() returns the real hash here.
    for (auto &h : m_torrents) {
        if (!h.is_valid()) continue;
        lt::torrent_status st = h.status();
        if (!st.has_metadata) continue;
        QString hash = QString::fromStdString(
            (std::ostringstream() << st.info_hashes.get_best()).str());
        if (int kbps = m_perTorrentDownLimit.value(hash, 0))
            h.set_download_limit(kbps * 1024);
        if (int kbps = m_perTorrentUpLimit.value(hash, 0))
            h.set_upload_limit(kbps * 1024);
    }

    qDebug() << "[session] loadResumeData complete:" << m_torrents.size() << "torrents loaded";

    if (!m_torrents.empty())
        emit torrentsUpdated();
}
QString SessionManager::resumeDataDir() const
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("resume");
}

void SessionManager::migrateLegacyResumeData()
{
    // v3.0 added setOrganizationName("BATorrent"), which shifted AppDataLocation
    // from <APPDATA>/BATorrent to <APPDATA>/BATorrent/BATorrent. Pre-3.0 users'
    // .resume/.torrent files live in the old dir; copy them over once so their
    // torrents don't vanish after upgrading. Non-destructive: copies (never
    // deletes) the originals.
    QSettings s;
    if (s.value(QStringLiteral("resumeMigrated"), false).toBool())
        return;   // run exactly once — else removing all torrents would resurrect
    s.setValue(QStringLiteral("resumeMigrated"), true);

    QDir newDir(resumeDataDir());
    if (!newDir.entryList({"*.resume"}, QDir::Files).isEmpty())
        return;   // already populated — nothing to migrate

    const QString appData = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir up(appData);
    if (!up.cdUp()) return;
    const QString legacyResume = up.filePath("resume");   // <APPDATA>/BATorrent/resume
    if (QDir::cleanPath(legacyResume) == QDir::cleanPath(newDir.absolutePath())) return;

    QDir legacyDir(legacyResume);
    const QStringList files = legacyDir.entryList({"*.resume", "*.torrent"}, QDir::Files);
    if (files.isEmpty()) return;

    newDir.mkpath(".");
    int n = 0;
    for (const QString &f : files)
        if (QFile::copy(legacyDir.filePath(f), newDir.filePath(f))) ++n;
    qDebug() << "[session] migrated" << n << "resume files from legacy dir" << legacyResume;
}
