// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// What a series is, independent of where you are looking at it from.
//
// Grouping torrents into shows, crossing a season against what is on disk,
// counting what a season actually holds: the library asks these questions of
// its own torrents and a search asks them of a title it found, and the answers
// must not differ. Pure functions over plain lists, so both can call them and
// a test can call them with neither.
import QtQuick

QtObject {
    id: root

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

}
