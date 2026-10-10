// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One episode on the series screen: thumbnail, number and title, synopsis, and
// whether it is actually downloaded. An episode we do not hold is drawn dim
// and does not offer Play — it is listed so a gap in the middle of a season is
// visible, not so it can be clicked into a dead end.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Rectangle {
    id: row
    property var ep
    signal playRequested()

    implicitHeight: 96
    radius: 10
    color: rowMa.containsMouse && row.ep.have ? Theme.hover : "transparent"
    Behavior on color { ColorAnimation { duration: Theme.durFast } }
    opacity: row.ep.have ? 1 : 0.45

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 14

        Rectangle {
            Layout.preferredWidth: 132
            Layout.preferredHeight: 74
            radius: 7
            color: Theme.track
            clip: true
            Image {
                id: still
                anchors.fill: parent
                source: row.ep.still || ""
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                cache: true
                visible: status === Image.Ready
            }
            IconImg {
                anchors.centerIn: parent
                visible: still.status !== Image.Ready
                src: "qrc:/icons/play.svg"
                tint: Theme.t4
                s: 18
            }
            Rectangle {
                anchors.centerIn: parent
                visible: row.ep.have && rowMa.containsMouse
                width: 34; height: 34; radius: 17
                color: "#cc101014"
                border.width: 1
                border.color: Theme.accent
                IconImg {
                    anchors.centerIn: parent
                    anchors.horizontalCenterOffset: 1
                    src: "qrc:/icons/play.svg"; tint: "#ffffff"; s: 15
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 3

            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    text: (row.ep.watched ? "✓  " : "")
                          + (i18n.language, i18n.t("hub_ep_n")).arg(row.ep.episode)
                          + (row.ep.title.length > 0 ? ("  ·  " + row.ep.title) : "")
                    Layout.fillWidth: true
                    color: Theme.t1
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Text {
                    visible: row.ep.runtime > 0
                    text: (i18n.language, i18n.t("hub_ep_minutes")).arg(row.ep.runtime)
                    color: Theme.t4
                    font.pixelSize: 11
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: text.length > 0
                text: row.ep.overview || ""
                color: Theme.t3
                font.pixelSize: 12
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
                maximumLineCount: 2
            }
            Text {
                visible: !row.ep.have
                text: (i18n.language, i18n.t("hub_ep_missing"))
                color: Theme.t4
                font.pixelSize: 11
                font.family: Theme.fontSans
            }
        }
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

    MouseArea {
        id: rowMa
        anchors.fill: parent
        hoverEnabled: true
        enabled: row.ep.have
        cursorShape: Qt.PointingHandCursor
        onClicked: row.playRequested()
    }
}
