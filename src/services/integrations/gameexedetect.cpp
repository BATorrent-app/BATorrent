// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/gameexedetect.h"

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
        QStringLiteral("python"), QStringLiteral("benchmark"), QStringLiteral("config"),
        QStringLiteral("settings"), QStringLiteral("cleanup") };
    static const QStringList installerHints = {
        QStringLiteral("setup"), QStringLiteral("install") };

    for (const auto &s : skip) {
        if (c.fileNameLower.contains(s)) {
            r.skip = true;
            return r;
        }
    }
    for (const auto &s : installerHints) {
        if (c.fileNameLower.contains(s)) {
            r.installer = true;
            return r;
        }
    }

    const int depth = c.relativePath.count(QLatin1Char('/'));
    qint64 score = c.sizeBytes;
    if (depth <= 1)
        score += qint64(800) * 1024 * 1024;
    if (c.fileNameLower.contains(QLatin1String("launcher")))
        score -= qint64(300) * 1024 * 1024;
    r.score = score;
    return r;
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
