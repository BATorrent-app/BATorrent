// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_MACFILEOPEN_H
#define BATORRENT_MACFILEOPEN_H

class QCoreApplication;
class SingleInstance;

namespace MacFileOpen {

// macOS never puts a double-clicked file in argv: Launch Services posts a
// QFileOpenEvent to the running process instead, which is why a .torrent could
// carry our icon, open the app, and then do nothing at all. The same event
// carries magnet: and bittorrent: links handed over by the browser.
//
// Routes through SingleInstance::deliver, which queues until the bridge exists
// — the event often lands before the QML engine is up.
void install(QCoreApplication *app, SingleInstance *single);

} // namespace MacFileOpen

#endif
