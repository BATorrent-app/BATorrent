// SPDX-License-Identifier: MIT
// What a HUB library tile says about each kind of thing in each state.
// The i18n stub echoes keys, so states read as their key names here.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

Item {
    id: root
    QtObject { id: page }
    HubFormat { id: fmt; page: page }

    TestCase {
        name: "HubFormat"

        function test_aDownloadSaysHowFarAndShowsTheBar() {
            var g = fmt.tileState({ installState: 0, progress: 0.42, size: 50 * 1073741824 }, "game")
            compare(g.state, "hub_tile_downloading")
            compare(g.bar, 0.42)
            compare(g.meta, "50.0 GB")
            var m = fmt.tileState({ completed: false, progress: 0.18 }, "movie")
            compare(m.state, "hub_tile_downloading")
            compare(m.bar, 0.18)
        }

        function test_aPausedDownloadIsQuieter() {
            var g = fmt.tileState({ installState: 0, progress: 0.08, paused: true }, "game")
            compare(g.state, "hub_tile_paused")
            compare(String(g.barColor), String(g.ink))
        }

        function test_anInstalledGameCountsItsHours() {
            var played = fmt.tileState({ installState: 4, playSeconds: 66240, size: 1 }, "game")
            compare(played.state, "hub_installed")
            compare(played.meta, "hub_tile_played")
            compare(played.bar, -1)
            var fresh = fmt.tileState({ installState: 4, playSeconds: 0, size: 12 * 1073741824 }, "game")
            compare(fresh.meta, "12.0 GB")
        }

        function test_aGameNotYetInstalledIsDimmed() {
            var g = fmt.tileState({ installState: 1, size: 1 }, "game")
            compare(g.state, "hub_ready_to_install")
            verify(g.dim)
            verify(!fmt.tileState({ installState: 4 }, "game").dim)
        }

        function test_aRunningGameSaysSo() {
            compare(fmt.tileState({ installState: 5, playing: true }, "game").state, "hub_playing_now")
        }

        function test_aShowMidEpisodeNamesTheEpisode() {
            var item = { completed: true, isSeries: true, seasons: [1, 2],
                         videos: [ { season: 1, episode: 2, watchedPct: 1, watched: true },
                                   { season: 1, episode: 3, watchedPct: 0.42 },
                                   { season: 1, episode: 4, watchedPct: 0 } ] }
            var s = fmt.tileState(item, "series")
            compare(s.state, "S1E03 · 42%", "the finished episode before it does not count")
            compare(s.bar, 0.42)
            compare(s.meta, "hub_n_seasons")
            var idle = fmt.tileState({ completed: true, seasons: [1] }, "series")
            compare(idle.state, "hub_tile_in_library")
            compare(idle.meta, "hub_n_season")
        }

        function test_aFilmReadsWatchedPartwayOrUntouched() {
            var done = fmt.tileState({ completed: true, watchedPct: 0.95 }, "movie")
            compare(done.state, "hub_tile_watched")
            compare(done.bar, 1)
            var mid = fmt.tileState({ completed: true, watchedPct: 0.5, durMs: 3600000, resumeMs: 1800000 }, "movie")
            compare(mid.bar, 0.5)
            compare(mid.state, "hub_time_left")
            compare(fmt.tileState({ completed: true }, "movie").state, "hub_tile_in_library")
        }

        function test_nothingInIsEmptyOut() {
            var n = fmt.tileState(null, "game")
            compare(n.state, "")
            compare(n.bar, -1)
        }
    }
}
