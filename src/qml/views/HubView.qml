// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// HUB page: what you were in the middle of on top, then your library as one
// rail per kind (games, series, films), then what to get next. A thin
// composer: the derived lists live in HubCompute, the wording in HubFormat.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: page
    signal openSearch(string query)
    // A title's page in Find: where a series opens, and every recommendation.
    signal openTitle(var work)
    property var api: typeof session !== "undefined" ? session : null
    property var library: []
    property var gameItems: []

    property string librarySearch: ""
    property string librarySort: "recent"   // recent | name | size
    property string libraryFilter: "all"    // all | game | series | movie
    readonly property var disco: typeof discovery !== "undefined" ? discovery : null

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
    readonly property var suggestedWatch: logic.suggestedWatch
    readonly property var suggestedGame: logic.suggestedGame
    readonly property bool empty: logic.empty
    readonly property var seriesShelf: logic.seriesGroups(library)
    // A show that resolved is one card in the series rail, however many
    // torrents hold it; one that did not still belongs there, alone.
    readonly property var seriesRail: {
        var grouped = ({})
        for (var g = 0; g < seriesShelf.length; g++)
            for (var m = 0; m < seriesShelf[g].members.length; m++)
                grouped[seriesShelf[g].members[m]] = true
        var loose = (library || []).filter(function (i) { return i.isSeries && !grouped[i.infoHash] })
        return applyView(seriesShelf.concat(loose))
    }
    readonly property var movieRail: applyView((library || []).filter(function (i) { return !i.isSeries }))
    readonly property var gameRail: applyView(gameItems || [])
    readonly property var counts: ({ all: gameRail.length + seriesRail.length + movieRail.length,
                                     game: gameRail.length, series: seriesRail.length, movie: movieRail.length })

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
    function hours(secs) { return fmt.hours(secs) }
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
    function tileState(item, kind) { return fmt.tileState(item, kind) }
    function gameMenuOpenFolder(hash) { logic.gameMenuOpenFolder(hash) }
    function fmtAgo(ms) { return fmt.fmtAgo(ms) }
    function openExePicker(hash, launchAfter) { logic.openExePicker(hash, launchAfter) }
    function genreKey(name) { return logic.genreKey(name) }

    property string gameHash: ""
    property bool gameOpen: false
    // Set when Downloads opened the game page: closing it goes back there.
    property int gameReturnPage: -1
    signal returnTo(int page)
    function openGame(hash, fromPage) {
        gameReturnPage = fromPage === undefined ? -1 : fromPage
        gameHash = hash
        gameOpen = true
    }
    function closeGame() {
        gameOpen = false
        if (gameReturnPage >= 0) { var p = gameReturnPage; gameReturnPage = -1; returnTo(p) }
    }

    property var detailItem: null
    property bool detailOpen: false
    function openDetail(item, isGame) { logic.openDetail(item, isGame) }
    function openItem(item, kind) {
        if (!item) return
        if (kind === "series") {
            var ep = logic.series.resumeEpisode(item.videos)
            openTitle({ title: item.title || "", year: item.year || "", type: "series",
                        tmdbId: item.tmdbId || 0, poster: item.poster || "",
                        season: ep ? ep.season : -1, episode: ep ? ep.episode : -1 })
        }
        else openDetail(item, kind === "game")
    }
    function openRec(item) {
        if (item) openTitle(item)
    }

    Rectangle { anchors.fill: parent; color: Theme.bg }

    Flickable {
        id: hubFlick
        anchors.fill: parent
        contentHeight: col.height + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        WheelScroller { flick: hubFlick }

        Column {
            id: col
            width: hubFlick.width

            HubContinueHeroes {
                width: col.width
                hub: page
            }

            Column {
                width: col.width
                topPadding: 48
                spacing: 14
                visible: page.empty
                IconImg {
                    anchors.horizontalCenter: parent.horizontalCenter
                    src: "qrc:/icons/hub.svg"
                    tint: Theme.t4
                    s: 46
                    opacity: 0.7
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: (i18n.language, i18n.t("hub_empty_title"))
                    color: Theme.t1
                    font.pixelSize: 18
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: (i18n.language, i18n.t("hub_empty_sub"))
                    color: Theme.t3
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                }
                BtnFlat {
                    anchors.horizontalCenter: parent.horizontalCenter
                    primary: true
                    icon: "qrc:/icons/search.svg"
                    text: (i18n.language, i18n.t("nav_find"))
                    onClicked: page.openSearch("")
                }
            }

            HubLibraryBar {
                width: col.width
                visible: !page.empty
                hub: page
                counts: page.counts
            }

            HubRail {
                width: col.width
                visible: items.length > 0 && (page.libraryFilter === "all" || page.libraryFilter === "game")
                hub: page
                kind: "game"
                title: (i18n.language, i18n.t("hub_kind_games"))
                items: page.gameRail
                onActivated: function (it) { page.openItem(it, "game") }
                onContext: function (it) { page.gameMenu.openFor(it.infoHash) }
            }
            HubRail {
                width: col.width
                visible: items.length > 0 && (page.libraryFilter === "all" || page.libraryFilter === "series")
                hub: page
                kind: "series"
                title: (i18n.language, i18n.t("hub_kind_series"))
                items: page.seriesRail
                onActivated: function (it) { page.openItem(it, "series") }
                onContext: function (it) { if (!it.isGroup) page.continueMenu.openFor(it.infoHash, it.fileIndex) }
            }
            HubRail {
                width: col.width
                visible: items.length > 0 && (page.libraryFilter === "all" || page.libraryFilter === "movie")
                hub: page
                kind: "movie"
                title: (i18n.language, i18n.t("hub_kind_movies"))
                items: page.movieRail
                onActivated: function (it) { page.openItem(it, "movie") }
                onContext: function (it) { page.continueMenu.openFor(it.infoHash, it.fileIndex) }
            }

            Item { width: 1; height: 16; visible: !page.empty }

            PosterShelf {
                landscape: true
                showSeeAll: false
                label: (i18n.language, i18n.t("hub_mylist"))
                items: page.api ? page.api.watchlist : []
                flickY: hubFlick.contentY
                flickH: hubFlick.height
                onActivated: function (it) { page.openRec(it) }
            }
            PosterShelf {
                landscape: true
                showSeeAll: false
                label: (i18n.language, i18n.t("hub_because_watched")).arg(page.recSeed ? (page.recSeed.title || "") : "")
                items: page.recSeed ? page.perTitleRecs : []
                flickY: hubFlick.contentY
                flickH: hubFlick.height
                onActivated: function (it) { page.openRec(it) }
            }
            PosterShelf {
                landscape: true
                showSeeAll: false
                label: (i18n.language, i18n.t("hub_because_played")).arg(page.gameSeed ? (page.gameSeed.title || "") : "")
                items: page.gameSeed ? page.gameRecs : []
                flickY: hubFlick.contentY
                flickH: hubFlick.height
                onActivated: function (it) { page.openRec(it) }
            }
            PosterShelf {
                landscape: true
                showSeeAll: false
                label: (i18n.language, i18n.t("hub_recommended"))
                items: page.recommendations
                flickY: hubFlick.contentY
                flickH: hubFlick.height
                onActivated: function (it) { page.openRec(it) }
            }
        }
    }

    HubGamePage { anchors.fill: parent; hub: page }
    HubDetailDrawer { anchors.fill: parent; hub: page }
}
