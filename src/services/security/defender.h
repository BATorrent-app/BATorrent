// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef DEFENDER_H
#define DEFENDER_H

#include <QString>

namespace Defender {

// Windows-only: add a folder to Windows Defender's exclusion list via an
// elevated (UAC) PowerShell call. Returns false on non-Windows, in Store
// builds, or if the user declines elevation. Safe to call with any path.
bool addExclusion(const QString &path);

}

#endif
