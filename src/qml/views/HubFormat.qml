// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

// Pure-ish Hub formatting / state labels. Lives next to HubView so cards and
// menus share one implementation without bloating the page root.
QtObject {
    id: root
    required property var page   // HubView — needs i18n + api for folder open

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
            if (v.idx === item.fileIndex && (v.season > 0 || v.episode > 0))
                return "S" + v.season + "E" + (v.episode < 10 ? "0" + v.episode : v.episode)
        }
        return ""
    }
    function fmtSize(b) {
        if (!b || b <= 0) return ""
        var u = ["B", "KB", "MB", "GB", "TB"]
        var i = Math.min(u.length - 1, Math.floor(Math.log(b) / Math.log(1024)))
        return (b / Math.pow(1024, i)).toFixed(i >= 3 ? 1 : 0) + " " + u[i]
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
        case 2: return i18n.t("hub_gs_extracting")
        case 3: return i18n.t("hub_gs_finish_setup")
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
}
