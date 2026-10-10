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

    // Every release that names a season and an episode, shaped like the videos
    // the library screen takes, so the same merge serves both.
    readonly property var offered: {
        var out = []
        var res = (sv.api && sv.api.results) ? sv.api.results : []
        for (var i = 0; i < res.length; i++) {
            var r = res[i]
            if (!(r.season > 0 && r.episode > 0)) continue
            out.push({ hash: "", idx: -1, name: r.name,
                       season: r.season, episode: r.episode,
                       watched: false, progress: 1 })
        }
        return out
    }
    readonly property var episodes: logic.mergeSeasonEpisodes(tmdbRows, offered, season)
    readonly property var seasonCounts: {
        var out = ({})
        for (var i = 0; i < seasons.length; i++) {
            var sn = seasons[i], total = 0
            for (var v = 0; v < offered.length; v++) if (offered[v].season === sn) ++total
            out[sn] = { have: total, total: total }
        }
        return out
    }

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
                counts: root.seasonCounts
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
        artUrl: {
            if (!root.sv.api) return ""
            var b = root.sv.api.workBackdrop || ""
            return b.length > 0 ? b : root.sv.fileUrl(root.sv.api.workPoster || "")
        }
        subtitle: {
            if (!root.sv.api) return ""
            var parts = []
            if ((root.sv.api.workYear || "").length > 0) parts.push(root.sv.api.workYear)
            if (root.sv.api.workRating > 0) parts.push(root.sv.api.workRating.toFixed(1))
            if (root.season >= 0) parts.push((i18n.language, i18n.t("hub_season_n")).arg(root.season))
            return parts.join("  ·  ")
        }
        summary: root.sv.api ? (root.sv.api.workOverview || "") : ""
        showBack: root.episode >= 0
        onBackRequested: { root.episode = -1; root.sv.episodeFilter = -1 }
    }
}
