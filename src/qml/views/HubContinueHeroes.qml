// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The top of the HUB: what you were watching and what you were playing, side
// by side. Either one alone takes the whole row.
import QtQuick
import "../theme"

Item {
    id: root
    property var hub
    readonly property bool watchSuggested: hub.continueItems.length === 0
    readonly property var watch: !watchSuggested ? hub.continueItems[0] : hub.suggestedWatch
    readonly property bool playSuggested: hub.continuePlaying.length === 0
    readonly property var play: !playSuggested ? hub.continuePlaying[0] : hub.suggestedGame
    visible: !!watch || !!play
    height: visible ? 300 + 24 : 0

    readonly property string watchType: watch && watch.isSeries ? "series" : "movie"
    property string watchArt
    property var playDetails: ({})
    property var playArt: ({})
    property int playAchUnlocked: 0
    property int playAchTotal: 0
    onWatchChanged: {
        watchArt = ""
        if (watch && (watch.tmdbId || 0) > 0 && hub.disco) hub.disco.fetchLogo(watch.tmdbId, watchType)
    }
    readonly property string playTitle: play ? (play.title || "").trim() : ""
    onPlayTitleChanged: {
        playDetails = ({})
        playArt = ({})
        playAchUnlocked = 0
        playAchTotal = 0
        if (playTitle && hub.disco) hub.disco.fetchGameDetails(playTitle)
    }
    Connections {
        target: root.hub.disco
        ignoreUnknownSignals: true
        function onTitleBackdropReady(id, type, url) {
            if (root.watch && id === root.watch.tmdbId && type === root.watchType) root.watchArt = url
        }
        function onGameDetailsReady(name, d) {
            if (name !== root.playTitle) return
            root.playDetails = d
            if (root.hub.api) root.hub.api.requestAchievements(root.play.infoHash, d.steamAppId || 0, root.playTitle)
        }
    }
    Connections {
        target: root.hub.api
        ignoreUnknownSignals: true
        function onAchievementsReady(hash, appId, list, unlocked, total) {
            if (!root.play || hash !== root.play.infoHash) return
            if (appId > 0 && !root.playDetails.steamHero && root.hub.api) root.playArt = root.hub.api.steamArt(appId)
            root.playAchUnlocked = unlocked
            root.playAchTotal = total
        }
    }

    readonly property real gap: 12
    readonly property real inner: width - 48
    // 1.35fr / 1fr, as the design has it.
    readonly property real watchW: !play ? inner : (!watch ? 0 : (inner - gap) * 1.35 / 2.35)

    HubContinueCard {
        visible: !!root.watch
        x: 24
        y: 24
        width: root.watchW
        kicker: (i18n.language, i18n.t(root.watchSuggested ? "hub_ready_to_watch" : "hub_continue_watching"))
        artSources: root.watchArt ? [root.watchArt] : []
        tmdbId: root.watch ? (root.watch.tmdbId || 0) : 0
        tmdbType: root.watchType
        title: root.watch ? (root.watch.title || "") : ""
        line: root.watch ? (root.watch.isSeries ? root.hub.episodeLabel(root.watch)
                                                : [root.watch.year].concat((root.watch.genres || []).slice(0, 1))
                                                      .filter(function (s) { return !!s }).join("  ·  ")) : ""
        buttonText: (i18n.language, i18n.t(root.watchSuggested ? "hub_gs_play" : "hub_resume"))
        buttonLight: true
        progress: root.watch && !root.watchSuggested ? (root.watch.watchedPct || 0) : -1
        note: root.watch && !root.watchSuggested
              ? root.hub.fmtLeft((root.watch.durMs || 0) - (root.watch.resumeMs || 0)) : ""
        onPressed: if (root.hub.api && root.watch)
            root.hub.api.playByHashFile(root.watch.infoHash, root.watch.fileIndex)
        onOpened: if (root.watch) root.hub.openItem(root.watch, root.watch.isSeries ? "series" : "movie")
    }

    HubContinueCard {
        visible: !!root.play
        x: root.watch ? 24 + root.watchW + root.gap : 24
        y: 24
        width: root.watch ? root.inner - root.watchW - root.gap : root.inner
        logoWidth: 200
        kicker: (i18n.language, i18n.t(!root.playSuggested ? "hub_continue_playing"
                                       : root.play && root.play.installState === 1 ? "hub_ready_to_install"
                                       : "hub_ready_to_play"))
        artSources: [root.playDetails.steamHero || root.playArt.hero, root.playDetails.backdrop]
                    .filter(function (u) { return !!u })
        fixedLogo: root.playDetails.steamLogo || root.playArt.logo || ""
        title: root.playTitle
        line: {
            if (!root.play) return ""
            var parts = []
            if ((root.play.playSeconds || 0) > 0)
                parts.push((i18n.language, i18n.t("hub_tile_played")).arg(root.hub.hours(root.play.playSeconds)))
            if (root.playAchTotal > 0)
                parts.push((i18n.language, i18n.t("hub_ach_short")).arg(root.playAchUnlocked).arg(root.playAchTotal))
            return parts.join("  ·  ")
        }
        buttonText: root.play ? root.hub.gameStateLabel(root.play) : ""
        buttonEnabled: root.hub.gameStateActionable(root.play)
        note: {
            if (!root.play || !(root.play.lastPlayed > 0)) return ""
            var ago = root.hub.fmtAgo(root.play.lastPlayed)
            // "Yesterday" alone, "last played yesterday" mid-sentence.
            return (i18n.language, i18n.t("hub_last_played_ago")).arg(ago.charAt(0).toLowerCase() + ago.slice(1))
        }
        onPressed: if (root.play) root.hub.gamePrimary(root.play)
        onOpened: if (root.play) root.hub.openItem(root.play, "game")
    }
}
