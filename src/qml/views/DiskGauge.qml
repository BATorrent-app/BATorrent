// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Rotating free-space gauge for the save volumes.
//
// Lives in the Downloads action row rather than the nav: free space is a fact
// you act on while managing downloads, and following the user into Find, HUB
// and Settings made it permanent chrome. Owns its own volume rotation so it can
// sit wherever it is useful instead of borrowing state from one parent.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Item {
    id: root

    // Raised when the gauge is clicked — the host owns the free-up-space panel.
    signal makeRoomRequested()

    readonly property var volumes: (typeof session !== "undefined") ? session.diskVolumes : []
    property int idx: 0
    readonly property var shown: volumes.length > 0
        ? volumes[Math.min(idx, volumes.length - 1)] : null
    // Drops the volume name and the free figure, keeping just the bar, when the
    // row runs out of width.
    readonly property bool tight: parent ? parent.width < 1040 : false

    visible: shown !== null
    Layout.alignment: Qt.AlignVCenter
    Layout.preferredHeight: 40
    Layout.preferredWidth: tight ? 76 : diskCol.implicitWidth + 24

    Timer {
        interval: 6000
        repeat: true
        running: root.volumes.length > 1
        onTriggered: root.idx = (root.idx + 1) % root.volumes.length
    }

    Rectangle {
        anchors.fill: parent
        radius: 9
        color: diskMa.containsMouse ? Theme.hover : "transparent"
        Behavior on color { ColorAnimation { duration: 130 } }
    }
    ColumnLayout {
        id: diskCol
        anchors.centerIn: parent
        spacing: 5
        opacity: 1
        Connections {
            target: root
            function onIdxChanged() { diskFade.restart() }
        }
        SequentialAnimation {
            id: diskFade
            NumberAnimation { target: diskCol; property: "opacity"; to: 0.25; duration: 110; easing.type: Easing.InCubic }
            NumberAnimation { target: diskCol; property: "opacity"; to: 1.0; duration: 160; easing.type: Easing.OutCubic }
        }
        RowLayout {
            visible: !root.tight
            spacing: 8
            Text {
                Layout.maximumWidth: 110
                text: root.shown ? root.shown.name : ""
                color: Theme.t4; elide: Text.ElideRight
                font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0
                font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
            }
            Text {
                text: root.shown
                      ? (i18n.language, i18n.t("status_free_space")).arg(root.shown.free) : ""
                color: Theme.t3
                font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.minimumWidth: 56
            Layout.preferredHeight: 3
            radius: 2; color: Theme.track
            Rectangle {
                readonly property real used: root.shown
                    ? Math.max(0.02, Math.min(1, root.shown.usedFraction)) : 0
                anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                width: parent.width * used
                radius: 2
                color: used > 0.95 ? Theme.accent : used > 0.85 ? Theme.amber : Theme.t4
            }
        }
    }
    MouseArea {
        id: diskMa
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.makeRoomRequested()
    }
    ToolTip.visible: diskMa.containsMouse
    ToolTip.text: {
        var lines = []
        for (var i = 0; i < root.volumes.length; ++i)
            lines.push(root.volumes[i].name + " — "
                       + (i18n.language, i18n.t("status_free_space")).arg(root.volumes[i].free))
        return lines.join("\n")
    }
    ToolTip.delay: 400
}
