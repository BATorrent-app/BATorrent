// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One-click recent save destinations with their free space — the escape hatch
// when the default disk is full and the right disk is one chip away. Fed by
// session.favoriteSavePaths() (MRU, learned from every add).
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Flow {
    id: root
    property string current: ""          // highlights the chip matching the field
    property var favs: []
    signal picked(string path)
    function reload() { favs = (typeof session !== "undefined") ? session.favoriteSavePaths() : [] }

    spacing: 6
    visible: favs.length > 0

    Repeater {
        model: root.favs
        Rectangle {
            required property var modelData
            readonly property bool cur: root.current === modelData.path
            implicitWidth: favTxt.implicitWidth + 18; implicitHeight: 24
            radius: 12
            color: favMa.containsMouse && !cur ? Theme.hover : Theme.field
            border.color: cur ? Theme.accent : Theme.hair; border.width: 1
            Behavior on border.color { ColorAnimation { duration: 100 } }
            Text {
                id: favTxt; anchors.centerIn: parent
                text: modelData.label + (modelData.free.length > 0 ? " · " + modelData.free : "")
                color: parent.cur ? Theme.t1 : Theme.t2
                font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            MouseArea {
                id: favMa; anchors.fill: parent
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: root.picked(modelData.path)
            }
            ToolTip.visible: favMa.containsMouse
            ToolTip.text: modelData.path
            ToolTip.delay: 400
        }
    }
}
