// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MEDIAREFRESH_H
#define BATORRENT_MEDIAREFRESH_H

class QObject;
class IEngine;

// On torrentFinished, poke Plex / Jellyfin library refresh when enabled.
namespace MediaRefresh {

void install(QObject *context, IEngine *eng);

} // namespace MediaRefresh

#endif
