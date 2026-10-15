// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The light primary: the one next step where red would mean "downloading" or
// "problem" (notices, the VPN band, Find's empty states, the player's wait).
// stage: on art or video, where the surface is always dark whatever the theme.
import QtQuick
import "../theme"

Rectangle {
    id: btn
    property string text
    property bool stage: false
    property int fontSize: 13
    signal clicked()

    implicitWidth: label.implicitWidth + 2 * Math.round(fontSize * 0.85)
    implicitHeight: Math.round(fontSize * 2.4)
    radius: Theme.rad(6)
    readonly property color ink: stage ? Theme.stageT1 : Theme.t1
    color: ma.containsMouse ? Qt.lighter(ink, 1.06) : ink
    Behavior on color { ColorAnimation { duration: Theme.durFast } }

    Text {
        id: label
        anchors.centerIn: parent
        text: btn.text
        textFormat: Text.PlainText
        color: btn.stage ? Theme.stageBg : Theme.bg
        font.pixelSize: btn.fontSize; font.weight: Font.DemiBold; font.family: Theme.fontSans
    }
    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: btn.clicked() }
}
