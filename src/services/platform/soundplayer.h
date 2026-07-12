// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

// A short, synthesized completion chime — not a licensed audio asset, and not
// QApplication::beep() (a user reported that on Windows it plays as the
// system's generic alert sound, indistinguishable from an error beep).
namespace SoundPlayer {

void playCompletionChime();

} // namespace SoundPlayer

#endif // SOUNDPLAYER_H
