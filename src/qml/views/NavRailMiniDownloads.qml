// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The collapsed rail's answer to NavRailDownloadSlot. Expanded, the rail shows
// one rotating card with title and speed; collapsed there is no room for that,
// and the space it would have used sat empty between the nav items and the
// bottom group. Here the same transfers become a short column of covers with a
// progress bar, which is the most a 64px column can honestly say.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: root
    required property var rail
    required property var car

    // Deliberately not car.dlList: on Downloads that switches to resume items,
    // which are finished things to continue watching and would show full bars in
    // a strip about transfers. It also goes dark on page 0, and page 0 is where
    // the empty rail is most visible. What is moving, then what is seeding.
    readonly property var list: car.downloadList.length > 0 ? car.downloadList : car.seedingList
    readonly property bool active: rail.collapsed && rail.showDownloadChip && list.length > 0
    readonly property int shownCount: Math.min(4, list.length)

    Layout.fillWidth: true
    Layout.topMargin: active ? 10 : 0
    spacing: 8
    clip: true
    Layout.preferredHeight: active ? implicitHeight : 0
    opacity: active ? 1 : 0
    Behavior on Layout.preferredHeight { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    Behavior on opacity { NumberAnimation { duration: 180 } }

    Rectangle {
        Layout.fillWidth: true; Layout.leftMargin: 18; Layout.rightMargin: 18
        Layout.preferredHeight: 1; color: Theme.hairSoft
    }

    Repeater {
        model: root.active ? root.shownCount : 0
        delegate: Item {
            id: cell
            required property int index
            readonly property var item: root.list[cell.index]
            readonly property real prog: cell.item ? Math.min(1, cell.item.progress || 0) : 0

            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 34
            Layout.preferredHeight: 46

            PosterThumb {
                id: thumb
                anchors.fill: parent
                posterUrl: cell.item ? (cell.item.poster || "") : ""
                label: cell.item ? (cell.item.title || "") : ""
                opacity: cellMa.containsMouse ? 1 : 0.86
                Behavior on opacity { NumberAnimation { duration: 130 } }
            }
            // Same idiom as the grid tile: the bar rides the cover's foot rather
            // than claiming a row of its own, which this column cannot spare.
            Rectangle {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 3; anchors.rightMargin: 3; anchors.bottomMargin: 3
                height: 4
                radius: 2
                color: Qt.rgba(0, 0, 0, 0.78)
                Rectangle {
                    anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                    width: Math.max(height, (parent.width) * cell.prog)
                    height: parent.height
                    radius: 2
                    color: (cell.item && cell.item.paused === true) ? Theme.t3 : Theme.accent
                    Behavior on width { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
                }
            }
            Rectangle {
                anchors.fill: parent
                radius: 5
                color: "transparent"
                border.width: 1
                border.color: cellMa.containsMouse ? Theme.accent : "transparent"
                Behavior on border.color { ColorAnimation { duration: 130 } }
            }
            MouseArea {
                id: cellMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (!cell.item) return
                    root.rail.selectTorrent(cell.item.infoHash)
                    root.rail.pageRequested(0)
                }
            }
            ToolTip.visible: cellMa.containsMouse
            ToolTip.text: cell.item
                ? (cell.item.title || "") + "  ·  " + Math.floor(cell.prog * 100) + "%"
                : ""
            ToolTip.delay: 400
        }
    }

    Text {
        visible: root.active && root.list.length > root.shownCount
        Layout.alignment: Qt.AlignHCenter
        text: "+" + (root.list.length - root.shownCount)
        color: Theme.t4
        font.pixelSize: 10
        font.weight: Font.Bold
        font.family: Theme.fontSans
        font.features: Theme.tnum
    }

    Rectangle {
        Layout.fillWidth: true; Layout.leftMargin: 18; Layout.rightMargin: 18
        Layout.preferredHeight: 1; color: Theme.hairSoft
    }
}
