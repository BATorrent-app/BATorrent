// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Full-surface screen for one owned game: the art and identity on top, what
// you can do and how much you have played right under it, then the media and
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
        stillOpen = ""
        flick.contentY = 0
        if (gameTitle.length > 0 && hub.disco) hub.disco.fetchGameDetails(gameTitle)
    }
    Connections {
        target: root.hub.disco
        ignoreUnknownSignals: true
        function onGameDetailsReady(name, d) { if (name === root.gameTitle) root.details = d }
    }

    Rectangle { anchors.fill: parent; color: Theme.bg }
    MouseArea { anchors.fill: parent }   // the page owns its clicks

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: col.height + Theme.sp6
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        WheelScroller { flick: flick }

        Column {
            id: col
            width: flick.width
            spacing: Theme.sp5

            WorkHero {
                width: col.width
                height: Math.max(340, root.height * 0.52)
                title: root.gameTitle
                artUrl: root.details.backdrop || (root.game ? root.hub.fileUrl(root.game.poster) : "")
                subtitle: {
                    var parts = []
                    var year = root.details.year || (root.game ? root.game.year : "")
                    if (year) parts.push(year)
                    if (root.details.maker) parts.push(root.details.maker)
                    var r = root.game && root.game.rating > 0 ? root.game.rating : (root.details.rating || 0)
                    if (r > 0) parts.push("★ " + Number(r).toFixed(1))
                    return parts.join("  ·  ")
                }
                genres: root.game && root.game.genres && root.game.genres.length > 0
                        ? root.game.genres : (root.details.genres || [])
                summary: (root.game && root.game.description) || root.details.overview || ""
                onBackRequested: root.hub.gameOpen = false
            }

            HubGameActions {
                x: Theme.sp5
                width: col.width - 2 * Theme.sp5
                hub: root.hub
                game: root.game
            }

            HubGameStills {
                x: Theme.sp5
                width: col.width - 2 * Theme.sp5
                stills: root.details.stills || []
                onStillPicked: function (url) { root.stillOpen = url }
            }

            HubGameFiles {
                x: Theme.sp5
                width: col.width - 2 * Theme.sp5
                hub: root.hub
                game: root.game
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
