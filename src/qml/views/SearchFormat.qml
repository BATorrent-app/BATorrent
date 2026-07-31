// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Shared Find-page formatters (seed health, sizes, file URLs, lang labels).
// Mirrors HubFormat — keeps SearchView composition-only.
import QtQuick
import "../theme"

QtObject {
    id: root

    readonly property var langNames: ({
        "PT": "Português", "EN": "English", "ES": "Español", "DE": "Deutsch",
        "IT": "Italiano", "FR": "Français", "RU": "Русский", "JA": "日本語",
        "UK": "Українська", "ZH": "中文", "KO": "한국어", "HI": "हिन्दी", "MULTI": "Multi"
    })

    function langName(c) { return langNames[c] || c }

    function typeLabel(t) {
        if (t === "movie") return i18n.t("search_type_movie")
        if (t === "series") return i18n.t("search_type_series")
        if (t === "game") return i18n.t("search_type_game")
        return ""
    }

    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        if (p.indexOf("http") === 0 || p.indexOf("qrc") === 0 || p.indexOf("file:") === 0)
            return p
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }

    function seedColor(n) {
        return n >= 50 ? Theme.grn : (n >= 10 ? "#e0a533" : (n >= 1 ? "#d97640" : Theme.t4))
    }
    function seedFill(n) {
        return n <= 0 ? 0 : Math.min(1, Math.log(n + 1) / Math.log(500))
    }
    function fmtSize(b) {
        if (!b || b <= 0) return "0 B"
        var u = ["B", "KB", "MB", "GB", "TB"]
        var i = Math.min(u.length - 1, Math.floor(Math.log(b) / Math.log(1024)))
        return (b / Math.pow(1024, i)).toFixed(i >= 3 ? 1 : 0) + " " + u[i]
    }
    function fmtCount(n) {
        return n >= 1000 ? (n / 1000).toFixed(1).replace(/\.0$/, "") + "k" : String(n)
    }
}
