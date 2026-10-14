// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The six-tab strip of the detail surfaces (General/Peers/Files/Trackers/
// Pieces/Graph). One indicator slides to the active tab (300ms, none under
// reduced motion). compact: tighter type, counts hidden, the tabs spread over
// the inspector's width, and the row scrolls if a locale still does not fit.
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
            spacing: tabs.compact ? Math.max(Theme.sp3, (tabs.width - tabWidths) / 5) : Theme.sp5
            property real tabWidths: {
                var w = 0
                for (var i = 0; i < tabRepeater.count; ++i) { var it = tabRepeater.itemAt(i); if (it) w += it.width }
                return w
            }
            Repeater {
                id: tabRepeater
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
                            color: on || tabMa.containsMouse ? Theme.t1 : Theme.t3
                            Behavior on color { ColorAnimation { duration: Theme.durFast } }
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
        Rectangle {
            id: indicator
            readonly property Item target: tabRepeater.count > tabs.current ? tabRepeater.itemAt(tabs.current) : null
            property bool ready: false
            Component.onCompleted: Qt.callLater(function () { indicator.ready = true })
            x: target ? strip.x + target.x : 0
            width: target ? target.width : 0
            y: tabs.height - 2
            height: 2
            radius: 1
            color: Theme.accent
            Behavior on x { enabled: indicator.ready && !Theme.reduceMotion
                NumberAnimation { duration: Theme.durSlower; easing.type: Easing.OutQuart } }
            Behavior on width { enabled: indicator.ready && !Theme.reduceMotion
                NumberAnimation { duration: Theme.durSlower; easing.type: Easing.OutQuart } }
        }
    }
}
