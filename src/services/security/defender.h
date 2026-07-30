// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef DEFENDER_H
#define DEFENDER_H

#include <QString>
#include <functional>

namespace Defender {

// Windows-only: queue an elevated (UAC) PowerShell call to exclude a folder.
// Never blocks the GUI thread. Returns true if the request was queued.
bool addExclusion(const QString &path);

// Optional completion callback (may run on the GUI thread via QProcess signals).
void addExclusionAsync(const QString &path, std::function<void(bool ok)> done);

}

#endif
