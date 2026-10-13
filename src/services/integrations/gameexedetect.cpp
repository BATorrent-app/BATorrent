// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/gameexedetect.h"
#include "services/integrations/installerprofile.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <climits>

namespace GameExeDetect {

ScoreResult scoreCandidate(const Candidate &c)
{
    ScoreResult r;
    static const QStringList skip = {
        QStringLiteral("unins"), QStringLiteral("redist"), QStringLiteral("vcredist"),
        QStringLiteral("vc_redist"), QStringLiteral("directx"), QStringLiteral("dxsetup"),
        QStringLiteral("dotnet"), QStringLiteral("dotnetfx"), QStringLiteral("_commonredist"),
        QStringLiteral("prereq"), QStringLiteral("crashreport"), QStringLiteral("crashpad"),
        QStringLiteral("crashhandler"), QStringLiteral("crash_handler"),
        QStringLiteral("python"), QStringLiteral("benchmark"), QStringLiteral("config"),
        QStringLiteral("settings"), QStringLiteral("cleanup"), QStringLiteral("quicksfv"),
        QStringLiteral("language selector"), QStringLiteral("languageselector") };
    static const QStringList installerHints = {
        QStringLiteral("setup"), QStringLiteral("install") };

    for (const auto &s : skip) {
        if (c.fileNameLower.contains(s)) {
            r.skip = true;
            return r;
        }
    }
    if (neverTheGame(c.fileNameLower)) {
        r.skip = true;
        return r;
    }
    for (const auto &s : installerHints) {
        if (c.fileNameLower.contains(s)) {
            r.installer = true;
            return r;
        }
    }

    // Folders between the game folder and the exe: 0 at the root, 1 in the one
    // folder an extracted archive usually makes ("Game/Game.exe").
    QString rel = c.relativePath;
    while (rel.startsWith(QLatin1Char('/')) || rel.startsWith(QLatin1Char('\\'))) rel.remove(0, 1);
    const int depth = rel.count(QLatin1Char('/')) + rel.count(QLatin1Char('\\'));
    // A repack's _Redist / _CommonRedist folder holds DirectX, VC++ and tools
    // like QuickSFV, none of them the game...
    const int lastSep = qMax(rel.lastIndexOf(QLatin1Char('/')), rel.lastIndexOf(QLatin1Char('\\')));
    const QString folders = lastSep > 0 ? rel.left(lastSep).toLower() : QString();
    // ...and a repack's _Cracks folder holds copies of the crack to apply by hand.
    if (folders.contains(QLatin1String("redist")) || folders.contains(QLatin1String("_crack"))) {
        r.skip = true;
        return r;
    }
    qint64 score = c.sizeBytes;
    // The root beats one folder down: Portal 2 keeps portal2.exe at the root
    // and a bigger map compiler in bin\.
    if (depth == 0)
        score += qint64(1000) * 1024 * 1024;
    else if (depth == 1)
        score += qint64(800) * 1024 * 1024;
    if (c.hasUnityData)
        score += qint64(2000) * 1024 * 1024;
    if (c.fileNameLower.contains(QLatin1String("launcher")))
        score -= qint64(300) * 1024 * 1024;
    r.score = score;
    return r;
}

bool neverTheGame(const QString &fileNameLower)
{
    static const QStringList never = {
        QStringLiteral("crashhandler"), QStringLiteral("crash_handler"), QStringLiteral("crashpad"),
        QStringLiteral("crashreport"), QStringLiteral("unins"),
        QStringLiteral("quicksfv"),   // FitGirl's checksum tool, saved as the game by one build
        // Source engine tools in bin\ (Portal 2's vbsp.exe was taken for the game)
        QStringLiteral("vbsp.exe"), QStringLiteral("vvis.exe"), QStringLiteral("vrad.exe"),
        QStringLiteral("hammer.exe"), QStringLiteral("studiomdl.exe"), QStringLiteral("hlmv.exe"),
        QStringLiteral("hlfaceposer.exe") };
    for (const auto &n : never)
        if (fileNameLower.contains(n)) return true;
    return false;
}

bool looksLikeGameFromFiles(bool hasExe, bool hasVideo)
{
    return hasExe && !hasVideo;
}

bool dataComplete(bool completedFlag, float progress)
{
    return completedFlag || progress >= 1.0f;
}

QString autodetect(const QString &folder, bool *isInstaller)
{
    if (isInstaller)
        *isInstaller = false;
    if (folder.isEmpty())
        return {};

    QString bestGame, installer;
    qint64 bestScore = -1;
    int scanned = 0;
    QDirIterator it(folder, {QStringLiteral("*.exe")}, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext() && scanned < 4000) {
        const QString path = it.next();
        ++scanned;
        Candidate c;
        c.fileNameLower = QFileInfo(path).fileName().toLower();
        c.relativePath = path.mid(folder.size());
        c.sizeBytes = QFileInfo(path).size();
        const QFileInfo exeInfo(path);
        c.hasUnityData = QFileInfo(exeInfo.absolutePath() + QLatin1Char('/')
                                   + exeInfo.completeBaseName() + QStringLiteral("_Data")).isDir();
        const ScoreResult r = scoreCandidate(c);
        if (r.skip)
            continue;
        if (r.installer) {
            if (installer.isEmpty())
                installer = path;
            continue;
        }
        if (r.score > bestScore) {
            bestScore = r.score;
            bestGame = path;
        }
    }
    // A repack's folder holds the installer and its payload; the game exists
    // only once that runs, so a stray tool exe beside them is never it.
    if (!installer.isEmpty()) {
        QStringList siblings;
        for (const QFileInfo &fi : QFileInfo(installer).dir().entryInfoList(QDir::Files))
            siblings << fi.fileName();
        if (InstallerProfile::isLikelyRepack(installer, siblings)) {
            if (isInstaller)
                *isInstaller = true;
            return installer;
        }
    }
    if (!bestGame.isEmpty())
        return bestGame;

#if defined(Q_OS_MACOS)
    QString bestApp;
    int bestDepth = INT_MAX;
    QDirIterator ait(folder, {QStringLiteral("*.app")}, QDir::Dirs, QDirIterator::Subdirectories);
    while (ait.hasNext()) {
        const QString path = ait.next();
        const int depth = path.mid(folder.size()).count(QLatin1Char('/'));
        if (depth < bestDepth) {
            bestDepth = depth;
            bestApp = path;
        }
    }
    if (!bestApp.isEmpty())
        return bestApp;
#endif

    if (!installer.isEmpty()) {
        if (isInstaller)
            *isInstaller = true;
        return installer;
    }
    return {};
}

} // namespace GameExeDetect
