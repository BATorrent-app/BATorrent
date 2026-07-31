// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge — game install pipeline (extract → installer → finalize).

#include "bridges/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/integrations/gameinstall.h"
#include "services/integrations/gameexedetect.h"
#include "services/integrations/installerprofile.h"
#include "services/metadata/metadataresolver.h"
#include "services/metadata/nameparser.h"
#include "services/platform/logger.h"
#include "services/platform/translator.h"
#include "services/platform/utils.h"

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

void QmlSessionBridge::installGame(const QString &infoHash)
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const TorrentInfo info = m_session->torrentAt(row);
    if (!GameExeDetect::dataComplete(info.completed, info.progress)) return;

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
    // Heavy directory walks belong off the GUI thread — a FitGirl tree on HDD
    // used to ghost the Get & Install overlay (Windows "Not Responding").
    auto *thread = QThread::create([this, infoHash, folder]() {
        bool isInstaller = false;
        const QString exe = GameExeDetect::autodetect(folder, &isInstaller);
        QString iso;
        if (exe.isEmpty()) {
            for (const QFileInfo &fi : QDir(folder).entryInfoList({QStringLiteral("*.iso")}, QDir::Files)) {
                iso = fi.absoluteFilePath();
                break;
            }
        }
        QMetaObject::invokeMethod(this, [this, infoHash, folder, exe, isInstaller, iso]() {
            if (exe.isEmpty()) {
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
                    if (row >= 0) {
                        const TorrentInfo info = m_session->torrentAt(row);
                        revealTorrentRoot(info.savePath, info.name);
                    }
                }
                m_gameInstallState.insert(infoHash, GIS_NeedsSetup);
                emit gamesChanged();
                emit toast(tr_("hub_install_need_exe"), QString());
                return;
            }
            if (!isInstaller) {
                // Crack copy is also filesystem-heavy — keep on worker next tick if needed;
                // for now do it here but folder is usually small post-detect.
                applyCrackIfPresent(folder, QFileInfo(exe).absolutePath());
                QSettings().setValue(QStringLiteral("gameExe/") + infoHash, exe);
                m_gameInstallState.remove(infoHash);
                emit gamesChanged();
                emit toast(tr_("hub_install_ready"), QFileInfo(exe).completeBaseName());
                return;
            }
            runInstaller(infoHash, exe, folder);
        }, Qt::QueuedConnection);
    });
    QObject::connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
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
            const QString exe = ok ? GameExeDetect::autodetect(targetDir, &inst) : QString();
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
        connect(p, &QProcess::errorOccurred, this, [this, p, infoHash](QProcess::ProcessError) {
            p->deleteLater();
            m_gameInstallState.insert(infoHash, GIS_Failed);
            emit toast(tr_("hub_install_need_exe"), QString());
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
        const QString exe = GameExeDetect::autodetect(folder, &inst);
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
        // Get & Install pending for this hash always continues the chain.
        if (m_pendingInstall.contains(infoHash))
            return;   // pollPendingInstall will call installGame when complete
        if (QSettings().value(QStringLiteral("gameAutoInstall"), false).toBool())
            installGame(infoHash);
        else
            emit gameReady(infoHash, name);
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
    const TorrentInfo info = m_session->torrentAt(m_selectedIndex);
    return gameInstallState(hash, GameExeDetect::dataComplete(info.completed, info.progress));
}

void QmlSessionBridge::installSelectedGame()
{
    if (hasSelection()) installGame(m_session->torrentHashAt(m_selectedIndex));
}

void QmlSessionBridge::playSelectedGame()
{
    if (hasSelection()) launchGame(m_session->torrentHashAt(m_selectedIndex));
}

void QmlSessionBridge::installWhenReady(const QString &infoHash, const QString &title)
{
    if (infoHash.isEmpty()) return;
    m_pendingInstall.insert(infoHash, qMakePair(title, QDateTime::currentSecsSinceEpoch()));
    m_installStarted.remove(infoHash);
    emit installProgress(infoHash, 0);
}

void QmlSessionBridge::cancelInstall(const QString &infoHash)
{
    m_pendingInstall.remove(infoHash);
    m_installStarted.remove(infoHash);
}

void QmlSessionBridge::pollPendingInstall()
{
    if (m_pendingInstall.isEmpty()) return;
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    for (const QString &hash : m_pendingInstall.keys()) {
        const auto pending = m_pendingInstall.value(hash);
        const QString title = pending.first;
        const int row = m_session->torrentIndexByInfoHash(hash);

        GameInstall::PendingIn pin;
        pin.ageSec = now - pending.second;
        pin.torrentMissing = (row < 0);
        pin.installAlreadyKicked = m_installStarted.contains(hash);

        double progress = 0.0;
        if (row >= 0) {
            const TorrentInfo info = m_session->torrentAt(row);
            pin.downloadDone = GameExeDetect::dataComplete(info.completed, info.progress);
            pin.state = gameInstallState(hash, pin.downloadDone);
            progress = pin.downloadDone ? 1.0 : double(info.progress);
        }

        using PA = GameInstall::PendingAction;
        const PA action = GameInstall::nextPendingAction(pin);
        switch (action) {
        case PA::Wait:
            break;
        case PA::EmitInstallProgress:
            emit installProgress(hash, 1.0);
            break;
        case PA::EmitDownloadProgress:
            emit installProgress(hash, progress);
            break;
        case PA::KickInstall:
            emit installProgress(hash, 1.0);
            m_installStarted.insert(hash);
            installGame(hash);
            break;
        case PA::LaunchAndFinish:
            m_pendingInstall.remove(hash);
            m_installStarted.remove(hash);
            launchGame(hash);
            emit installFinished(hash, title);
            break;
        case PA::FinishOnly:
            m_pendingInstall.remove(hash);
            m_installStarted.remove(hash);
            emit installFinished(hash, title);
            break;
        case PA::FailNeedSetup:
            m_pendingInstall.remove(hash);
            m_installStarted.remove(hash);
            emit installFailed(title, QStringLiteral("gi_need_setup"));
            break;
        case PA::FailGeneric:
        case PA::FailTimeout:
            m_pendingInstall.remove(hash);
            m_installStarted.remove(hash);
            emit installFailed(title, QStringLiteral("gi_failed"));
            break;
        }
    }
}
