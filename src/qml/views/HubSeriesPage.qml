// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Full-surface screen for one series. Two columns on purpose: the art holds
// the left for the whole height while the episodes scroll on the right, so
// browsing a show never scrolls its own identity off the screen.
//
// Episodes come from HubCompute.mergeSeasonEpisodes — TMDB's list of the
// season crossed with what is on disk — so a gap in the middle of a season is
// visible as a gap, and every row says whether it can be played right now.
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

    WorkHero {
        id: hero
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: panel.left
        title: root.show ? (root.show.title || "") : ""
        artUrl: root.show && root.show.poster ? root.hub.fileUrl(root.show.poster) : ""
        subtitle: {
            if (!root.show) return ""
            var parts = []
            if (root.show.year) parts.push(root.show.year)
            var n = root.show.seasons ? root.show.seasons.length : 0
            if (n > 0) parts.push((i18n.language, i18n.t(n === 1 ? "hub_n_season" : "hub_n_seasons")).arg(n))
            return parts.join("  ·  ")
        }
        genres: root.show && root.show.genres ? root.show.genres : []
        summary: root.show ? (root.show.description || "") : ""
        onBackRequested: root.hub.seriesOpen = false
    }

    Rectangle {
        id: panel
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: Math.min(440, Math.max(320, root.width * 0.38))
        color: Qt.rgba(0, 0, 0, 0.55)

        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.hair }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            HubSeasonPicker {
                Layout.fillWidth: true
                hub: root.hub
                show: root.show
                season: root.season
                onSeasonPicked: function (s) { root.season = s }
            }

            ListView {
                id: epList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.episodes
                boundsBehavior: Flickable.StopAtBounds
                spacing: 0
                WheelScroller { flick: epList }
                delegate: HubEpisodeRow {
                    required property var modelData
                    width: epList.width
                    ep: modelData
                    // The one to come back to: the first episode that is here
                    // and has not been watched. A show should say where you
                    // left off without being asked.
                    isNext: modelData.have && !modelData.watched
                            && modelData.episode === root.nextEpisode
                    onPlayRequested: if (root.hub.api && modelData.have)
                        root.hub.api.playFile(modelData.hash, modelData.idx)
                }
            }
        }
    }

    readonly property int nextEpisode: {
        for (var i = 0; i < episodes.length; i++)
            if (episodes[i].have && !episodes[i].watched) return episodes[i].episode
        return -1
    }
}
