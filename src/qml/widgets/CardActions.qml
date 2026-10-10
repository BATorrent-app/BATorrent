// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The hover actions over a title's art: Get & Watch in the middle, My List in
// the corner. Fills the art it sits on; the card says when it is hovered.
import QtQuick
import "../theme"

Item {
    id: acts
    property bool hovered: false
    property bool watchlistEnabled: false
    property bool saved: false
    property int playSize: 46
    property bool showPlay: true
    signal getWatch()
    signal watchlistToggle()

    readonly property bool busy: pbMa.containsMouse || wlMa.containsMouse

    // dark glass disc; red only as the hover accent (ring + glyph), never a
    // filled surface: same language as the grid tiles
    Rectangle {
        visible: acts.showPlay && (acts.hovered || pbMa.containsMouse)
        anchors.centerIn: parent
        width: acts.playSize; height: acts.playSize; radius: acts.playSize / 2
        color: "#cc101014"
        border.color: pbMa.containsMouse ? Theme.accent : Qt.rgba(1, 1, 1, 0.25)
        border.width: 1
        scale: pbMa.containsMouse ? 1.08 : 1.0
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
        IconImg {
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: 1
            src: "qrc:/icons/play.svg"
            tint: pbMa.containsMouse ? Theme.accent : "#ffffff"
            s: Math.round(acts.playSize * 0.39)
        }
        MouseArea {
            id: pbMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: acts.getWatch()
        }
    }

    Rectangle {
        visible: acts.watchlistEnabled && (acts.hovered || wlMa.containsMouse || acts.saved)
        x: 6; y: 6
        width: 26; height: 26; radius: 13
        color: wlMa.containsMouse ? "#cc000000" : "#99000000"
        IconImg {
            anchors.centerIn: parent
            src: "qrc:/icons/heart.svg"
            tint: acts.saved ? Theme.accent : "#ffffff"
            s: 14
        }
        MouseArea {
            id: wlMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: acts.watchlistToggle()
        }
    }
}
