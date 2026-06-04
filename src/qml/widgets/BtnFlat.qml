// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: bat-dialog.css .btn — flat/cancel button.
// height 32 (sm:28); padding 0 14 (sm: 0 11); radius 7; font 12/500 (sm: 11.5); bg field; border hair; color t2.
// :hover → border rgba dark/light boost + color t1.
import QtQuick
import "../theme"

Rectangle {
    id: btn
    property string text
    property string icon: ""               // qrc path opcional (esquerda)
    property bool sm: false
    property bool primary: false
    signal clicked()

    implicitWidth: row.implicitWidth + (sm ? 22 : 28)
    implicitHeight: sm ? 28 : 32
    radius: 7
    color: btn.primary
        ? (ma.containsMouse ? Theme.accentDark : Theme.accent)
        : (Theme.isDark ? Qt.rgba(0,0,0,0) : Qt.rgba(0,0,0,0))  // transparent (bg vem do field via parent se preciso)
    border.color: btn.primary
        ? "transparent"
        : (ma.containsMouse
            ? (Theme.isDark ? Qt.rgba(1,1,1,0.20) : Qt.rgba(0,0,0,0.22))
            : Theme.hair)
    border.width: btn.primary ? 0 : 1

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 7
        IconImg {
            visible: btn.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            src: btn.icon
            tint: btn.primary ? "#ffffff" : (ma.containsMouse ? Theme.t1 : Theme.t2)
            s: 14
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: btn.text
            color: btn.primary ? "#ffffff" : (ma.containsMouse ? Theme.t1 : Theme.t2)
            font.pixelSize: btn.sm ? 11 : 12
            font.weight: btn.primary ? Font.DemiBold : Font.Medium
            font.family: Theme.fontSans
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: btn.clicked()
    }
}
