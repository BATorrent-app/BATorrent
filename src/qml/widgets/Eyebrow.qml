// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: bat-dialog.css .eyebrow — small UPPERCASE caption.
// 9px / weight 700 / letter-spacing 1.8 / Theme.t4 / UPPERCASE.
// .eyebrow.red → color Theme.accent (mais saturado que t4).
import QtQuick
import "../theme"

Text {
    property bool red: false
    property var uiPalette: Theme
    color: red ? uiPalette.accent : uiPalette.t4
    font.pixelSize: 9
    font.weight: Font.Bold
    font.letterSpacing: 1.8
    font.family: uiPalette.fontSans
    font.capitalization: Font.AllUppercase
}
