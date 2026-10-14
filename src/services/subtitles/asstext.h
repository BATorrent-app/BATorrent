// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SERVICES_SUBTITLES_ASSTEXT_H
#define SERVICES_SUBTITLES_ASSTEXT_H

#include <QString>

// One line of an ASS/SSA subtitle (what anime ships with) turned into what
// the player's overlay can draw: Qt rich text and where on screen it goes.
// Italic, bold, underline, line breaks and the \an / \a position are kept;
// drawings (\p1), exact positions and the fansub's own fonts and colours
// are dropped, since they need a real ASS renderer (libass) to come out right.
namespace AssText {

struct Line {
    QString html;     // StyledText: <i>, <b>, <u>, <br>, entities
    int align = 2;    // numpad position: 1-3 bottom, 4-6 middle, 7-9 top
    bool top() const { return align >= 7; }
    bool middle() const { return align >= 4 && align <= 6; }
};

// Accepts the bare text or a whole Dialogue event (the nine comma fields
// before the text, as FFmpeg hands it over).
Line toHtml(const QString &raw);

} // namespace AssText

#endif
