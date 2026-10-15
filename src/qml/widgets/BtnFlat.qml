// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Flat button for Cancel and other secondary actions.
import QtQuick
import "../theme"

Rectangle {
    id: btn
    property string text
    property string icon: ""               // qrc path opcional (esquerda)
    property bool sm: false
    property bool lg: false                // Settings: sits beside 16px labels
    property bool primary: false
    property bool danger: false            // a destructive action (Delete theme)
    property var uiPalette: Theme
    readonly property color onAccent: uiPalette.inkOnAccent !== undefined ? uiPalette.inkOnAccent : "#ffffff"
    signal clicked()

    implicitWidth: row.implicitWidth + (lg ? 32 : sm ? 22 : 28)
    implicitHeight: lg ? 40 : sm ? 28 : 32
    radius: Theme.rad(lg ? 9 : 7)
    opacity: enabled ? 1 : 0.45

    activeFocusOnTab: true
    Keys.onReturnPressed: btn.clicked()
    Keys.onEnterPressed: btn.clicked()
    Keys.onSpacePressed: btn.clicked()

    scale: ma.pressed ? uiPalette.pressScale : 1
    Behavior on scale { NumberAnimation { duration: uiPalette.durFast; easing.type: Theme.easeOut } }

    // keyboard-focus ring (Tab reaches here; mouse clicks never move focus)
    Rectangle {
        visible: btn.activeFocus
        anchors.fill: parent
        anchors.margins: -3
        radius: btn.radius + 3
        color: "transparent"
        border.color: btn.uiPalette.focusRing
        border.width: btn.uiPalette.focusRingWidth
    }
    color: btn.primary
        ? (ma.containsMouse ? uiPalette.accentDark : uiPalette.accent)
        : (btn.danger && ma.containsMouse ? Qt.rgba(Theme.down.r, Theme.down.g, Theme.down.b, 0.1) : "transparent")
    border.color: btn.primary
        ? "transparent"
        : btn.danger ? Qt.rgba(Theme.downText.r, Theme.downText.g, Theme.downText.b, 0.35)
        : (ma.containsMouse
            ? (uiPalette.isDark ? Qt.rgba(1,1,1,0.20) : Qt.rgba(0,0,0,0.22))
            : uiPalette.hair)
    border.width: btn.primary ? 0 : 1

    Row {
        id: row
        anchors.centerIn: parent
        spacing: 7
        IconImg {
            visible: btn.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            src: btn.icon
            tint: btn.primary ? btn.onAccent : btn.danger ? Theme.downText : (ma.containsMouse ? btn.uiPalette.t1 : btn.uiPalette.t2)
            s: btn.lg ? 16 : 14
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: btn.text
            color: btn.primary ? btn.onAccent : btn.danger ? Theme.downText : (ma.containsMouse ? btn.uiPalette.t1 : btn.uiPalette.t2)
            font.pixelSize: btn.lg ? 14 : btn.sm ? 11 : 12
            font.weight: btn.primary ? Font.DemiBold : Font.Medium
            font.family: btn.uiPalette.fontSans
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
