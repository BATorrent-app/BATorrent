// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QVector>

// The app's sounds, synthesized: no licensed audio, and no QApplication::beep
// (on Windows that is the generic alert, which reads as an error).
namespace SoundSynth {

enum class Sound {
    Completion,        // a download finished: two soft notes
    Achievement,       // a rising major arpeggio
    AchievementRare,   // the same with a fourth note and a high shimmer
};

// Interleaved float samples in [-1, 1] at `sampleRate`, `channels` 1 or 2.
// Starts and ends on silence, so it never clicks.
QVector<float> render(Sound sound, int sampleRate, int channels);

} // namespace SoundSynth
