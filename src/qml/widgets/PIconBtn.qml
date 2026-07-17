// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Icon-only player control: circular hover glow, tooltip, accent when active.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Item {
    id: btn
    property string src
    property int s: 18
    property string tip: ""
    property bool active: false
    signal clicked()

    implicitWidth: 32; implicitHeight: 32

    Rectangle {
        anchors.fill: parent; radius: width / 2
        color: bma.containsMouse ? "#1effffff" : "transparent"
        Behavior on color { ColorAnimation { duration: 100 } }
    }
    IconImg {
        anchors.centerIn: parent
        src: btn.src; s: btn.s
        tint: btn.active ? Theme.accent : (bma.containsMouse ? Theme.t1 : Theme.t2)
    }
    // state dot — "on" reads at a glance without filling the control
    Rectangle {
        visible: btn.active
        width: 4; height: 4; radius: 2; color: Theme.accent
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom; anchors.bottomMargin: 2
    }
    MouseArea {
        id: bma; anchors.fill: parent
        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
        onClicked: btn.clicked()
    }
    ToolTip.visible: bma.containsMouse && btn.tip.length > 0
    ToolTip.text: btn.tip
    ToolTip.delay: 500
}
