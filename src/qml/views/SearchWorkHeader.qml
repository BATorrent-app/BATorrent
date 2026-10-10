// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The picked work, at the top of its releases. The same WorkHero the library
// uses for a series: a drill-down and a thing you already own are the same
// question — what is this — and they were answering it with two different
// screens, one of them a 46px thumbnail.
//
// Everything here already arrived with the title row. It was simply never read.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

WorkHero {
    id: root
    required property var sv

    Layout.fillWidth: true
    Layout.preferredHeight: 230
    visible: !sv.browse && sv.api && sv.api.singleTitleView && !sv.isEpisodes
             && (sv.api.workTitle || "").length > 0

    title: sv.api ? (sv.api.workTitle || "") : ""
    // The backdrop when TMDB had one, the poster when it did not. Cropped
    // either way, so a portrait poster fills the band instead of sitting in it.
    artUrl: sv.api ? sv.heroArt(sv.api.workBackdrop, sv.api.workPoster) : ""
    subtitle: {
        if (!sv.api) return ""
        var parts = []
        if ((sv.api.workYear || "").length > 0) parts.push(sv.api.workYear)
        var t = sv.typeLabel(sv.api.workType || "")
        if (t.length > 0) parts.push(t)
        if (sv.api.workRating > 0) parts.push(sv.api.workRating.toFixed(1))
        return parts.join("  ·  ")
    }
    logoUrl: sv.api ? (sv.api.workLogo || "") : ""
    summary: sv.api ? (sv.api.workOverview || "") : ""
    showBack: false
}
