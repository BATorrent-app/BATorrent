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
};

struct ScoreResult {
    qint64 score = -1;
    bool installer = false;
    bool skip = false;
};

ScoreResult scoreCandidate(const Candidate &c);

// True when torrent metadata didn't label a game but the file list looks like one.
bool looksLikeGameFromFiles(bool hasExe, bool hasVideo);

// Manual "completed" OR progress finished — seeding games stay playable.
bool dataComplete(bool completedFlag, float progress);

// Best-guess executable under folder. Sets *isInstaller when returning a setup.exe.
// Returns empty if nothing runnable.
QString autodetect(const QString &folder, bool *isInstaller = nullptr);

} // namespace GameExeDetect
