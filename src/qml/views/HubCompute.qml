// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Hub derived lists + play/install actions. Host (HubView) owns library state
// and format/menus; this leaf keeps the page root a shelf composer.
// Pure genre/applyView helpers already live in C++ HubLogic via discovery.
import QtQuick

QtObject {
    id: root
    required property var page

    readonly property var continueItems: {
        var lib = page.library || []
        return lib.filter(function (i) { return (i.resumeMs || 0) > 0 })
            .sort(function (a, b) { return (b.resumeAt || 0) - (a.resumeAt || 0) }).slice(0, 3)
    }
    readonly property var continuePlaying: {
        var games = page.gameItems || []
        return games.filter(function (i) { return (i.lastPlayed || 0) > 0 })
            .sort(function (a, b) { return (b.lastPlayed || 0) - (a.lastPlayed || 0) }).slice(0, 3)
    }
    readonly property var suggestedGame: {
        var games = page.gameItems || []
        for (var i = 0; i < games.length; i++)
            if (games[i].installState === 4) return games[i]
        return null
    }
    readonly property bool empty: (page.library || []).length === 0 && (page.gameItems || []).length === 0

    // newest in your library (movies + games), front and centre — Plex/Netflix style.
    // Runs through applyView like every other shelf (search box must filter this too).
    readonly property var recentlyAdded: {
        var all = applyView((page.library || []).concat(page.gameItems || []))
        if (page.librarySort !== "name")
            all.sort(function (a, b) { return (b.addedTime || 0) - (a.addedTime || 0) })
        return all.slice(0, 12)
    }

    function genreKey(name) {
        return page.disco ? page.disco.genreKey(name) : ""
    }
    readonly property string topGenre: {
        if (!page.disco) return ""
        var names = []
        var games = page.gameItems || []
        for (var g = 0; g < games.length; g++) {
            var gg = games[g].genres || []
            for (var i = 0; i < gg.length; i++) names.push(gg[i])
        }
        var lib = page.library || []
        for (var m = 0; m < lib.length; m++) {
            var mg = lib[m].genres || []
            for (var j = 0; j < mg.length; j++) names.push(mg[j])
        }
        return page.disco.topGenreFromNames(names)
    }
    readonly property var recommendations: {
        if (topGenre.length === 0 || !page.disco) return []
        var rows = page.disco.rows || []
        var owned = []
        var lib = page.library || []
        for (var i = 0; i < lib.length; i++) owned.push(lib[i].title || "")
        var games = page.gameItems || []
        for (var j = 0; j < games.length; j++) owned.push(games[j].title || "")
        var cand = []
        for (var r = 0; r < rows.length; r++) {
            if (rows[r].genre !== topGenre) continue
            var items = rows[r].items || []
            for (var k = 0; k < items.length; k++) cand.push(items[k])
        }
        return page.disco.excludeOwned(cand, owned, 12)
    }

    readonly property var recSeed: {
        for (var i = 0; i < continueItems.length; i++)
            if ((continueItems[i].tmdbId || 0) > 0) return continueItems[i]
        return null
    }
    property var perTitleRecs: []
    onRecSeedChanged: {
        perTitleRecs = []
        if (recSeed && page.disco)
            page.disco.fetchRecommendations(recSeed.tmdbId, recSeed.isSeries ? "series" : "movie")
    }

    readonly property var gameSeed: continuePlaying.length > 0 ? continuePlaying[0] : null
    property var gameRecs: []
    onGameSeedChanged: {
        gameRecs = []
        if (gameSeed && page.disco)
            page.disco.fetchGameRecommendations(gameSeed.title || "")
    }

    function applyView(list) {
        if (page.disco) return page.disco.applyLibraryView(list, page.librarySearch, page.librarySort)
        return list
    }

    function playMovie(item) {
        if (!page.api) return
        if (item.videos && item.videos.length > 1) page.episodeMenu.openFor(item)
        else page.api.playByHash(item.infoHash)
    }
    function playGame(hash) {
        if (page.api) page.api.launchGame(hash)
    }
    // installState ints mirror QmlSessionBridge::GameInstallState:
    //   0 Downloading · 1 ReadyToInstall · 2 Extracting · 3 Installing
    //   4 Ready · 5 Playing · 6 NeedsSetup · 7 Failed
    function gamePrimary(item) {
        if (!page.api || !item) return
        switch (item.installState) {
        case 4: page.api.launchGame(item.infoHash); break
        case 1: case 7: page.api.installGame(item.infoHash); break
        case 6: page.openExePicker(item.infoHash, true); break
        case 3: gameMenuOpenFolder(item.infoHash); break
        default: break
        }
    }
    function gameMenuOpenFolder(hash) {
        if (!page.api) return
        var folder = page.api.gameFolder(hash)
        if (folder && folder.length > 0) Qt.openUrlExternally(page.fileUrl(folder))
    }
    function openExePicker(hash, launchAfter) {
        page.exePicker.pendingHash = hash
        page.exePicker.launchAfter = launchAfter
        var folder = page.api ? page.api.gameFolder(hash) : ""
        if (folder.length > 0) page.exePicker.currentFolder = page.fileUrl(folder)
        page.exePicker.open()
    }
    function openDetail(item, isGame) {
        page.detailItem = item
        page.detailIsGame = isGame === true
        page.detailOpen = true
    }
    function refresh() {
        page.library = page.api ? page.api.movieLibrary() : []
        page.gameItems = page.api ? page.api.gameLibrary() : []
        if (page.disco) page.disco.load()
    }
}
