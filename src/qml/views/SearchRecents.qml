// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Recent-search chips for the store-build browse landing (no catalog).
import QtQuick
import QtQuick.Layouts
import "../theme"

Flow {
    id: root
    required property var sv

    Layout.fillWidth: false
    Layout.alignment: Qt.AlignHCenter
    Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; Layout.topMargin: 2
    spacing: 8
    visible: sv.browse && !sv.catalogAvailable && sv.recentList.length > 0

    Text {
        text: (i18n.language, i18n.t("search_recent")) + ":"
        color: Theme.t4; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
        height: 24; verticalAlignment: Text.AlignVCenter
    }
    Repeater {
        model: root.sv.recentList
        delegate: Rectangle {
            required property var modelData
            radius: 12
            color: rcMa.containsMouse ? Theme.hover : Theme.field
            border.color: Theme.hair; border.width: 1
            implicitWidth: rcTxt.implicitWidth + 22
            implicitHeight: 24
            Text {
                id: rcTxt; anchors.centerIn: parent; text: modelData
                color: Theme.t2; font.pixelSize: 11; font.family: Theme.fontSans
            }
            MouseArea {
                id: rcMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: root.sv.runQuery(modelData)
            }
        }
    }
}
