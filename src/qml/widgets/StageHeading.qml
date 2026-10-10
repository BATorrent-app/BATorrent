// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The top of a stage panel: its name with a control beside it, then one line
// saying how the list is ordered and where the rest of it is.
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: head
    property string title
    property string note
    property string linkText
    default property alias controls: controlRow.data
    signal linkClicked()

    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.bottomMargin: 18
        spacing: 16
        Text {
            Layout.fillWidth: true
            text: head.title
            color: Theme.stageT1
            font.pixelSize: 20
            font.weight: Font.DemiBold
            font.letterSpacing: -0.2
            font.family: Theme.fontSans
        }
        Row { id: controlRow; spacing: 14 }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.bottomMargin: 14
        spacing: 14
        Row { id: noteLead; spacing: 14; visible: children.length > 0 }
        Text {
            Layout.fillWidth: true
            text: head.note
            color: Theme.stageT4
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
            elide: Text.ElideRight
        }
        Row { id: noteControls; spacing: 14 }
        Text {
            visible: head.linkText.length > 0
            text: head.linkText
            color: linkMa.containsMouse ? Theme.stageT1 : Theme.stageT2
            font.pixelSize: 12
            font.family: Theme.fontSans
            MouseArea {
                id: linkMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: head.linkClicked()
            }
        }
    }
    property alias noteControls: noteControls.data
    property alias noteLead: noteLead.data
}
