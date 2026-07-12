// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The six-tab strip of the detail surfaces (General/Peers/Files/Trackers/
// Pieces/Graph). compact: tighter type, counts hidden, and the row scrolls
// horizontally so long locales never clip in the 340px inspector.
import QtQuick
import "../theme"

Item {
    id: tabs
    property int current: 0
    property bool compact: false
    property var counts: ["", "", "", "", "", ""]
    signal select(int idx)

    readonly property var labels: [
        (i18n.language, i18n.t("detail_general")),
        (i18n.language, i18n.t("detail_peers")),
        (i18n.language, i18n.t("detail_files")),
        (i18n.language, i18n.t("detail_trackers")),
        (i18n.language, i18n.t("detail_pieces")),
        (i18n.language, i18n.t("detail_graph"))
    ]

    Flickable {
        anchors.fill: parent
        contentWidth: strip.implicitWidth
        interactive: contentWidth > width
        clip: true
        Row {
            id: strip
            height: tabs.height
            spacing: tabs.compact ? Theme.sp4 : Theme.sp5
            Repeater {
                model: 6
                delegate: Item {
                    height: strip.height
                    width: tabRow.implicitWidth
                    readonly property bool on: tabs.current === index
                    Row {
                        id: tabRow
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 7
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: tabs.labels[index]
                            color: on ? Theme.t1 : (tabMa.containsMouse ? Theme.t2 : Theme.t3)
                            font.pixelSize: tabs.compact ? 12 : 13
                            font.weight: on ? Font.DemiBold : Font.Medium
                            font.family: Theme.fontSans
                        }
                        Text {
                            visible: !tabs.compact && tabs.counts[index] !== ""
                            anchors.verticalCenter: parent.verticalCenter
                            text: tabs.counts[index]
                            color: Theme.t4
                            font.pixelSize: 11
                            font.family: Theme.fontSans
                            font.features: Theme.tnum
                        }
                    }
                    Rectangle {
                        visible: on
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: 2
                        color: Theme.accent
                    }
                    MouseArea {
                        id: tabMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: tabs.select(index)
                    }
                }
            }
        }
    }
}
