// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Contextual rail download carousel (bottom slot). Host owns DownloadCarousel
// + fade animation; this leaf owns the card chrome and hover arrows.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: root
    required property var rail
    required property var car
    property alias contentOpacity: dlContent.opacity
    readonly property bool slotHovered: dlHov.hovered

    Layout.fillWidth: true
    Layout.leftMargin: 18; Layout.rightMargin: 18
    Layout.bottomMargin: 4
    spacing: 9
    clip: true
    Layout.preferredHeight: rail.showDl ? implicitHeight : 0
    opacity: rail.showDl ? 1 : 0
    Behavior on Layout.preferredHeight { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    Behavior on opacity { NumberAnimation { duration: 180 } }

    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hair }

    RowLayout {
        Layout.fillWidth: true
        Text {
            text: (i18n.language, i18n.t(root.car.slotResume ? "nav_continue"
                  : (root.car.slotSeed ? "nav_seeding" : "nav_downloading")))
            color: Theme.t4; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0
            font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
        }
        Item { Layout.fillWidth: true }
        Text {
            visible: root.car.dlList.length > 1
            text: (root.car.dlShown + 1) + "/" + root.car.dlList.length
            color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans; font.features: Theme.tnum
        }
    }

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: 60
        HoverHandler { id: dlHov }
        RowLayout {
            id: dlContent
            anchors.fill: parent
            spacing: 12
            PosterThumb {
                Layout.preferredWidth: 50; Layout.preferredHeight: 60; Layout.alignment: Qt.AlignVCenter
                posterUrl: root.car.dlItem ? (root.car.dlItem.poster || "") : ""
                label: root.car.dlItem ? (root.car.dlItem.title || "") : ""
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                spacing: 6
                Text {
                    Layout.fillWidth: true
                    text: root.car.dlItem ? (root.car.dlItem.title || "") : ""
                    color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold
                    font.family: Theme.fontSans; elide: Text.ElideRight
                }
                RowLayout {
                    Layout.fillWidth: true; spacing: 6
                    IconImg {
                        visible: root.car.dlItem && (root.car.slotResume || root.car.dlItem.paused === true)
                        src: root.car.slotResume ? "qrc:/icons/play.svg" : "qrc:/icons/pause.svg"
                        tint: Theme.accent; s: 11
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Text {
                        text: !root.car.dlItem ? ""
                              : root.car.slotResume ? i18n.t("hub_resume")
                              : (root.car.dlItem.paused === true) ? i18n.t("state_paused")
                              : root.car.slotSeed ? ("↑ " + (root.car.dlItem.upSpeed || ""))
                              : ("↓ " + (root.car.dlItem.downSpeed || ""))
                        color: Theme.accent; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                    Item { Layout.fillWidth: true }
                    Text {
                        text: !root.car.dlItem ? ""
                              : root.car.slotResume ? (root.car.dlItem.metric || "")
                              : root.car.slotSeed ? ("⇅ " + (root.car.dlItem.ratio || "0.00"))
                              : (Math.floor((root.car.dlItem.progress || 0) * 100) + "%")
                        color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold
                        font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                }
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 4; radius: 2; color: Theme.track
                    Rectangle {
                        anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                        width: parent.width * Math.min(1, root.car.dlItem ? (root.car.dlItem.progress || 0) : 0)
                        radius: 2; color: Theme.accent
                    }
                }
            }
        }
        MouseArea {
            id: dlMa; anchors.fill: parent; cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (!root.car.dlItem) return
                if (root.car.slotResume) {
                    if (root.car.dlItem.kind === "game") session.launchGame(root.car.dlItem.infoHash)
                    else session.playByHash(root.car.dlItem.infoHash)
                } else {
                    root.rail.selectTorrent(root.car.dlItem.infoHash); root.rail.pageRequested(0)
                }
            }
        }

        Rectangle {
            anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
            width: 24; height: 24; radius: 12
            color: "#ee15151a"; border.color: Theme.hair; border.width: 1
            visible: root.car.dlList.length > 1
            opacity: dlHov.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 130 } }
            Text { anchors.centerIn: parent; text: "‹"; color: Theme.t1; font.pixelSize: 16; font.family: Theme.fontSans }
            MouseArea {
                anchors.fill: parent; enabled: dlHov.hovered; cursorShape: Qt.PointingHandCursor
                onClicked: root.car.prev()
            }
        }
        Rectangle {
            anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
            width: 24; height: 24; radius: 12
            color: "#ee15151a"; border.color: Theme.hair; border.width: 1
            visible: root.car.dlList.length > 1
            opacity: dlHov.hovered ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 130 } }
            Text { anchors.centerIn: parent; text: "›"; color: Theme.t1; font.pixelSize: 16; font.family: Theme.fontSans }
            MouseArea {
                anchors.fill: parent; enabled: dlHov.hovered; cursorShape: Qt.PointingHandCursor
                onClicked: root.car.next()
            }
        }
    }

    Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hair }
}
