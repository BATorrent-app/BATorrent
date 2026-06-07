// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// HUB page (4.0 step ⑤) — your library of watchable video torrents. "Continue
// watching" (resume from where you stopped, with a watched-% bar on the poster)
// plus a grid of every movie; click → embedded player. Built on session.movieLibrary().
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "theme"
import "widgets"

Item {
    id: page
    property var api: typeof session !== "undefined" ? session : null
    property var library: []
    readonly property var continueItems: library.filter(function (i) { return (i.resumeMs || 0) > 0 })

    function refresh() { library = api ? api.movieLibrary() : [] }
    onVisibleChanged: if (visible) refresh()
    Component.onCompleted: refresh()

    Rectangle { anchors.fill: parent; color: Theme.bg }

    // empty state
    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - 80, 360)
        spacing: 12
        visible: page.library.length === 0
        IconImg { Layout.alignment: Qt.AlignHCenter; src: "qrc:/icons/play.svg"; tint: Theme.t4; s: 44 }
        Text {
            Layout.alignment: Qt.AlignHCenter; Layout.fillWidth: true
            text: (i18n.language, i18n.t("hub_empty_title"))
            color: Theme.t2; font.pixelSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        }
        Text {
            Layout.alignment: Qt.AlignHCenter; Layout.fillWidth: true
            text: (i18n.language, i18n.t("hub_empty_sub"))
            color: Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        }
    }

    Flickable {
        anchors.fill: parent
        visible: page.library.length > 0
        contentHeight: col.implicitHeight + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 20

            // Continue watching
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; Layout.topMargin: Theme.sp5
                spacing: 12
                visible: page.continueItems.length > 0
                Text {
                    text: (i18n.language, i18n.t("hub_continue"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 268
                    orientation: ListView.Horizontal
                    spacing: 16
                    clip: true
                    model: page.continueItems
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: HubCard { item: modelData; onPlay: if (page.api) page.api.playByHash(modelData.infoHash) }
                }
            }

            // Library grid
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                Layout.topMargin: page.continueItems.length > 0 ? 0 : Theme.sp5
                Layout.bottomMargin: Theme.sp5
                spacing: 12
                Text {
                    text: (i18n.language, i18n.t("hub_movies"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                GridLayout {
                    Layout.fillWidth: true
                    columnSpacing: 18
                    rowSpacing: 20
                    columns: Math.max(1, Math.floor((page.width - 2 * Theme.sp5 + columnSpacing) / (150 + columnSpacing)))
                    Repeater {
                        model: page.library
                        delegate: HubCard { item: modelData; onPlay: if (page.api) page.api.playByHash(modelData.infoHash) }
                    }
                }
            }
        }
    }

    // 2:3 poster card with hover play, a download badge (while incomplete) and a
    // watched-% bar along the bottom. Emits play().
    component HubCard: Item {
        id: card
        property var item
        property int cardW: 150
        readonly property int cardH: Math.round(cardW * 1.5)
        signal play()
        implicitWidth: cardW
        implicitHeight: cardH + 38

        Item {
            id: art
            width: card.cardW; height: card.cardH
            scale: ma.containsMouse ? 1.04 : 1.0
            Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

            Rectangle {
                anchors.fill: parent; radius: 10; color: "#161618"; clip: true
                border.color: ma.containsMouse ? Theme.accent : Theme.hair; border.width: 1
                opacity: img.status === Image.Ready ? 0 : 1
                Behavior on opacity { NumberAnimation { duration: 250 } }
                Behavior on border.color { ColorAnimation { duration: 140 } }
                Image {
                    anchors.centerIn: parent; width: parent.width * 0.4; height: width
                    source: "qrc:/images/logo.svg"; sourceSize: Qt.size(width * 2, width * 2)
                    fillMode: Image.PreserveAspectFit; opacity: 0.3
                }
            }
            Image {
                id: img; anchors.fill: parent
                source: card.item.poster || ""
                fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true
                visible: false; sourceSize: Qt.size(card.cardW * 2, card.cardH * 2)
            }
            Rectangle { id: mask; anchors.fill: parent; radius: 10; visible: false; layer.enabled: true }
            MultiEffect {
                anchors.fill: parent; source: img; maskEnabled: true; maskSource: mask
                opacity: img.status === Image.Ready ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
            }

            // hover play overlay
            Rectangle {
                anchors.fill: parent; radius: 10; color: "#66000000"
                opacity: ma.containsMouse ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 140 } }
                IconImg { anchors.centerIn: parent; src: "qrc:/icons/play.svg"; tint: "white"; s: 40 }
            }

            // download badge (still downloading)
            Rectangle {
                visible: !card.item.completed
                anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 6
                radius: 6; color: "#cc000000"
                implicitWidth: dlt.width + 12; implicitHeight: 18
                Text {
                    id: dlt; anchors.centerIn: parent
                    text: "↓ " + Math.round((card.item.progress || 0) * 100) + "%"
                    color: Theme.accent; font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
            }

            // watched-% bar
            Rectangle {
                visible: (card.item.watchedPct || 0) > 0
                anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
                anchors.margins: 5
                height: 4; radius: 2; color: "#99000000"
                Rectangle {
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: parent.width * Math.min(1, card.item.watchedPct)
                    radius: 2; color: Theme.accent
                }
            }
        }

        Text {
            anchors.top: art.bottom; anchors.topMargin: 8
            width: card.cardW
            text: card.item.title || ""
            color: ma.containsMouse ? Theme.t1 : Theme.t2
            font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
            elide: Text.ElideRight; maximumLineCount: 1
            Behavior on color { ColorAnimation { duration: 140 } }
        }

        MouseArea {
            id: ma; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: card.play()
        }
    }
}
