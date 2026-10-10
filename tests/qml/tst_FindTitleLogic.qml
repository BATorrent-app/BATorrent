// SPDX-License-Identifier: MIT
// The release lists beside a picked title's art: ordering, the per-episode
// index (packs included), the library overlay and the status line.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

TestCase {
    name: "FindTitleLogic"

    FindTitleLogic { id: logic }

    function tr(k) { return k + "(%1)" }

    function rel(o) {
        var base = { name: "", quality: "", source: "", seedsN: 0, sizeBytes: 0,
                     provider: "", langs: [], audioMode: "original", season: -1,
                     episode: -1, version: "", trustWhy: "" }
        for (var k in o) base[k] = o[k]
        return base
    }

    function test_sortsBySeedsByDefaultAndKeepTheBridgeIndex() {
        var rows = logic.sortRows(logic.tagged([ rel({ name: "a", seedsN: 5 }),
                                                 rel({ name: "b", seedsN: 900 }),
                                                 rel({ name: "c", seedsN: 40 }) ]), "seeds")
        compare(rows.map(function (r) { return r.name }), ["b", "c", "a"])
        compare(rows[0]._idx, 1)
    }

    function test_sortsBySizeBothWays() {
        var rows = logic.tagged([ rel({ name: "s", sizeBytes: 1 }), rel({ name: "l", sizeBytes: 9 }) ])
        compare(logic.sortRows(rows, "size_desc")[0].name, "l")
        compare(logic.sortRows(rows, "size_asc")[0].name, "s")
    }

    function test_gamesAreNewestBuildFirstThenSeeds() {
        function cmp(a, b) { return a === b ? 0 : (a > b ? 1 : -1) }
        var rows = logic.sortRows(logic.tagged([ rel({ name: "old", version: "1.0.0", seedsN: 900 }),
                                                 rel({ name: "new-dodi", version: "1.0.3", seedsN: 311 }),
                                                 rel({ name: "new-fg", version: "1.0.3", seedsN: 842 }) ]),
                                  "newest", cmp)
        compare(rows.map(function (r) { return r.name }), ["new-fg", "new-dodi", "old"])
    }

    function test_filtersNarrowWhatTheListShows() {
        var rows = [ rel({ name: "4k", quality: "4K", sizeBytes: 50 }),
                     rel({ name: "cam", quality: "1080p", source: "CAM", sizeBytes: 5 }),
                     rel({ name: "dub", quality: "1080p", audioMode: "dub", sizeBytes: 5 }) ]
        function names(f) { return logic.applyFilters(rows, f).map(function (r) { return r.name }) }
        compare(names({ quality: "1080p" }), ["cam", "dub"])
        compare(names({ hideCam: true }), ["4k", "dub"])
        compare(names({ myLang: true }), ["dub"])
        compare(names({ fits: true, free: 10 }), ["cam", "dub"])
        compare(names({ fits: true, free: -1 }).length, 3)      // unknown disk hides nothing
    }

    function test_qualityChipsCountWhatIsThere() {
        var c = logic.qualityCounts([ rel({ quality: "1080p" }), rel({ quality: "4K" }),
                                      rel({ quality: "1080p" }), rel({ quality: "" }) ])
        compare(c, [ { value: "", count: 4 }, { value: "4K", count: 1 }, { value: "1080p", count: 2 } ])
    }

    function test_metaLineNamesSourceAndLanguage() {
        compare(logic.metaLine(rel({ provider: "YTS", langs: ["EN"] }), tr), "YTS  ·  EN")
        compare(logic.metaLine(rel({ provider: "BitSearch", audioMode: "dub", langs: ["PT", "EN"] }), tr),
                "BitSearch  ·  search_audio_dub(%1)  ·  PT/EN")
        compare(logic.metaLine(rel({ provider: "FitGirl", langs: ["A", "B", "C", "D"] }), tr),
                "FitGirl  ·  find_n_languages(4)")
    }

    function test_aPackIsOfferedOnEveryEpisodeNotCountedAsOne() {
        var idx = logic.seasonIndex([ rel({ name: "e1", season: 1, episode: 1, quality: "1080p", seedsN: 10 }),
                                  rel({ name: "pack", season: 1, episode: -1, seedsN: 50 }),
                                  rel({ name: "s2", season: 2, episode: 1 }) ], 1)
        compare(Object.keys(idx.byEp), ["1"])
        compare(idx.packs.length, 1)
        var own = logic.episodeReleases(idx, 1)
        compare(own.map(function (r) { return r.name }), ["e1", "pack"])
        compare(logic.episodeReleases(idx, 4).map(function (r) { return r.name }), ["pack"])
    }

    function test_episodeSourcesAreBestQualityFirst() {
        var idx = logic.seasonIndex([ rel({ name: "720", season: 1, episode: 3, quality: "720p", seedsN: 999 }),
                                  rel({ name: "4k", season: 1, episode: 3, quality: "4K", seedsN: 10 }) ], 1)
        compare(logic.episodeReleases(idx, 3)[0].name, "4k")
    }

    function test_mainReleaseHonoursThePreferenceThenFallsBackToBest() {
        var rows = [ rel({ name: "4k", quality: "4K", seedsN: 5 }),
                     rel({ name: "hd", quality: "1080p", seedsN: 1 }),
                     rel({ name: "cam", quality: "1080p", source: "CAM", seedsN: 999 }) ]
        compare(logic.mainRelease(rows, "1080p").name, "hd")
        compare(logic.mainRelease(rows, "720p").name, "4k")
        compare(logic.mainRelease([], "4K"), null)
    }

    function test_episodeRowsCrossTmdbIndexersAndTheLibrary() {
        var idx = logic.seasonIndex([ rel({ name: "e2", season: 1, episode: 2, quality: "4K", seedsN: 30 }),
                                  rel({ name: "e9", season: 1, episode: 9, quality: "720p" }) ], 1)
        var lib = logic.libraryEpisodes([ { tmdbId: 7, videos: [ { season: 1, episode: 1, watched: true, progress: 1 } ] },
                                      { tmdbId: 8, videos: [ { season: 1, episode: 2, watched: true, progress: 1 } ] } ], 7)
        var rows = logic.episodeRows([ { episode: 1, name: "One" }, { episode: 2, name: "Two" },
                                   { episode: 3, name: "Three" } ], idx, lib, 1, "4K")
        compare(rows.map(function (r) { return r.episode }), [1, 2, 3, 9])
        verify(rows[0].lib !== null)
        verify(rows[1].lib === null)                 // another show's file is not ours
        compare(rows[1].main.name, "e2")
        compare(rows[2].offered, false)
        compare(rows[3].title, "")                   // indexers have it, TMDB does not list it
    }

    function test_theStatusLineSaysWhatTheEpisodeIs() {
        compare(logic.episodeStatus({ lib: { watched: true, progress: 1 } }).key, "find_ep_watched")
        compare(logic.episodeStatus({ lib: { progress: 1, watchedPct: 0.42 } }).arg, 42)
        compare(logic.episodeStatus({ lib: { progress: 0.3 } }).key, "hub_ep_coming")
        compare(logic.episodeStatus({ lib: { progress: 1 } }).key, "find_ep_in_library")
        compare(logic.episodeStatus({ lib: null, main: rel({}) }).key, "")
        compare(logic.episodeStatus({ lib: null, main: null, pack: rel({}) }).key, "find_ep_in_pack")
        compare(logic.episodeStatus({ lib: null, main: null, pack: null }).tone, "dim")
    }

    function test_bigLabelAndPreference() {
        compare(logic.bigLabel(rel({ quality: "4K" }), false), "4K")
        compare(logic.bigLabel(rel({ quality: "1080p", source: "CAM" }), false), "CAM")
        compare(logic.bigLabel(rel({ version: "1.0.3" }), true), "v1.0.3")
        compare(logic.preferredQuality(3), "4K")
        compare(logic.preferredQuality(0), "1080p")
    }

    function test_onlyRowsNamingTheWholeTitleReachTheStage() {
        var rows = logic.onTitle([ rel({ name: "Resident Evil 2026" }), rel({ name: "Evil Dead" }) ],
                             function (n) { return n.indexOf("Resident") >= 0 ? 100 : 50 })
        compare(rows.length, 1)
        compare(rows[0]._idx, 0)
    }

    function test_theNextEpisodeIsTheNextOneOnOffer() {
        var res = [ rel({ name: "e3", season: 1, episode: 3, quality: "1080p", seedsN: 5 }),
                    rel({ name: "e4-720", season: 1, episode: 4, quality: "720p", seedsN: 50 }),
                    rel({ name: "e4-1080", season: 1, episode: 4, quality: "1080p", seedsN: 9 }),
                    rel({ name: "s2e1", season: 2, episode: 1, quality: "1080p" }) ]
        var n = logic.nextEpisode(res, 1, 3, "1080p")
        compare(n.episode, 4)
        compare(n.release.name, "e4-1080")          // the preferred quality, not the best seeded
        n = logic.nextEpisode(res, 1, 4, "1080p")
        compare([n.season, n.episode, n.release.name], [2, 1, "s2e1"])   // across the season break
        compare(logic.nextEpisode(res, 2, 1, "1080p"), null)
    }

    function test_aPackAloneSendsYouToChoose() {
        var res = [ rel({ name: "pack", season: 1, episode: -1 }) ]
        var n = logic.nextEpisode(res, 1, 2, "1080p")
        compare(n.episode, 3)
        compare(n.release, null)
    }

    function test_fitFraction() {
        compare(logic.fitFraction(100, 25), 0.25)
        compare(logic.fitFraction(100, 400), 1)
        compare(logic.fitFraction(0, 10), 0)
        compare(logic.fitFraction(100, -1), 0)
    }
}
