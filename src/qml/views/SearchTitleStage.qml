// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A picked title, in search: its art and identity, and beside them what can
// be got — a series' episodes, a film's or a game's releases. The flat table
// it replaced is still one click away, under "All releases".
import QtQuick
import "../theme"
import "../widgets"

TitleStage {
    id: root
    required property var sv

    readonly property var api: sv.api
    readonly property string kind: api ? (api.workType || "") : ""

    FindTitleLogic { id: logic }

    onKindChanged: if (api) api.fetchWorkStills()
    Connections {
        target: root.api
        ignoreUnknownSignals: true
        function onWorkChanged() { if (root.api) root.api.fetchWorkStills() }
    }

    title: api ? (api.workTitle || "") : ""
    artUrl: api ? sv.heroArt(api.workBackdrop || (api.workStills.length > 0 ? api.workStills[0] : ""),
                             api.workPoster) : ""
    logoUrl: api ? (api.workLogo || "") : ""
    rating: api ? (api.workRating || 0) : 0
    facts: {
        if (!api) return []
        var out = []
        if ((api.workYear || "").length > 0) out.push(api.workYear)
        if (kind === "series" && api.workSeasonCount > 0)
            out.push((i18n.language, i18n.t(api.workSeasonCount === 1 ? "hub_n_season" : "hub_n_seasons"))
                     .arg(api.workSeasonCount))
        else if (kind === "game" && (api.workMaker || "").length > 0)
            out.push(api.workMaker)
        else if (kind === "movie")
            out.push(sv.typeLabel("movie"))
        return out
    }
    badge: {
        if (kind === "game") return "PC"
        var q = sv.qualityOptions
        return q.indexOf("4K") >= 0 ? "4K" : (q.length > 0 ? "HD" : "")
    }
    summary: api ? (api.workOverview || "") : ""
    cast: kind === "series" && episodes.seasonCast.length > 0 ? episodes.seasonCast : (api ? api.workCast : [])
    genres: api ? api.workGenres : []

    // The player asks for the episode after the one it is showing. Only for a
    // torrent taken from this page: another show could be playing.
    function nextFor(hash, fileIdx) {
        if (kind !== "series" || !api || !sv.addedHashes[(hash || "").toLowerCase()]) return null
        if (typeof session === "undefined") return null
        var at = session.episodeOf(hash, fileIdx)
        if (!(at.season >= 0 && at.episode > 0)) return null
        return logic.nextEpisode(api.results, at.season, at.episode, episodes.pref)
    }
    function playNext(next) {
        if (!next || !api) return
        if (next.release) { api.addAndWatch(next.release._idx); return }
        episodes.season = next.season
        episodes.episode = next.episode
    }

    SearchEpisodesPanel {
        id: episodes
        anchors.fill: parent
        visible: root.kind === "series"
        sv: root.sv
        logic: logic
    }
    SearchReleasesPanel {
        anchors.fill: parent
        visible: root.kind !== "series"
        isGame: root.kind === "game"
        sv: root.sv
        logic: logic
    }
}
