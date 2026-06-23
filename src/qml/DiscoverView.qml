// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Discover content page (4.0 step ③). Rotating hero + horizontal poster rows
// from DiscoveryService (TMDB/IGDB). Click a poster → openSearch(title), which
// Main.qml routes to the Search page.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "theme"
import "widgets"

Rectangle {
    id: page
    color: Theme.bg

    signal openSearch(string query)

    readonly property var api: typeof discovery !== "undefined" ? discovery : null
    readonly property var heroList: api ? api.hero : []
    readonly property var rowList: api ? api.rows : []

    property int heroIndex: 0
    property int shownIndex: 0
    readonly property var heroItem: (heroList.length > shownIndex) ? heroList[shownIndex] : null

    // lazily-fetched YouTube key for the current hero (TMDB list endpoints don't
    // carry videos, so we ask per hero as it shows)
    property string heroTrailerKey: ""
    onHeroItemChanged: {
        heroTrailerKey = ""
        if (api && heroItem && (heroItem.type === "movie" || heroItem.type === "series")
            && (heroItem.tmdbId || 0) > 0)
            api.fetchTrailer(heroItem.tmdbId, heroItem.type)
    }
    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onTrailerReady(tmdbId, youtubeKey) {
            if (page.heroItem && (page.heroItem.tmdbId || 0) === tmdbId) page.heroTrailerKey = youtubeKey
        }
    }

    onVisibleChanged: if (visible && api) api.load()
    Component.onCompleted: if (visible && api) api.load()

    onHeroIndexChanged: heroFade.restart()
    SequentialAnimation {
        id: heroFade
        NumberAnimation { target: heroContent; property: "opacity"; to: 0; duration: 220; easing.type: Easing.InCubic }
        ScriptAction { script: page.shownIndex = page.heroIndex }
        NumberAnimation { target: heroContent; property: "opacity"; to: 1; duration: 420; easing.type: Easing.OutCubic }
    }
    Timer {
        interval: 7000
        running: page.visible && page.heroList.length > 1
        repeat: true
        onTriggered: page.heroIndex = (page.heroIndex + 1) % page.heroList.length
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: col.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: col
            width: flick.width
            spacing: 26

            // ---------- hero ----------
            Item {
                Layout.fillWidth: true
                // cinematic banner: ~44% of the viewport, never cramped
                Layout.preferredHeight: Math.max(420, Math.round(page.height * 0.44))
                visible: page.heroItem !== null

                Item {
                    id: heroContent
                    anchors.fill: parent

                    Image {
                        anchors.fill: parent
                        source: page.heroItem ? (page.heroItem.backdrop || "") : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                    }
                    // bottom + left scrim for text legibility
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00000000" }
                            GradientStop { position: 0.45; color: "#33000000" }
                            GradientStop { position: 1.0; color: "#ee0b0b0d" }
                        }
                    }

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: Theme.sp5
                        anchors.rightMargin: Theme.sp5
                        anchors.bottomMargin: Theme.sp5
                        spacing: 8

                        Text {
                            text: page.heroItem ? page.heroItem.title : ""
                            color: "#ffffff"
                            font.pixelSize: 30; font.weight: Font.Bold; font.family: Theme.fontSans
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Text {
                            Layout.fillWidth: true
                            visible: text.length > 0
                            text: {
                                if (!page.heroItem) return ""
                                var parts = []
                                if ((page.heroItem.year || "").length > 0) parts.push(page.heroItem.year)
                                if ((page.heroItem.rating || 0) > 0) parts.push("★ " + page.heroItem.rating.toFixed(1))
                                return parts.join("    ·    ")
                            }
                            color: "#c8c8cc"; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                        }
                        Text {
                            text: page.heroItem ? page.heroItem.overview : ""
                            color: "#d8d8dc"
                            font.pixelSize: 13; font.family: Theme.fontSans
                            Layout.fillWidth: true
                            Layout.maximumWidth: 620
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                        RowLayout {
                            spacing: 10
                            BtnFlat {
                                visible: page.heroItem && page.heroItem.type !== "game"
                                primary: true
                                text: (i18n.language, i18n.t("gw_get_and_watch"))
                                onClicked: if (page.heroItem && typeof search !== "undefined")
                                               search.getAndWatch(page.heroItem.title,
                                                                  page.heroItem.year || "",
                                                                  page.heroItem.type || "movie")
                            }
                            BtnFlat {
                                visible: page.heroTrailerKey !== ""
                                text: (i18n.language, i18n.t("gw_trailer"))
                                onClicked: Qt.openUrlExternally("https://www.youtube.com/watch?v=" + page.heroTrailerKey)
                            }
                            BtnFlat {
                                primary: page.heroItem && page.heroItem.type === "game"
                                text: (i18n.language, i18n.t("empty_search_btn"))
                                onClicked: if (page.heroItem) page.openSearch(page.heroItem.title)
                            }
                        }
                    }
                }
            }

            // ---------- poster rows ----------
            Repeater {
                model: page.rowList
                delegate: ColumnLayout {
                    id: rowItem
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 10

                    // selective rendering: a row only loads its covers once it
                    // scrolls within the viewport (+buffer), then stays loaded —
                    // so the visible rows show first instead of all ~40 at once.
                    readonly property bool inView: (y + height) > (flick.contentY - 400)
                                                   && y < (flick.contentY + flick.height + 400)
                    property bool everInView: false
                    onInViewChanged: if (inView) everInView = true
                    Component.onCompleted: if (inView) everInView = true

                    Text {
                        text: rowItem.modelData.label
                        color: Theme.t1
                        font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                        Layout.leftMargin: Theme.sp5
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 252
                        orientation: ListView.Horizontal
                        clip: true
                        spacing: 16
                        boundsBehavior: Flickable.StopAtBounds
                        leftMargin: Theme.sp5
                        rightMargin: Theme.sp5
                        model: rowItem.everInView ? rowItem.modelData.items : []
                        delegate: PosterCard {
                            id: pcard
                            required property var modelData
                            posterW: 150
                            title: pcard.modelData.title
                            poster: pcard.modelData.poster
                            year: pcard.modelData.year
                            rating: pcard.modelData.rating
                            type: pcard.modelData.type
                            synopsis: pcard.modelData.overview || ""
                            watchlistEnabled: typeof session !== "undefined"
                            saved: typeof session !== "undefined"
                                   && (session.watchlist, session.inWatchlist(pcard.modelData.title, pcard.modelData.type))
                            onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                                title: pcard.modelData.title, type: pcard.modelData.type,
                                poster: pcard.modelData.poster, year: pcard.modelData.year })
                            onActivated: page.openSearch(pcard.modelData.title)
                            onGetWatch: if (typeof search !== "undefined")
                                            search.getAndWatch(pcard.modelData.title,
                                                               pcard.modelData.year || "",
                                                               pcard.modelData.type || "movie")
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }
        }
    }

    // skeleton while the first fetch is in flight
    ColumnLayout {
        id: skeleton
        anchors.fill: parent
        anchors.leftMargin: 0
        spacing: 26
        visible: page.api && page.api.loading && page.rowList.length === 0
        opacity: 0.75
        SequentialAnimation on opacity {
            running: skeleton.visible
            loops: Animation.Infinite
            NumberAnimation { to: 0.45; duration: 700; easing.type: Easing.InOutQuad }
            NumberAnimation { to: 0.75; duration: 700; easing.type: Easing.InOutQuad }
        }
        Rectangle { Layout.fillWidth: true; Layout.preferredHeight: Math.max(420, Math.round(page.height * 0.44)); color: Theme.elev }
        Repeater {
            model: 2
            ColumnLayout {
                spacing: 10
                Layout.leftMargin: Theme.sp5
                Rectangle { width: 150; height: 16; radius: 4; color: Theme.track }
                RowLayout {
                    spacing: 16
                    Repeater {
                        model: 7
                        Rectangle { width: 150; height: 225; radius: 10; color: Theme.elev; border.color: Theme.hair; border.width: 1 }
                    }
                }
            }
        }
        Item { Layout.fillHeight: true }
    }

    // empty / no-keys state (not loading)
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 14
        visible: page.rowList.length === 0 && !(page.api && page.api.loading)
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: page.api && page.api.statusText.length > 0 ? page.api.statusText : (i18n.language, i18n.t("discover_empty"))
            color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
        }
    }

    // manual refresh (spins while loading)
    Rectangle {
        anchors.top: parent.top; anchors.right: parent.right
        anchors.topMargin: Theme.sp4; anchors.rightMargin: Theme.sp4
        width: 34; height: 34; radius: 17
        visible: page.rowList.length > 0
        color: rfMa.containsMouse ? Theme.hover : Qt.rgba(0, 0, 0, 0.35)
        border.color: Theme.hair; border.width: 1
        IconImg {
            anchors.centerIn: parent
            src: "qrc:/icons/refresh.svg"
            tint: rfMa.containsMouse ? Theme.t1 : Theme.t3
            s: 16
            RotationAnimation on rotation {
                running: page.api && page.api.loading
                from: 0; to: 360; duration: 900; loops: Animation.Infinite
            }
        }
        MouseArea {
            id: rfMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: if (page.api) page.api.refresh()
        }
    }
}
