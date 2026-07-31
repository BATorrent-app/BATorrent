// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_BOOTHEALTH_H
#define BATORRENT_BOOTHEALTH_H

// Startup crash sentinel + libtorrent ABI guards before SessionManager construction.
namespace BootHealth {

enum class CheckResult {
    Continue,
    ContinueSafeMode, // skip auto-update this run
    ExitZero,
    ExitOne,
};

CheckResult checkCrashSentinel();
CheckResult checkLibtorrentAbi();

} // namespace BootHealth

#endif
