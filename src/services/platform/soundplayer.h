// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include "services/platform/soundsynth.h"

// Plays the app's synthesized sounds on the default output, fire and forget.
namespace SoundPlayer {

void play(SoundSynth::Sound sound);
inline void playCompletionChime() { play(SoundSynth::Sound::Completion); }

} // namespace SoundPlayer

#endif // SOUNDPLAYER_H
