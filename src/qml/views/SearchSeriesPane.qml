// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A series, in search, on the screen the library already uses for one. The
// only thing that differs is what "have" means: on disk there, offered by an
// indexer here. Same seasons, same episode rows, same answer to the same
// question.
//
// The season tabs this replaces were built from the releases that came back,
// so a season nobody had simply did not exist as far as the screen was
// concerned. TMDB's list shows every one and marks which can be got.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: root
    required property var sv

    property int season: -1
    property int episode: -1          // -1 = the episode list; otherwise its sources
    property var tmdbRows: []

    readonly property var work: sv.api
    readonly property int tmdbId: sv.api ? (sv.api.workTmdbId || 0) : 0
    readonly property var seasons: sv.seasonTabs

    SeriesLogic { id: logic }

    // What can actually be got for the season on screen, shaped like the videos
    // the library screen takes so the same merge serves both.
    //
    // A season pack is the reason this is not just "results that name an
    // episode". Most of a season arrives inside one, so counting only the
    // releases with SxxExx in the name marked six of seven episodes as having
    // no source while a pack holding all of them sat in the same result list.
    readonly property var offered: {
        var out = []
        var direct = ({})
        var covered = false
        var res = (sv.api && sv.api.results) ? sv.api.results : []
        for (var i = 0; i < res.length; i++) {
            var r = res[i]
            if (r.season !== root.season) continue
            if (r.episode > 0) {
                if (direct[r.episode]) continue
                direct[r.episode] = true
                out.push({ hash: "", idx: -1, name: r.name,
                           season: r.season, episode: r.episode,
                           watched: false, progress: 1 })
            } else {
                covered = true          // a pack: every episode of the season
            }
        }
        if (covered) {
            for (var t = 0; t < tmdbRows.length; t++) {
                var ep = tmdbRows[t].episode
                if (ep > 0 && !direct[ep])
                    out.push({ hash: "", idx: -1, name: "", season: root.season,
                               episode: ep, watched: false, progress: 1 })
            }
        }
        return out
    }
    readonly property var episodes: logic.mergeSeasonEpisodes(tmdbRows, offered, season)

    // Asking once at creation was asking before a title had been picked: the
    // pane outlives the work, so the trigger is the work arriving.
    onTmdbIdChanged: if (tmdbId > 0 && sv.api) sv.api.fetchWorkStills()
    onSeasonsChanged: if (season < 0 && seasons.length > 0) season = seasons[0]
    onSeasonChanged: { episode = -1; requestSeason() }
    function requestSeason() {
        tmdbRows = []
        if (tmdbId > 0 && sv.disco && season >= 0) sv.disco.fetchEpisodes(tmdbId, season)
    }
    Connections {
        target: root.sv.disco
        ignoreUnknownSignals: true
        function onEpisodesReady(id, s, eps) {
            if (id !== root.tmdbId || s !== root.season) return
            root.tmdbRows = eps
        }
    }

    Rectangle {
        id: panel
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Math.min(460, Math.max(330, root.width * 0.4))
        color: Qt.rgba(0, 0, 0, 0.35)
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.hair }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            SeasonPicker {
                Layout.fillWidth: true
                visible: root.episode < 0
                seasons: root.seasons
                season: root.season
                // No tally here. In the library the denominator is the number
                // of episodes a season has; in a search it would be the number
                // of releases that happen to mention it, and "Season 8 95/95"
                // is a number that means nothing.
                onSeasonPicked: function (s) { root.season = s }
            }

            ListView {
                id: epList
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.episode < 0
                clip: true
                model: root.episodes
                boundsBehavior: Flickable.StopAtBounds
                WheelScroller { flick: epList }
                delegate: EpisodeRow {
                    required property var modelData
                    width: epList.width
                    ep: modelData
                    haveLabel: (i18n.language, i18n.t("find_ep_available"))
                    missingLabel: (i18n.language, i18n.t("find_ep_no_source"))
                    onPlayRequested: {
                        root.episode = modelData.episode
                        root.sv.episodeFilter = modelData.episode
                        root.sv.seasonFilter = root.season
                    }
                }
            }

            // The sources for the chosen episode: the list that was always
            // here, now reached by picking the episode instead of reading
            // season markers off release names.
            SearchListPane {
                Layout.fillWidth: true
                Layout.fillHeight: true
                visible: root.episode >= 0
                sv: root.sv
            }
        }
    }

    WorkHero {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: panel.left
        title: root.sv.api ? (root.sv.api.workTitle || "") : ""
        artUrl: root.sv.api
            ? root.sv.heroArt(root.sv.api.workBackdrop, root.sv.api.workPoster) : ""
        subtitle: {
            if (!root.sv.api) return ""
            var parts = []
            if ((root.sv.api.workYear || "").length > 0) parts.push(root.sv.api.workYear)
            if (root.sv.api.workRating > 0) parts.push(root.sv.api.workRating.toFixed(1))
            if (root.season >= 0) parts.push((i18n.language, i18n.t("hub_season_n")).arg(root.season))
            return parts.join("  ·  ")
        }
        logoUrl: root.sv.api ? (root.sv.api.workLogo || "") : ""
        summary: root.sv.api ? (root.sv.api.workOverview || "") : ""
        showBack: root.episode >= 0
        onBackRequested: { root.episode = -1; root.sv.episodeFilter = -1 }
    }
}
