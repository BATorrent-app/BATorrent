// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The browse surface of the Find page: featured billboard + poster shelves
// (catalog rows, the watchlist, game rows) in one scroll, with the loading
// skeleton and the keyless empty state. Type-filtered from outside; poster
// clicks surface as findRequested(title).
import QtQuick
import QtQuick.Controls
import "theme"
import "widgets"

Item {
    id: browse
    property string typeFilter: "all"   // all | game | movie | series
    property bool active: true          // gates the billboard rotation
    signal findRequested(string title)
    signal typeFilterRequested(string type)

    readonly property alias scrollY: flick.contentY

    readonly property var api: typeof discovery !== "undefined" ? discovery : null
    readonly property var heroList: api ? api.hero : []
    readonly property var rowList: api ? api.rows : []

    // global hero is backdrop-gated and games-first, so it can hold zero movies/
    // series — when a type is filtered and the global hero has none, derive the
    // hero from that type's own rows so every filter gets a banner.
    readonly property var heroFiltered: {
        if (typeFilter === "all") return heroList
        var fromHero = heroList.filter(function(h) { return h && h.type === typeFilter })
        if (fromHero.length > 0) return fromHero
        var out = []
        for (var i = 0; i < rowsFiltered.length && out.length < 6; ++i) {
            var items = rowsFiltered[i].items || []
            for (var j = 0; j < items.length; ++j) {
                var it = items[j]
                if (it && (it.overview || "").length > 0 && (it.poster || "").length > 0) { out.push(it); break }
            }
        }
        return out
    }
    readonly property var rowsFiltered: typeFilter === "all" ? rowList
        : rowList.filter(function(r) { return r && r.items && r.items.length > 0 && r.items[0].type === typeFilter })
    readonly property var videoRows: rowsFiltered.filter(function(r) { return r.items[0].type !== "game" })
    readonly property var gameRows: rowsFiltered.filter(function(r) { return r.items[0].type === "game" })

    // the watchlist as a browse shelf ("what I marked to find later")
    readonly property var watchlistItems: {
        var wl = (typeof session !== "undefined") ? session.watchlist : []
        if (typeFilter === "all") return wl
        return wl.filter(function(w) { return w && w.type === typeFilter })
    }

    function getWatch(item) {
        if (typeof search !== "undefined")
            search.getAndWatch(item.title || "", item.year || "", item.type || "movie")
    }

    onVisibleChanged: if (visible && api) api.load()
    Component.onCompleted: if (visible && api) api.load()

    // Plain Column, not ColumnLayout: layout size hints proved unreliable for
    // the file-component shelves here (children collapsed to zero height), so
    // the scroll column positions by explicit child geometry instead.
    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: col.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        WheelScroller { flick: flick }

        Column {
            id: col
            width: flick.width
            spacing: 26

            Item { width: 1; height: 2 }

            // ---------- featured billboard (+ floating refresh) ----------
            Item {
                x: Theme.sp5
                width: col.width - Theme.sp5 - Theme.sp4
                height: 320
                visible: browse.heroFiltered.length > 0

                FindBillboard {
                    id: billboard
                    anchors.fill: parent
                    model: browse.heroFiltered
                    active: browse.active && browse.visible
                    onDetailsRequested: function(title) { browse.findRequested(title) }
                }
                // refresh — floats over the billboard's top-right corner
                Rectangle {
                    anchors.top: parent.top; anchors.right: parent.right
                    anchors.margins: Theme.sp4
                    width: 34; height: 34; radius: 17
                    z: 6
                    color: rfMa.containsMouse ? Theme.hover : Qt.rgba(0, 0, 0, 0.35)
                    border.color: Theme.hair; border.width: 1
                    IconImg {
                        anchors.centerIn: parent
                        src: "qrc:/icons/refresh.svg"
                        tint: rfMa.containsMouse ? Theme.t1 : Theme.t3
                        s: 16
                        RotationAnimation on rotation {
                            running: browse.api && browse.api.loading
                            from: 0; to: 360; duration: 900; loops: Animation.Infinite
                        }
                    }
                    MouseArea {
                        id: rfMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: if (browse.api) browse.api.refresh()
                    }
                }
            }

            // ---------- shelves: catalog video rows · watchlist · game rows ----------
            Repeater {
                model: browse.videoRows
                delegate: PosterShelf {
                    required property var modelData
                    label: modelData.label
                    items: modelData.items
                    flickY: flick.contentY
                    flickH: flick.height
                    onActivated: function(item) { browse.findRequested(item.title) }
                    onGetWatch: function(item) { browse.getWatch(item) }
                    onSeeAllRequested: function(type) { browse.typeFilterRequested(type) }
                }
            }

            PosterShelf {
                label: (i18n.language, i18n.t("hub_mylist"))
                items: browse.watchlistItems
                showSeeAll: false
                flickY: flick.contentY
                flickH: flick.height
                onActivated: function(item) { browse.findRequested(item.title) }
                onGetWatch: function(item) { browse.getWatch(item) }
            }

            Repeater {
                model: browse.gameRows
                delegate: PosterShelf {
                    required property var modelData
                    label: modelData.label
                    items: modelData.items
                    flickY: flick.contentY
                    flickH: flick.height
                    onActivated: function(item) { browse.findRequested(item.title) }
                    onGetWatch: function(item) { browse.getWatch(item) }
                    onSeeAllRequested: function(type) { browse.typeFilterRequested(type) }
                }
            }

            Item { width: 1; height: 8 }
        }
    }

    // skeleton while the first fetch is in flight
    Column {
        id: skeleton
        anchors.fill: parent
        anchors.topMargin: 10
        spacing: 26
        clip: true
        visible: browse.api && browse.api.loading && browse.rowList.length === 0
        opacity: 0.75
        SequentialAnimation on opacity {
            running: skeleton.visible
            loops: Animation.Infinite
            NumberAnimation { to: 0.45; duration: 700; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 0.75; duration: 700; easing.type: Easing.InOutQuad }
        }
        Rectangle {
            x: Theme.sp5
            width: skeleton.width - Theme.sp5 - Theme.sp4
            height: 320; radius: 16; color: Theme.elev
        }
        Repeater {
            model: 2
            Column {
                x: Theme.sp5
                spacing: 10
                Rectangle { width: 150; height: 16; radius: 4; color: Theme.track }
                Row {
                    spacing: 16
                    Repeater {
                        model: 7
                        Rectangle { width: 150; height: 225; radius: 10; color: Theme.elev; border.color: Theme.hair; border.width: 1 }
                    }
                }
            }
        }
    }

    // empty / no-keys / empty-filter state (not loading)
    Text {
        anchors.centerIn: parent
        visible: browse.rowsFiltered.length === 0 && !(browse.api && browse.api.loading)
        text: browse.api && browse.api.statusText.length > 0 ? browse.api.statusText : (i18n.language, i18n.t("discover_empty"))
        color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
    }
}
