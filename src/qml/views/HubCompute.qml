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

    // newest in your library (movies + games), front and centre: Plex/Netflix style.
    // Runs through applyView like every other shelf (search box must filter this too).
    // One card per series instead of one per torrent: three season packs of the
    // same show are three entries in the library and should be one thing on the
    // shelf (Sherwan #21).
    //
    // Grouped by TMDB id and nothing else. Two torrents whose names both parse
    // to the same show still stay apart when either failed to resolve, because
    // the alternative is guessing from a filename, and a wrong merge hides a
    // download inside somebody else's series where nobody will look for it.
    //
    // The videos of every member are merged into one list, each carrying the
    // hash it came from, so the existing episode menu can play across torrents
    // without knowing a group exists.
    function seriesGroups(items) {
        var byId = ({})
        var order = []
        for (var i = 0; i < (items || []).length; i++) {
            var it = items[i]
            if (!it || !it.isSeries || !(it.tmdbId > 0)) continue
            var k = String(it.tmdbId)
            if (!byId[k]) {
                byId[k] = { infoHash: it.infoHash, tmdbId: it.tmdbId, title: it.title,
                            poster: it.poster, year: it.year, genres: it.genres,
                            description: it.description, isSeries: true, isGroup: true,
                            members: [], videos: [], seasons: [] }
                order.push(k)
            }
            var g = byId[k]
            g.members.push(it.infoHash)
            // The member that actually has art names the group: a season pack
            // that never resolved a poster would otherwise leave the card blank.
            if (!g.poster && it.poster) {
                g.poster = it.poster
                g.title = it.title
                g.year = it.year
            }
            var vids = it.videos || []
            for (var v = 0; v < vids.length; v++) {
                var src = vids[v]
                g.videos.push({ hash: it.infoHash, idx: src.idx, name: src.name,
                                season: src.season, episode: src.episode,
                                watched: src.watched === true,
                                // the file's own progress, not the torrent's
                                progress: src.progress || 0 })
            }
        }

        var out = []
        for (var o = 0; o < order.length; o++) {
            var grp = byId[order[o]]
            grp.videos = root.dedupeEpisodes(grp.videos)
            var seen = ({})
            for (var e = 0; e < grp.videos.length; e++) {
                var sn = grp.videos[e].season
                if (sn >= 0 && !seen[sn]) { seen[sn] = true; grp.seasons.push(sn) }
            }
            grp.seasons.sort(function (a, b) { return a - b })
            out.push(grp)
        }
        return out
    }

    // How much of each season is on disk, for the season picker. Stremio marks
    // a season with a dot you have to guess at; a count says which season is
    // worth opening before you open it.
    function seasonHave(groupVideos, season) {
        var n = 0
        var v = groupVideos || []
        for (var i = 0; i < v.length; i++)
            if (v[i].season === season && v[i].episode >= 0 && (v[i].progress || 0) >= 1) n++
        return n
    }

    // The season as TMDB knows it, crossed with what is actually on disk.
    //
    // Both halves matter: listing only what was downloaded hides that episode 4
    // is missing from the middle of a pack, and listing only TMDB turns a
    // library screen into a catalogue of things you cannot watch. Every row
    // says which it is, and carries the torrent to play when it is here.
    //
    // Episodes we hold that TMDB does not list (a special, a renumbered
    // release) are kept at the end rather than dropped: the file exists, and
    // not showing it is how a download becomes invisible.
    function mergeSeasonEpisodes(tmdbEpisodes, groupVideos, season) {
        var have = ({})
        var vids = groupVideos || []
        for (var i = 0; i < vids.length; i++)
            if (vids[i].season === season && vids[i].episode >= 0)
                have[vids[i].episode] = vids[i]

        var rows = []
        var listed = ({})
        var eps = tmdbEpisodes || []
        for (var e = 0; e < eps.length; e++) {
            var t = eps[e]
            var v = have[t.episode]
            listed[t.episode] = true
            rows.push({ season: season, episode: t.episode,
                        title: t.name || "", overview: t.overview || "",
                        still: t.still || "", runtime: t.runtime || 0,
                        airDate: t.air_date || "",
                        have: v !== undefined,
                        hash: v ? v.hash : "", idx: v ? v.idx : -1,
                        watched: v ? v.watched === true : false,
                        progress: v ? (v.progress || 0) : 0 })
        }
        for (var k in have) {
            if (listed[k]) continue
            var o = have[k]
            rows.push({ season: season, episode: o.episode,
                        title: o.name || "", overview: "", still: "", runtime: 0,
                        airDate: "", have: true, hash: o.hash, idx: o.idx,
                        watched: o.watched === true, progress: o.progress || 0 })
        }
        rows.sort(function (a, b) { return a.episode - b.episode })
        return rows
    }

    // The same episode in two releases (a season pack and a single) is one
    // episode to watch. The copy that is further along wins: offering the one
    // at 0% when a finished file sits right beside it is the wrong answer.
    function dedupeEpisodes(videos) {
        var best = ({})
        var loose = []
        for (var i = 0; i < videos.length; i++) {
            var v = videos[i]
            if (!(v.season >= 0 && v.episode >= 0)) { loose.push(v); continue }
            var k = v.season + "_" + v.episode
            if (!best[k] || (v.progress || 0) > (best[k].progress || 0)) best[k] = v
        }
        var out = []
        for (var k2 in best) out.push(best[k2])
        out.sort(function (a, b) {
            return a.season !== b.season ? a.season - b.season : a.episode - b.episode
        })
        return out.concat(loose)
    }

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
