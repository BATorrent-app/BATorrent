// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — games slice ("Steam dos jogos piratas"). The game
// library projection, exe/folder resolution, launch + running-game polling,
// and the install pipeline (extract → detect installer → run/guide → finalize)
// plus the selected-game QML actions. Split out of qmlsessionbridge.cpp
// verbatim; no behaviour change.

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"   // full IEngine + TorrentInfo (m_session calls)
#include "services/integrations/gameinstall.h"
#include "services/integrations/gameexedetect.h"
#include "services/metadata/nameparser.h"
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


QVariantList QmlSessionBridge::gameLibrary() const
{
    QVariantList out;
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
        m["completed"]  = info.completed;
        m["size"]       = info.totalSize;
        m["addedTime"]  = qint64(info.addedTime);
        m["hasExe"]     = !gameExe(hash).isEmpty();
        m["lastPlayed"] = QSettings().value(QStringLiteral("gamePlayed/") + hash, 0).toLongLong();
        m["description"] = description;
        m["genres"]     = genres;
        m["rating"]     = rating;
        m["year"]       = gyear > 0 ? QString::number(gyear) : QString();
        m["playSeconds"] = QSettings().value(QStringLiteral("gameSeconds/") + hash, 0).toLongLong();
        m["launches"]   = QSettings().value(QStringLiteral("gameLaunches/") + hash, 0).toLongLong();
        m["playing"]    = m_runningGames.contains(hash);
        m["installState"] = gameInstallState(hash, GameExeDetect::dataComplete(info.completed, info.progress));
        out << m;
    }
    return out;
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
    // a manual exe supersedes any stalled install flow — the card flips to Play
    m_gameInstallState.remove(infoHash);
    emit gamesChanged();
    emit toast(tr_("hub_exe_set"), QFileInfo(path).fileName());
}

QString QmlSessionBridge::gameFolder(const QString &infoHash) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    const TorrentInfo info = m_session->torrentAt(row);
    const QString root = info.savePath + QStringLiteral("/") + info.name;
    return QFileInfo(root).isDir() ? root : info.savePath;
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

// mac games are .app bundles (directories) — QProcess can't exec those, but
// `open -W` can, and it stays alive until the game quits so playtime tracking
// via the pid still works.
static bool startGameProcess(const QString &exe, qint64 *pid)
{
#if defined(Q_OS_MACOS)
    if (exe.endsWith(QStringLiteral(".app"), Qt::CaseInsensitive) && QFileInfo(exe).isDir())
        return QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-W"), exe},
                                       QFileInfo(exe).absolutePath(), pid);
#endif
    if (QProcess::startDetached(exe, {}, QFileInfo(exe).absolutePath(), pid))
        return true;
#if defined(Q_OS_WIN)
    // cracked/installer exes commonly require elevation — CreateProcess fails
    // with ERROR_ELEVATION_REQUIRED, ShellExecute shows the UAC prompt instead
    // (no pid → no playtime tracking for this launch; better than a dead button)
    const QString wd = QFileInfo(exe).absolutePath();
    const auto r = reinterpret_cast<INT_PTR>(
        ShellExecuteW(nullptr, L"open",
                      reinterpret_cast<const wchar_t *>(exe.utf16()), nullptr,
                      reinterpret_cast<const wchar_t *>(wd.utf16()), SW_SHOWNORMAL));
    return r > 32;
#else
    return false;
#endif
}

void QmlSessionBridge::launchGame(const QString &infoHash)
{
    QString exe = gameExe(infoHash);              // a manual override always wins
    bool isInstaller = false;
    if (exe.isEmpty() || !QFile::exists(exe))
        exe = GameExeDetect::autodetect(gameFolder(infoHash), &isInstaller);

    if (!exe.isEmpty() && QFile::exists(exe)) {
        qint64 pid = 0;
        // Detached so the game survives BATorrent closing; the returned pid lets
        // us poll for exit and credit playtime.
        if (startGameProcess(exe, &pid)) {
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            QSettings s;
            s.setValue(QStringLiteral("gamePlayed/") + infoHash, nowMs);
            s.setValue(QStringLiteral("gameLaunches/") + infoHash,
                       s.value(QStringLiteral("gameLaunches/") + infoHash, 0).toLongLong() + 1);
            if (!isInstaller && pid > 0 && !m_runningGames.contains(infoHash)) {
                m_runningGames.insert(infoHash, pid);
                m_gameStartMs.insert(infoHash, nowMs);
                emit gamesChanged();   // "playing now"
            }
            emit toast(isInstaller ? tr_("hub_game_installing") : tr_("hub_game_launch"),
                       QFileInfo(exe).completeBaseName());
            return;
        }
        emit toast(tr_("hub_launch_failed"), QFileInfo(exe).fileName());
    }
    // nothing runnable (or the exe wouldn't start) → open the folder so the
    // user can run it by hand or set the right executable
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const TorrentInfo info = m_session->torrentAt(row);
    revealTorrentRoot(info.savePath, info.name);
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

    if (m_runningGames.isEmpty()) return;
    bool changed = false;
    for (const QString &hash : m_runningGames.keys()) {
        if (pidAlive(m_runningGames.value(hash))) continue;
        const qint64 secs = (QDateTime::currentMSecsSinceEpoch() - m_gameStartMs.value(hash)) / 1000;
        if (secs > 30) {                       // ignore quick bounces / launchers handing off
            QSettings s;
            s.setValue(QStringLiteral("gameSeconds/") + hash,
                       s.value(QStringLiteral("gameSeconds/") + hash, 0).toLongLong() + secs);
        }
        m_runningGames.remove(hash);
        m_gameStartMs.remove(hash);
        changed = true;
    }
    if (changed) emit gamesChanged();
}

// ---- Game install orchestrator (the "Steam pirata" one-click chain) --------

int QmlSessionBridge::gameInstallState(const QString &infoHash, bool completed) const
{
    const QString exe = gameExe(infoHash);
    GameInstall::DeriveIn in;
    in.running = m_runningGames.contains(infoHash);
    in.overlay = m_gameInstallState.value(infoHash, -1);
    in.hasExePath = !exe.isEmpty();
    in.exeExists = in.hasExePath && QFileInfo::exists(exe);
    in.completed = completed;
    // Do not mutate QSettings from this getter — QML bindings re-enter it. Stale
    // paths are cleared from pollRunningGames / a deferred slot instead.
    return GameInstall::derive(in);
}
