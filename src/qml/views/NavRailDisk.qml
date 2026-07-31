// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Multi-volume disk gauges for the expanded nav rail (click → free up space).
import QtQuick
import QtQuick.Layouts
import "../theme"

Column {
    id: root
    required property var rail

    Layout.fillWidth: true
    Layout.leftMargin: 18; Layout.rightMargin: 18
    spacing: 11
    visible: !rail.collapsed && typeof session !== "undefined" && session.diskVolumes.length > 0

    Repeater {
        model: typeof session !== "undefined" ? session.diskVolumes : []
        delegate: Item {
            id: dvItem
            required property var modelData
            width: parent.width
            height: 30
            Text {
                id: dvName
                anchors.left: parent.left; anchors.top: parent.top
                anchors.right: dvFree.left; anchors.rightMargin: 8
                text: modelData.name
                color: dvMa.containsMouse ? Theme.t2 : Theme.t4; elide: Text.ElideRight
                font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0
                font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
            }
            Text {
                id: dvFree
                anchors.right: parent.right; anchors.top: parent.top
                text: (i18n.language, i18n.t("status_free_space")).arg(modelData.free)
                color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                height: 3; radius: 2; color: Theme.track
                Rectangle {
                    height: parent.height; radius: 2
                    width: parent.width * Math.max(0.02, Math.min(1, modelData.usedFraction))
                    color: modelData.usedFraction > 0.95 ? Theme.accent
                         : modelData.usedFraction > 0.85 ? Theme.amber : Theme.t4
                }
            }
            MouseArea {
                id: dvMa
                anchors.fill: parent
                anchors.margins: -4
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.rail.makeRoomRequested()
            }
        }
    }
}
