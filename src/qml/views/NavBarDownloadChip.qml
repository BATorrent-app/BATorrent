// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Condensed download carousel chip for the top nav bar.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"
import "../widgets"

Rectangle {
    id: root
    required property var bar
    required property var car
    property alias contentOpacity: chipContent.opacity
    readonly property bool chipHovered: chipHover.hovered

    visible: car.dlList.length > 0 && bar.showDownloadChip
    Layout.alignment: Qt.AlignVCenter
    Layout.preferredHeight: 40
    Layout.preferredWidth: chipContent.implicitWidth + 20
    radius: 9
    color: chipHover.hovered ? Theme.hover : "transparent"
    Behavior on color { ColorAnimation { duration: 130 } }
    HoverHandler { id: chipHover }

    RowLayout {
        id: chipContent
        anchors.centerIn: parent
        spacing: 10

        PosterThumb {
            Layout.preferredWidth: 24; Layout.preferredHeight: 32
            posterUrl: root.car.dlItem ? (root.car.dlItem.poster || "") : ""
            label: root.car.dlItem ? (root.car.dlItem.title || "") : ""
        }
        ColumnLayout {
            visible: !root.bar.tightChip
            Layout.alignment: Qt.AlignVCenter
            spacing: 3
            Text {
                Layout.maximumWidth: 130
                text: root.car.dlItem ? (root.car.dlItem.title || "") : ""
                color: Theme.t2
                font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            RowLayout {
                spacing: 6
                IconImg {
                    visible: root.car.dlItem && (root.car.slotResume || root.car.dlItem.paused === true)
                    src: root.car.slotResume ? "qrc:/icons/play.svg" : "qrc:/icons/pause.svg"
                    tint: Theme.accent; s: 9
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: !root.car.dlItem ? ""
                          : root.car.slotResume ? i18n.t("hub_resume")
                          : (root.car.dlItem.paused === true) ? i18n.t("state_paused")
                          : root.car.slotSeed ? ("↑ " + (root.car.dlItem.upSpeed || ""))
                          : ("↓ " + (root.car.dlItem.downSpeed || ""))
                    color: Theme.accent
                    font.pixelSize: 10; font.family: Theme.fontSans; font.features: Theme.tnum
                }
                Rectangle {
                    visible: !root.car.slotResume
                    Layout.preferredWidth: 46; Layout.preferredHeight: 2
                    Layout.alignment: Qt.AlignVCenter
                    radius: 1; color: Theme.track
                    Rectangle {
                        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                        width: parent.width * Math.min(1, root.car.dlItem ? (root.car.dlItem.progress || 0) : 0)
                        radius: 1; color: Theme.accent
                    }
                }
            }
        }
        Text {
            visible: root.car.dlList.length > 1
            text: (root.car.dlShown + 1) + "/" + root.car.dlList.length
            color: Theme.t4
            font.pixelSize: 10; font.family: Theme.fontSans; font.features: Theme.tnum
        }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (!root.car.dlItem) return
            if (root.car.slotResume) {
                if (root.car.dlItem.kind === "game") session.launchGame(root.car.dlItem.infoHash)
                else session.playByHash(root.car.dlItem.infoHash)
            } else {
                root.bar.selectTorrent(root.car.dlItem.infoHash); root.bar.pageRequested(0)
            }
        }
    }
    ToolTip.visible: chipHover.hovered
    ToolTip.text: (i18n.language, i18n.t(root.car.slotResume ? "nav_continue"
                  : (root.car.slotSeed ? "nav_seeding" : "nav_downloading")))
    ToolTip.delay: 400
}
