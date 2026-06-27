// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Shared context-menu chrome: panel bg, hairline border, radius 8.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Menu {
    modal: true
    implicitWidth: 220
    topPadding: 6
    bottomPadding: 6
    background: Rectangle {
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        radius: 11
    }
    delegate: BatMenuItem {}
}
