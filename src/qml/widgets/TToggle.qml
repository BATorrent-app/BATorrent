// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: bat-dialog.css .toggle — track 38×21 radius 999, knob 15×15.
// off: bg field; knob #8c8884 (dark) / #fff (light); border 1 hair.
// on:  bg accent; knob #fff; border transparent. knob desliza 2 → 19.
import QtQuick
import "../theme"

Rectangle {
    id: tg
    property bool on: false
    signal toggled(bool on)

    implicitWidth: 38
    implicitHeight: 21
    radius: 999
    color: on ? Theme.accent : Theme.field
    border.color: on ? "transparent" : Theme.hair
    border.width: 1

    Rectangle {
        width: 15
        height: 15
        radius: 7.5
        anchors.verticalCenter: parent.verticalCenter
        x: tg.on ? 19 : 2
        color: tg.on
            ? "#ffffff"
            : (Theme.isDark ? "#8c8884" : "#ffffff")
        Behavior on x { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
        Behavior on color { ColorAnimation { duration: 160 } }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: { tg.on = !tg.on; tg.toggled(tg.on) }
    }
}
