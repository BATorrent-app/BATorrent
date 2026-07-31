// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Client-side filter/sort + series tab derivation for Find results.
// Relevance scoring stays in C++ SearchRanker (via the bridge); this leaf only
// filters/sorts the bridge's results for the panes.
import QtQuick

QtObject {
    id: root
    required property var sv   // SearchView host (filters, mode flags, api)

    function distinctTokens(role, order) {
        if (!sv || !sv.api) return []
        var seen = {}, out = []
        var res = sv.api.results
        for (var i = 0; i < res.length; i++) {
            var v = res[i][role]
            if (v && !seen[v]) { seen[v] = true; out.push(v) }
        }
        if (order && order.length > 0) {
            out.sort(function (a, b) {
                var ia = order.indexOf(a), ib = order.indexOf(b)
                if (ia < 0) ia = 99; if (ib < 0) ib = 99
                return ia - ib
            })
        } else {
            out.sort()
        }
        return out
    }

    readonly property var qualityOptions: distinctTokens("quality", ["4K", "1080p", "720p", "480p"])
    readonly property var sourceOptions: distinctTokens("source", ["Remux", "BluRay", "WEB", "HDTV", "DVD", "CAM"])
    readonly property var groupOptions: distinctTokens("releaseGroup", [])
    readonly property var providerOptions: distinctTokens("provider", [])

    readonly property bool hasVersions: {
        var rs = (sv && sv.api) ? sv.api.results : []
        for (var i = 0; i < rs.length; i++)
            if ((rs[i].version || "").length > 0) return true
        return false
    }

    readonly property var langOptions: {
        var seen = {}, out = []
        var rs = (sv && sv.api) ? sv.api.results : []
        for (var i = 0; i < rs.length; i++) {
            var ls = rs[i].langs || []
            for (var j = 0; j < ls.length; j++)
                if (ls[j] && !seen[ls[j]]) { seen[ls[j]] = true; out.push(ls[j]) }
        }
        out.sort()
        return out
    }

    readonly property var seasonTabs: {
        if (!sv || !sv.api || !(sv.isSeriesDrill || sv.isEpisodes)) return []
        var seen = {}, out = []
        var res = sv.api.results
        for (var i = 0; i < res.length; i++) {
            var s = res[i].season
            if (s > 0 && !seen[s]) { seen[s] = true; out.push(s) }
        }
        out.sort(function (a, b) { return a - b })
        return out
    }
    readonly property int packCount: {
        if (!sv || !sv.api || !sv.isSeriesDrill) return 0
        var n = 0, res = sv.api.results
        for (var i = 0; i < res.length; i++)
            if (res[i].pack === true) n++
        return n
    }
    readonly property var episodeTabs: {
        if (!sv || !sv.api || !sv.isSeriesDrill || sv.seasonFilter <= 0) return []
        var seen = {}, out = []
        var res = sv.api.results
        for (var i = 0; i < res.length; i++) {
            var r = res[i]
            if (r.season === sv.seasonFilter && (r.episode || 0) > 0 && !seen[r.episode]) {
                seen[r.episode] = true; out.push(r.episode)
            }
        }
        out.sort(function (a, b) { return a - b })
        return out
    }

    readonly property double saveFree: (typeof session !== "undefined" && session.diskVolumes
        && session.diskVolumes.length > 0) ? (session.diskVolumes[0].freeBytes || 0) : -1
    readonly property int wontFit: {
        if (saveFree < 0 || !sv || !sv.api) return 0
        var n = 0, res = sv.api.results
        for (var i = 0; i < res.length; ++i)
            if ((res[i].sizeBytes || 0) > saveFree) ++n
        return n
    }

    function computeView() {
        if (!sv || !sv.api) return []
        var arr = []
        var res = sv.api.results
        var qsets = sv.api.queryWordSets()
        for (var i = 0; i < res.length; i++) {
            var o = res[i]; o._idx = i
            o._rel = sv.api.relevanceMulti(o.name, qsets)
            arr.push(o)
        }
        if (sv.isEpisodes) {
            if (sv.seasonFilter > 0) arr = arr.filter(function (r) { return r.season === sv.seasonFilter })
            arr.sort(function (a, b) { return a._idx - b._idx })
            return arr
        }
        if (sv.isSeriesDrill) {
            if (sv.seasonFilter === -1) arr = arr.filter(function (r) { return r.pack === true })
            else if (sv.seasonFilter > 0) arr = arr.filter(function (r) {
                if (r.pack === true) return r.season === sv.seasonFilter || (r.season || -1) < 0
                if (sv.episodeFilter > 0) return r.season === sv.seasonFilter && r.episode === sv.episodeFilter
                return r.season === sv.seasonFilter
            })
        }
        if (sv.qualityFilter !== "") arr = arr.filter(function (r) { return r.quality === sv.qualityFilter })
        if (sv.sourceFilter !== "") arr = arr.filter(function (r) { return r.source === sv.sourceFilter })
        if (sv.groupFilter !== "" && !sv.showCatalogBrowse) arr = arr.filter(function (r) { return r.releaseGroup === sv.groupFilter })
        if (sv.providerFilter !== "") arr = arr.filter(function (r) { return r.provider === sv.providerFilter })
        if (sv.langFilter !== "") arr = arr.filter(function (r) { return (r.langs || []).indexOf(sv.langFilter) !== -1 })
        if (sv.audioModeFilter !== "") arr = arr.filter(function (r) { return (r.audioMode || "original") === sv.audioModeFilter })
        if (sv.minSeeds > 0) arr = arr.filter(function (r) { return (r.seedsN || 0) >= sv.minSeeds })
        if (sv.sortKey === "seeders") arr.sort(function (a, b) { return (b.seedsN || 0) - (a.seedsN || 0) })
        else if (sv.sortKey === "size") arr.sort(function (a, b) { return (b.sizeBytes || 0) - (a.sizeBytes || 0) })
        else if (sv.sortKey === "name") arr.sort(function (a, b) { return (a.name || "").localeCompare(b.name || "") })
        else if (sv.sortKey === "version") arr.sort(function (a, b) { return sv.api ? sv.api.compareBuildVersions(b.version || "", a.version || "") : 0 })
        else {
            var gameRank = sv.api && (sv.api.workType === "game" || sv.api.mode === "games")
            if (gameRank) {
                arr.sort(function (a, b) {
                    var g = sv.api.compareGameReleases(a, b); if (g) return -g
                    var rd = (b._rel || 0) - (a._rel || 0); if (rd) return rd
                    return a._idx - b._idx
                })
            } else {
                var nativeFirst = sv.api && sv.api.singleTitleView
                                  && (typeof settings === "undefined" || settings.get("preferNativeLang") !== false)
                function langRank(r) {
                    switch (r.audioMode) {
                    case "dub": return 2
                    case "sub": return 1
                    default:    return 0
                    }
                }
                arr.sort(function (a, b) {
                    if (nativeFirst) {
                        var l0 = langRank(b) - langRank(a); if (l0) return l0
                    }
                    var rd = (b._rel || 0) - (a._rel || 0); if (rd) return rd
                    var nd = (b.native ? 1 : 0) - (a.native ? 1 : 0); if (nd) return nd
                    var sd = (b.seedsN || 0) - (a.seedsN || 0); if (sd) return sd
                    return a._idx - b._idx
                })
            }
        }
        return arr
    }

    // re-evaluates when computeView() reads sv.api.results / filter props
    property var viewModel: computeView()
}
