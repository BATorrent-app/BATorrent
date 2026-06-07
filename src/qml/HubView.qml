// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// HUB page (4.0 step ⑤) — your library of watchable video torrents. "Continue
// watching" (resume from where you stopped, with a watched-% bar on the poster)
// plus a grid of every movie; click → embedded player. Built on session.movieLibrary().
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "theme"
import "widgets"

Item {
    id: page
    property var api: typeof session !== "undefined" ? session : null
    property var library: []
    property var gameItems: []
    // most-recent first, capped — the two "continue" rails at the top
    readonly property var continueItems: library.filter(function (i) { return (i.resumeMs || 0) > 0 })
        .sort(function (a, b) { return (b.resumeAt || 0) - (a.resumeAt || 0) }).slice(0, 3)
    readonly property var continuePlaying: gameItems.filter(function (i) { return (i.lastPlayed || 0) > 0 })
        .sort(function (a, b) { return (b.lastPlayed || 0) - (a.lastPlayed || 0) }).slice(0, 3)
    readonly property bool empty: library.length === 0 && gameItems.length === 0

    // continue rails are sized to hold exactly 3 cards each
    readonly property int railCardW: 134
    readonly property int railSpacing: 16
    readonly property int railW: 3 * railCardW + 2 * railSpacing

    function refresh() {
        library = api ? api.movieLibrary() : []
        gameItems = api ? api.gameLibrary() : []
    }
    onVisibleChanged: if (visible) refresh()
    Component.onCompleted: refresh()

    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        if (p.indexOf("file:") === 0) return p
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }

    // Play a game: launchGame uses the manual exe if set, else auto-detects one,
    // else opens the folder. "Set executable…" (right-click) is the override.
    function playGame(hash) {
        if (api) api.launchGame(hash)
    }
    function openExePicker(hash, launchAfter) {
        exePicker.pendingHash = hash
        exePicker.launchAfter = launchAfter
        var folder = api ? api.gameFolder(hash) : ""
        if (folder.length > 0) exePicker.currentFolder = page.fileUrl(folder)
        exePicker.open()
    }

    Rectangle { anchors.fill: parent; color: Theme.bg }

    // The hub always shows its structure (greeting + the two continue rails) so
    // a fresh, empty library reads as onboarding rather than a dead screen.
    Flickable {
        anchors.fill: parent
        contentHeight: col.implicitHeight + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 22

            // greeting
            Text {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; Layout.topMargin: Theme.sp5 + 4
                text: (i18n.language, i18n.t("hub_greeting"))
                color: Theme.t1; font.pixelSize: 25; font.weight: Font.Bold; font.family: Theme.fontSans
            }

            // Continue watching | Continue playing — side by side, ≤3 each, with a
            // placeholder prompt when nothing has been watched/played yet.
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 32

                // each rail is sized to hold exactly 3 cards (cards row or placeholder)
                ColumnLayout {
                    Layout.preferredWidth: page.railW; Layout.alignment: Qt.AlignTop
                    spacing: 12
                    Text {
                        text: (i18n.language, i18n.t("hub_continue"))
                        color: Theme.t1; font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                    Row {
                        spacing: page.railSpacing
                        visible: page.continueItems.length > 0
                        Repeater {
                            model: page.continueItems
                            delegate: HubCard { cardW: page.railCardW; item: modelData; onPlay: if (page.api) page.api.playByHash(modelData.infoHash) }
                        }
                    }
                    RailPlaceholder { visible: page.continueItems.length === 0; text: (i18n.language, i18n.t("hub_watch_placeholder")) }
                }

                ColumnLayout {
                    Layout.preferredWidth: page.railW; Layout.alignment: Qt.AlignTop
                    spacing: 12
                    Text {
                        text: (i18n.language, i18n.t("hub_continue_playing"))
                        color: Theme.t1; font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                    Row {
                        spacing: page.railSpacing
                        visible: page.continuePlaying.length > 0
                        Repeater {
                            model: page.continuePlaying
                            delegate: HubCard {
                                cardW: page.railCardW; item: modelData; requireDoubleClick: true
                                onPlay: page.playGame(modelData.infoHash)
                                onContext: gameMenu.openFor(modelData.infoHash)
                            }
                        }
                    }
                    RailPlaceholder { visible: page.continuePlaying.length === 0; text: (i18n.language, i18n.t("hub_play_placeholder")) }
                }

                Item { Layout.fillWidth: true }   // keep both rails left-aligned at their 3-card width
            }

            // Your games
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 12
                visible: page.gameItems.length > 0
                Text {
                    text: (i18n.language, i18n.t("hub_games"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                GridLayout {
                    Layout.fillWidth: true
                    columnSpacing: 18
                    rowSpacing: 20
                    columns: Math.max(1, Math.floor((page.width - 2 * Theme.sp5 + columnSpacing) / (150 + columnSpacing)))
                    Repeater {
                        model: page.gameItems
                        delegate: HubCard {
                            item: modelData
                            requireDoubleClick: true
                            onPlay: page.playGame(modelData.infoHash)
                            onContext: gameMenu.openFor(modelData.infoHash)
                        }
                    }
                }
            }

            // Your movies
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                Layout.bottomMargin: Theme.sp5
                spacing: 12
                visible: page.library.length > 0
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

    // dashed-feel prompt shown in a "continue" rail when it has nothing yet
    component RailPlaceholder: Rectangle {
        property alias text: ph.text
        Layout.fillWidth: true
        Layout.preferredHeight: Math.round(page.railCardW * 1.5)   // a poster-sized empty slot
        radius: 10
        color: "transparent"
        border.color: Theme.hairSoft; border.width: 1
        Text {
            id: ph
            anchors.centerIn: parent
            width: parent.width - 32
            color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        }
    }

    // 2:3 poster card with hover play, a download badge (while incomplete) and a
    // watched-% bar along the bottom. Emits play().
    component HubCard: Item {
        id: card
        property var item
        property int cardW: 150
        property bool requireDoubleClick: false
        readonly property int cardH: Math.round(cardW * 1.5)
        signal play()
        signal context()
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
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function (mouse) {
                if (mouse.button === Qt.RightButton) { card.context(); return }
                if (!card.requireDoubleClick) card.play()
            }
            onDoubleClicked: if (card.requireDoubleClick) card.play()
        }
    }

    // ---- games: manual executable picker + per-game context menu ----
    FileDialog {
        id: exePicker
        property string pendingHash: ""
        property bool launchAfter: true
        title: (i18n.language, i18n.t("hub_set_exe"))
        onAccepted: {
            if (!page.api || pendingHash.length === 0) return
            page.api.setGameExe(pendingHash, selectedFile.toString())
            page.refresh()
            if (launchAfter) page.api.launchGame(pendingHash)
        }
    }

    Menu {
        id: gameMenu
        property string hash: ""
        function openFor(h) { hash = h; popup() }
        modal: true
        implicitWidth: 200
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
        component GItem: MenuItem {
            id: gi
            implicitHeight: 30
            padding: 0
            contentItem: Text {
                leftPadding: 14; rightPadding: 14
                text: gi.text; color: gi.highlighted ? Theme.t1 : Theme.t2
                font.pixelSize: 12; font.family: Theme.fontSans; verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle { color: gi.highlighted ? Theme.hover : "transparent"; radius: 5 }
        }
        GItem {
            text: (i18n.language, i18n.t("hub_set_exe"))
            onTriggered: page.openExePicker(gameMenu.hash, false)
        }
        GItem {
            text: (i18n.language, i18n.t("hub_open_folder"))
            onTriggered: if (page.api) Qt.openUrlExternally(page.fileUrl(page.api.gameFolder(gameMenu.hash)))
        }
    }
}
