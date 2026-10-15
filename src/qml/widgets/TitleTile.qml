// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A title as a wide tile: its backdrop with its own lettering in the corner
// (the name in text when it has none), and one line under it that says what
// the person already has of it ("S01 · 1 of 8 watched", "Watched") or, when
// nothing, rating, year and kind in grey. The same picture the
// title's page opens on, so a click lands where the tile already promised.
import QtQuick
import "../theme"

Item {
    id: tile
    property var item: ({})
    property bool watchlistEnabled: typeof session !== "undefined"
    property bool saved: false
    // { text } from OwnedIndex.stateOf(item), or null when it is not in the library
    property var owned: null
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
        radius: Theme.rad(4)
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
            border.color: Theme.t1
            opacity: ma.containsMouse ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
        }
    }

    Row {
        id: meta
        anchors.top: frame.bottom
        anchors.topMargin: 9
        width: parent.width
        spacing: 6
        Rectangle {
            visible: !!tile.owned
            anchors.verticalCenter: parent.verticalCenter
            width: 6; height: 6; radius: 3
            color: Theme.grn
        }
        Text {
            width: Math.min(implicitWidth, parent.width - 12)
            elide: Text.ElideRight
            textFormat: Text.StyledText
            font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            color: Theme.t3
            text: {
                i18n.language
                var facts = []
                if ((tile.item.rating || 0) > 0) facts.push("★ " + tile.item.rating.toFixed(1))
                if (tile.item.year) facts.push(tile.item.year)
                if (!tile.owned) facts.push(tile.typeLabel(tile.item.type || ""))
                facts = facts.filter(function (f) { return String(f).length > 0 })
                if (!tile.owned) return facts.join(" · ")
                return "<font color='" + Theme.t2 + "'>" + tile.owned.text + "</font>"
                       + (facts.length ? " · " + facts.join(" · ") : "")
            }
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
