// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_ENGINECHILD_H
#define BATORRENT_ENGINECHILD_H

// Headless --engine / --engine-selftest branches (internal/ENGINE_SPLIT_PLAN.md).
// Must run before QApplication so the child stays QCoreApplication-only.
namespace EngineChild {

// If argv requests an engine branch, runs it and writes the process exit code.
// Returns true when main should return that code immediately.
bool tryRun(int argc, char *argv[], int *exitCode);

} // namespace EngineChild

#endif
