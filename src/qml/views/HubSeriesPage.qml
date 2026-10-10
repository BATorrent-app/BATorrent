// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Full-surface screen for one series: backdrop, synopsis, a season picker and
// the episodes of the chosen season. Not the 420px detail drawer — a show is
// the thing you browse, and browsing it inside a side panel is what made the
// episode popup feel like a file list instead of a library.
//
// Episodes come from HubCompute.mergeSeasonEpisodes: TMDB's list of the season
// crossed with what is on disk, so a missing middle episode is visible as
// missing rather than silently absent.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: root
    property var hub
    readonly property var show: hub.seriesItem
    property int season: -1
    property var episodes: []

    visible: hub.seriesOpen || slide > 0.001
    property real slide: hub.seriesOpen ? 1 : 0
    Behavior on slide { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
    opacity: slide

    // Opening on whichever season the show starts with, and asking TMDB for it.
    onShowChanged: {
        if (!show) { episodes = []; return }
        season = (show.seasons && show.seasons.length > 0) ? show.seasons[0] : -1
        requestSeason()
    }
    onSeasonChanged: requestSeason()
    function requestSeason() {
        episodes = rebuild([])
        if (show && show.tmdbId > 0 && hub.api && season >= 0)
            hub.api.fetchEpisodes(show.tmdbId, season)
    }
    function rebuild(tmdbRows) {
        if (!show) return []
        return hub.mergeSeasonEpisodes(tmdbRows, show.videos, season)
    }
    Connections {
        target: hub.api
        ignoreUnknownSignals: true
        function onEpisodesReady(tmdbId, s, eps) {
            if (!root.show || tmdbId !== root.show.tmdbId || s !== root.season) return
            root.episodes = root.rebuild(eps)
        }
    }

    Rectangle { anchors.fill: parent; color: Theme.bg }
    MouseArea { anchors.fill: parent }   // the page owns its clicks

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: col.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        WheelScroller { flick: flick }

        ColumnLayout {
            id: col
            width: flick.width
            spacing: 0

            HubSeriesHero {
                Layout.fillWidth: true
                hub: root.hub
                show: root.show
                season: root.season
                onSeasonPicked: function (s) { root.season = s }
                onCloseRequested: root.hub.seriesOpen = false
            }

            Repeater {
                model: root.episodes
                delegate: HubEpisodeRow {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.leftMargin: Theme.sp5
                    Layout.rightMargin: Theme.sp5
                    ep: modelData
                    onPlayRequested: if (root.hub.api && modelData.have)
                        root.hub.api.playFile(modelData.hash, modelData.idx)
                }
            }

            Item { Layout.fillWidth: true; Layout.preferredHeight: Theme.sp5 }
        }
    }
}
