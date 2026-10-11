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
                            members: [], videos: [], seasons: [],
                            size: 0, addedTime: 0, completed: true, paused: false, progress: 1 }
                order.push(k)
            }
            var g = byId[k]
            g.members.push(it.infoHash)
            // The group reads as its least finished member: one season still
            // downloading means the show is still downloading.
            g.size += it.size || 0
            g.addedTime = Math.max(g.addedTime, it.addedTime || 0)
            if (!it.completed) {
                g.completed = false
                g.progress = Math.min(g.progress, it.progress || 0)
                if (it.paused) g.paused = true
            }
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
                                watchedPct: src.watchedPct || 0,
                                resumeAt: src.resumeAt || 0,
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

    // Where a show picks up: the episode last played, the next one on disk if
    // that was finished, the first on disk if nothing was played. null if none.
    function resumeEpisode(groupVideos) {
        var vids = (groupVideos || []).filter(function (v) { return v.season >= 0 && v.episode >= 0 })
        if (vids.length === 0) return null
        vids.sort(function (a, b) { return a.season !== b.season ? a.season - b.season : a.episode - b.episode })
        var last = null
        for (var i = 0; i < vids.length; i++)
            if ((vids[i].resumeAt || 0) > 0 && (!last || vids[i].resumeAt > last.resumeAt)) last = vids[i]
        if (!last) return vids[0]
        if (last.watched !== true) return last
        var at = vids.indexOf(last)
        return at + 1 < vids.length ? vids[at + 1] : last
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
