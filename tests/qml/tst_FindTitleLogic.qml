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

    // A game's chips are its release groups: known ones first, the unnamed last.
    function test_groupChipsCountWhoMadeEachRelease() {
        var rows = [rel({ name: "a", releaseGroup: "RUNE" }), rel({ name: "b", releaseGroup: "KaOs" }),
                    rel({ name: "c", releaseGroup: "FitGirl" }), rel({ name: "d", releaseGroup: "FitGirl" }),
                    rel({ name: "e" })]
        function known(g) { return g === "FitGirl" || g === "RUNE" }
        var chips = logic.groupCounts(rows, known, "Other")
        compare(chips.map(function (c) { return c.value }), ["", "FitGirl", "RUNE", "KaOs", "?"])
        compare(chips[0].count, 5)
        compare(chips[1].count, 2)
        compare(chips[4].label, "Other")
        var picked = logic.applyFilters(rows, { quality: "FitGirl", isGame: true }).map(function (r) { return r.name })
        compare(picked, ["c", "d"])
        compare(logic.applyFilters(rows, { quality: "?", isGame: true }).length, 1)
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

    function test_aSeasonSizedEpisodeIsOfferedAsAPack() {
        var GB = 1000 * 1000 * 1000
        // Next to a 27 GB pack, a 27 GB "episode 1" is the season under an episode's name.
        var idx = logic.seasonIndex([ rel({ name: "fake e1", season: 1, episode: 1, sizeBytes: 27 * GB }),
                                      rel({ name: "e1", season: 1, episode: 1, sizeBytes: 3 * GB }),
                                      rel({ name: "pack", season: 1, episode: -1, sizeBytes: 27 * GB }) ], 1)
        compare(idx.byEp["1"].map(function (r) { return r.name }), ["e1"])
        compare(idx.packs.length, 2)
        // No pack listed: many times the usual episode gives it away.
        idx = logic.seasonIndex([ rel({ name: "fake e1", season: 1, episode: 1, sizeBytes: 27 * GB }),
                                  rel({ name: "e2", season: 1, episode: 2, sizeBytes: 3 * GB }),
                                  rel({ name: "e3", season: 1, episode: 3, sizeBytes: 3 * GB }),
                                  rel({ name: "e4", season: 1, episode: 4, sizeBytes: 3 * GB }) ], 1)
        verify(idx.byEp["1"] === undefined)
        compare(idx.packs.map(function (r) { return r.name }), ["fake e1"])
        // A 4K episode beside 1080p ones is just a bigger episode.
        idx = logic.seasonIndex([ rel({ name: "4k e1", season: 1, episode: 1, sizeBytes: 9 * GB }),
                                  rel({ name: "e2", season: 1, episode: 2, sizeBytes: 3 * GB }),
                                  rel({ name: "e3", season: 1, episode: 3, sizeBytes: 3 * GB }) ], 1)
        compare(idx.byEp["1"][0].name, "4k e1")
        compare(idx.packs.length, 0)
    }

    function test_episodeSourcesAreBestQualityFirst() {
        var idx = logic.seasonIndex([ rel({ name: "720", season: 1, episode: 3, quality: "720p", seedsN: 999 }),
                                  rel({ name: "4k", season: 1, episode: 3, quality: "4K", seedsN: 10 }) ], 1)
        compare(logic.episodeReleases(idx, 3)[0].name, "4k")
    }

    function test_anAddonRowStandsForItsPackOnThatEpisode() {
        var GB = 1000 * 1000 * 1000
        // Torrentio's row for E3 of a 321 GB batch, and the same batch as the
        // season pack an indexer found: one row, the one that knows the file.
        var idx = logic.seasonIndex([
            rel({ name: "batch e3", season: 1, episode: 3, sizeBytes: 1.3 * GB, seedsN: 561,
                  fromAddon: true, inPack: true, fileIdx: 2, coverHash: "aa" }),
            rel({ name: "batch", season: 1, episode: -1, sizeBytes: 321 * GB, seedsN: 400, coverHash: "aa" }),
            rel({ name: "other pack", season: 1, episode: -1, sizeBytes: 30 * GB, seedsN: 20, coverHash: "bb" }),
            rel({ name: "single e3", season: 1, episode: 3, sizeBytes: 0.4 * GB, seedsN: 1, coverHash: "cc" }) ], 1)
        compare(logic.episodeReleases(idx, 3).map(function (r) { return r.name }),
                ["batch e3", "single e3", "other pack"])
        // on another episode the indexer's pack row is still offered
        compare(logic.episodeReleases(idx, 4).map(function (r) { return r.name }), ["batch", "other pack"])
    }

    function test_anAddonRowIsNeverMovedToThePacks() {
        var GB = 1000 * 1000 * 1000
        var idx = logic.seasonIndex([
            rel({ name: "addon e1", season: 1, episode: 1, sizeBytes: 40 * GB, fromAddon: true, coverHash: "aa" }),
            rel({ name: "pack", season: 1, episode: -1, sizeBytes: 27 * GB, coverHash: "bb" }) ], 1)
        compare(idx.byEp["1"].map(function (r) { return r.name }), ["addon e1"])
        compare(idx.packs.length, 1)
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
        // A game's pill is who released it; the version moved to the meta line.
        compare(logic.bigLabel(rel({ version: "1.0.3", releaseGroup: "FitGirl" }), true), "FitGirl")
        compare(logic.bigLabel(rel({ version: "1.0.3" }), true), "?")
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

    // ---- one episode's releases ----

    function test_anEpisodeIsNamedByWhatItIs() {
        compare(logic.readableName(rel({ name: "True.Detective.S01E01.1080p.BluRay.DDP5.1.x264", source: "BluRay",
                                         season: 1, episode: 1 }), tr), "BluRay · 5.1")
        // nothing to say: the raw name stays
        compare(logic.readableName(rel({ name: "weird name", season: 1, episode: 1 }), tr), "weird name")
    }

    function test_aPackSaysWhichSeasons() {
        compare(logic.readableName(rel({ name: "Show S01-S04 Complete 1080p", season: 1, source: "WEB" }), tr),
                "find_rel_seasons(1) · find_rel_complete(%1) · WEB")
        compare(logic.readableName(rel({ name: "Show Season 1 Extended 1080p", season: 1, source: "BluRay" }), tr),
                "find_rel_season(1) · find_rel_extended(%1) · BluRay")
    }

    function test_theLanguageLinePutsYoursFirstAndStrong() {
        var bits = logic.langBits(rel({ name: "Show S01E01 DUAL 1080p", audioMode: "dub", langs: ["PT", "EN"], codec: "x264" }),
                                  "PT-BR", tr)
        compare(bits[0].mine, true)
        compare(bits[0].text, "find_rel_dub_sub(PT-BR)")
        compare(bits[1].text, "find_rel_audio(EN)")
        compare(bits[2].text, "H.264")
        // no data, no text
        compare(logic.langBits(rel({ name: "x" }), "PT-BR", tr).length, 0)
    }

    function test_recommendedPrefersYourLanguageThenPicture() {
        var rows = logic.tagged([ rel({ name: "a", quality: "4K", seedsN: 500, sizeBytes: 100 }),
                                  rel({ name: "b", quality: "1080p", seedsN: 80, sizeBytes: 100, audioMode: "dub" }),
                                  rel({ name: "c", quality: "1080p", seedsN: 10, sizeBytes: 100, audioMode: "dub" }) ])
        // the bridge's pick wins when it is in the list
        compare(logic.recommendedIdx(rows, 0, -1), 0)
        // otherwise: dubbed in your language, well seeded, before a bigger picture
        compare(logic.recommendedIdx(rows, -1, -1), 1)
        // what does not fit is never the pick
        compare(logic.recommendedIdx([rows[0]], -1, 50), -1)
    }

    function test_fewSeedsAndWhatDoesNotFit() {
        verify(logic.hasFewSeeds(rel({ seedsN: 29 })))
        verify(!logic.hasFewSeeds(rel({ seedsN: 30 })))
        var f = logic.fitOf(rel({ sizeBytes: 500 }), 200)
        verify(!f.fits)
        compare(f.short, 300)
        verify(logic.fitOf(rel({ sizeBytes: 500 }), -1).fits)
    }
}
