// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Rotating disk gauge for the top nav bar (worst/current save volume).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

Item {
    id: root
    required property var bar

    visible: bar.diskShown !== null
    Layout.alignment: Qt.AlignVCenter
    Layout.preferredHeight: 40
    Layout.preferredWidth: bar.tightDisk ? 76 : diskCol.implicitWidth + 24

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
            target: root.bar
            function onDiskIdxChanged() { diskFade.restart() }
        }
        SequentialAnimation {
            id: diskFade
            NumberAnimation { target: diskCol; property: "opacity"; to: 0.25; duration: 110; easing.type: Easing.InCubic }
            NumberAnimation { target: diskCol; property: "opacity"; to: 1.0; duration: 160; easing.type: Easing.OutCubic }
        }
        RowLayout {
            visible: !root.bar.tightDisk
            spacing: 8
            Text {
                Layout.maximumWidth: 110
                text: root.bar.diskShown ? root.bar.diskShown.name : ""
                color: Theme.t4; elide: Text.ElideRight
                font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0
                font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
            }
            Text {
                text: root.bar.diskShown
                      ? (i18n.language, i18n.t("status_free_space")).arg(root.bar.diskShown.free) : ""
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
                readonly property real used: root.bar.diskShown
                    ? Math.max(0.02, Math.min(1, root.bar.diskShown.usedFraction)) : 0
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
        onClicked: root.bar.makeRoomRequested()
    }
    ToolTip.visible: diskMa.containsMouse
    ToolTip.text: {
        var lines = []
        for (var i = 0; i < root.bar.diskVolumes.length; ++i)
            lines.push(root.bar.diskVolumes[i].name + " — "
                       + (i18n.language, i18n.t("status_free_space")).arg(root.bar.diskVolumes[i].free))
        return lines.join("\n")
    }
    ToolTip.delay: 400
}
