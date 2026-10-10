// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One episode beside a series' art. The still is where you play what is
// already here; the row opens the episode's sources; the circle takes the
// release the line underneath names.
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: row
    property int number
    property string title
    property string still
    property string subLine
    property color subColor: Theme.stageMuted
    property string slowText
    property bool offered: false
    property bool playable: false
    property bool canDownload: false
    property real watchedPct: 0
    property bool watched: false
    signal openRequested()
    signal playRequested()
    signal downloadRequested()

    implicitHeight: 106
    radius: 6
    color: ma.containsMouse && row.offered ? Theme.stageHover : "transparent"
    Behavior on color { ColorAnimation { duration: Theme.durFast } }
    opacity: row.offered || row.playable ? 1 : 0.55

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: row.offered ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (row.offered) row.openRequested()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 16

        Text {
            Layout.preferredWidth: 24
            horizontalAlignment: Text.AlignHCenter
            text: row.number
            color: row.offered || row.playable ? Theme.stageDim : Qt.rgba(1, 1, 1, 0.2)
            font.pixelSize: 22
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }

        Rectangle {
            Layout.preferredWidth: 152
            Layout.preferredHeight: 86
            radius: 4
            color: Theme.stagePanel
            clip: true
            Image {
                id: img
                anchors.fill: parent
                source: row.still
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                opacity: status !== Image.Ready ? 0
                         : (!row.offered && !row.playable ? 0.28 : (row.watched ? 0.6 : 1))
                Behavior on opacity { NumberAnimation { duration: Theme.durSlow } }
            }
            Rectangle {
                anchors.centerIn: parent
                visible: row.playable && ma.containsMouse
                width: 34; height: 34; radius: 17
                color: Qt.rgba(0, 0, 0, 0.35)
                border.width: 1.5
                border.color: Qt.rgba(1, 1, 1, 0.85)
                IconImg {
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: 1
                    src: "qrc:/icons/play.svg"; tint: "white"; s: 12
                }
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: row.playRequested()
                }
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 3
                visible: row.watchedPct > 0 || row.watched
                color: Qt.rgba(1, 1, 1, 0.25)
                Rectangle {
                    height: parent.height
                    width: parent.width * (row.watched ? 1 : row.watchedPct)
                    color: Theme.accent
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                Layout.fillWidth: true
                text: row.title
                color: !row.offered && !row.playable ? Theme.stageT4
                       : (row.watched ? Theme.stageT2 : Theme.stageT1)
                font.pixelSize: 14
                font.weight: Font.Medium
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Row {
                spacing: 8
                Text {
                    text: row.subLine
                    color: row.subColor
                    font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                }
                Text {
                    visible: row.slowText.length > 0
                    text: "· " + row.slowText
                    color: Theme.warn
                    font.pixelSize: 12; font.family: Theme.fontSans
                }
            }
        }

        Item {
            Layout.preferredWidth: 32
            Layout.preferredHeight: 32
            StageRoundButton {
                anchors.centerIn: parent
                size: 32
                icon: "qrc:/icons/download.svg"
                visible: row.canDownload
                opacity: ma.containsMouse ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
                onClicked: row.downloadRequested()
            }
        }
    }
}
