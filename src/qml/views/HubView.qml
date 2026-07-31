// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// HUB page — library of watchable video torrents + games. Thin orchestrator:
// owns search/sort/detail state, composes HubCompute + rails + shelves + drawer.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../theme"
import "../widgets"

Item {
    id: page
    signal openSearch(string query)
    property var api: typeof session !== "undefined" ? session : null
    property var library: []
    property var gameItems: []

    property string librarySearch: ""
    property string librarySort: "recent"   // recent | name
    readonly property var disco: typeof discovery !== "undefined" ? discovery : null

    readonly property int railCardW: 134
    readonly property int railSpacing: 16
    readonly property int railW: 3 * railCardW + 2 * railSpacing

    HubFormat { id: fmt; page: page }
    HubMenus {
        id: menus
        page: page
    }
    HubCompute { id: logic; page: page }

    property alias exePicker: menus.exePicker
    property alias gameMenu: menus.gameMenu
    property alias continueMenu: menus.continueMenu
    property alias episodeMenu: menus.episodeMenu

    readonly property var continueItems: logic.continueItems
    readonly property var continuePlaying: logic.continuePlaying
    readonly property var suggestedGame: logic.suggestedGame
    readonly property bool empty: logic.empty
    readonly property var recentlyAdded: logic.recentlyAdded
    readonly property string topGenre: logic.topGenre
    readonly property var recommendations: logic.recommendations
    readonly property var recSeed: logic.recSeed
    property alias perTitleRecs: logic.perTitleRecs
    readonly property var gameSeed: logic.gameSeed
    property alias gameRecs: logic.gameRecs

    function refresh() { logic.refresh() }
    onVisibleChanged: if (visible) refresh()

    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onTorrentFinished(name, hash) { if (page.visible) page.refresh() }
        function onGamesChanged() { if (page.visible) page.gameItems = page.api ? page.api.gameLibrary() : [] }
    }
    Connections {
        target: page.disco
        ignoreUnknownSignals: true
        function onRecommendationsReady(tmdbId, items) {
            if (!page.recSeed || page.recSeed.tmdbId !== tmdbId) return
            var owned = []
            for (var i = 0; i < page.library.length; i++) owned.push(page.library[i].title || "")
            page.perTitleRecs = page.disco ? page.disco.excludeOwned(items, owned, 12) : []
        }
        function onGameRecommendationsReady(gameName, items) {
            if (!page.gameSeed || (page.gameSeed.title || "") !== gameName) return
            var owned = []
            for (var i = 0; i < page.gameItems.length; i++) owned.push(page.gameItems[i].title || "")
            page.gameRecs = page.disco ? page.disco.excludeOwned(items, owned, 12) : []
        }
    }

    function fmtPlaytime(secs) { return fmt.fmtPlaytime(secs) }
    function fmtLeft(ms) { return fmt.fmtLeft(ms) }
    function episodeLabel(item) { return fmt.episodeLabel(item) }
    function fmtSize(b) { return fmt.fmtSize(b) }
    function applyView(list) { return logic.applyView(list) }
    function fmtTime(ms) { return fmt.fmtTime(ms) }
    function fileUrl(p) { return fmt.fileUrl(p) }
    function playMovie(item) { logic.playMovie(item) }
    function playGame(hash) { logic.playGame(hash) }
    function gamePrimary(item) { logic.gamePrimary(item) }
    function gameStateLabel(item) { return fmt.gameStateLabel(item) }
    function gameStateActionable(item) { return fmt.gameStateActionable(item) }
    function cardStatus(item, isGame) { return fmt.cardStatus(item, isGame) }
    function gameMenuOpenFolder(hash) { logic.gameMenuOpenFolder(hash) }
    function fmtAgo(ms) { return fmt.fmtAgo(ms) }
    function openExePicker(hash, launchAfter) { logic.openExePicker(hash, launchAfter) }
    function genreKey(name) { return logic.genreKey(name) }

    property var detailItem: null
    property bool detailIsGame: false
    property bool detailOpen: false
    function openDetail(item, isGame) { logic.openDetail(item, isGame) }

    Rectangle { anchors.fill: parent; color: Theme.bg }

    Flickable {
        id: hubFlick
        anchors.fill: parent
        contentHeight: col.implicitHeight + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        WheelScroller { flick: hubFlick }

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 22

            Text {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5
                Layout.rightMargin: Theme.sp5
                Layout.topMargin: Theme.sp5 + 4
                text: (i18n.language, i18n.t("hub_greeting"))
                color: Theme.t1
                font.pixelSize: 25
                font.weight: Font.Bold
                font.family: Theme.fontSans
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 48
                spacing: 14
                visible: page.empty
                IconImg {
                    Layout.alignment: Qt.AlignHCenter
                    src: "qrc:/icons/hub.svg"
                    tint: Theme.t4
                    s: 46
                    opacity: 0.7
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: (i18n.language, i18n.t("hub_empty_title"))
                    color: Theme.t1
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: (i18n.language, i18n.t("hub_empty_sub"))
                    color: Theme.t3
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 4
                    spacing: 12
                    BtnFlat {
                        primary: true
                        icon: "qrc:/icons/search.svg"
                        text: (i18n.language, i18n.t("nav_find"))
                        onClicked: page.openSearch("")
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5
                Layout.rightMargin: Theme.sp5
                spacing: Theme.sp3
                visible: page.library.length > 0 || page.gameItems.length > 0
                TFld {
                    Layout.preferredWidth: 260
                    Layout.preferredHeight: 32
                    icon: "qrc:/icons/search.svg"
                    clearable: true
                    placeholder: (i18n.language, i18n.t("hub_search_lib"))
                    onTextChanged: page.librarySearch = text
                }
                Item { Layout.fillWidth: true }
                TSelect {
                    Layout.preferredWidth: 150
                    Layout.preferredHeight: 32
                    property var keys: ["recent", "name"]
                    model: [i18n.t("hub_sort_recent"), i18n.t("hub_sort_name")]
                    onActivated: page.librarySort = keys[currentIndex]
                }
            }

            HubContinueRails {
                Layout.fillWidth: true
                page: page
            }

            HubShelves {
                Layout.fillWidth: true
                page: page
            }
        }
    }

    HubDetailDrawer { anchors.fill: parent; hub: page }
}
