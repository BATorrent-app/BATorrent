// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIACHILD_H
#define BATORRENT_MEDIACHILD_H

// Headless --media branch: the isolated decoder (internal/ISOLATED_DECODER_PLAN.md).
// Must run before QApplication so the child never becomes a GUI app.
namespace MediaChild {

bool tryRun(int argc, char *argv[], int *exitCode);

} // namespace MediaChild

#endif
