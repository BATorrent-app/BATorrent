// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QtGlobal>

// Pure helpers for the game install / Get & Install state machine.
// Int codes must stay aligned with QmlSessionBridge::GameInstallState (QML contract).
namespace GameInstall {

constexpr int Downloading    = 0;
constexpr int ReadyToInstall = 1;
constexpr int Extracting     = 2;
constexpr int Installing     = 3;
constexpr int Ready          = 4;
constexpr int Playing        = 5;
constexpr int NeedsSetup     = 6;
constexpr int Failed         = 7;

struct DeriveIn {
    int overlay = -1;       // transient overlay, or -1 if none
    bool running = false;
    bool hasExePath = false;
    bool exeExists = false;
    bool completed = false;
};

// Derived display/install state. Does not mutate settings — caller clears orphans.
int derive(const DeriveIn &in);

// True when settings hold an exe path that no longer exists on disk.
bool shouldClearStaleExe(const DeriveIn &in);

enum class PendingAction {
    Wait,
    EmitInstallProgress,
    EmitDownloadProgress,
    KickInstall,
    LaunchAndFinish,
    FinishOnly,
    FailNeedSetup,
    FailGeneric,
    FailTimeout,
};

struct PendingIn {
    int state = Downloading;
    bool downloadDone = false;
    bool installAlreadyKicked = false;
    bool torrentMissing = false;
    qint64 ageSec = 0;
    qint64 missingTimeoutSec = 180;
    qint64 overallTimeoutSec = 1800;
};

PendingAction nextPendingAction(const PendingIn &in);

} // namespace GameInstall
