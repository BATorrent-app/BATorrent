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
        property bool detailIsGame: false
        property bool detailOpen: false
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

    // --- series grouping (Sherwan #21) ---
    //
    // The rule that matters is the one he wrote twice: when the match is not
    // certain, do not group. A wrong merge hides a download inside somebody
    // else's series, where nobody thinks to look for it.
    TestCase {
        name: "HubComputeSeries"
        when: windowShown

        function mk() { return createTemporaryObject(computeComp, root) }

        function ep(idx, season, episode, name) {
            return { idx: idx, season: season, episode: episode, name: name || ("f" + idx) }
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
            var c = mk()
            var g = c.seriesGroups([
                show("pack", 1399, "Show", [ep(3, 1, 1)], { progress: 0.1 }),
                show("single", 1399, "Show", [ep(0, 1, 1)], { progress: 1 })
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
        function test_everyListedEpisodeSaysWhetherWeHaveIt() {
            var c = mk()
            var rows = c.mergeSeasonEpisodes(
                [ { episode: 1, name: "Chapter 1", overview: "o1", still: "s1.jpg", runtime: 39 },
                  { episode: 2, name: "Chapter 2" },
                  { episode: 3, name: "Chapter 3" } ],
                [ { season: 1, episode: 1, hash: "h1", idx: 4, name: "f" },
                  { season: 1, episode: 3, hash: "h2", idx: 0, name: "f" } ],
                1)
            compare(rows.length, 3)
            compare(rows[0].have, true);  compare(rows[0].hash, "h1"); compare(rows[0].idx, 4)
            compare(rows[1].have, false, "the gap in the middle is shown, not hidden")
            compare(rows[1].hash, "")
            compare(rows[2].have, true);  compare(rows[2].hash, "h2")
            compare(rows[0].overview, "o1")
            compare(rows[0].still, "s1.jpg")
        }

        function test_otherSeasonsDoNotLeakIn() {
            var c = mk()
            var rows = c.mergeSeasonEpisodes(
                [ { episode: 1, name: "S2E1" } ],
                [ { season: 1, episode: 1, hash: "wrong", idx: 0 },
                  { season: 2, episode: 1, hash: "right", idx: 0 } ],
                2)
            compare(rows[0].hash, "right")
        }

        function test_aFileTmdbDoesNotListIsStillShown() {
            // A special, or a release that numbers episodes its own way. The
            // file is there; leaving it out makes a download invisible.
            var c = mk()
            var rows = c.mergeSeasonEpisodes(
                [ { episode: 1, name: "Chapter 1" } ],
                [ { season: 1, episode: 1, hash: "h1", idx: 0 },
                  { season: 1, episode: 9, hash: "h9", idx: 2, name: "bonus.mkv" } ],
                1)
            compare(rows.length, 2)
            compare(rows[1].episode, 9)
            compare(rows[1].have, true)
            compare(rows[1].title, "bonus.mkv", "with the filename, since TMDB has no title for it")
        }

        function test_aSeasonWeHaveNothingOfStillLists() {
            var c = mk()
            var rows = c.mergeSeasonEpisodes([ { episode: 1, name: "A" } ], [], 1)
            compare(rows.length, 1)
            compare(rows[0].have, false)
        }

        function test_withoutTmdbTheFilesCarryTheScreen() {
            // No network, or a show TMDB never answered for: the screen must
            // still list what can actually be played.
            var c = mk()
            var rows = c.mergeSeasonEpisodes(null,
                [ { season: 1, episode: 2, hash: "h", idx: 1, name: "e2.mkv" },
                  { season: 1, episode: 1, hash: "h", idx: 0, name: "e1.mkv" } ], 1)
            compare(rows.length, 2)
            compare(rows[0].episode, 1, "still in episode order")
            compare(rows[1].episode, 2)
        }

        function test_nothingInNothingOut() {
            var c = mk()
            compare(c.seriesGroups([]).length, 0)
            compare(c.seriesGroups(null).length, 0)
        }
    }
}
