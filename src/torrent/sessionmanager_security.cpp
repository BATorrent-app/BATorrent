// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// SessionManager — safety/security slice. The memory guard (private-footprint
// watchdog), per-torrent fault isolation, warn-only threat scanning, the opt-in
// Defender exclusion, and the retrying trash/delete helpers. Split out of
// sessionmanager.cpp verbatim; no behaviour change.

#include "torrent/sessionmanager.h"
#include "torrent/memguard.h"
#include "services/platform/translator.h"
#include "services/security/suspiciousscan.h"
#include "services/security/defender.h"

#if defined(Q_OS_MACOS)
#include <mach/mach.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <psapi.h>
#elif defined(Q_OS_LINUX)
#include <unistd.h>
#include <cstdio>
#endif

#include <libtorrent/torrent_info.hpp>
#include <libtorrent/torrent_flags.hpp>
#include <QCoreApplication>
#include <QSettings>
#include <QDateTime>
#include <QTimer>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <sstream>

// Resident set size of this process, in bytes; -1 if it can't be read.
// The app's PRIVATE memory footprint — deliberately NOT the working/resident set.
// libtorrent 2.x maps the download files (mmap storage), so the working set grows
// ~1:1 with what's being downloaded as file-cache pages go resident. Those pages
// are reclaimable by the OS on demand, not a leak — but the old WorkingSetSize /
// resident_size readings counted them, so the memory guard false-paused real
// downloads "every ~1 GB" (Windows user report). Private memory excludes the
// file-backed mmap cache, so the guard only sees genuine allocation growth.
static qint64 currentRssBytes()
{
#if defined(Q_OS_MACOS)
    // phys_footprint == Activity Monitor's "Memory": excludes clean file-backed
    // (mmap) pages, unlike resident_size.
    task_vm_info_data_t info;
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO,
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS)
        return static_cast<qint64>(info.phys_footprint);
    return -1;
#elif defined(Q_OS_WIN)
    // PrivateUsage == private commit (pagefile-backed). File-mapped pages are
    // file-backed, so the libtorrent mmap cache is excluded.
    PROCESS_MEMORY_COUNTERS_EX pmc;
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&pmc), sizeof(pmc)))
        return static_cast<qint64>(pmc.PrivateUsage);
    return -1;
#elif defined(Q_OS_LINUX)
    // statm "resident" still counts file-backed pages; acceptable for the guard's
    // coarse purpose, and the Linux mmap path is less prone to the Windows blow-up.
    qint64 rss = -1;
    if (FILE *f = std::fopen("/proc/self/statm", "r")) {
        long total = 0, resident = 0;
        if (std::fscanf(f, "%ld %ld", &total, &resident) == 2 && resident > 0)
            rss = static_cast<qint64>(resident) * sysconf(_SC_PAGESIZE);
        std::fclose(f);
    }
    return rss;
#else
    return -1;
#endif
}

// Safety valve against a runaway allocation bug eating the user's machine.
// Stage 1 (over the cap): pause everything — the likeliest growth source is
// piece/disk buffers — and warn, at most once per 5 min. Recoverable.
// Stage 2 (2× the cap, i.e. the pause didn't help → genuine runaway): save
// state and quit gracefully before the OS starts swapping/OOM-killing.
void SessionManager::checkMemoryGuard()
{
    const int capMB = QSettings("BATorrent", "BATorrent").value("memGuardMB", 8192).toInt();
    if (capMB <= 0) return;   // 0 = disabled

    const qint64 rss = currentRssBytes();
    if (rss < 0) return;
    const qint64 rssMB = rss / (1024 * 1024);

    switch (bat::memGuardEvaluate(rssMB, capMB)) {
    case bat::MemGuardAction::Panic:
        qCritical() << "[session] MEMORY GUARD PANIC: private" << rssMB
                    << "MB >= 2x cap" << capMB << "MB — saving state and quitting";
        saveResumeData();
        QCoreApplication::quit();
        return;
    case bat::MemGuardAction::Pause: {
        static qint64 lastWarn = 0;
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        if (now - lastWarn >= 300) {
            lastWarn = now;
            qWarning() << "[session] MEMORY GUARD: private" << rssMB
                       << "MB >= cap" << capMB << "MB — pausing all torrents";
            pauseAll();
            emit torrentError(tr_("warn_mem_guard").arg(rssMB).arg(capMB));
        }
        return;
    }
    case bat::MemGuardAction::None:
        return;
    }
}

void SessionManager::saveSecurityWarned()
{
    QSettings("BATorrent", "BATorrent")
        .setValue("securityWarned", QStringList(m_securityWarned.values()));
}

// Fault isolation: a torrent that keeps erroring (bad disk sector, vanished
// file, corrupt resume) shouldn't be allowed to thrash forever and drag the
// whole app down. Count clustered errors; once a torrent crosses the threshold
// we pause it and tell the user, instead of retrying endlessly.
void SessionManager::noteTorrentFault(const lt::torrent_handle &h, const QString &name)
{
    if (!h.is_valid()) return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    qint64 &last = m_faultLastAt[h];
    int &n = m_faultCount[h];
    if (now - last > 120) n = 0;     // errors must cluster (<2 min apart) to count as thrashing
    last = now;
    if (++n < 8) return;

    lt::torrent_status st = h.status();
    if (!(st.flags & lt::torrent_flags::paused)) {
        h.pause();
        const QString nm = name.isEmpty() ? QString::fromStdString(st.name) : name;
        qWarning() << "[session] fault-isolated torrent (repeated errors):" << nm;
        emit torrentError(tr_("warn_torrent_faulted").arg(nm));
    }
    n = 0;   // reset; if the user fixes it, resumes, and it re-faults, we isolate again
    if (m_faultCount.size() > 500) { m_faultCount.clear(); m_faultLastAt.clear(); }
}

// Opt-in (default off): add a new download root to Defender's exclusions once.
// Keyed by save root so torrents sharing a folder cost a single UAC prompt.
void SessionManager::maybeAutoExcludeDefender(const QString &savePath)
{
#if defined(Q_OS_WIN) && !defined(BAT_STORE_BUILD)
    if (!m_autoDefenderExclude || savePath.isEmpty()) return;
    if (m_defenderExcludedRoots.contains(savePath)) return;
    m_defenderExcludedRoots.insert(savePath);
    QSettings("BATorrent", "BATorrent")
        .setValue("defenderExcludedRoots", QStringList(m_defenderExcludedRoots.values()));
    Defender::addExclusion(savePath);
#else
    Q_UNUSED(savePath);
#endif
}

// Warn-only malware awareness. Runs the local heuristic over the torrent's file
// list and, if it flags anything, raises a single non-blocking warning per
// torrent (never blocks, deletes, or claims "safe"). Silent when clean.
void SessionManager::scanTorrentForThreats(const lt::torrent_handle &h, const QString &name)
{
    if (!m_warnSuspiciousFiles || !h.is_valid()) return;
    auto ti = h.torrent_file();
    if (!ti) return;

    const QString hash = QString::fromStdString(
        (std::ostringstream() << h.info_hashes().get_best()).str());
    if (m_securityWarned.contains(hash)) return;

    const auto &fs = ti->files();
    QList<SuspiciousScan::ScanFile> scanFiles;
    for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
        QString p = QString::fromStdString(fs.file_path(i));
        if (p.endsWith(QLatin1String(".!bt"))) p.chop(4);   // ignore in-progress suffix
        scanFiles.append({ p, static_cast<qint64>(fs.file_size(i)) });
    }

    const auto findings = SuspiciousScan::scan(scanFiles);
    if (findings.isEmpty()) return;

    m_securityWarned.insert(hash);
    saveSecurityWarned();

    QStringList files;
    for (const auto &f : findings) {
        const QString shown = QFileInfo(f.file).fileName();
        if (!files.contains(shown)) files << shown;
    }
    qWarning() << "[session] suspicious files in" << name << ":" << files;
    emit suspiciousFilesDetected(name, files);
}

// The schedule*() retry timers die if the app quits inside the backoff window,
// which used to silently leave the data on disk ("delete doesn't always
// delete"). The pending targets are persisted and retried on the next launch
// (ctor), when the old session's file locks are guaranteed gone.
static void addPendingTargets(const char *key, const QStringList &targets)
{
    QSettings s("BATorrent", "BATorrent");
    QStringList pending = s.value(QLatin1String(key)).toStringList();
    for (const QString &t : targets)
        if (!pending.contains(t)) pending << t;
    s.setValue(QLatin1String(key), pending);
}

static void clearDoneTargets(const char *key, const QStringList &attempted,
                             const QStringList &remaining)
{
    QSettings s("BATorrent", "BATorrent");
    QStringList pending = s.value(QLatin1String(key)).toStringList();
    for (const QString &p : attempted)
        if (!remaining.contains(p)) pending.removeAll(p);
    if (pending.isEmpty()) s.remove(QLatin1String(key));
    else s.setValue(QLatin1String(key), pending);
}

void SessionManager::scheduleTrash(const QStringList &targets, int attempt)
{
    // back off a little more each round; ~30s budget before we give up and leave
    // the files on disk (never force-delete).
    if (attempt == 0) addPendingTargets("pendingTrashTargets", targets);
    const int delay = qMin(2000 + attempt * 800, 4000);
    QTimer::singleShot(delay, this, [this, targets, attempt]() {
        QStringList remaining;
        for (const QString &p : targets) {
            if (!QFileInfo::exists(p)) continue;          // gone (trashed, or never there)
            if (!QFile::moveToTrash(p)) remaining << p;   // still locked — try again
        }
        clearDoneTargets("pendingTrashTargets", targets, remaining);
        if (remaining.isEmpty()) return;
        if (attempt < 10) scheduleTrash(remaining, attempt + 1);
        else qWarning() << "[session] moveToTrash gave up (files still locked):" << remaining;
    });
}

void SessionManager::scheduleDelete(const QStringList &targets, int attempt)
{
    if (attempt == 0) addPendingTargets("pendingDeleteTargets", targets);
    const int delay = qMin(2000 + attempt * 800, 4000);
    QTimer::singleShot(delay, this, [this, targets, attempt]() {
        QStringList remaining;
        for (const QString &p : targets) {
            QFileInfo fi(p);
            if (!fi.exists()) continue;
            const bool ok = fi.isDir() ? QDir(p).removeRecursively() : QFile::remove(p);
            if (!ok) remaining << p;
        }
        clearDoneTargets("pendingDeleteTargets", targets, remaining);
        if (remaining.isEmpty()) return;
        if (attempt < 10) scheduleDelete(remaining, attempt + 1);
        else qWarning() << "[session] permanent delete gave up (files still locked):" << remaining;
    });
}
