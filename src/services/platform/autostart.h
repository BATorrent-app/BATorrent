// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>
#include <QStringList>

// Start BATorrent when the user logs in. Registry Run key on Windows, a
// LaunchAgent on macOS, an autostart .desktop on Linux.
//
// The entry always carries kAutostartFlag, which is the point: "start minimized
// to tray" should hide the window when the *system* launched us, not when the
// user double-clicked the icon. Without a way to tell those apart, that setting
// made a deliberate launch look like the app had failed to open.
namespace Autostart {

// Passed to the executable by the entry we write; never typed by a user.
inline constexpr char kAutostartFlag[] = "--autostart";

bool isSupported();
bool isEnabled();
bool setEnabled(bool on);

// True when this process was started by the entry above. Pure, so the launch
// rule can be tested without touching the registry or the filesystem.
bool launchedBySystem(const QStringList &args);

} // namespace Autostart
