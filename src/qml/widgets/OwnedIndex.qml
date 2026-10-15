// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// What the person already has of a title the catalogs show: the HUB's
// libraries, looked up by TMDB id (films and shows) or by name (games). The
// Find page reads it to say "S01 · 1 of 8 watched" or "Watched" under a tile,
// and "In your library · S1 E2 is next" on the best match.
import QtQuick

QtObject {
    id: idx
    readonly property var api: typeof session !== "undefined" ? session : null
    property var shows: ({})    // "t<tmdbId>" or "n<title>" → [library items]
    property var games: ({})    // lower-case title → game item
    property int gen: 0         // bumps on every rebuild, for bindings to follow

    function key(s) { return String(s || "").toLowerCase().replace(/[^a-z0-9]+/g, " ").trim() }

    function refresh() {
        if (!api) return
        var s = ({}), g = ({})
        var lib = api.movieLibrary()
        for (var i = 0; i < lib.length; i++) {
            var m = lib[i]
            var k = m.tmdbId ? "t" + m.tmdbId : "n" + key(m.title)
            if (!s[k]) s[k] = []
            s[k].push(m)
        }
        var gl = api.gameLibrary()
        for (var j = 0; j < gl.length; j++) g[key(gl[j].title)] = gl[j]
        shows = s
        games = g
        gen++
    }

    // { text, next } or null when the title is not in the library.
    // next: { season, episode } of the episode to watch, for a show.
    function stateOf(it) {
        if (!it) return null
        var _ = gen
        var type = it.type || ""
        if (type === "game") {
            var gm = games[key(it.title || it.name)]
            if (!gm) return null
            return { text: i18n.t(gm.installState === 4 ? "hub_installed" : "hub_tile_in_library"), next: null }
        }
        var list = (it.tmdbId ? shows["t" + it.tmdbId] : null) || shows["n" + key(it.title || it.name)]
        if (!list || list.length === 0) return null
        if (type === "series" || list[0].isSeries) {
            // every episode of the show across its torrents, once each
            var seen = ({}), eps = []
            for (var a = 0; a < list.length; a++) {
                var vids = list[a].videos || []
                for (var b = 0; b < vids.length; b++) {
                    var v = vids[b]
                    if (!(v.season >= 0) || !(v.episode > 0)) continue
                    var c = v.season + ":" + v.episode
                    if (seen[c]) continue
                    seen[c] = true
                    eps.push(v)
                }
            }
            eps.sort(function (x, y) { return x.season - y.season || x.episode - y.episode })
            var done = eps.filter(function (v) { return v.watched || (v.watchedPct || 0) >= 0.9 })
            var next = null
            for (var e = 0; e < eps.length; e++)
                if (!(eps[e].watched || (eps[e].watchedPct || 0) >= 0.9)) { next = eps[e]; break }
            if (eps.length === 0 || done.length === 0)
                return { text: i18n.t("hub_tile_in_library"), next: next ? { season: next.season, episode: next.episode } : null }
            if (!next) return { text: i18n.t("hub_tile_watched"), next: null }
            var season = eps.filter(function (v) { return v.season === next.season })
            var seasonDone = season.filter(function (v) { return v.watched || (v.watchedPct || 0) >= 0.9 }).length
            return { text: i18n.t("find_state_series").arg(pad(next.season)).arg(seasonDone).arg(season.length),
                     next: { season: next.season, episode: next.episode } }
        }
        var mv = list[0]
        var w = mv.watchedPct || 0
        if (w >= 0.9) return { text: i18n.t("hub_tile_watched"), next: null }
        if (w > 0) {
            var left = Math.max(0, (mv.durMs || 0) - (mv.resumeMs || 0))
            var mins = Math.round(left / 60000), h = Math.floor(mins / 60)
            return { text: i18n.t("hub_time_left").replace("%1", h > 0 ? (h + "h " + (mins % 60) + "m") : (mins + "m")), next: null }
        }
        return { text: i18n.t("hub_tile_in_library"), next: null }
    }
    function pad(n) { return (n < 10 ? "0" : "") + n }
}
