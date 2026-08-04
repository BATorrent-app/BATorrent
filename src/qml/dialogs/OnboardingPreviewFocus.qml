// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "../theme"

// Temporary callout over the region the current wizard step is about.
// White, not accent: a red box read as permanent chrome (same ink as the
// brand and selected tiles) instead of a transient "look here".
//
// Parenting to the target (not mapToItem) — layout can settle after the first
// map, and a stale point left the callout on the wrong tile ~1 in 5 times.
Item {
    id: focus
    required property Item targetItem

    parent: targetItem
    anchors.fill: parent
    z: 20

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(1, 1, 1, 0.05)
        border.width: 2
        border.color: Qt.rgba(1, 1, 1, 0.72)
        radius: 4
    }
}
