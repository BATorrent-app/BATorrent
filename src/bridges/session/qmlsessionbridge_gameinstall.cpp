// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// QmlSessionBridge: game install pipeline (extract → installer → finalize).

#include "bridges/session/qmlsessionbridge.h"
#include "torrent/sessionmanager.h"
#include "services/integrations/gameinstall.h"
#include "services/integrations/gameexedetect.h"
#include "services/integrations/installerprofile.h"
#include "services/metadata/metadataresolver.h"
#include "services/metadata/nameparser.h"
#include "services/security/archiveextractor.h"
#include "services/security/archivescan.h"
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

    // torrentHasArchives reads the file list, so it stays true after extraction.
    // What is on disk decides whether to unpack, not a memory of having done it:
    // the user may have deleted the extracted folder since.
    if (m_session->torrentHasArchives(row)) {
        m_gameInstallState.insert(infoHash, GIS_Extracting);
        emit gamesChanged();
        // A game exe there with no half-done unpack means it is already
        // unpacked, by hand or by a run closed before it could say so. An
        // installer only counts once this session unpacked it: a repack's
        // setup.exe can ship outside its archives.
        const QString folder = gameFolder(infoHash);
        auto *thread = QThread::create([this, infoHash, folder]() {
            bool isInstaller = false;
            const bool halfDone = QFileInfo::exists(ArchiveExtractor::inProgressMarker(folder));
            const QString exe = halfDone ? QString() : GameExeDetect::autodetect(folder, &isInstaller);
            QMetaObject::invokeMethod(this, [this, infoHash, exe, isInstaller]() {
                if (!exe.isEmpty() && (!isInstaller || m_extracted.contains(infoHash))) {
                    m_extracted.insert(infoHash);
                    finalizeInstall(infoHash);
                    return;
                }
                const int r = m_session->torrentIndexByInfoHash(infoHash);
                if (r < 0) {
                    m_gameInstallState.remove(infoHash);
                    emit gamesChanged();
                    return;
                }
                m_extractProgress.insert(infoHash, 0);
                m_session->extractTorrent(r, QString());   // → extractionCompleted → onExtractionCompleted
            });
        });
        connect(thread, &QThread::finished, thread, &QObject::deleteLater);
        thread->start();
        return;
    }
    finalizeInstall(infoHash);
}

// Cracked files are what Defender quarantines first, often mid-extraction,
// leaving a game that will not start. Excluding the folder before unpacking
// avoids that; it is the user's call from the menu, never automatic, and it
// covers only this torrent's own folder.
void QmlSessionBridge::installGameExcluded(const QString &infoHash)
{
#ifdef Q_OS_WIN
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return;
    const TorrentInfo info = m_session->torrentAt(row);
    // The game's own folder, made ahead of the unpack for a lone archive. Never
    // the save folder itself: that would exclude every other download with it.
    const QString folder = gameFolder(infoHash);
    if (QDir::cleanPath(folder) == QDir::cleanPath(info.savePath)) {
        emit toast(tr_("hub_defender_no_folder"), QString());
        return;
    }
    QDir().mkpath(folder);
    const QString native = QDir::toNativeSeparators(QDir::cleanPath(folder));
    QString quoted = native;
    quoted.replace(QLatin1Char('\''), QStringLiteral("''"));   // PowerShell single-quoted literal
    const QString params = QStringLiteral("-NoProfile -NonInteractive -WindowStyle Hidden -Command "
                                          "\"Add-MpPreference -ExclusionPath '%1' -ErrorAction Stop\"").arg(quoted);
    // The UAC prompt and the cmdlet block: wait off the GUI thread.
    auto *thread = QThread::create([this, infoHash, native, params]() {
        SHELLEXECUTEINFOW sei = {};
        sei.cbSize = sizeof(sei);
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;
        sei.lpVerb = L"runas";
        sei.lpFile = L"powershell.exe";
        sei.lpParameters = reinterpret_cast<const wchar_t *>(params.utf16());
        sei.nShow = SW_HIDE;
        DWORD code = 1;
        if (ShellExecuteExW(&sei) && sei.hProcess) {
            WaitForSingleObject(sei.hProcess, 60000);
            GetExitCodeProcess(sei.hProcess, &code);
            CloseHandle(sei.hProcess);
        }
        QMetaObject::invokeMethod(this, [this, infoHash, native, code]() {
            const bool ok = code == 0;
            qInfo().noquote() << "[defender] exclusion for" << native << (ok ? "added" : "not added")
                              << "exit" << code;
            emit toast(ok ? tr_("hub_defender_added") : tr_("hub_defender_not_added"), native);
            installGame(infoHash);   // declined or refused, the install still goes ahead
        });
    });
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
#else
    installGame(infoHash);
#endif
}

void QmlSessionBridge::onExtractionProgress(const QString &infoHash, int percent)
{
    if (m_extractProgress.value(infoHash, -1) == percent) return;
    m_extractProgress.insert(infoHash, percent);
    emit gamesChanged();
}

void QmlSessionBridge::onExtractionCompleted(const QString &infoHash, bool success)
{
    m_extractProgress.remove(infoHash);
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
    // Heavy directory walks belong off the GUI thread: a FitGirl tree on HDD
    // can ghost the Get & Install overlay (Windows "Not Responding").
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
                // Crack copy is also filesystem-heavy: keep on worker next tick if needed;
                // for now do it here but folder is usually small post-detect.
                applyCrackIfPresent(folder, QFileInfo(exe).absolutePath());
                QSettings().setValue(QStringLiteral("gameExe/") + infoHash, exe);
                m_gameInstallState.remove(infoHash);
                emit gamesChanged();
                emit toast(tr_("hub_install_ready"), QFileInfo(exe).completeBaseName());
                return;
            }
            // Installed already (an earlier run, by hand, or a wrong exe saved
            // and since dropped): no reason to put the user through setup again.
            const QString installed = installedGameExe(infoHash, 0);
            if (!installed.isEmpty()) {
                QSettings().setValue(QStringLiteral("gameExe/") + infoHash, installed);
                m_gameInstallState.remove(infoHash);
                emit gamesChanged();
                emit toast(tr_("hub_install_ready"), QFileInfo(installed).completeBaseName());
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
    // Tier B: a silenceable generic installer (NOT a repack; those need the user's
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
        m_installSince.insert(infoHash, QDateTime::currentMSecsSinceEpoch());
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
        // A repack installs wherever the user pointed it, not beside the torrent.
        QString exe = installedGameExe(hash, m_installSince.take(hash));
        if (exe.isEmpty())
            exe = GameExeDetect::autodetect(folder, &inst);
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

QString QmlSessionBridge::installedGameExe(const QString &infoHash, qint64 sinceMs) const
{
    const int row = m_session->torrentIndexByInfoHash(infoHash);
    if (row < 0) return {};
    const QString title = NameParser::parse(m_session->torrentAt(row).name).cleanTitle;
    const QString dir = InstallerProfile::installDirFor(InstallerProfile::uninstallEntries(), sinceMs, title);
    if (dir.isEmpty() || !QFileInfo(dir).isDir()) return {};
    bool inst = false;
    const QString exe = GameExeDetect::autodetect(dir, &inst);
    qInfo().noquote() << "[install]" << title << "is installed in" << dir << "exe" << exe;
    return inst ? QString() : exe;
}

bool QmlSessionBridge::isGameTorrent(int row) const
{
    if (row < 0) return false;

    // File evidence is authoritative once metadata is present, and it overrides
    // the name/resolver guess (which can't tell "The Matrix" the movie from the
    // game). Two rules:
    //   * an executable anywhere ⇒ game. Movies never ship a .exe, and games
    //     routinely bundle cutscene videos with the exe buried in subfolders:
    //     so a stray .mp4 must NOT veto a real game.
    //   * videos but no executable ⇒ movie/series, even when the NAME matches a
    //     game ("Super Mario Galaxy the movie"). Without this a movie like
    //     "The Matrix" wrongly got an Install action.
    //   * an archive whose name reads as a game release counts as an exe: an
    //     OnlineFix torrent is just "Game.Build.123-OFME.rar", the exe inside it.
    bool hasExe = false, hasVideo = false;
    for (const auto &f : m_session->filesAt(row)) {
        QString p = f.path.toLower();
        if (p.endsWith(QStringLiteral(".!bt"))) p.chop(4);   // in-progress: "movie.mkv.!bt"
        if (p.endsWith(QStringLiteral(".exe"))) hasExe = true;
        else if (ArchiveScan::isArchive(p)
                 && NameParser::parse(QFileInfo(f.path).completeBaseName()).contentType == ContentType::Game)
            hasExe = true;
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
