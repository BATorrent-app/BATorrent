// SPDX-License-Identifier: MIT
// Watch path characterization: Hub continue-watching shelf + playMovie →
// playByHash. Fails if HUB library → player wiring regresses.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

Item {
    id: root
    width: 400
    height: 200

    QtObject {
        id: mockApi
        property var playCalls: []
        function playByHash(hash) { playCalls.push(hash) }
        function launchGame(hash) { playCalls.push("game:" + hash) }
        function movieLibrary() { return [] }
        function gameLibrary() { return [] }
    }

    QtObject {
        id: mockEpisodeMenu
        property var openCalls: []
        function openFor(item) { openCalls.push(item) }
    }

    QtObject {
        id: mockPage
        property var api: mockApi
        property var disco: null
        property var episodeMenu: mockEpisodeMenu
        property var library: [
            { title: "Old", infoHash: "aaa", resumeMs: 1000, resumeAt: 10,
              videos: [{ idx: 0 }], addedTime: 1 },
            { title: "Fresh", infoHash: "bbb", resumeMs: 5000, resumeAt: 99,
              videos: [{ idx: 0 }], addedTime: 2 },
            { title: "Unplayed", infoHash: "ccc", resumeMs: 0, resumeAt: 0,
              videos: [{ idx: 0 }], addedTime: 3 }
        ]
        property var gameItems: []
        property string librarySearch: ""
        property string librarySort: "added"
        property var detailItem: null
        property bool detailOpen: false
        property string gameHash: ""
        property bool gameOpen: false
        function openGame(h) { gameHash = h; gameOpen = true }
        property var exePicker: null
        function fileUrl(p) { return "file://" + p }
    }

    Component {
        id: computeComp
        HubCompute { page: mockPage }
    }

    TestCase {
        name: "HubCompute"
        when: windowShown
        width: 400
        height: 200

        function init() {
            mockApi.playCalls = []
            mockEpisodeMenu.openCalls = []
        }

        function test_continueItemsOrdersByResumeAt() {
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            compare(c.continueItems.length, 2)
            compare(c.continueItems[0].infoHash, "bbb")
            compare(c.continueItems[1].infoHash, "aaa")
            compare(c.empty, false)
        }

        function test_withNothingUnderWayTheTopSuggestsWhatIsReady() {
            var lib = mockPage.library
            var games = mockPage.gameItems
            mockPage.library = [
                { title: "Seen", infoHash: "s", completed: true, watchedPct: 0.95, addedTime: 9 },
                { title: "Half", infoHash: "h", completed: true, resumeMs: 500, addedTime: 8 },
                { title: "Coming", infoHash: "c", completed: false, addedTime: 7 },
                { title: "Older", infoHash: "o", completed: true, addedTime: 2 },
                { title: "Newer", infoHash: "n", completed: true, addedTime: 5 }
            ]
            mockPage.gameItems = [
                { title: "Repack", infoHash: "r", installState: 1, addedTime: 9 },
                { title: "Ready", infoHash: "y", installState: 4, addedTime: 3 },
                { title: "Down", infoHash: "d", installState: 0, addedTime: 10 }
            ]
            var c = createTemporaryObject(computeComp, root)
            compare(c.suggestedWatch.infoHash, "n", "newest finished, unwatched, not already being resumed")
            compare(c.suggestedGame.infoHash, "y", "a game ready to play beats one to install")
            mockPage.gameItems = [ { title: "Repack", infoHash: "r", installState: 1 },
                                   { title: "Down", infoHash: "d", installState: 0 } ]
            compare(c.suggestedGame.infoHash, "r", "then the one ready to install")
            mockPage.gameItems = [ { title: "Down", infoHash: "d", installState: 0 } ]
            compare(c.suggestedGame, null, "a download is nothing to play yet")
            mockPage.library = lib
            mockPage.gameItems = games
        }

        function test_openDetailRoutesGamesToTheirPage() {
            mockPage.detailOpen = false
            mockPage.gameOpen = false
            var c = createTemporaryObject(computeComp, root)
            c.openDetail({ infoHash: "g1", title: "Hades" }, true)
            compare(mockPage.gameOpen, true)
            compare(mockPage.gameHash, "g1")
            compare(mockPage.detailOpen, false)
            c.openDetail({ infoHash: "aaa", title: "Old" }, false)
            compare(mockPage.detailOpen, true)
            compare(mockPage.detailItem.infoHash, "aaa")
            mockPage.gameOpen = false
            mockPage.detailOpen = false
        }

        function test_playMovieCallsPlayByHash() {
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            c.playMovie({ infoHash: "bbb", videos: [{ idx: 0 }] })
            compare(mockApi.playCalls.length, 1)
            compare(mockApi.playCalls[0], "bbb")
            compare(mockEpisodeMenu.openCalls.length, 0)
        }

        function test_playMovieOpensEpisodeMenuWhenMultiVideo() {
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            c.playMovie({
                infoHash: "ser",
                videos: [{ idx: 0 }, { idx: 1 }]
            })
            compare(mockEpisodeMenu.openCalls.length, 1)
            compare(mockApi.playCalls.length, 0)
        }

        function test_playMovieNoopsWithoutApi() {
            mockPage.api = null
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            c.playMovie({ infoHash: "x", videos: [{ idx: 0 }] })
            compare(mockApi.playCalls.length, 0)
            mockPage.api = mockApi
        }
    }

    // --- series grouping (TeoVeo #21) ---
    //
    // The rule that matters is the one he wrote twice: when the match is not
    // certain, do not group. A wrong merge hides a download inside somebody
    // else's series, where nobody thinks to look for it.
    TestCase {
        name: "HubComputeSeries"
        when: windowShown

        function mk() { return createTemporaryObject(computeComp, root) }

        function ep(idx, season, episode, name, progress) {
            return { idx: idx, season: season, episode: episode, name: name || ("f" + idx),
                     progress: progress === undefined ? 1 : progress }
        }
        function show(hash, tmdbId, title, vids, extra) {
            var o = { infoHash: hash, tmdbId: tmdbId, title: title, isSeries: true,
                      poster: "p.jpg", year: "2019", genres: [], videos: vids, progress: 1 }
            for (var k in (extra || {})) o[k] = extra[k]
            return o
        }

        function test_seasonsOfOneShowBecomeOneCard() {
            var c = mk()
            var g = c.seriesGroups([
                show("h1", 1399, "The Mandalorian", [ep(0, 1, 1), ep(1, 1, 2)]),
                show("h2", 1399, "The Mandalorian", [ep(0, 2, 1)])
            ])
            compare(g.length, 1, "two season packs of one show are one card")
            compare(g[0].members.length, 2)
            compare(g[0].seasons, [1, 2])
            compare(g[0].videos.length, 3)
        }

        function test_everyEpisodeKnowsWhichTorrentHoldsIt() {
            var c = mk()
            var g = c.seriesGroups([
                show("h1", 1399, "Show", [ep(0, 1, 1)]),
                show("h2", 1399, "Show", [ep(7, 2, 1)])
            ])
            var v = g[0].videos
            compare(v[0].hash, "h1"); compare(v[0].idx, 0)
            compare(v[1].hash, "h2"); compare(v[1].idx, 7)
        }

        function test_howFarIntoAnEpisodeSurvivesTheGrouping() {
            var c = mk()
            var e = ep(0, 1, 1); e.watchedPct = 0.42
            var g = c.seriesGroups([ show("h1", 1399, "Show", [e]) ])
            compare(g[0].videos[0].watchedPct, 0.42)
        }

        function test_anUnresolvedShowIsNeverMergedIntoAnother() {
            var c = mk()
            var g = c.seriesGroups([
                show("h1", 1399, "The Mandalorian", [ep(0, 1, 1)]),
                show("h2", 0,    "The Mandalorian S02", [ep(0, 2, 1)])
            ])
            compare(g.length, 1, "only the resolved one forms a group")
            compare(g[0].members, ["h1"], "the unresolved torrent stays on its own")
        }

        function test_differentShowsNeverShareACard() {
            var c = mk()
            var g = c.seriesGroups([
                show("h1", 1399, "A", [ep(0, 1, 1)]),
                show("h2", 1400, "B", [ep(0, 1, 1)])
            ])
            compare(g.length, 2)
        }

        function test_moviesAreNotSeries() {
            var c = mk()
            var g = c.seriesGroups([
                { infoHash: "m1", tmdbId: 500, title: "A Movie", isSeries: false, videos: [ep(0, -1, -1)] }
            ])
            compare(g.length, 0)
        }

        function test_theSameEpisodeTwiceKeepsTheFurtherCopy() {
            // The file's progress decides, not the torrent's: in a season pack
            // episode 1 can be done while the pack sits at 10%, and judging by
            // the pack would throw away the copy that is ready to watch.
            var c = mk()
            var g = c.seriesGroups([
                show("pack",   1399, "Show", [ep(3, 1, 1, "e1", 0.1)]),
                show("single", 1399, "Show", [ep(0, 1, 1, "e1", 1.0)])
            ])
            compare(g[0].videos.length, 1, "one episode, not two")
            compare(g[0].videos[0].hash, "single", "the finished copy wins over the 10% one")
        }

        function test_episodesComeBackInWatchingOrder() {
            var c = mk()
            var g = c.seriesGroups([
                show("h1", 1399, "Show", [ep(0, 2, 2), ep(1, 1, 10), ep(2, 1, 2)])
            ])
            var seq = []
            for (var i = 0; i < g[0].videos.length; i++)
                seq.push("S" + g[0].videos[i].season + "E" + g[0].videos[i].episode)
            compare(seq, ["S1E2", "S1E10", "S2E2"], "by season then episode, numerically")
        }

        function test_aCardWithoutArtBorrowsItFromAMemberThatHasIt() {
            var c = mk()
            var g = c.seriesGroups([
                show("h1", 1399, "Raw.Name.S01", [ep(0, 1, 1)], { poster: "", title: "Raw.Name.S01" }),
                show("h2", 1399, "The Mandalorian", [ep(0, 2, 1)])
            ])
            compare(g[0].poster, "p.jpg")
            compare(g[0].title, "The Mandalorian", "and the readable title comes with it")
        }

        // --- the season screen: TMDB's list crossed with what is on disk ---
        function test_aShowReadsAsItsLeastFinishedSeason() {
            var c = mk()
            var g = c.seriesGroups([
                show("s1", 1399, "Show", [ep(0, 1, 1)], { size: 10, addedTime: 100, completed: true, progress: 1 }),
                show("s2", 1399, "Show", [ep(0, 2, 1)], { size: 4, addedTime: 300, completed: false,
                                                           progress: 0.3, paused: true }),
                show("s3", 1399, "Show", [ep(0, 3, 1)], { size: 6, addedTime: 200, completed: false, progress: 0.7 })
            ])
            compare(g.length, 1)
            compare(g[0].size, 20, "sizes add up")
            compare(g[0].addedTime, 300, "as new as its newest season")
            compare(g[0].completed, false, "one season still coming means the show is")
            compare(g[0].progress, 0.3, "the furthest-behind season sets the bar")
            compare(g[0].paused, true)
            var done = c.seriesGroups([show("d", 7, "Done", [ep(0, 1, 1)], { completed: true })])
            compare(done[0].completed, true)
            compare(done[0].paused, false)
        }

        function test_aShowPicksUpWhereItWasLeft() {
            var c = mk()
            var v = function (s, e, at, watched) { return { season: s, episode: e, resumeAt: at, watched: watched === true } }
            compare(c.series.resumeEpisode([v(1, 2, 0), v(1, 1, 0)]).episode, 1, "nothing played: the first on disk")
            var mid = c.series.resumeEpisode([v(1, 1, 100, true), v(1, 2, 300), v(1, 3, 200)])
            compare(mid.episode, 2, "the one played last, not the furthest")
            var done = c.series.resumeEpisode([v(1, 1, 100, true), v(1, 2, 500, true), v(2, 1, 0)])
            compare(done.season, 2, "finished: on to the next one on disk")
            compare(done.episode, 1)
            var end = c.series.resumeEpisode([v(1, 1, 0), v(1, 2, 900, true)])
            compare(end.episode, 2, "the last one finished stays put")
            compare(c.series.resumeEpisode([]), null)
            compare(c.series.resumeEpisode([{ season: -1, episode: -1 }]), null, "loose files aren't episodes")
        }

        function test_nothingInNothingOut() {
            var c = mk()
            compare(c.seriesGroups([]).length, 0)
            compare(c.seriesGroups(null).length, 0)
        }
    }
}
