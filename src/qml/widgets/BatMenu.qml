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

    // same short fade as the library context menu, plus a slight grow
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 120; easing.type: Theme.easeOut }
            NumberAnimation { property: "scale"; from: Theme.grow(0.96); to: 1.0; duration: 160; easing.type: Theme.easeOut }
        }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 80; easing.type: Theme.easeIn }
    }
}
