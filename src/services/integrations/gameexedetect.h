// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>

// Pure-ish game exe heuristics used by the session bridge. Disk walks stay here
// so QmlSessionBridge stays glue; scoring rules are unit-tested via scoreCandidate.
namespace GameExeDetect {

struct Candidate {
    QString fileNameLower;   // basename, lowercased
    QString relativePath;    // path relative to game folder (leading slash ok)
    qint64 sizeBytes = 0;
    // A "<name>_Data" folder sits next to it: Unity's mark of the game's own exe,
    // which is often smaller than the crash handler beside it.
    bool hasUnityData = false;
};

struct ScoreResult {
    qint64 score = -1;
    bool installer = false;
    bool skip = false;
};

ScoreResult scoreCandidate(const Candidate &c);

// A crash handler or an uninstaller: never the game, even when saved as one.
// Older builds saved Unity's crash handler as the exe, and a saved exe
// outranks detection. Narrower than the skip list, which a manual pick of,
// say, a game called "Settings" must still get past.
bool neverTheGame(const QString &fileNameLower);

// True when torrent metadata didn't label a game but the file list looks like one.
bool looksLikeGameFromFiles(bool hasExe, bool hasVideo);

// Manual "completed" OR progress finished: seeding games stay playable.
bool dataComplete(bool completedFlag, float progress);

// Best-guess executable under folder. Sets *isInstaller when returning a setup.exe.
// Returns empty if nothing runnable.
QString autodetect(const QString &folder, bool *isInstaller = nullptr);

} // namespace GameExeDetect
