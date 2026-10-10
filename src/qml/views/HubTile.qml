// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One thing in your library as a wide tile: its art with its own lettering,
// a progress edge when something is moving, and its state under it. Films and
// shows take TMDB's textless backdrop, games Steam's library art via IGDB.
import QtQuick
import "../theme"
import "../widgets"

Item {
    id: tile
    property var hub
    property var item: ({})
    property string kind            // "game" | "series" | "movie"
    signal activated()
    signal context()

    readonly property var st: (i18n.language, hub.tileState(item, kind))
    readonly property real artH: Math.round(width * 9 / 16)
    implicitWidth: 252
    implicitHeight: artH + 9 + stateRow.implicitHeight

    readonly property bool isGame: kind === "game"
    readonly property string tmdbType: kind === "series" ? "series" : "movie"
    property string backdrop
    property var game: ({})

    readonly property var disco: hub.disco
    Component.onCompleted: if (isGame && disco && item.title) disco.fetchGameDetails(item.title)
    Connections {
        target: tile.disco
        ignoreUnknownSignals: true
        function onTitleBackdropReady(id, type, url) {
            if (!tile.isGame && id === tile.item.tmdbId && type === tile.tmdbType) tile.backdrop = url
        }
        function onGameDetailsReady(name, d) {
            if (tile.isGame && name === (tile.item.title || "").trim()) tile.game = d
        }
    }

    Rectangle {
        id: frame
        width: parent.width
        height: tile.artH
        radius: 4
        clip: true
        color: "#17171a"

        // The design's placeholder: diagonal stripes, so a tile with no art yet
        // reads as waiting rather than broken.
        Canvas {
            anchors.fill: parent
            visible: art.status !== Image.Ready
            onPaint: {
                var ctx = getContext("2d")
                ctx.fillStyle = "#17171a"
                ctx.fillRect(0, 0, width, height)
                ctx.strokeStyle = "#1c1c1f"
                ctx.lineWidth = 10
                for (var x = -height; x < width + height; x += 20) {
                    ctx.beginPath(); ctx.moveTo(x, height); ctx.lineTo(x + height, 0); ctx.stroke()
                }
            }
        }

        ChainImage {
            id: art
            anchors.fill: parent
            sourceSize.width: 504
            sources: tile.isGame ? [tile.game.steamHero, tile.game.backdrop].filter(function (u) { return !!u })
                                 : (tile.backdrop ? [tile.backdrop] : [])
            opacity: status === Image.Ready ? (tile.st.dim ? 0.55 : 1) : 0
        }
        Rectangle {
            anchors.fill: parent
            opacity: tile.st.dim ? 0.55 : 1
            gradient: Gradient {
                GradientStop { position: 0.45; color: Qt.rgba(0, 0, 0, 0) }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.65) }
            }
        }
        TitleMark {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.bottomMargin: 12
            width: maxWidth
            height: maxHeight
            maxWidth: frame.width * 0.58
            maxHeight: frame.height * 0.4
            opacity: tile.st.dim ? 0.55 : 1
            tmdbId: tile.isGame ? 0 : (tile.item.tmdbId || 0)
            type: tile.tmdbType
            title: tile.item.title || ""
            fixedLogo: tile.isGame ? (tile.game.steamLogo || "") : ""
        }
        Rectangle {
            visible: tile.st.bar >= 0
            anchors.bottom: parent.bottom
            width: parent.width
            height: 3
            color: Qt.rgba(1, 1, 1, 0.2)
            Rectangle {
                height: parent.height
                width: parent.width * Math.max(0, Math.min(1, tile.st.bar))
                color: tile.st.barColor
                Behavior on width { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
            }
        }
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            border.width: 2
            border.color: Qt.rgba(1, 1, 1, 0.85)
            opacity: ma.containsMouse ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
        }
    }

    Row {
        id: stateRow
        anchors.top: frame.bottom
        anchors.topMargin: 9
        width: parent.width
        spacing: 8
        clip: true
        Text {
            text: tile.st.state
            color: tile.st.ink
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            text: tile.st.meta
            color: Theme.stageDim
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
    }

    MouseArea {
        id: ma
        anchors.fill: frame
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function (mouse) {
            if (mouse.button === Qt.RightButton) tile.context()
            else tile.activated()
        }
    }
}
