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
    function qualityFromName(n) {
        n = ("" + n).toLowerCase()
        if (/2160p|\buhd\b|\b4k\b/.test(n)) return "4K"
        if (/1080p/.test(n)) return "1080p"
        if (/720p/.test(n))  return "720p"
        if (/480p/.test(n))  return "480p"
        return ""
    }
    function audioFromName(n) {
        n = ("" + n).toLowerCase()
        if (/7\.1/.test(n)) return "7.1"
        if (/5\.1/.test(n)) return "5.1"
        if (/atmos/.test(n)) return "Atmos"
        if (/\b2\.0\b|stereo|aac2/.test(n)) return "2.0"
        return ""
    }
    // External sidecar cue lookup. Returns { idx, text }.
    function cueAt(cues, idx, rawPos, offsetMs) {
        if (!cues || cues.length === 0)
            return { idx: idx, text: "" }
        var pos = rawPos - (offsetMs || 0)
        var i = idx
        if (i >= 0 && i < cues.length) {
            var c = cues[i]
            if (pos >= c.start && pos <= c.end)
                return { idx: i, text: c.text }
            if (pos > c.end && i + 1 < cues.length && pos < cues[i + 1].start)
                return { idx: i, text: "" }
        }
        var lo = 0, hi = cues.length - 1, found = -1
        while (lo <= hi) {
            var mid = (lo + hi) >> 1
            if (cues[mid].start <= pos) { found = mid; lo = mid + 1 } else hi = mid - 1
        }
        return {
            idx: found,
            text: (found >= 0 && pos <= cues[found].end) ? cues[found].text : ""
        }
    }
}
