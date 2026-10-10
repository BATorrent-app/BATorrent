// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Effects
import "../theme"
import "../widgets"

Column {
    id: root
    property var stills: []
    signal stillPicked(string url)
    visible: stills.length > 0
    spacing: Theme.sp3

    Text {
        text: (i18n.language, i18n.t("game_media"))
        color: Theme.t1
        font.pixelSize: 17
        font.weight: Font.Bold
        font.family: Theme.fontSans
    }

    ListView {
        id: strip
        width: root.width
        height: 180
        orientation: ListView.Horizontal
        spacing: Theme.sp3
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: root.stills
        delegate: Item {
            id: still
            required property string modelData
            width: 320
            height: strip.height
            Rectangle {
                id: shot
                anchors.fill: parent
                color: Theme.elev
                visible: false
                layer.enabled: true
                Image {
                    anchors.fill: parent
                    source: still.modelData
                    sourceSize.width: 640
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    cache: true
                    scale: ma.containsMouse ? 1.04 : 1
                    Behavior on scale { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
                    opacity: status === Image.Ready ? 1 : 0
                    Behavior on opacity { NumberAnimation { duration: Theme.durSlower } }
                }
            }
            Rectangle { id: shotMask; anchors.fill: parent; radius: 10; color: "white"; visible: false; layer.enabled: true }
            MultiEffect { source: shot; anchors.fill: parent; maskEnabled: true; maskSource: shotMask }
            MouseArea {
                id: ma
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.stillPicked(still.modelData)
            }
        }
    }
}
