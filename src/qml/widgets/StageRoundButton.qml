// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The outlined circle a stage row offers its action in.
import QtQuick
import "../theme"

Item {
    id: btn
    property int size: 32
    property string icon
    signal clicked()

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.width: 1.5
        border.color: ma.containsMouse ? Theme.stageT1 : Theme.stageGhost
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }
    IconImg {
        anchors.centerIn: parent
        src: btn.icon
        tint: Theme.stageT1
        s: Math.round(btn.size * 0.4)
    }
    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: btn.clicked()
    }
}
