// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include "services/games/achievementformats.h"

#include <QList>
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

struct Candidate {
    QString path;
    AchievementFormats::Format format;
};

// Every system-wide file an emulator could have written for this appid.
QList<Candidate> systemCandidates(int appId, const Roots &roots);
// The ones that live inside the game's folder (gbe's steam_settings, 3DM,
// ALI213, and gbe's local_save_path when the game sets one).
QList<Candidate> gameDirCandidates(int appId, const QString &gameDir);

// Every unlock any of those files records. The earliest time wins when two
// files name the same achievement.
AchievementFormats::Unlocks readUnlocks(int appId, const QString &gameDir, const Roots &roots);
AchievementFormats::Unlocks readUnlocks(int appId, const QString &gameDir);

// steam_appid.txt, or an AppId= in an emulator's ini, near the game's exe. 0 if none.
int appIdFromGameDir(const QString &gameDir);

} // namespace AchievementFiles
