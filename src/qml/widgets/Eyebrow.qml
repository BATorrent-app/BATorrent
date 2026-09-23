// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Small uppercase caption above a section.
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
