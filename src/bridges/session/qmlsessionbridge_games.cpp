// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: games slice ("Steam dos jogos piratas"). The game
// library projection, exe/folder resolution, launch + running-game polling,
// and the install pipeline (extract → detect installer → run/guide → finalize)
// plus the selected-game QML actions. Split out of qmlsessionbridge.cpp
// verbatim; no behaviour change.

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"   // full IEngine + TorrentInfo (m_session calls)
#include "services/games/playlog.h"
#include "services/integrations/gameinstall.h"
#include "services/integrations/gameexedetect.h"
#include "services/security/archivescan.h"
#include "services/metadata/nameparser.h"
#include "services/metadata/releasegroup.h"
#include "services/integrations/installerprofile.h"
#include "services/metadata/metadataresolver.h"
#include "services/platform/logger.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"   // revealTorrentRoot()

#include <QProcess>
#include <QThread>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QDateTime>
#include <QUrl>
#if defined(Q_OS_WIN)
#include <windows.h>
#include <shellapi.h>
#include <tlhelp32.h>
#else
#include <csignal>
#include <cerrno>
#endif

static_assert(GameInstall::Downloading == 0
           && GameInstall::ReadyToInstall == 1
           && GameInstall::Extracting == 2
           && GameInstall::Installing == 3
           && GameInstall::Ready == 4
           && GameInstall::Playing == 5
           && GameInstall::NeedsSetup == 6
           && GameInstall::Failed == 7);


static QString keptKey(const QString &hash) { return QStringLiteral("keptGames/") + hash; }
static QString keptName(const QString &hash)
{
    return QSettings().value(keptKey(hash) + QStringLiteral("/name")).toString();
}

QVariantList QmlSessionBridge::gameLibrary() const
{
    QVariantList out;
    QSet<QString> present;
    const int n = m_session->torrentCount();
    for (int row = 0; row < n; ++row) {
        const TorrentInfo info = m_session->torrentAt(row);
        const QString hash = m_session->torrentHashAt(row);
        if (hash.isEmpty()) continue;

        bool isGame = false;
        QString poster, title, description;
        QStringList genres;
        double rating = 0; int gyear = 0;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (meta.valid && meta.contentType == ContentType::Game) {
                isGame = true;
                title = meta.title;
                if (!meta.posterPath.isEmpty()) poster = QUrl::fromLocalFile(meta.posterPath).toString();
                description = meta.description;
                genres = meta.genres;
                rating = meta.rating;
                gyear = meta.year;
            }
        }
        if (!isGame) {
            const ParsedName pn = NameParser::parse(info.name);
            if (pn.contentType == ContentType::Game) {
                isGame = true;
                title = pn.cleanTitle.isEmpty() ? info.name : pn.cleanTitle;
            } else {
                bool hasExe = false, hasVideo = false;
                const auto files = m_session->filesAt(row);
                for (const auto &f : files) {
                    QString p = f.path.toLower();
                    if (p.endsWith(QStringLiteral(".!bt"))) p.chop(4);   // in-progress: "movie.mkv.!bt"
                    if (p.endsWith(QStringLiteral(".exe"))) hasExe = true;
                    else if (p.endsWith(QStringLiteral(".mkv")) || p.endsWith(QStringLiteral(".mp4"))
                             || p.endsWith(QStringLiteral(".avi"))) hasVideo = true;
                }
                if (GameExeDetect::looksLikeGameFromFiles(hasExe, hasVideo)) {
                    isGame = true;
                    title = pn.cleanTitle.isEmpty() ? info.name : pn.cleanTitle;
                }
            }
        }
        if (!isGame) continue;

        QVariantMap m;
        m["infoHash"]  = hash;
        m["title"]     = title.isEmpty() ? info.name : title;
        m["poster"]    = poster;
        m["progress"]   = double(info.progress);
        // A torrent paused or finished in an earlier session never fired the
        // finish this flag remembers; libtorrent's piece count still knows.
        m["completed"]  = info.completed || info.finished;
        m["paused"]     = info.paused && !info.queued;
        m["size"]       = info.totalSize;
        m["addedTime"]  = qint64(info.addedTime);
        m["hasExe"]     = !gameExe(hash).isEmpty();
        const PlayLog::Summary played = m_playLog->summary(hash);
        m["lastPlayed"] = played.lastPlayedMs;
        m["description"] = description;
        m["genres"]     = genres;
        m["rating"]     = rating;
        m["year"]       = gyear > 0 ? QString::number(gyear) : QString();
        m["playSeconds"] = played.totalSecs;
        m["launches"]   = played.launches;
        m["playing"]    = m_runningGames.contains(hash);
        m["group"]      = ReleaseGroup::detect(info.name);
        m["installState"] = gameInstallState(hash, GameExeDetect::dataComplete(info.completed, info.progress));
        m["extractProgress"] = m_extractProgress.value(hash, -1);   // 0-100 while unpacking, else -1
        out << m;
        // Remembered while the torrent is here, so the game outlives it.
        if (m["hasExe"].toBool() && keptName(hash) != info.name) {
            QSettings s;
            s.setValue(keptKey(hash) + QStringLiteral("/name"), info.name);
            s.setValue(keptKey(hash) + QStringLiteral("/size"), info.totalSize);
            s.setValue(keptKey(hash) + QStringLiteral("/added"), qint64(info.addedTime));
        }
        present.insert(hash);
    }

    // Games whose torrent is gone but which are still installed: the repack
    // was 60 GB of dead weight once the game sat in its own folder, and its
    // playtime and achievements should not go with it. Shown while the exe
    // is there; uninstalled, or deleted along with the torrent, they drop out.
    QSettings s;
    s.beginGroup(QStringLiteral("keptGames"));
    const QStringList kept = s.childGroups();
    s.endGroup();
    for (const QString &hash : kept) {
        if (present.contains(hash)) continue;
        const QString exe = gameExe(hash);
        if (exe.isEmpty() || !QFileInfo::exists(exe)) continue;
        const QString name = keptName(hash);
        QVariantMap m;
        QString title = NameParser::parse(name).cleanTitle;
        if (m_resolver && m_resolver->hasCached(hash)) {
            const auto meta = m_resolver->cached(hash);
            if (meta.valid) {
                if (!meta.title.isEmpty()) title = meta.title;
                if (!meta.posterPath.isEmpty()) m["poster"] = QUrl::fromLocalFile(meta.posterPath).toString();
                m["description"] = meta.description;
                m["genres"] = meta.genres;
                m["rating"] = meta.rating;
                m["year"] = meta.year > 0 ? QString::number(meta.year) : QString();
            }
        }
        const PlayLog::Summary played = m_playLog->summary(hash);
        m["infoHash"] = hash;
        m["title"] = title.isEmpty() ? name : title;
        m["progress"] = 1.0;
        m["completed"] = true;
        m["paused"] = false;
        m["size"] = s.value(keptKey(hash) + QStringLiteral("/size")).toLongLong();
        m["addedTime"] = s.value(keptKey(hash) + QStringLiteral("/added")).toLongLong();
        m["hasExe"] = true;
        m["lastPlayed"] = played.lastPlayedMs;
        m["playSeconds"] = played.totalSecs;
        m["launches"] = played.launches;
        m["playing"] = m_runningGames.contains(hash);
        m["group"] = ReleaseGroup::detect(name);
        m["installState"] = gameInstallState(hash, true);
        m["extractProgress"] = -1;
        m["torrentless"] = true;
        out << m;
    }
    return out;
}

bool QmlSessionBridge::gameTorrentless(const QString &infoHash) const
{
    return m_session->torrentIndexByInfoHash(infoHash) < 0 && QSettings().contains(keptKey(infoHash) + QStringLiteral("/name"));
}

void QmlSessionBridge::forgetGame(const QString &infoHash)
{
    if (m_session->torrentIndexByInfoHash(infoHash) >= 0) return;   // only a game whose torrent is gone
    QSettings s;
    s.remove(keptKey(infoHash));
    s.remove(QStringLiteral("gameExe/") + infoHash);
    emit gamesChanged();
}

QString QmlSessionBridge::gameExe(const QString &infoHash) const
{
    return QSettings().value(QStringLiteral("gameExe/") + infoHash).toString();
}

void QmlSessionBridge::setGameExe(const QString &infoHash, const QString &fileUrl)
{
    const QString path = fileUrl.startsWith(QStringLiteral("file:")) ? QUrl(fileUrl).toLocalFile() : fileUrl;
    if (path.isEmpty()) return;
    QSettings().setValue(QStringLiteral("gameExe/") + infoHash, path);
    // a manual exe supersedes any stalled install flow: the card flips to Play
    m_gameInstallState.remove(infoHash);
    emit gamesChanged();
    emit toast(tr_("hub_exe_set"), QFileInfo(path).fileName());
}

QString QmlSessionBridge::gameFolder(const QString &infoHash) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) {
        // A game kept after its torrent went: the folder its exe is in.
        const QString exe = gameExe(infoHash);
        return exe.isEmpty() ? QString() : QFileInfo(exe).absolutePath();
    }
    const TorrentInfo info = m_session->torrentAt(row);
    const QString root = info.savePath + QStringLiteral("/") + info.name;
    if (QFileInfo(root).isDir()) return root;
    // A lone archive unpacks into a folder of its own beside it (ArchiveExtractor):
    // that is the game, not the whole save folder with every other download.
    if (ArchiveScan::isArchive(info.name))
        return info.savePath + QStringLiteral("/") + ArchiveScan::unpackFolderName(info.name);
    return info.savePath;
}

static bool pidAlive(qint64 pid)
{
    if (pid <= 0) return false;
#if defined(Q_OS_WIN)
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, DWORD(pid));
    if (!h) return false;
    const DWORD r = WaitForSingleObject(h, 0);
    CloseHandle(h);
    return r == WAIT_TIMEOUT;   // still running
#else
    return ::kill(pid_t(pid), 0) == 0 || errno != ESRCH;
#endif
}

#if defined(Q_OS_WIN)
// The exe a running pid was started from, "" when it can't be read.
static QString imageOf(DWORD pid)
{
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return {};
    wchar_t buf[MAX_PATH * 2];
    DWORD len = DWORD(sizeof(buf) / sizeof(buf[0]));
    const QString path = QueryFullProcessImageNameW(h, 0, buf, &len) ? QString::fromWCharArray(buf, int(len)) : QString();
    CloseHandle(h);
    return path;
}

static bool isUnder(const QString &path, const QString &folder)
{
    return !path.isEmpty()
           && path.startsWith(QDir::toNativeSeparators(QDir::cleanPath(folder)) + QLatin1Char('\\'), Qt::CaseInsensitive);
}
#endif

// A handle kept on a launched game, so its exit code can be read once it is
// gone: the one clue to why a game "closed right after starting" (0xC0000135
// is a missing DLL, 0xC0000005 a crash). 0 where there is no such handle.
static quintptr holdProcess(qint64 pid)
{
#if defined(Q_OS_WIN)
    return quintptr(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, DWORD(pid)));
#else
    Q_UNUSED(pid);
    return 0;
#endif
}

// The held process's exit code as "0x…", "" without a handle; lets the handle go.
static QString releaseProcess(quintptr handle)
{
#if defined(Q_OS_WIN)
    if (!handle) return {};
    DWORD code = 0;
    const bool ok = GetExitCodeProcess(HANDLE(handle), &code);
    CloseHandle(HANDLE(handle));
    return ok ? QStringLiteral("0x%1").arg(qulonglong(code), 8, 16, QLatin1Char('0')) : QString();
#else
    Q_UNUSED(handle);
    return {};
#endif
}

static QString runningKey(const QString &hash) { return QStringLiteral("runningGames/") + hash; }

// A running process whose exe lives under folder, 0 if none. A launcher that
// starts the real game and exits (Unreal's bootstrap, many repacks) hands off to
// it; following that pid keeps "playing" and the playtime honest. Never a crash
// handler: a Unity game that dies on boot starts one from the same folder.
static qint64 processUnder(const QString &folder)
{
#if defined(Q_OS_WIN)
    if (folder.isEmpty()) return 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    qint64 found = 0;
    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    for (BOOL ok = Process32FirstW(snap, &pe); ok && !found; ok = Process32NextW(snap, &pe)) {
        const QString path = imageOf(pe.th32ProcessID);
        if (isUnder(path, folder) && !GameExeDetect::neverTheGame(QFileInfo(path).fileName().toLower()))
            found = pe.th32ProcessID;
    }
    CloseHandle(snap);
    return found;
#else
    Q_UNUSED(folder);
    return 0;
#endif
}

// pid is alive and runs an exe from under folder. A pid saved across an app
// restart may since belong to something else entirely: Windows reuses them.
static bool pidUnder(qint64 pid, const QString &folder)
{
    if (!pidAlive(pid)) return false;
#if defined(Q_OS_WIN)
    return isUnder(imageOf(DWORD(pid)), folder);
#else
    Q_UNUSED(folder);
    return true;
#endif
}

// A game that is running is remembered in settings, so closing or restarting
// BATorrent mid-game (an update, a crash, Quit from the tray) does not throw
// the session away: the next start picks the game back up, or credits it up to
// the last time it was seen alive. Stored as "pid;startMs;lastAliveMs;exe".
void QmlSessionBridge::persistRunningGame(const QString &hash, qint64 nowMs)
{
    QSettings().setValue(runningKey(hash),
                         QStringLiteral("%1;%2;%3;%4").arg(m_runningGames.value(hash))
                             .arg(m_gameStartMs.value(hash)).arg(nowMs)
                             .arg(m_runningExe.value(hash)));
}

void QmlSessionBridge::restoreRunningGames()
{
    QSettings s;
    s.beginGroup(QStringLiteral("runningGames"));
    const QStringList hashes = s.childKeys();
    s.endGroup();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    for (const QString &hash : hashes) {
        const QString key = runningKey(hash);
        const QStringList f = s.value(key).toString().split(QLatin1Char(';'));
        s.remove(key);
        if (f.size() < 4) continue;
        const qint64 pid = f[0].toLongLong(), startMs = f[1].toLongLong(), lastAlive = f[2].toLongLong();
        const QString exe = f.mid(3).join(QLatin1Char(';'));   // a path may hold ';'
        if (startMs <= 0 || exe.isEmpty()) continue;
        if (pidUnder(pid, QFileInfo(exe).absolutePath())) {
            qInfo().noquote() << "[game] still running from before, following pid" << pid << exe;
            m_runningGames.insert(hash, pid);
            m_gameStartMs.insert(hash, startMs);
            m_runningExe.insert(hash, exe);
            m_runningHandle.insert(hash, holdProcess(pid));
            persistRunningGame(hash, nowMs);
            startAchievementWatch(hash);
        } else if (lastAlive > startMs) {
            const qint64 secs = (lastAlive - startMs) / 1000;
            qInfo().noquote() << "[game] ended while closed, credited" << secs << "s:" << exe;
            m_playLog->recordSession(hash, startMs, secs);
        }
    }
    if (!m_runningGames.isEmpty()) emit gamesChanged();
}

// mac games are .app bundles (directories): QProcess can't exec those, but
// `open -W` can, and it stays alive until the game quits so playtime tracking
// via the pid still works.
static bool startGameProcess(const QString &exe, qint64 *pid, QString *why)
{
#if defined(Q_OS_MACOS)
    if (exe.endsWith(QStringLiteral(".app"), Qt::CaseInsensitive) && QFileInfo(exe).isDir())
        return QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-W"), exe},
                                       QFileInfo(exe).absolutePath(), pid);
#endif
    QProcess proc;
    proc.setProgram(exe);
    proc.setWorkingDirectory(QFileInfo(exe).absolutePath());
    if (proc.startDetached(pid))
        return true;
    *why = proc.errorString();
#if defined(Q_OS_WIN)
    // cracked/installer exes commonly require elevation: CreateProcess fails
    // with ERROR_ELEVATION_REQUIRED, ShellExecute shows the UAC prompt instead
    // (no pid → no playtime tracking for this launch; better than a dead button)
    const QString wd = QFileInfo(exe).absolutePath();
    const auto r = reinterpret_cast<INT_PTR>(
        ShellExecuteW(nullptr, L"open",
                      reinterpret_cast<const wchar_t *>(exe.utf16()), nullptr,
                      reinterpret_cast<const wchar_t *>(wd.utf16()), SW_SHOWNORMAL));
    if (r <= 32) *why += QStringLiteral("; ShellExecute error %1").arg(qint64(r));
    return r > 32;
#else
    return false;
#endif
}

void QmlSessionBridge::launchGame(const QString &infoHash)
{
    QString exe = gameExe(infoHash);              // a manual override always wins
    // except a crash handler or uninstaller, which older builds saved as the exe
    if (!exe.isEmpty() && GameExeDetect::neverTheGame(QFileInfo(exe).fileName().toLower())) {
        qInfo().noquote() << "[game] ignoring saved exe" << exe;
        QSettings().remove(QStringLiteral("gameExe/") + infoHash);
        exe.clear();
    }
    bool isInstaller = false;
    if (exe.isEmpty() || !QFile::exists(exe)) {
        const int row = m_session->torrentIndexByInfoHash(infoHash);
        const QString title = row < 0 ? QString() : NameParser::parse(m_session->torrentAt(row).name).cleanTitle;
        exe = GameExeDetect::autodetect(gameFolder(infoHash), &isInstaller, title);
    }
    // Only a setup in the torrent: the game may already be installed elsewhere,
    // by a run of it this app lost track of, or by hand.
    if (isInstaller) {
        const QString installed = installedGameExe(infoHash, 0);
        if (!installed.isEmpty()) {
            exe = installed;
            isInstaller = false;
            QSettings().setValue(QStringLiteral("gameExe/") + infoHash, exe);
        }
    }

    if (!exe.isEmpty() && QFile::exists(exe)) {
        qint64 pid = 0;
        QString why;
        // Detached so the game survives BATorrent closing; the returned pid lets
        // us poll for exit and credit playtime.
        if (startGameProcess(exe, &pid, &why)) {
            qInfo().noquote() << "[game] launched" << exe << "pid" << pid
                              << (isInstaller ? "(installer)" : "");
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            m_playLog->recordLaunch(infoHash, nowMs);
            if (!isInstaller && pid > 0 && !m_runningGames.contains(infoHash))
                followRunningGame(infoHash, pid, exe, nowMs);
            emit toast(isInstaller ? tr_("hub_game_installing") : tr_("hub_game_launch"),
                       QFileInfo(exe).completeBaseName());
            return;
        }
        qWarning().noquote() << "[game] could not start" << exe << "-" << why;
        emit toast(tr_("hub_launch_failed"), QFileInfo(exe).fileName());
    } else {
        qInfo().noquote() << "[game] no exe found under" << gameFolder(infoHash);
    }
    // nothing runnable (or the exe wouldn't start) → open the folder so the
    // user can run it by hand or set the right executable
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const TorrentInfo info = m_session->torrentAt(row);
    revealTorrentRoot(info.savePath, info.name);
}

void QmlSessionBridge::followRunningGame(const QString &hash, qint64 pid, const QString &exe, qint64 nowMs)
{
    m_runningGames.insert(hash, pid);
    m_gameStartMs.insert(hash, nowMs);
    m_runningExe.insert(hash, exe);
    m_runningHandle.insert(hash, holdProcess(pid));
    persistRunningGame(hash, nowMs);
    startAchievementWatch(hash);
    emit gamesChanged();   // "playing now"
}

void QmlSessionBridge::noticeOutsideLaunches(qint64 nowMs)
{
#if defined(Q_OS_WIN)
    // One walk of the process list every few seconds covers every game.
    if (nowMs - m_outsideScanMs < 5000) return;
    m_outsideScanMs = nowMs;

    QHash<QString, QString> folderOf;   // hash → folder its saved exe lives in
    QSettings s;
    s.beginGroup(QStringLiteral("gameExe"));
    const QStringList keys = s.childKeys();
    s.endGroup();
    for (const QString &hash : keys) {
        if (m_runningGames.contains(hash) || m_installWatch.contains(hash)) continue;
        const QString exe = s.value(QStringLiteral("gameExe/") + hash).toString();
        if (exe.isEmpty() || GameExeDetect::neverTheGame(QFileInfo(exe).fileName().toLower())) continue;
        const int row = m_session->torrentIndexByInfoHash(hash);
        if (row < 0 && !gameTorrentless(hash)) continue;
        const QString folder = QFileInfo(exe).absolutePath();
        // An exe loose in the save folder would claim everything run from Downloads.
        if (row >= 0 && QDir::cleanPath(folder) == QDir::cleanPath(m_session->torrentAt(row).savePath)) continue;
        folderOf.insert(hash, folder);
    }
    if (folderOf.isEmpty()) return;

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    for (BOOL ok = Process32FirstW(snap, &pe); ok && !folderOf.isEmpty(); ok = Process32NextW(snap, &pe)) {
        const QString path = imageOf(pe.th32ProcessID);
        if (path.isEmpty() || GameExeDetect::neverTheGame(QFileInfo(path).fileName().toLower())) continue;
        for (auto it = folderOf.begin(); it != folderOf.end(); ++it) {
            if (!isUnder(path, it.value())) continue;
            qInfo().noquote() << "[game] noticed running outside the app:" << path << "pid" << pe.th32ProcessID;
            m_playLog->recordLaunch(it.key(), nowMs);
            followRunningGame(it.key(), pe.th32ProcessID, path, nowMs);
            folderOf.erase(it);
            break;
        }
    }
    CloseHandle(snap);
#else
    Q_UNUSED(nowMs);
#endif
}

// Poll launched games for exit; credit elapsed time to the per-game total.
void QmlSessionBridge::pollRunningGames()
{
    // Clear orphan gameExe paths off the getter path (QML bindings re-enter it).
    {
        QSettings s;
        s.beginGroup(QStringLiteral("gameExe"));
        const QStringList keys = s.childKeys();
        s.endGroup();
        bool cleared = false;
        for (const QString &hash : keys) {
            const QString path = s.value(QStringLiteral("gameExe/") + hash).toString();
            GameInstall::DeriveIn in;
            in.running = m_runningGames.contains(hash);
            in.overlay = m_gameInstallState.value(hash, -1);
            in.hasExePath = !path.isEmpty();
            in.exeExists = in.hasExePath && QFileInfo::exists(path);
            if (GameInstall::shouldClearStaleExe(in)) {
                s.remove(QStringLiteral("gameExe/") + hash);
                cleared = true;
            }
        }
        if (cleared) emit gamesChanged();
    }

    if (!m_runningRestored) {
        m_runningRestored = true;
        restoreRunningGames();
    }
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    noticeOutsideLaunches(nowMs);
    if (m_runningGames.isEmpty()) return;
    // Once a minute is enough for "last seen alive": it bounds what a session
    // that ends while the app is closed can be credited.
    const bool saveAlive = nowMs - m_runningSavedMs >= 60 * 1000;
    if (saveAlive) m_runningSavedMs = nowMs;
    bool changed = false;
    for (const QString &hash : m_runningGames.keys()) {
        if (pidAlive(m_runningGames.value(hash))) {
            if (saveAlive) persistRunningGame(hash, nowMs);
            continue;
        }
        // The launcher handed off to the game proper: follow that one instead.
        // Searched under the launched exe's own folder, never the torrent's save
        // path, which for a single-file torrent is the whole Downloads folder.
        const QString exe = m_runningExe.value(hash);
        if (const qint64 next = exe.isEmpty() ? 0 : processUnder(QFileInfo(exe).absolutePath())) {
            qInfo() << "[game] launcher exited" << releaseProcess(m_runningHandle.take(hash))
                    << "- following pid" << next;
            m_runningGames.insert(hash, next);
            m_runningHandle.insert(hash, holdProcess(next));
            persistRunningGame(hash, nowMs);
            continue;
        }
        QSettings().remove(runningKey(hash));
        m_runningExe.remove(hash);
        const qint64 startMs = m_gameStartMs.value(hash);
        const qint64 secs = (QDateTime::currentMSecsSinceEpoch() - startMs) / 1000;
        m_playLog->recordSession(hash, startMs, secs);
        const QString code = releaseProcess(m_runningHandle.take(hash));
        qInfo().noquote() << "[game] exited after" << secs << "s, exit code" << (code.isEmpty() ? QStringLiteral("?") : code)
                          << ":" << exe;
        // Gone within seconds with nothing of it left running: it did not really
        // start (wrong exe, missing runtime, a crash on boot). Say so instead of
        // just flipping back to Play.
        if (secs < 10) emit toast(tr_("hub_game_quit_early"), QFileInfo(exe).fileName());
        m_runningGames.remove(hash);
        m_gameStartMs.remove(hash);
        stopAchievementWatch(hash);
        changed = true;
    }
    if (changed) emit gamesChanged();
}

QVariantList QmlSessionBridge::gameActivity(const QString &infoHash, int days) const
{
    QVariantList out;
    for (const qint64 secs : m_playLog->dailySecs(infoHash, days, QDateTime::currentMSecsSinceEpoch()))
        out << secs;
    return out;
}

void QmlSessionBridge::importLegacyPlaytime()
{
    QSettings s;
    const QStringList groups = { QStringLiteral("gameSeconds"), QStringLiteral("gamePlayed"),
                                 QStringLiteral("gameLaunches") };
    QSet<QString> hashes;
    for (const QString &g : groups) {
        s.beginGroup(g);
        for (const QString &k : s.childKeys()) hashes.insert(k);
        s.endGroup();
    }
    if (hashes.isEmpty()) return;
    for (const QString &h : hashes)
        m_playLog->importTotals(h, s.value(QStringLiteral("gameSeconds/") + h).toLongLong(),
                                s.value(QStringLiteral("gamePlayed/") + h).toLongLong(),
                                s.value(QStringLiteral("gameLaunches/") + h).toLongLong());
    // The old keys stay: a build from before the log still reads them, and
    // importTotals ignores a game the log already has, so nothing doubles.
    m_playLog->save();
}

// ---- Game install orchestrator (the "Steam pirata" one-click chain) --------

int QmlSessionBridge::gameInstallState(const QString &infoHash, bool completed) const
{
    QString exe = gameExe(infoHash);
    // One a build saved by mistake (a crash handler, FitGirl's QuickSFV) is no
    // install: the card offers Install again rather than Play.
    if (!exe.isEmpty() && GameExeDetect::neverTheGame(QFileInfo(exe).fileName().toLower()))
        exe.clear();
    GameInstall::DeriveIn in;
    in.running = m_runningGames.contains(infoHash);
    in.overlay = m_gameInstallState.value(infoHash, -1);
    in.hasExePath = !exe.isEmpty();
    in.exeExists = in.hasExePath && QFileInfo::exists(exe);
    in.completed = completed;
    // Do not mutate QSettings from this getter: QML bindings re-enter it. Stale
    // paths are cleared from pollRunningGames / a deferred slot instead.
    return GameInstall::derive(in);
}
