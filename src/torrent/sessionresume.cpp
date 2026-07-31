// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrent/sessionresume.h"

#include <QDir>
#include <QFileInfo>

namespace SessionResume {

QString resumeFileName(const QString &hash)
{
    return hash + QStringLiteral(".resume");
}

QString corruptSidecarName(const QString &resumeFileName)
{
    return resumeFileName + QStringLiteral(".corrupt");
}

QString removedHistoryDir(const QString &resumeDataDir)
{
    return QFileInfo(QDir(resumeDataDir), QStringLiteral("../removed")).absoluteFilePath();
}

QString partsSidecarName(const QString &hash)
{
    return QStringLiteral(".") + hash + QStringLiteral(".parts");
}

QString legacyResumeDir(const QString &appDataLocation)
{
    if (appDataLocation.isEmpty())
        return {};
    // Path math only — AppData may not exist yet in tests / first run.
    const QString parent = QFileInfo(QDir::cleanPath(appDataLocation)).path();
    if (parent.isEmpty() || parent == QLatin1String("."))
        return {};
    return QDir(parent).filePath(QStringLiteral("resume"));
}

bool shouldCopyLegacyResumes(bool newDirHasResumes,
                             const QString &legacyResumePath,
                             const QString &newResumePath,
                             bool legacyHasResumeOrTorrentFiles)
{
    if (newDirHasResumes)
        return false;
    if (legacyResumePath.isEmpty() || newResumePath.isEmpty())
        return false;
    if (QDir::cleanPath(legacyResumePath) == QDir::cleanPath(newResumePath))
        return false;
    return legacyHasResumeOrTorrentFiles;
}

CorruptResumeAction corruptResumeAction(bool hasTorrentInfo)
{
    return hasTorrentInfo ? CorruptResumeAction::RecoverRecheck
                          : CorruptResumeAction::Quarantine;
}

SuffixReconcile reconcileIncompleteSuffix(const std::string &effectivePath,
                                          std::int64_t wantSize,
                                          bool wanted,
                                          const DiskFilePresence &disk)
{
    const bool suffixed = effectivePath.size() >= 4
        && effectivePath.compare(effectivePath.size() - 4, 4, kIncompleteSuffix) == 0;
    const std::string base = suffixed
        ? effectivePath.substr(0, effectivePath.size() - 4)
        : effectivePath;

    SuffixReconcile out;
    out.chosen = effectivePath;
    out.wantedFullSize = true;

    if (disk.plainIsFile && disk.plainSize == wantSize)
        out.chosen = base;
    else if (disk.btIsFile && disk.btSize == wantSize)
        out.chosen = base + kIncompleteSuffix;
    else if (wanted)
        out.wantedFullSize = false;

    return out;
}

bool stripIncompleteSuffix(std::string &path)
{
    if (path.size() < 4
        || path.compare(path.size() - 4, 4, kIncompleteSuffix) != 0)
        return false;
    path.resize(path.size() - 4);
    return true;
}

std::string withIncompleteSuffix(const std::string &path)
{
    if (path.size() >= 4
        && path.compare(path.size() - 4, 4, kIncompleteSuffix) == 0)
        return path;
    return path + kIncompleteSuffix;
}

QStringList topLevelTrashNames(const QStringList &filePaths)
{
    QStringList tops;
    for (QString p : filePaths) {
        p.replace(QLatin1Char('\\'), QLatin1Char('/'));
        const int slash = p.indexOf(QLatin1Char('/'));
        const QString top = slash > 0 ? p.left(slash) : p;
        if (!top.isEmpty() && !tops.contains(top))
            tops.append(top);
    }
    return tops;
}

QStringList trashTargetsForRemoval(const QString &savePath,
                                   const QStringList &topLevelNames,
                                   const QString &hash)
{
    QStringList targets;
    const QDir dir(savePath);
    for (const QString &t : topLevelNames) {
        if (t.isEmpty())
            continue;
        QString base = t;
        if (base.endsWith(QLatin1String(kIncompleteSuffix)))
            base.chop(4);
        targets << dir.filePath(base)
                << dir.filePath(base + QLatin1String(kIncompleteSuffix));
    }
    if (!hash.isEmpty())
        targets << dir.filePath(partsSidecarName(hash));
    return targets;
}

bool shouldScheduleFileRemoval(bool deleteFiles, const QStringList &trashTargets)
{
    return deleteFiles && !trashTargets.isEmpty();
}

bool shouldEmitTorrentFinished(bool downloadedThisSession, bool completedAtStartup)
{
    return downloadedThisSession && !completedAtStartup;
}

QStringList removedHistoryHashesToPrune(
    const std::vector<std::pair<qint64, QString>> &sortedOldestFirst,
    int keep)
{
    QStringList prune;
    if (keep < 0)
        keep = 0;
    const int excess = static_cast<int>(sortedOldestFirst.size()) - keep;
    if (excess <= 0)
        return prune;
    prune.reserve(excess);
    for (int i = 0; i < excess; ++i)
        prune.append(sortedOldestFirst[static_cast<size_t>(i)].second);
    return prune;
}

} // namespace SessionResume
