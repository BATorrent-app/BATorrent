// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The "Graph" detail pane: per-torrent speed history (down = accent,
// up = amber) drawn by the shared SpeedGraph, plus legend and empty state.
import QtQuick
import "../theme"

Item {
    id: pane
    property var dl: []
    property var ul: []
    property bool hasData: false

    Item {
        visible: pane.hasData
        anchors.fill: parent
        anchors.margins: Theme.sp5

        Row {
            anchors.right: parent.right; anchors.top: parent.top; spacing: 9
            z: 1
            Text { text: "↓"; color: Theme.accent; font.pixelSize: 11; font.bold: true; font.family: Theme.fontSans }
            Text { text: "↑"; color: Theme.amber;  font.pixelSize: 11; font.bold: true; font.family: Theme.fontSans }
        }
        Rectangle {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: 1; color: Theme.hairSoft
        }
        SpeedGraph {
            anchors.fill: parent
            dl: pane.dl
            ul: pane.ul
        }
    }
    Text {
        anchors.centerIn: parent
        visible: !pane.hasData
        text: (i18n.language, i18n.t("empty_no_selection"))
        color: Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans
    }
}
