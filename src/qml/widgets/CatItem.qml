// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Compact menu item used in the category menus (context menu + filter bar).
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

MenuItem {
    id: cmi
    implicitHeight: 30
    padding: 0
    contentItem: Text {
        leftPadding: 14; rightPadding: 14
        text: cmi.text
        color: cmi.highlighted ? Theme.t1 : Theme.t2
        font.pixelSize: 12; font.family: Theme.fontSans
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle { color: cmi.highlighted ? Theme.hover : "transparent"; radius: 5 }
}
