// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The nav bar's contextual chip: "Continue Bleach S01E02" with how far you
// got, or, away from Downloads, the download in flight.
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

    // On Downloads only what you can continue: a live download there is
    // already on screen, bigger, and the chip would just repeat it.
    visible: car.dlList.length > 0 && bar.showDownloadChip
             && (bar.currentIndex !== 0 || car.slotResume)
    Layout.alignment: Qt.AlignVCenter
    Layout.preferredHeight: 36
    Layout.preferredWidth: chipContent.implicitWidth + 16
    radius: Theme.rad(9)
    color: chipHover.hovered ? Theme.elev : Theme.panel
    border.color: Theme.hairSoft
    border.width: 1
    Behavior on color { ColorAnimation { duration: Theme.durFast } }
    HoverHandler { id: chipHover }

    readonly property var item: car.dlItem
    readonly property string lead: !item ? ""
        : car.slotResume ? (i18n.language, i18n.t(item.kind === "game" ? "nav_chip_play" : "nav_chip_continue")).arg(item.title || "")
        : (item.title || "")
    readonly property string tail: !item ? ""
        : car.slotResume ? (item.code || item.metric || "")
        : item.paused === true ? (i18n.language, i18n.t("state_paused"))
        : car.slotSeed ? ("↑ " + (item.upSpeed || ""))
        : ("↓ " + (item.downSpeed || ""))

    RowLayout {
        id: chipContent
        x: 4
        anchors.verticalCenter: parent.verticalCenter
        spacing: 10

        PosterThumb {
            Layout.preferredWidth: 22; Layout.preferredHeight: 30
            posterUrl: root.item ? (root.item.poster || "") : ""
            label: root.item ? (root.item.title || "") : ""
        }
        ColumnLayout {
            visible: !root.bar.tightChip
            Layout.preferredWidth: 150
            Layout.alignment: Qt.AlignVCenter
            spacing: 4
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text {
                    Layout.fillWidth: true
                    text: root.lead
                    color: Theme.t1
                    font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Text {
                    text: root.tail
                    color: Theme.t4
                    font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 2
                radius: Theme.rad(1)
                color: Theme.track
                visible: !root.car.slotResume || (root.item && root.item.kind !== "game")
                Rectangle {
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: parent.width * Math.min(1, root.item ? (root.item.progress || 0) : 0)
                    radius: Theme.rad(1)
                    color: Theme.down
                }
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (!root.item) return
            if (root.car.slotResume) {
                if (root.item.kind === "game") session.launchGame(root.item.infoHash)
                else session.playByHash(root.item.infoHash)
            } else {
                root.bar.selectTorrent(root.item.infoHash); root.bar.pageRequested(0)
            }
        }
    }
    ToolTip.visible: chipHover.hovered
    ToolTip.text: (i18n.language, i18n.t(root.car.slotResume ? "nav_continue"
                  : (root.car.slotSeed ? "nav_seeding" : "nav_downloading")))
    ToolTip.delay: 400
}
