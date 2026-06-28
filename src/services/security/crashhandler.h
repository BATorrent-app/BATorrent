// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>

// Lightweight in-process crash capture: on a fatal signal/exception it writes a
// backtrace + app version to a crash-<epoch>.log under the crash dir, then lets
// the OS take its default action. This is the dependency-free first step toward
// field crash diagnosis (the MS-Store crashes have no repro on the dev box) — a
// full out-of-process minidump pipeline (Breakpad/Crashpad) + symbol server is
// the robust upgrade, but needs a hosting decision first.
namespace CrashHandler {

void install(const QString &crashDir, const QString &version);

// Most-recent crash report text (for an opt-in "send report" prompt), or "".
QString lastReport(const QString &crashDir);
void clearReports(const QString &crashDir);

} // namespace CrashHandler
