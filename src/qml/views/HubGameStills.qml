// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "../widgets"
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
                RoundedImage {
                    anchors.fill: parent
                    radius: Theme.rad(4)
                    sourceWidth: 640
                    sources: [still.modelData]
                    imageScale: ma.containsMouse ? 1.04 : 1
                }
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
