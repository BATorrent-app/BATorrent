// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QString>

// Classifies a release name by how it delivers the *user's* language: dubbed
// audio, original audio with subtitles, or neither. This is the axis a viewer
// actually chooses on ("I want it dubbed" vs "I hate dubs, give me subs") and
// which a single "language" tag can't express. Relative to userLang: the same
// "Dublado PT-BR" release is Dubbed for a PT user and Original for a FR one.
namespace AudioMode {

enum Mode { Original = 0, Subbed = 1, Dubbed = 2 };

// userLang is a 2-letter UI code ("PT", "ES", ...). Returns Original for EN
// (English content has no dub/sub axis for an English speaker) and for any
// language without a token table.
Mode classify(const QString &releaseName, const QString &userLang);

// Stable string for the QML result map / filter ("dub" | "sub" | "original").
QString key(Mode m);

}
