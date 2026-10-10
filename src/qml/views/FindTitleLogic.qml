// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The release lists a picked title's page shows beside its art: a film's and a
// game's releases, a season's episodes, one episode's sources. Pure functions
// over the bridge's flat result rows, so tst_FindTitleLogic can drive them.
import QtQuick

QtObject {
    id: root

    readonly property var qualities: ["4K", "1080p", "720p", "480p"]

    function qualityRank(q) {
        var i = qualities.indexOf(q || "")
        return i < 0 ? 0 : qualities.length - i
    }

    // The settings combo stores 2160p; everything on screen says 4K.
    function preferredQuality(settingIndex) {
        var opts = ["", "1080p", "720p", "4K"]
        return opts[settingIndex] || "1080p"
    }

    function isCam(r) { return (r.source || "") === "CAM" }

    function bigLabel(r, isGame) {
        if (isGame) return (r.version || "").length > 0 ? "v" + r.version : (r.quality || "—")
        if (isCam(r)) return "CAM"
        return r.quality || "—"
    }

    function tagged(results) {
        var out = []
        for (var i = 0; i < (results || []).length; i++) {
            var r = results[i]
            if (r._idx === undefined) r._idx = i
            out.push(r)
        }
        return out
    }

    // Only rows that name the whole title. Indexers pad a title search with
    // anything sharing a word; the full table under "All releases" keeps them.
    function onTitle(results, relevanceOf) {
        return tagged(results).filter(function (r) { return relevanceOf(r.name || "") >= 100 })
    }

    function bySeeds(a, b) {
        var d = (b.seedsN || 0) - (a.seedsN || 0)
        return d !== 0 ? d : a._idx - b._idx
    }

    // Newest build first for a game: a repack of the latest patch beats a
    // better-seeded copy of an older one, because the older one is what you
    // then patch.
    function sortRows(rows, key, compareVersions) {
        var out = rows.slice()
        if (!key) return out
        if (key === "size_desc")
            out.sort(function (a, b) { return (b.sizeBytes || 0) - (a.sizeBytes || 0) || bySeeds(a, b) })
        else if (key === "size_asc")
            out.sort(function (a, b) { return (a.sizeBytes || 0) - (b.sizeBytes || 0) || bySeeds(a, b) })
        else if (key === "newest" && compareVersions)
            out.sort(function (a, b) {
                return compareVersions(b.version || "", a.version || "") || bySeeds(a, b)
            })
        else
            out.sort(bySeeds)
        return out
    }

    // f: { quality, hideCam, myLang, fits, free } — free < 0 when unknown.
    function applyFilters(rows, f) {
        return rows.filter(function (r) {
            if (f.quality && r.quality !== f.quality) return false
            if (f.hideCam && isCam(r)) return false
            if (f.myLang && r.native !== true && r.audioMode !== "dub") return false
            if (f.fits && f.free >= 0 && (r.sizeBytes || 0) > f.free) return false
            return true
        })
    }

    // The quality chips: every quality present, in rank order, with its count.
    function qualityCounts(rows) {
        var counts = ({})
        for (var i = 0; i < rows.length; i++) {
            var q = rows[i].quality || ""
            if (q) counts[q] = (counts[q] || 0) + 1
        }
        var out = [{ value: "", count: rows.length }]
        for (var j = 0; j < qualities.length; j++)
            if (counts[qualities[j]]) out.push({ value: qualities[j], count: counts[qualities[j]] })
        return out
    }

    function hasNative(rows) {
        for (var i = 0; i < rows.length; i++)
            if (rows[i].native === true || rows[i].audioMode === "dub") return true
        return false
    }

    function hasCam(rows) {
        for (var i = 0; i < rows.length; i++) if (isCam(rows[i])) return true
        return false
    }

    // The meta line of a compact row: where it is from, what it speaks.
    function metaLine(r, tr) {
        var parts = [r.sub || r.provider || ""]
        var ls = r.langs || []
        if (r.audioMode === "dub") parts.push(tr("search_audio_dub"))
        else if (r.audioMode === "sub") parts.push(tr("search_audio_sub"))
        if (ls.length > 3) parts.push(tr("find_n_languages").arg(ls.length))
        else if (ls.length > 0) parts.push(ls.join("/"))
        return parts.filter(function (p) { return p.length > 0 }).join("  ·  ")
    }

    // What a season's results hold, per episode. A pack (season, no episode)
    // covers every episode of its season, so it is kept apart and offered on
    // each one instead of being counted as an episode of its own.
    function seasonIndex(results, season) {
        var byEp = ({})
        var packs = []
        var rows = tagged(results)
        for (var i = 0; i < rows.length; i++) {
            var r = rows[i]
            if (r.season !== season) continue
            if ((r.episode || 0) > 0) {
                if (!byEp[r.episode]) byEp[r.episode] = []
                byEp[r.episode].push(r)
            } else {
                packs.push(r)
            }
        }
        for (var k in byEp) byEp[k].sort(bySeeds)
        packs.sort(bySeeds)
        return { byEp: byEp, packs: packs }
    }

    // One episode's sources: its own releases best quality first, then the
    // season packs that contain it.
    function episodeReleases(index, episode) {
        var own = (index.byEp[episode] || []).slice().sort(function (a, b) {
            var q = qualityRank(b.quality) - qualityRank(a.quality)
            return q !== 0 ? q : bySeeds(a, b)
        })
        return own.concat(index.packs)
    }

    // The release a row speaks for: the preferred quality when someone has
    // it, otherwise the best quality on offer.
    function mainRelease(rows, pref) {
        var pool = (rows || []).filter(function (r) { return !isCam(r) })
        var wanted = pool.filter(function (r) { return r.quality === pref })
        if (wanted.length > 0) pool = wanted
        pool.sort(function (a, b) {
            var q = qualityRank(b.quality) - qualityRank(a.quality)
            return q !== 0 ? q : bySeeds(a, b)
        })
        return pool.length > 0 ? pool[0] : null
    }

    // The episode after (season, episode) among the results, and the release to
    // take for it. A season pack is never that release: playing one opens its
    // largest file, which is rarely the episode asked for.
    function nextEpisode(results, season, episode, pref) {
        var here = seasonIndex(results, season)
        var own = here.byEp[episode + 1]
        if (own) return { season: season, episode: episode + 1, release: mainRelease(own, pref) || own[0] }
        var after = seasonIndex(results, season + 1).byEp[1]
        if (after) return { season: season + 1, episode: 1, release: mainRelease(after, pref) || after[0] }
        // only a pack holds it: the episode's sources are where to choose
        if (here.packs.length > 0) return { season: season, episode: episode + 1, release: null }
        return null
    }

    // season_episode -> what is on this disk, for one show.
    function libraryEpisodes(groups, tmdbId) {
        var out = ({})
        if (!(tmdbId > 0)) return out
        for (var g = 0; g < (groups || []).length; g++) {
            if (groups[g].tmdbId !== tmdbId) continue
            var vids = groups[g].videos || []
            for (var v = 0; v < vids.length; v++)
                out[vids[v].season + "_" + vids[v].episode] = vids[v]
        }
        return out
    }

    // An episode row on the search screen: TMDB's listing, what the indexers
    // offer for it, and whether it is already here.
    function episodeRows(tmdbEpisodes, index, library, season, pref) {
        var rows = []
        var listed = ({})
        var eps = tmdbEpisodes || []
        function row(n, title, still) {
            var own = index.byEp[n] || []
            var main = mainRelease(own, pref) || (own.length > 0 ? own[0] : null)
            var lib = library[season + "_" + n] || null
            return { episode: n, title: title || "", still: still || "",
                     main: main, pack: !main && index.packs.length > 0 ? index.packs[0] : null,
                     lib: lib,
                     offered: own.length > 0 || index.packs.length > 0 }
        }
        for (var e = 0; e < eps.length; e++) {
            listed[eps[e].episode] = true
            rows.push(row(eps[e].episode, eps[e].name, eps[e].still))
        }
        for (var k in index.byEp) {
            var n = Number(k)
            if (!listed[n]) rows.push(row(n, "", ""))
        }
        rows.sort(function (a, b) { return a.episode - b.episode })
        return rows
    }

    // The line under an episode's title, as { key, arg, tone }. tone picks the
    // colour: "dim" no source, "ok" on disk, "accent" part-watched, "" plain.
    function episodeStatus(ep) {
        var lib = ep.lib
        if (lib) {
            if (lib.watched === true) return { key: "find_ep_watched", arg: "", tone: "ok" }
            if ((lib.progress || 0) < 1)
                return { key: "hub_ep_coming", arg: Math.floor((lib.progress || 0) * 100), tone: "" }
            if ((lib.watchedPct || 0) > 0)
                return { key: "find_ep_pct_watched", arg: Math.round(lib.watchedPct * 100), tone: "accent" }
            return { key: "find_ep_in_library", arg: "", tone: "ok" }
        }
        if (ep.main) return { key: "", arg: "", tone: "" }
        if (ep.pack) return { key: "find_ep_in_pack", arg: "", tone: "" }
        return { key: "find_ep_no_sources", arg: "", tone: "dim" }
    }

    function isSlow(r) { return !!r && (r.seedsN || 0) < 10 }

    // How much of the space a game needs is free: the fill of the disk bar.
    function fitFraction(needBytes, freeBytes) {
        if (!(needBytes > 0) || freeBytes < 0) return 0
        return Math.max(0, Math.min(1, freeBytes / needBytes))
    }
}
