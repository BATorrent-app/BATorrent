// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

QtObject {
    function fmt(ms) {
        if (ms <= 0) return "0:00"
        var s = Math.floor(ms / 1000), h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), ss = s % 60
        var p = function(n){ return (n < 10 ? "0" : "") + n }
        return (h > 0 ? h + ":" + p(m) : m) + ":" + p(ss)
    }
    function fmtBytes(b) {
        if (!b || b <= 0) return "0 MB"
        var u = ["KB", "MB", "GB", "TB"], i = -1
        do { b /= 1024; i++ } while (b >= 1024 && i < u.length - 1)
        return b.toFixed(b < 10 ? 1 : 0) + " " + u[i]
    }
    function fmtAhead(ms) {
        var s = Math.floor(ms / 1000)
        if (s >= 3600) return Math.floor(s / 3600) + "h " + Math.floor((s % 3600) / 60) + "m buffered ahead"
        if (s >= 60)   return Math.floor(s / 60) + " min buffered ahead"
        return s + "s buffered ahead"
    }
    function fmtRunway(ms) {
        var s = Math.floor(ms / 1000)
        if (s >= 3600) return Math.floor(s / 3600) + "h " + Math.floor((s % 3600) / 60) + "m"
        if (s >= 60)   return Math.floor(s / 60) + " min"
        return s + "s"
    }
    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        if (/^(file|qrc|https?|image):/.test(p)) return p
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }
}
