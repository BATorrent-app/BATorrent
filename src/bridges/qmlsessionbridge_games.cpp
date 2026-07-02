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
#include "services/integrations/installerprofile.h"
#include "services/metadata/metadataresolver.h"
#include "services/platform/logger.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"   // revealTorrentRoot()

#include <QProcess>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QSettings>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QDateTime>
#include <QUrl>
#if defined(Q_OS_WIN)
#include <windows.h>
#else
#include <csignal>
#include <cerrno>
#endif

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
                if (hasExe && !hasVideo) { isGame = true; title = pn.cleanTitle.isEmpty() ? info.name : pn.cleanTitle; }
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
        m["installState"] = gameInstallState(hash, info.completed);
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
}

QString QmlSessionBridge::gameFolder(const QString &infoHash) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    const TorrentInfo info = m_session->torrentAt(row);
    const QString root = info.savePath + QStringLiteral("/") + info.name;
    return QFileInfo(root).isDir() ? root : info.savePath;
}

// Best-guess game executable inside a folder. Skips redists/uninstallers,
// treats setup/installer separately, and prefers the largest .exe near the
// root (where a game's main binary usually lives). A heuristic — the user can
// always override via "Set executable…". Returns "" if nothing runnable.
static QString autodetectGameExe(const QString &folder, bool *isInstaller)
{
    if (isInstaller) *isInstaller = false;
    if (folder.isEmpty()) return {};
    static const QStringList skip = {
        "unins", "redist", "vcredist", "vc_redist", "directx", "dxsetup", "dotnet",
        "dotnetfx", "oalinst", "_commonredist", "prereq", "crashreport", "crashpad",
        "python", "benchmark", "config", "settings", "cleanup" };
    static const QStringList installerHints = { "setup", "install" };

    QString bestGame, installer;
    qint64 bestScore = -1;
    int scanned = 0;
    QDirIterator it(folder, {QStringLiteral("*.exe")}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && scanned < 4000) {
        const QString path = it.next();
        ++scanned;
        const QString lower = QFileInfo(path).fileName().toLower();
        bool skipIt = false;
        for (const auto &s : skip) if (lower.contains(s)) { skipIt = true; break; }
        if (skipIt) continue;
        bool inst = false;
        for (const auto &s : installerHints) if (lower.contains(s)) { inst = true; break; }
        if (inst) { if (installer.isEmpty()) installer = path; continue; }

        const QString rel = path.mid(folder.size());
        const int depth = rel.count(QLatin1Char('/'));
        qint64 score = QFileInfo(path).size();
        if (depth <= 1) score += qint64(800) * 1024 * 1024;            // root binary strongly preferred
        if (lower.contains("launcher")) score -= qint64(300) * 1024 * 1024;
        if (score > bestScore) { bestScore = score; bestGame = path; }
    }
    if (!bestGame.isEmpty()) return bestGame;
    if (!installer.isEmpty()) { if (isInstaller) *isInstaller = true; return installer; }
    return {};
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

void QmlSessionBridge::launchGame(const QString &infoHash)
{
    QString exe = gameExe(infoHash);              // a manual override always wins
    bool isInstaller = false;
    if (exe.isEmpty() || !QFile::exists(exe))
        exe = autodetectGameExe(gameFolder(infoHash), &isInstaller);

    if (!exe.isEmpty() && QFile::exists(exe)) {
        const QString wd = QFileInfo(exe).absolutePath();
        qint64 pid = 0;
        // Detached so the game survives BATorrent closing; the returned pid lets
        // us poll for exit and credit playtime.
        if (QProcess::startDetached(exe, {}, wd, &pid)) {
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
    }
    // nothing runnable found → open the folder so the user can run/set it
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const TorrentInfo info = m_session->torrentAt(row);
    revealTorrentRoot(info.savePath, info.name);
}

// Poll launched games for exit; credit elapsed time to the per-game total.
void QmlSessionBridge::pollRunningGames()
{
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
    if (m_runningGames.contains(infoHash)) return GIS_Playing;
    if (m_gameInstallState.contains(infoHash)) return m_gameInstallState.value(infoHash);
    if (!completed) return GIS_Downloading;
    if (!gameExe(infoHash).isEmpty()) return GIS_Ready;
    return GIS_ReadyToInstall;
}

void QmlSessionBridge::installGame(const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    if (!m_session->torrentAt(row).completed) return;   // nothing to install yet

    const int st = m_gameInstallState.value(infoHash, -1);
    if (st == GIS_Extracting || st == GIS_Installing) return;   // already in flight

    // torrentHasArchives reads the file list, so it stays true even after a prior
    // extraction — m_extracted guards against unpacking twice.
    if (m_session->torrentHasArchives(row) && !m_extracted.contains(infoHash)) {
        m_gameInstallState.insert(infoHash, GIS_Extracting);
        emit gamesChanged();
        m_session->extractTorrent(row, QString());   // → extractionCompleted → onExtractionCompleted
        return;
    }
    finalizeInstall(infoHash);
}

void QmlSessionBridge::onExtractionCompleted(const QString &infoHash, bool success)
{
    if (success) m_extracted.insert(infoHash);
    if (m_gameInstallState.value(infoHash, -1) != GIS_Extracting) return;   // not our install flow
    if (!success) {
        m_gameInstallState.insert(infoHash, GIS_Failed);
        emit gamesChanged();
        emit toast(tr_("hub_install_failed"), QString());
        return;
    }
    finalizeInstall(infoHash);
}

// Scene "copy crack" step: if the extracted tree carries a group/Crack folder,
// copy its files over the game's directory (overwriting the DRM stub).
static void applyCrackIfPresent(const QString &root, const QString &gameDir)
{
    if (root.isEmpty() || gameDir.isEmpty()) return;
    QStringList subdirs;
    for (const QFileInfo &fi : QDir(root).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
        subdirs << fi.fileName();
    const QString crack = InstallerProfile::crackDir(subdirs);
    if (crack.isEmpty()) return;
    const QString crackPath = QDir(root).filePath(crack);
    for (const QFileInfo &fi : QDir(crackPath).entryInfoList(QDir::Files)) {
        const QString dest = QDir(gameDir).filePath(fi.fileName());
        QFile::remove(dest);
        QFile::copy(fi.absoluteFilePath(), dest);
    }
}

void QmlSessionBridge::finalizeInstall(const QString &infoHash)
{
    const QString folder = gameFolder(infoHash);
    bool isInstaller = false;
    const QString exe = autodetectGameExe(folder, &isInstaller);

    if (exe.isEmpty()) {
        // Tier D: a scene release whose game sits inside an .iso → can't run without
        // mounting. On Windows, mount it so the disc appears; everywhere, guide the user.
        QString iso;
        for (const QFileInfo &fi : QDir(folder).entryInfoList({QStringLiteral("*.iso")}, QDir::Files)) {
            iso = fi.absoluteFilePath(); break;
        }
#ifdef Q_OS_WIN
        if (!iso.isEmpty()) {
            QProcess::startDetached(QStringLiteral("powershell"), {
                QStringLiteral("-NoProfile"), QStringLiteral("-Command"),
                QStringLiteral("Mount-DiskImage -ImagePath '%1'")
                    .arg(QString(iso).replace(QLatin1Char('\''), QStringLiteral("''"))) });
        }
#endif
        if (!iso.isEmpty()) {
            const int row = m_session->torrentIndexByInfoHash(infoHash);
            if (row >= 0) { const TorrentInfo info = m_session->torrentAt(row);
                            revealTorrentRoot(info.savePath, info.name); }
        }
        m_gameInstallState.insert(infoHash, GIS_NeedsSetup);
        emit gamesChanged();
        emit toast(tr_("hub_install_need_exe"), QString());
        return;
    }
    if (!isInstaller) {                                   // portable/cracked → register & done
        applyCrackIfPresent(folder, QFileInfo(exe).absolutePath());
        QSettings().setValue(QStringLiteral("gameExe/") + infoHash, exe);
        m_gameInstallState.remove(infoHash);             // → derived Ready
        emit gamesChanged();
        emit toast(tr_("hub_install_ready"), QFileInfo(exe).completeBaseName());
        return;
    }
    runInstaller(infoHash, exe, folder);
}

void QmlSessionBridge::runInstaller(const QString &infoHash, const QString &installerExe,
                                    const QString &folder)
{
    QStringList siblings;
    for (const QFileInfo &fi : QDir(QFileInfo(installerExe).absolutePath()).entryInfoList(QDir::Files))
        siblings << fi.fileName();
    const InstallerProfile::Engine engine = InstallerProfile::detectEngine(installerExe);
    const bool repack = InstallerProfile::isLikelyRepack(installerExe, siblings);
    const QString targetDir = QDir(folder).filePath(QStringLiteral("_BATorrent"));

    m_gameInstallState.insert(infoHash, GIS_Installing);
    emit gamesChanged();

#ifdef Q_OS_WIN
    // Tier B: a silenceable generic installer (NOT a repack — those need the user's
    // component/language choices) → drive it unattended into a known dir we can scan.
    const InstallerProfile::SilentInvocation si =
        InstallerProfile::silentInvocation(engine, installerExe, targetDir);
    if (si.supported && !repack) {
        QDir().mkpath(targetDir);
        auto *p = new QProcess(this);
        const QString program = si.program.isEmpty() ? installerExe : si.program;
        if (!si.rawTail.isEmpty())
            p->setNativeArguments(si.rawTail);   // NSIS /D= must stay last and unquoted
        connect(p, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                [this, p, infoHash, targetDir](int code, QProcess::ExitStatus) {
            p->deleteLater();
            const bool ok = (code == 0 || code == 3010 || code == 1641);  // MSI reboot codes = success
            bool inst = false;
            const QString exe = ok ? autodetectGameExe(targetDir, &inst) : QString();
            if (!exe.isEmpty() && !inst) {
                QSettings().setValue(QStringLiteral("gameExe/") + infoHash, exe);
                m_gameInstallState.remove(infoHash);
                emit toast(tr_("hub_install_ready"), QFileInfo(exe).completeBaseName());
            } else {
                m_gameInstallState.insert(infoHash, GIS_NeedsSetup);
                emit toast(tr_("hub_install_need_exe"), QString());
            }
            emit gamesChanged();
        });
        p->start(program, si.args);
        return;
    }
#else
    Q_UNUSED(engine);
    Q_UNUSED(repack);
#endif

    // Guided (repacks/unknown/non-Windows): open the installer; pollInstallWatch
    // detects when it exits, then registers the produced exe (or asks the user).
    qint64 pid = 0;
    if (QProcess::startDetached(installerExe, {}, QFileInfo(installerExe).absolutePath(), &pid)
        && pid > 0) {
        m_installWatch.insert(infoHash, pid);
        emit toast(tr_("hub_game_installing"), QFileInfo(installerExe).completeBaseName());
    } else {
        const int row = m_session->torrentIndexByInfoHash(infoHash);
        if (row >= 0) {
            const TorrentInfo info = m_session->torrentAt(row);
            revealTorrentRoot(info.savePath, info.name);
        }
        m_gameInstallState.insert(infoHash, GIS_NeedsSetup);
        emit gamesChanged();
    }
}

void QmlSessionBridge::pollInstallWatch()
{
    if (m_installWatch.isEmpty()) return;
    bool changed = false;
    for (const QString &hash : m_installWatch.keys()) {
        if (pidAlive(m_installWatch.value(hash))) continue;   // installer still open
        m_installWatch.remove(hash);
        bool inst = false;
        const QString folder = gameFolder(hash);
        const QString exe = autodetectGameExe(folder, &inst);
        if (!exe.isEmpty() && !inst) {
            applyCrackIfPresent(folder, QFileInfo(exe).absolutePath());
            QSettings().setValue(QStringLiteral("gameExe/") + hash, exe);
            m_gameInstallState.remove(hash);
            emit toast(tr_("hub_install_ready"), QFileInfo(exe).completeBaseName());
        } else {
            m_gameInstallState.insert(hash, GIS_NeedsSetup);   // user points us at it
            emit toast(tr_("hub_install_need_exe"), QString());
        }
        changed = true;
    }
    if (changed) emit gamesChanged();
}

bool QmlSessionBridge::isGameTorrent(int row) const
{
    if (row < 0) return false;

    // File evidence is authoritative once metadata is present, and it overrides
    // the name/resolver guess (which can't tell "The Matrix" the movie from the
    // game). Two rules:
    //   * an executable anywhere ⇒ game. Movies never ship a .exe, and games
    //     routinely bundle cutscene videos with the exe buried in subfolders —
    //     so a stray .mp4 must NOT veto a real game.
    //   * videos but no executable ⇒ movie/series, even when the NAME matches a
    //     game ("Super Mario Galaxy the movie"). Without this a movie like
    //     "The Matrix" wrongly got an Install action.
    bool hasExe = false, hasVideo = false;
    for (const auto &f : m_session->filesAt(row)) {
        QString p = f.path.toLower();
        if (p.endsWith(QStringLiteral(".!bt"))) p.chop(4);   // in-progress: "movie.mkv.!bt"
        if (p.endsWith(QStringLiteral(".exe"))) hasExe = true;
        else if (p.endsWith(QStringLiteral(".mkv")) || p.endsWith(QStringLiteral(".mp4"))
                 || p.endsWith(QStringLiteral(".avi")) || p.endsWith(QStringLiteral(".mov"))
                 || p.endsWith(QStringLiteral(".m4v")) || p.endsWith(QStringLiteral(".webm")))
            hasVideo = true;
    }
    if (hasExe) return true;
    if (hasVideo) return false;

    // No decisive files yet (magnet still resolving metadata): fall back to the
    // name / cached metadata guess.
    const QString hash = m_session->torrentHashAt(row);
    if (!hash.isEmpty() && m_resolver && m_resolver->hasCached(hash)) {
        const auto meta = m_resolver->cached(hash);
        if (meta.valid && meta.contentType == ContentType::Game) return true;
    }
    return NameParser::parse(m_session->torrentAt(row).name).contentType == ContentType::Game;
}

void QmlSessionBridge::onGameTorrentFinished(const QString &name, const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    if (isGameTorrent(row)) {
        if (QSettings().value(QStringLiteral("gameAutoInstall"), false).toBool())
            installGame(infoHash);
        return;
    }
    // a finished movie/series → offer one-click playback (the "completion loop")
    if (m_session->torrentHasVideo(row))
        emit movieReady(infoHash, name);
}

bool QmlSessionBridge::selectedIsGame() const
{
    return hasSelection() && isGameTorrent(m_selectedIndex);
}

int QmlSessionBridge::selectedGameState() const
{
    if (!hasSelection() || !isGameTorrent(m_selectedIndex)) return -1;
    const QString hash = m_session->torrentHashAt(m_selectedIndex);
    return gameInstallState(hash, m_session->torrentAt(m_selectedIndex).completed);
}

void QmlSessionBridge::installSelectedGame()
{
    if (hasSelection()) installGame(m_session->torrentHashAt(m_selectedIndex));
}

void QmlSessionBridge::playSelectedGame()
{
    if (hasSelection()) launchGame(m_session->torrentHashAt(m_selectedIndex));
}
