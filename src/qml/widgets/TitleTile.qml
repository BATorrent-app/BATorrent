// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A title as a wide tile: its backdrop with its own lettering on it, rating,
// year and kind underneath. The same picture the title's page opens on, so a
// click lands where the tile already promised.
import QtQuick
import "../theme"

Item {
    id: tile
    property var item: ({})
    property bool watchlistEnabled: typeof session !== "undefined"
    property bool saved: false
    signal activated()
    signal watchlistToggle()

    readonly property string art: (item.backdrop || "").length > 0 ? item.backdrop : (item.poster || "")
    readonly property real artH: Math.round(width * 9 / 16)

    implicitWidth: 252
    implicitHeight: artH + 9 + meta.implicitHeight

    function typeLabel(t) {
        return t === "movie" ? i18n.t("search_type_movie")
             : t === "series" ? i18n.t("search_type_series")
             : t === "game" ? i18n.t("search_type_game") : ""
    }

    Rectangle {
        id: frame
        width: parent.width
        height: tile.artH
        radius: 4
        color: Theme.stagePanel
        clip: true

        Image {
            id: img
            anchors.fill: parent
            source: tile.art
            sourceSize.width: 504
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            opacity: status === Image.Ready ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.durSlow } }
        }
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.45; color: Qt.rgba(0, 0, 0, 0) }
                GradientStop { position: 1.0;  color: Qt.rgba(0, 0, 0, 0.65) }
            }
        }
        TitleMark {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.bottomMargin: 10
            width: maxWidth
            height: maxHeight
            maxWidth: frame.width * 0.58
            maxHeight: frame.height * 0.42
            tmdbId: tile.item.tmdbId || 0
            type: tile.item.type || ""
            title: tile.item.title || tile.item.name || ""
        }
        CardActions {
            anchors.fill: parent
            showPlay: false
            hovered: ma.containsMouse
            watchlistEnabled: tile.watchlistEnabled
            saved: tile.saved
            onWatchlistToggle: tile.watchlistToggle()
        }
        // the hover ring sits on top of the art, not around the tile
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
        id: meta
        anchors.top: frame.bottom
        anchors.topMargin: 9
        spacing: 8
        Text {
            visible: (tile.item.rating || 0) > 0
            text: "★ " + (tile.item.rating || 0).toFixed(1)
            color: Theme.star
            font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        Text {
            text: tile.item.year || ""
            color: Theme.t4
            font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        Text {
            text: (i18n.language, tile.typeLabel(tile.item.type || ""))
            color: Theme.t4
            font.pixelSize: 12; font.family: Theme.fontSans
        }
    }

    MouseArea {
        id: ma
        anchors.fill: frame
        z: -1
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: tile.activated()
    }
}
