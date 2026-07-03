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
    color: chip.active ? Theme.accent : (cma.containsMouse ? "#33ffffff" : "#1d1d20")
    Text {
        id: ct; anchors.centerIn: parent; text: chip.label
        color: chip.active ? "#ffffff" : Theme.t1
        font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
    }
    MouseArea { id: cma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: chip.clicked() }
}
