// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/games/achievementformats.h"

#include <QList>
#include <QStringList>
#include <QString>

// Where a cracked game's emulator keeps its unlocks, and which Steam appid the
// game's own files declare. The places mirror Hydra's scan list.
namespace AchievementFiles {

// The folders the paths hang off. Empty fields are skipped, so a test (or a
// platform without that folder) just leaves them blank.
struct Roots {
    QString appData;        // %APPDATA%
    QString localAppData;   // %LOCALAPPDATA%
    QString publicDocs;     // C:\Users\Public\Documents
    QString programData;    // C:\ProgramData
    QString documents;      // the user's Documents
    QString gseSaves;       // gbe_fork on Linux: $XDG_DATA_HOME/GSE Saves
};
Roots systemRoots();

// On macOS and Linux a cracked game runs inside a Wine prefix, and its
// emulator writes to that prefix's own Windows folders.
// The prefix a path lives in (the folder holding drive_c), "" when none.
QString prefixOf(const QString &path);
// One Roots per user folder inside a prefix's drive_c/users.
QList<Roots> prefixRoots(const QString &prefix);
// The prefixes worth looking in for a game: the one it is installed in, then
// the usual bottle homes (CrossOver, Whisky, ~/.wine, Bottles, Heroic, and
// Proton's compatdata for this appid).
QStringList prefixesFor(int appId, const QString &gameDir);
// Every set of roots to search for a game: this machine's, then its prefixes'.
QList<Roots> rootsFor(int appId, const QString &gameDir);

struct Candidate {
    QString path;
    AchievementFormats::Format format;
};

// Every system-wide file an emulator could have written for this appid.
QList<Candidate> systemCandidates(int appId, const Roots &roots);
// The ones that live inside the game's folder (gbe's steam_settings, 3DM,
// ALI213, and gbe's local_save_path when the game sets one).
QList<Candidate> gameDirCandidates(int appId, const QString &gameDir);

// Both lists together: worked out once and reread while a game runs, so the
// game folder is walked once rather than on every check.
QList<Candidate> candidates(int appId, const QString &gameDir, const Roots &roots);
QList<Candidate> candidates(int appId, const QString &gameDir, const QList<Roots> &roots);

// Every unlock any of those files records. The earliest time wins when two
// files name the same achievement.
AchievementFormats::Unlocks readUnlocks(const QList<Candidate> &files);
AchievementFormats::Unlocks readUnlocks(int appId, const QString &gameDir, const Roots &roots);
AchievementFormats::Unlocks readUnlocks(int appId, const QString &gameDir);

// Progress toward the counting achievements; the furthest reading wins.
AchievementFormats::Progress readProgress(const QList<Candidate> &files);

// Both from a single read of each file.
struct State {
    AchievementFormats::Unlocks unlocked;
    AchievementFormats::Progress progress;
};
State readState(const QList<Candidate> &files);

// What `now` holds that `before` did not, oldest unlock first.
QStringList newlyUnlocked(const AchievementFormats::Unlocks &before, const AchievementFormats::Unlocks &now);

// steam_appid.txt, or an AppId= in an emulator's ini, near the game's exe. 0 if none.
int appIdFromGameDir(const QString &gameDir);

} // namespace AchievementFiles
