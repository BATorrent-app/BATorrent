// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Full-surface screen for one owned game: the banner, the band with what to
// do next and how much you have played, then activity and screenshots beside
// the files. The item is looked up live by hash so playtime and install state
// keep moving while the page is open.
import QtQuick
import "../theme"
import "../widgets"

Item {
    id: root
    property var hub
    readonly property var game: {
        var list = hub.gameItems || []
        for (var i = 0; i < list.length; i++)
            if (list[i].infoHash === hub.gameHash) return list[i]
        return null
    }
    property var details: ({})
    property bool detailsReady: false
    property var achievements: []
    property var steamArt: ({})
    property int achUnlocked: 0
    property int achTotal: 0
    function askAchievements() {
        if (root.game && root.hub.api)
            root.hub.api.requestAchievements(root.game.infoHash, root.details.steamAppId || 0, root.gameTitle)
    }
    property string stillOpen: ""
    // Held past the close so the image fades with the scrim instead of vanishing first.
    property string stillShown: ""
    onStillOpenChanged: if (stillOpen.length > 0) stillShown = stillOpen

    visible: hub.gameOpen || slide > 0.001
    property real slide: hub.gameOpen ? 1 : 0
    Behavior on slide { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
    opacity: slide

    readonly property string gameTitle: game ? (game.title || "") : ""
    onGameTitleChanged: {
        details = ({})
        detailsReady = false
        achievements = []
        steamArt = ({})
        achUnlocked = 0
        achTotal = 0
        stillOpen = ""
        flick.contentY = 0
        if (gameTitle.length > 0 && hub.disco) hub.disco.fetchGameDetails(gameTitle)
    }
    Connections {
        target: root.hub.disco
        ignoreUnknownSignals: true
        function onGameDetailsReady(name, d) {
            if (name !== root.gameTitle.trim()) return
            root.details = d
            root.detailsReady = true
            root.askAchievements()
        }
    }

    Connections {
        target: root.hub.api
        ignoreUnknownSignals: true
        function onAchievementsReady(hash, appId, list, unlocked, total) {
            if (!root.game || hash !== root.game.infoHash) return
            // IGDB had no Steam record but the store search found one: its art too.
            if (appId > 0 && !root.details.steamHero) root.steamArt = root.hub.api.steamArt(appId)
            root.achievements = list
            root.achUnlocked = unlocked
            root.achTotal = total
        }
    }
    // A session that just ended may have unlocked something.
    readonly property bool playing: !!game && game.playing === true
    onPlayingChanged: if (!playing && detailsReady) askAchievements()

    Rectangle { anchors.fill: parent; color: Theme.bg }
    MouseArea { anchors.fill: parent }   // the page owns its clicks

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: col.height
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        WheelScroller { flick: flick }

        Column {
            id: col
            width: flick.width

            HubGameHero {
                width: col.width
                height: 400
                title: root.gameTitle
                steamHero: root.details.steamHero || root.steamArt.hero || ""
                steamLogo: root.details.steamLogo || root.steamArt.logo || ""
                // Nothing until IGDB answers: the portrait cover cropped to a banner is
                // a blown-up blur, and it flashed for the second before the real art.
                fallbackArt: !root.detailsReady ? ""
                             : (root.details.backdrop || (root.game ? root.hub.fileUrl(root.game.poster) : ""))
                facts: {
                    var out = []
                    var year = root.details.year || (root.game ? root.game.year : "")
                    if (year) out.push(year)
                    if (root.details.maker) out.push(root.details.maker)
                    var genres = root.game && root.game.genres && root.game.genres.length > 0
                                 ? root.game.genres : (root.details.genres || [])
                    if (genres.length > 0) out.push(genres[0])
                    return out
                }
                onBackRequested: root.hub.closeGame()
            }

            HubGameActions {
                width: col.width
                hub: root.hub
                game: root.game
                achUnlocked: root.achUnlocked
                achTotal: root.achTotal
                onSettingsRequested: if (root.game) root.hub.gameMenu.openFor(root.game.infoHash)
            }

            Item {
                id: body
                width: col.width
                // Side by side when there is room for both; the files go under otherwise.
                readonly property bool wide: width >= 960
                readonly property real sideW: 360
                readonly property real mainW: wide ? width - 96 - 48 - sideW : width - 96
                height: 32 + 48 + (wide ? Math.max(mainCol.height, side.height)
                                        : mainCol.height + 40 + side.height)

                Column {
                    id: mainCol
                    x: 48
                    y: 32
                    width: body.mainW
                    spacing: 40

                    HubGameAchievements {
                        width: mainCol.width
                        hub: root.hub
                        list: root.achievements
                        unlocked: root.achUnlocked
                        total: root.achTotal
                    }

                    HubGameActivity {
                        width: mainCol.width
                        hub: root.hub
                        game: root.game
                    }

                    HubGameStills {
                        width: mainCol.width
                        stills: root.details.stills || []
                        onStillPicked: function (url) { root.stillOpen = url }
                    }
                }

                HubGameFiles {
                    id: side
                    x: body.wide ? body.width - 48 - body.sideW : 48
                    y: body.wide ? 32 : 32 + mainCol.height + 40
                    width: body.wide ? body.sideW : body.mainW
                    hub: root.hub
                    game: root.game
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.88)
        visible: opacity > 0.001
        opacity: root.stillOpen.length > 0 ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durBase } }
        MouseArea { anchors.fill: parent; onClicked: root.stillOpen = "" }
        Image {
            anchors.fill: parent
            anchors.margins: Theme.sp6
            source: root.stillShown
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            cache: true
            scale: root.stillOpen.length > 0 ? 1 : 0.96
            Behavior on scale { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        }
    }
}
