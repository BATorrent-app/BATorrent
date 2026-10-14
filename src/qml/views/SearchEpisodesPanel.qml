// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A series' episodes beside its art. TMDB says what the season holds, the
// indexers say what can be got, the library says what is already here; each
// row answers all three. Picking an episode swaps the list for its sources.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: panel
    required property var sv
    required property var logic

    property int season: -1
    property int episode: -1            // -1 = the episode list
    property bool allReleases: false
    property string pref: logic.preferredQuality(
        typeof settings !== "undefined" ? parseInt(settings.get("preferredQuality") || 1) : 1)
    property var tmdbRows: []
    property var seasonCast: []
    property var library: ({})

    readonly property int tmdbId: sv.api ? (sv.api.workTmdbId || 0) : 0
    readonly property var seasons: {
        var seen = ({}), out = []
        var n = sv.api ? (sv.api.workSeasonCount || 0) : 0
        for (var s = 1; s <= n; s++) { seen[s] = true; out.push(s) }
        var tabs = sv.seasonTabs
        for (var i = 0; i < tabs.length; i++) if (!seen[tabs[i]]) out.push(tabs[i])
        return out.sort(function (a, b) { return a - b })
    }
    readonly property var index: {
        var t0 = Date.now()
        var idx = logic.seasonIndex(sv.results, season)
        console.debug("[find] seasonIndex " + sv.results.length + " rows " + (Date.now() - t0) + " ms")
        return idx
    }
    // The list only takes new rows while it is the one on screen: a new array
    // rebuilds every delegate, stills included, and with an episode's sources
    // open each addon reply did that to all of a season's rows unseen.
    property var episodes: []
    Binding {
        target: panel
        property: "episodes"
        when: panel.episode < 0
        restoreMode: Binding.RestoreNone
        value: {
            var t0 = Date.now()
            var rows = panel.logic.episodeRows(panel.tmdbRows, panel.index, panel.library, panel.season, panel.pref)
            console.debug("[find] episodeRows " + rows.length + " episodes " + (Date.now() - t0) + " ms")
            return rows
        }
    }

    SeriesLogic { id: series }
    function seasonLabel(s) {
        return s === 0 ? (i18n.language, i18n.t("hub_specials"))
                       : (i18n.language, i18n.t("hub_season_n")).arg(s)
    }
    function refreshLibrary() {
        var groups = typeof session !== "undefined" ? series.seriesGroups(session.movieLibrary()) : []
        library = logic.libraryEpisodes(groups, tmdbId)
    }

    onTmdbIdChanged: { season = -1; episode = -1; allReleases = false; refreshLibrary(); takePending() }
    onSeasonsChanged: if (season < 0 && seasons.length > 0) season = seasons[0]
    // Opened from the HUB: the season you are in, scrolled to the episode.
    property int focusEpisode: -1
    function takePending() {
        var p = sv.pendingEpisode
        if (!p || tmdbId <= 0 || p.tmdbId !== tmdbId) return
        sv.pendingEpisode = null
        season = p.season
        focusEpisode = p.episode
    }
    onEpisodesChanged: if (focusEpisode > 0) Qt.callLater(scrollToFocus)
    function scrollToFocus() {
        for (var i = 0; i < episodes.length; i++) {
            if (episodes[i].episode !== focusEpisode) continue
            epList.positionViewAtIndex(i, ListView.Center)
            focusEpisode = -1
            return
        }
    }
    // The page can open on a title that was already picked; no change signal then.
    Component.onCompleted: {
        refreshLibrary()
        takePending()
        if (season < 0 && seasons.length > 0) season = seasons[0]
    }
    onSeasonChanged: { episode = -1; requestSeason() }
    // The stream addons answer per episode (what Stremio asks), so they are
    // asked when one is opened, not for the whole season.
    onEpisodeChanged: if (episode > 0 && sv.api) sv.api.fetchEpisodeStreams(season, episode)
    onVisibleChanged: if (visible) refreshLibrary()
    function requestSeason() {
        tmdbRows = []
        seasonCast = []
        if (tmdbId > 0 && sv.disco && season >= 0) sv.disco.fetchEpisodes(tmdbId, season)
    }
    Connections {
        target: panel.sv.disco
        ignoreUnknownSignals: true
        function onEpisodesReady(id, s, eps) {
            if (id === panel.tmdbId && s === panel.season) panel.tmdbRows = eps
        }
        function onSeasonCastReady(id, s, cast) {
            if (id === panel.tmdbId && s === panel.season) panel.seasonCast = cast
        }
    }
    Connections {
        target: panel.sv.api
        ignoreUnknownSignals: true
        function onAddedTorrent() { libTimer.restart() }
    }
    // A torrent only becomes a library entry once some of it is on disk.
    Timer { id: libTimer; interval: 1500; onTriggered: panel.refreshLibrary() }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        visible: panel.episode < 0

        StageHeading {
            Layout.fillWidth: true
            title: (i18n.language, i18n.t("find_episodes"))
            note: {
                if (panel.sv.api && panel.sv.api.searching) return (i18n.language, i18n.t("search_searching2"))
                var bits = []
                var air = panel.tmdbRows.length > 0 ? (panel.tmdbRows[0].air_date || "") : ""
                if (air.length >= 4) bits.push(air.substring(0, 4))
                if (panel.tmdbRows.length > 0)
                    bits.push((i18n.language, i18n.t("search_episodes_n")).arg(panel.tmdbRows.length))
                return bits.join("  ·  ")
            }
            linkText: (i18n.language, i18n.t(panel.allReleases ? "find_simple_view" : "find_all_releases"))
            onLinkClicked: panel.allReleases = !panel.allReleases

            StageSelect {
                height: 34
                fontSize: 13
                options: panel.seasons.map(function (s) { return { value: s, label: panel.seasonLabel(s) } })
                value: panel.season
                onPicked: function (v) { panel.season = v }
            }
            noteControls: StageSelect {
                bare: true
                options: panel.logic.qualities.slice(0, 3).map(function (q) { return { value: q, label: q } })
                value: panel.pref
                onPicked: function (v) { panel.pref = v }
            }
        }

        ListView {
            id: epList
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: !panel.allReleases
            clip: true
            spacing: 2
            bottomMargin: 20
            boundsBehavior: Flickable.StopAtBounds
            model: panel.episodes
            WheelScroller { flick: epList }
            delegate: StageEpisodeRow {
                required property var modelData
                readonly property var status: panel.logic.episodeStatus(modelData)
                readonly property var pick: modelData.main || modelData.pack
                width: epList.width
                number: modelData.episode
                title: modelData.title.length > 0 ? modelData.title
                       : (i18n.language, i18n.t("hub_ep_n")).arg(modelData.episode)
                still: modelData.still
                offered: modelData.offered
                playable: modelData.lib !== null && (modelData.lib.progress || 0) > 0.02
                watched: modelData.lib !== null && modelData.lib.watched === true
                watchedPct: modelData.lib !== null ? (modelData.lib.watchedPct || 0) : 0
                canDownload: modelData.lib === null && pick !== null
                subLine: {
                    if (status.key.length === 0) return modelData.main.quality + "  ·  " + (modelData.main.sizeStr || "")
                    var line = (i18n.language, i18n.t(status.key))
                    return status.arg === "" ? line : line.arg(status.arg)
                }
                subColor: status.tone === "dim" ? Qt.rgba(1, 1, 1, 0.3) : Theme.stageMuted
                slowText: modelData.lib === null && panel.logic.isSlow(modelData.main)
                          ? (i18n.language, i18n.t("find_slow")) : ""
                onOpenRequested: panel.episode = modelData.episode
                onPlayRequested: if (typeof session !== "undefined")
                    session.playFile(modelData.lib.hash, modelData.lib.idx)
                onDownloadRequested: if (panel.sv.api) panel.sv.api.activateResult(pick._idx)
            }
        }

        SearchSeasonReleases {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: panel.allReleases
            sv: panel.sv
            index: panel.index
            season: panel.season
            titles: panel.tmdbRows
        }
    }

    SearchEpisodeSources {
        anchors.fill: parent
        visible: panel.episode >= 0
        sv: panel.sv
        logic: panel.logic
        index: panel.index
        season: panel.season
        episode: panel.episode
        title: {
            for (var i = 0; i < panel.tmdbRows.length; i++)
                if (panel.tmdbRows[i].episode === panel.episode) return panel.tmdbRows[i].name || ""
            return ""
        }
        // back where it was: the list may have been rebuilt meanwhile
        onBackRequested: { panel.focusEpisode = panel.episode; panel.episode = -1 }
    }
}
