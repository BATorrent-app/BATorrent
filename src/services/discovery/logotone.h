// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

class QImage;

namespace LogoTone {

// True when a logo's visible pixels are mostly dark, so it would vanish on the
// app's dark art and has to be drawn light. Alpha weights each pixel: the
// transparent box around the lettering says nothing.
bool isDark(const QImage &logo);

} // namespace LogoTone
