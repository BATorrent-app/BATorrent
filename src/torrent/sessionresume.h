// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>
#include <QStringList>
#include <QtGlobal>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Pure resume / remove / finish-alert policy peeled from SessionManager so
// characterisation tests can pin behaviour without an lt::session.
namespace SessionResume {

inline constexpr char kIncompleteSuffix[] = ".!bt";
inline constexpr int kRemovedHistoryKeep = 50;

QString resumeFileName(const QString &hash);
QString corruptSidecarName(const QString &resumeFileName);
QString removedHistoryDir(const QString &resumeDataDir);
QString partsSidecarName(const QString &hash);

// Pre-3.0 AppData was <APPDATA>/BATorrent; v3+ is <APPDATA>/BATorrent/BATorrent.
// Legacy resumes sit at parent(appData)/resume.
QString legacyResumeDir(const QString &appDataLocation);

// True when a one-shot copy from legacy → new is warranted (caller already
// flipped the resumeMigrated flag; new dir empty; paths differ; legacy has files).
bool shouldCopyLegacyResumes(bool newDirHasResumes,
                             const QString &legacyResumePath,
                             const QString &newResumePath,
                             bool legacyHasResumeOrTorrentFiles);

enum class CorruptResumeAction {
    Quarantine,     // no torrent_info → rename to .corrupt and skip
    RecoverRecheck, // has ti → clear piece state, add, force_recheck
};

CorruptResumeAction corruptResumeAction(bool hasTorrentInfo);

struct DiskFilePresence {
    bool plainIsFile = false;
    qint64 plainSize = 0;
    bool btIsFile = false;
    qint64 btSize = 0;
};

struct SuffixReconcile {
    std::string chosen;          // relative path after .!bt ↔ plain reconcile
    bool wantedFullSize = true;  // false if this file is wanted but neither variant matches size
};

// Prefer full-size plain over full-size .!bt; else keep effective path.
// Unwanted (priority-0) misses do not spoil "all wanted full size".
SuffixReconcile reconcileIncompleteSuffix(const std::string &effectivePath,
                                          std::int64_t wantSize,
                                          bool wanted,
                                          const DiskFilePresence &disk);

bool stripIncompleteSuffix(std::string &path);
std::string withIncompleteSuffix(const std::string &path);

// Top-level names for trash: first path segment, backslash-normalised.
QStringList topLevelTrashNames(const QStringList &filePaths);

// Both base and base.!bt under savePath, plus the .{hash}.parts sidecar.
QStringList trashTargetsForRemoval(const QString &savePath,
                                   const QStringList &topLevelNames,
                                   const QString &hash);

// Permanent vs trash is a caller choice; this only says whether any targets
// should be scheduled when deleteFiles is requested.
bool shouldScheduleFileRemoval(bool deleteFiles, const QStringList &trashTargets);

// Finish-alert mute: payload==0 (resume verify) OR marked complete at startup.
bool shouldEmitTorrentFinished(bool downloadedThisSession, bool completedAtStartup);

// Oldest-first pairs; when count > keep, return the hashes that must be pruned.
QStringList removedHistoryHashesToPrune(
    const std::vector<std::pair<qint64, QString>> &sortedOldestFirst,
    int keep = kRemovedHistoryKeep);

} // namespace SessionResume
