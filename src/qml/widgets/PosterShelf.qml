// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Labeled horizontal poster row (header + "See all" + PosterCards) used by the
// browse shelves. Sized explicitly for a plain Column host: layout size hints
// proved unreliable for file components in the browse scroll column.
// Delegates stay unpopulated until the shelf first scrolls into view
// (flickY/flickH drive the same lazy gate the Discover rows used).
import QtQuick
import "../theme"

Item {
    id: shelf
    property string label
    property var items: []
    property bool showSeeAll: true
    // Wide 16:9 tiles with the title's lettering instead of 2:3 posters.
    property bool landscape: false
    // viewport of the hosting Flickable, for the lazy-population gate
    property real flickY: 0
    property real flickH: 0
    signal activated(var item)
    signal getWatch(var item)
    // The whole row, not its type: See all means "show me this shelf".
    signal seeAllRequested(string rowLabel, var rowItems)

    width: parent ? parent.width : 0
    readonly property int rowH: landscape ? 172 : 252
    height: header.height + (landscape ? 14 : 10) + rowH
    visible: items && items.length > 0

    readonly property bool inView: (y + height) > (flickY - 400) && y < (flickY + flickH + 400)
    property bool everInView: false
    onInViewChanged: if (inView) everInView = true
    Component.onCompleted: if (inView) everInView = true

    Row {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: Theme.sp5
        spacing: 10
        Text {
            id: labelTxt
            text: shelf.label
            color: Theme.t1
            font.pixelSize: shelf.landscape ? 18 : 17
            font.weight: shelf.landscape ? Font.DemiBold : Font.Bold
            font.letterSpacing: shelf.landscape ? -0.2 : 0
            font.family: Theme.fontSans
        }
        Text {
            anchors.baseline: labelTxt.baseline
            visible: shelf.showSeeAll && shelf.items && shelf.items.length > 0
            text: (i18n.language, i18n.t("see_all"))
            color: saMa.containsMouse ? Theme.t1 : Theme.t4
            font.pixelSize: shelf.landscape ? 12 : 13
            font.weight: shelf.landscape ? Font.Normal : Font.Medium
            font.family: Theme.fontSans
            MouseArea {
                id: saMa; anchors.fill: parent; anchors.margins: -4
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: if (shelf.items && shelf.items.length > 0)
                               shelf.seeAllRequested(shelf.label, shelf.items)
            }
        }
    }
    ListView {
        anchors.top: header.bottom
        anchors.topMargin: shelf.landscape ? 14 : 10
        anchors.left: parent.left
        anchors.right: parent.right
        height: shelf.rowH
        orientation: ListView.Horizontal
        clip: true
        spacing: shelf.landscape ? 10 : 16
        boundsBehavior: Flickable.StopAtBounds
        leftMargin: Theme.sp5
        rightMargin: Theme.sp5
        model: shelf.everInView ? shelf.items : []
        delegate: shelf.landscape ? tileDelegate : posterDelegate
    }

    function savedOf(it) {
        return typeof session !== "undefined"
               && (session.watchlist, session.inWatchlist(it.title || "", it.type || ""))
    }
    function toggleSaved(it) {
        if (typeof session !== "undefined") session.toggleWatchlist({
            title: it.title || "", type: it.type || "",
            poster: it.poster || "", year: it.year || "" })
    }

    Component {
        id: tileDelegate
        TitleTile {
            required property var modelData
            width: 252
            item: modelData
            saved: shelf.savedOf(modelData)
            onWatchlistToggle: shelf.toggleSaved(modelData)
            onActivated: shelf.activated(modelData)
        }
    }
    Component {
        id: posterDelegate
        PosterCard {
            id: pcard
            required property var modelData
            posterW: 150
            title: pcard.modelData.title || ""
            poster: pcard.modelData.poster || ""
            year: pcard.modelData.year || ""
            rating: pcard.modelData.rating || 0
            type: pcard.modelData.type || ""
            synopsis: pcard.modelData.overview || ""
            watchlistEnabled: typeof session !== "undefined"
            saved: shelf.savedOf(pcard.modelData)
            onWatchlistToggle: shelf.toggleSaved(pcard.modelData)
            onActivated: shelf.activated(pcard.modelData)
            onGetWatch: shelf.getWatch(pcard.modelData)
        }
    }
}
