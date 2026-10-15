// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// "just now", "12 min ago", "3 h ago", "yesterday", "5 days ago" from unix
// seconds, in the app's language. t is i18n.t, passed in because a library
// script has no context of its own.
.pragma library

function ago(secs, t) {
    if (!secs || secs <= 0) return ""
    var d = Math.max(0, Date.now() / 1000 - secs)
    if (d < 60) return t("time_now")
    if (d < 3600) return t("time_min").arg(Math.floor(d / 60))
    if (d < 86400) return t("time_h").arg(Math.floor(d / 3600))
    if (d < 2 * 86400) return t("time_yesterday")
    return t("time_days").arg(Math.floor(d / 86400))
}

// whole days since, for "today" and "yesterday" when the hour doesn't matter
function days(secs, t) {
    if (!secs || secs <= 0) return ""
    var n = Math.floor((Date.now() / 1000 - secs) / 86400)
    if (n <= 0) return t("time_today")
    if (n === 1) return t("time_yesterday")
    return t("time_days").arg(n)
}

// "Checked 12 seconds ago" style: seconds while under a minute
function secondsAgo(ms, t) {
    var s = Math.max(0, Math.floor((Date.now() - ms) / 1000))
    return s < 60 ? t("time_sec").arg(s) : ago(ms / 1000, t)
}

// "2026-10-09" in the app's language: long ("October 9, 2026") or month and year
function date(iso, localeName, full) {
    if (!iso) return ""
    var p = iso.split("-")
    var d = new Date(+p[0], +p[1] - 1, +p[2])
    var loc = Qt.locale(localeName)
    // 0 is Locale.LongFormat; the enum is not in scope in a library script
    return full ? d.toLocaleDateString(loc, 0).replace(/^[^,]*,\s*/, "")
                : d.toLocaleDateString(loc, "MMM yyyy")
}
