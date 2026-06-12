// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Shared menu row. Disabled items stay visible but muted; set hideWhenDisabled
// for menus that collapse irrelevant actions instead (e.g. pause vs resume).
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

MenuItem {
    id: mi
    property bool hideWhenDisabled: false
    property int elideMode: Text.ElideRight
    implicitHeight: !enabled && hideWhenDisabled ? 1 : 30
    visible: enabled || !hideWhenDisabled
    padding: 0
    contentItem: Text {
        leftPadding: 14
        rightPadding: 14
        text: (mi.checkable && mi.checked ? "✓ " : "") + mi.text
        color: !mi.enabled ? Theme.t4 : (mi.highlighted ? Theme.t1 : Theme.t2)
        font.pixelSize: 12
        font.family: Theme.fontSans
        verticalAlignment: Text.AlignVCenter
        elide: mi.elideMode
    }
    background: Rectangle {
        color: mi.highlighted ? Theme.hover : "transparent"
        radius: 5
    }
    arrow: Text {
        visible: mi.subMenu
        text: "›"
        color: mi.highlighted ? Theme.t1 : Theme.t4
        font.pixelSize: 16
        font.family: Theme.fontSans
        x: mi.width - width - 12
        y: (mi.height - height) / 2
    }
}
