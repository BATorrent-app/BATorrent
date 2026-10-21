// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_BOOTHEALTH_H
#define BATORRENT_BOOTHEALTH_H

class QWidget;

// Startup crash sentinel + libtorrent ABI guards before SessionManager construction.
namespace BootHealth {

enum class CheckResult {
    Continue,
    ContinueSafeMode, // skip auto-update this run
    ExitZero,
    ExitOne,
};

// `cover`: the splash, hidden before the recovery box opens: on top of it, it
// hid the box's text and buttons, one of which wipes the settings.
CheckResult checkCrashSentinel(QWidget *cover = nullptr);
CheckResult checkLibtorrentAbi();

} // namespace BootHealth

#endif
