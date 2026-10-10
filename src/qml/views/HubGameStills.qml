// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Effects
import "../theme"

Column {
    id: root
    property var stills: []
    signal stillPicked(string url)
    visible: stills.length > 0
    spacing: 16

    Text {
        text: (i18n.language, i18n.t("game_screenshots"))
        color: Theme.t1
        font.pixelSize: 18
        font.weight: Font.DemiBold
        font.family: Theme.fontSans
    }

    Grid {
        id: grid
        width: root.width
        columns: 3
        spacing: 8
        readonly property real tileW: (width - spacing * (columns - 1)) / columns
        Repeater {
            model: root.stills.slice(0, 6)
            delegate: Item {
                id: still
                required property string modelData
                width: grid.tileW
                height: Math.round(grid.tileW * 9 / 16)
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
                Rectangle { id: shotMask; anchors.fill: parent; radius: 4; color: "white"; visible: false; layer.enabled: true }
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
}
