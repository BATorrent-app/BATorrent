// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/integrations/gameinstall.h"

namespace GameInstall {

bool shouldClearStaleExe(const DeriveIn &in)
{
    return in.overlay < 0 && !in.running && in.hasExePath && !in.exeExists;
}

int derive(const DeriveIn &in)
{
    if (in.running) return Playing;
    if (in.overlay >= 0) return in.overlay;
    if (in.hasExePath && in.exeExists) return Ready;
    // Stale path: never report Ready — ReadyToInstall/Downloading only.
    if (!in.completed) return Downloading;
    return ReadyToInstall;
}

PendingAction nextPendingAction(const PendingIn &in)
{
    if (in.torrentMissing) {
        if (in.ageSec > in.missingTimeoutSec) return PendingAction::FailTimeout;
        return PendingAction::Wait;
    }

    if (in.state == Ready) return PendingAction::LaunchAndFinish;
    if (in.state == Playing) return PendingAction::FinishOnly;
    if (in.state == NeedsSetup) return PendingAction::FailNeedSetup;
    if (in.state == Failed) return PendingAction::FailGeneric;

    // Only time out the *install* half. Large game torrents routinely exceed
    // 30 minutes; the overlay Cancel is the escape hatch while downloading.
    if (in.downloadDone && in.ageSec > in.overallTimeoutSec)
        return PendingAction::FailTimeout;

    if (in.downloadDone && !in.installAlreadyKicked
        && in.state != Extracting && in.state != Installing)
        return PendingAction::KickInstall;

    if (in.state == Extracting || in.state == Installing)
        return PendingAction::EmitInstallProgress;
    return PendingAction::EmitDownloadProgress;
}

} // namespace GameInstall
