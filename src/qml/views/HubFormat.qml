// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "../theme"

// Pure-ish Hub formatting / state labels. Lives next to HubView so cards and
// menus share one implementation without bloating the page root.
QtObject {
    id: root
    required property var page   // HubView: needs i18n + api for folder open

    function fmtPlaytime(secs) {
        if (!secs || secs <= 0) return ""
        var h = Math.floor(secs / 3600), m = Math.floor((secs % 3600) / 60)
        return h > 0 ? (h + "h " + m + "m") : (m + "m")
    }
    function fmtLeft(ms) {
        if (!ms || ms <= 0) return ""
        var s = Math.floor(ms / 1000), h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60)
        return i18n.t("hub_time_left").replace("%1", h > 0 ? (h + "h " + m + "m") : (m + "m"))
    }
    function episodeLabel(item) {
        if (!item || !item.isSeries || !item.videos) return ""
        for (var i = 0; i < item.videos.length; i++) {
            var v = item.videos[i]
            if (v.idx === item.fileIndex && v.code)
                return v.code
        }
        return ""
    }
    function fmtSize(b) {
        if (!b || b <= 0) return ""
        var u = ["B", "KB", "MB", "GB", "TB"]
        var i = Math.min(u.length - 1, Math.floor(Math.log(b) / Math.log(1024)))
        // the language's decimal mark: "39,9 GB" in Portuguese
        return Number(b / Math.pow(1024, i)).toLocaleString(typeof i18n !== "undefined" && i18n.localeName ? Qt.locale(i18n.localeName) : Qt.locale("C"), "f", i >= 3 ? 1 : 0) + " " + u[i]
    }
    function fmtTime(ms) {
        if (!ms || ms <= 0) return ""
        var s = Math.floor(ms / 1000), h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), ss = s % 60
        function pad(n) { return (n < 10 ? "0" : "") + n }
        return (h > 0 ? h + ":" + pad(m) : m + "") + ":" + pad(ss)
    }
    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        if (p.indexOf("http") === 0 || p.indexOf("qrc") === 0 || p.indexOf("file:") === 0)
            return p
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }
    function fmtAgo(ms) {
        if (!ms || ms <= 0) return ""
        var h = Math.floor((Date.now() - ms) / 3600000)
        if (h < 24) return h < 1 ? i18n.t("hub_today") : i18n.t("hub_hours_ago").replace("%1", h)
        var d = Math.floor(h / 24)
        if (d === 1) return i18n.t("hub_yesterday")
        return i18n.t("hub_days_ago").replace("%1", d)
    }
    function gameStateLabel(item) {
        if (!item) return ""
        switch (item.installState) {
        case 0: return "↓ " + Math.floor((item.progress || 0) * 100) + "%"
        case 1: return i18n.t("hub_gs_install")
        case 2: return (item.extractProgress >= 0)
                       ? i18n.t("hub_gs_extracting") + " " + item.extractProgress + "%"
                       : i18n.t("hub_gs_extracting")
        // A repack's own window or a silent GOG setup: either way it is installing.
        // "Finish setup" read as stuck while nothing was on screen to finish.
        case 3: return (item.installProgress >= 0)
                       ? i18n.t("hub_gs_installing") + " " + item.installProgress + "%"
                       : i18n.t("hub_gs_installing")
        case 4: return i18n.t("hub_gs_play")
        case 5: return i18n.t("hub_gs_playing")
        case 6: return i18n.t("hub_gs_setup")
        case 7: return i18n.t("hub_gs_retry")
        }
        return ""
    }
    function gameStateActionable(item) {
        if (!item) return false
        var s = item.installState
        return s === 1 || s === 4 || s === 6 || s === 7
    }
    function cardStatus(item, isGame) {
        if (!item) return ""
        if (item.playing === true) return i18n.t("hub_playing_now")
        var sz = root.fmtSize(item.size || 0)
        var dot = sz ? ("  ·  " + sz) : ""
        if (isGame) {
            switch (item.installState) {
            case 0: return "↓ " + Math.floor((item.progress || 0) * 100) + "%" + dot
            case 4: return i18n.t("hub_ready_to_play") + dot
            case 1: return i18n.t("hub_installed") + dot
            default: return root.gameStateLabel(item) + dot
            }
        }
        return item.completed ? (i18n.t("hub_installed") + dot)
                              : ("↓ " + Math.floor((item.progress || 0) * 100) + "%" + dot)
    }

    // Playtime. Hours with one decimal read "0.0 h" for anything under three
    // minutes, which looked like nothing was counted: minutes until the first hour.
    function hours(secs) {
        if (!(secs > 0)) return i18n.t("hub_play_minutes").arg(0)
        if (secs < 60) return i18n.t("hub_play_under_minute")
        if (secs < 3600) return i18n.t("hub_play_minutes").arg(Math.floor(secs / 60))
        return i18n.t("hub_play_hours").arg((secs / 3600).toFixed(1))
    }

    // What a library tile says under its art: a state in its own colour, a
    // quieter fact beside it, and the bar along the art's bottom edge (-1 = none).
    // `kind` is "game", "series" or "movie".
    function tileState(item, kind) {
        var out = { state: "", ink: Theme.t2, meta: "", bar: -1, barColor: Theme.down, dim: false }
        if (!item) return out
        var size = root.fmtSize(item.size || 0)
        var pct = Math.floor((item.progress || 0) * 100)
        var downloading = kind === "game" ? item.installState === 0 : !item.completed
        if (downloading) {
            out.bar = item.progress || 0
            out.meta = size
            if (item.paused) {
                out.state = i18n.t("hub_tile_paused").arg(pct)
                out.ink = Theme.t4
                out.barColor = Theme.t4
            } else {
                out.state = i18n.t("hub_tile_downloading").arg(pct)
                out.ink = Theme.accentText
            }
            return out
        }
        if (kind === "game") {
            out.meta = size
            if (item.playing) { out.state = i18n.t("hub_playing_now"); out.ink = Theme.grn }
            else if (item.installState === 4) {
                out.state = i18n.t("hub_installed")
                if ((item.playSeconds || 0) > 0) out.meta = i18n.t("hub_tile_played").arg(root.hours(item.playSeconds))
            } else if (item.installState === 1) {
                // the setup is on disk: ready, said plainly, with Install on the art
                out.state = i18n.t("hub_ready_to_install")
                out.ink = Theme.t2
                out.dim = true
            } else {
                out.state = root.gameStateLabel(item)
                if (item.installState === 2 && item.extractProgress >= 0) {
                    out.bar = item.extractProgress / 100
                    out.ink = Theme.accentText
                }
            }
            return out
        }
        if (kind === "series") {
            var n = (item.seasons || []).length
            out.meta = n > 0 ? i18n.t(n === 1 ? "hub_n_season" : "hub_n_seasons").arg(n) : size
            // A show is a group of torrents: the episode under way is in its videos.
            var vids = item.videos || []
            var cur = null
            for (var i = 0; i < vids.length; i++) {
                var w = vids[i].watchedPct || 0
                if (w > 0 && w < 0.9 && !vids[i].watched) { cur = vids[i]; break }
            }
            if (cur) {
                out.state = "S" + cur.season + "E" + (cur.episode < 10 ? "0" : "") + cur.episode
                          + " · " + Math.round(cur.watchedPct * 100) + "%"
                out.bar = cur.watchedPct
            } else out.state = i18n.t("hub_tile_in_library")
            return out
        }
        out.meta = size
        var watched = item.watchedPct || 0
        if (watched >= 0.9) {
            out.state = i18n.t("hub_tile_watched")
            out.ink = Theme.t4
            out.bar = 1
            out.barColor = Qt.rgba(1, 1, 1, 0.5)
        } else if (watched > 0) {
            out.state = root.fmtLeft((item.durMs || 0) - (item.resumeMs || 0)) || (Math.round(watched * 100) + "%")
            out.bar = watched
        } else out.state = i18n.t("hub_tile_in_library")
        return out
    }
}
