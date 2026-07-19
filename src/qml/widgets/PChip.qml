// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Player chip: compact dark pill button (play controls, subtitle panel).
import QtQuick
import "../theme"

Rectangle {
    id: chip
    property string label: ""
    property bool active: false
    signal clicked()
    implicitHeight: 26
    implicitWidth: ct.width + 18
    radius: 7
    // active = accent ring + accent text; the surface stays dark (color is a
    // signal, not a fill)
    color: cma.containsMouse ? "#2bffffff" : "#1d1d20"
    border.color: chip.active ? Theme.accent : "transparent"
    border.width: 1
    scale: cma.pressed ? 0.96 : 1.0
    Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
    Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
    Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
    Text {
        id: ct; anchors.centerIn: parent; text: chip.label
        color: chip.active ? Theme.accent : Theme.t1
        font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
        Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
    }
    MouseArea { id: cma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: chip.clicked() }
}
