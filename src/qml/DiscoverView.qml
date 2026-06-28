// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Discover content page. Type filter (All/Games/Movies/Series) + a contained
// "hybrid" hero (ambient blurred art + crisp poster) + horizontal poster rows
// from DiscoveryService (TMDB/IGDB). Click a poster → openSearch(title).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
import "theme"
import "widgets"

Rectangle {
    id: page
    color: Theme.bg

    signal openSearch(string query)

    readonly property var api: typeof discovery !== "undefined" ? discovery : null
    readonly property var heroList: api ? api.hero : []
    readonly property var rowList: api ? api.rows : []

    // ----- type filter (client-side; rows are homogeneous by item.type) -----
    property string typeFilter: "all"   // all | game | movie | series
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
    onTypeFilterChanged: { page.heroIndex = 0; page.shownIndex = 0 }

    property int heroIndex: 0
    property int shownIndex: 0
    readonly property var heroItem: (heroFiltered.length > shownIndex) ? heroFiltered[shownIndex] : null

    function fmtGB(bytes) {
        if (!bytes || bytes <= 0) return ""
        var gb = bytes / (1024 * 1024 * 1024)
        return (gb >= 10 ? Math.round(gb) : gb.toFixed(1)) + " GB"
    }
    function fmtCount(n) {
        return n >= 1000 ? (n / 1000).toFixed(1).replace(/\.0$/, "") + "k" : String(n)
    }

    // lazily-fetched trailer key + source summary for the current hero
    property string heroTrailerKey: ""
    property var heroSummary: null      // {count, bestSize, maxSeeds} for heroItem.title
    onHeroItemChanged: {
        heroTrailerKey = ""
        heroSummary = null
        if (api && heroItem && (heroItem.type === "movie" || heroItem.type === "series")
            && (heroItem.tmdbId || 0) > 0)
            api.fetchTrailer(heroItem.tmdbId, heroItem.type)
        if (typeof search !== "undefined" && heroItem && (heroItem.title || "").length > 0)
            search.summarizeSources(heroItem.title)
    }
    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onTrailerReady(tmdbId, youtubeKey) {
            if (page.heroItem && (page.heroItem.tmdbId || 0) === tmdbId) page.heroTrailerKey = youtubeKey
        }
    }
    Connections {
        target: typeof search !== "undefined" ? search : null
        ignoreUnknownSignals: true
        function onSourceSummary(title, count, bestSize, maxSeeds) {
            if (page.heroItem && page.heroItem.title === title)
                page.heroSummary = { count: count, bestSize: bestSize, maxSeeds: maxSeeds }
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
        id: heroTimer
        interval: 7000
        running: page.visible && page.heroFiltered.length > 1
        repeat: true
        onTriggered: page.heroIndex = (page.heroIndex + 1) % page.heroFiltered.length
    }

    // segmented filter button
    component Seg: Rectangle {
        id: seg
        property string label
        property string value
        readonly property bool on: page.typeFilter === seg.value
        signal clicked()
        radius: 8
        implicitHeight: 30
        implicitWidth: segTxt.implicitWidth + 26
        color: on ? Theme.accentTint : (segMa.containsMouse ? Theme.hover : "transparent")
        Behavior on color { ColorAnimation { duration: 120 } }
        Text {
            id: segTxt
            anchors.centerIn: parent
            text: seg.label
            color: seg.on ? Theme.accentText : (segMa.containsMouse ? Theme.t2 : Theme.t3)
            font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
        }
        MouseArea {
            id: segMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: seg.clicked()
        }
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

            // ---------- header: title + filter + refresh (scrolls with content) ----------
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5
                Layout.rightMargin: Theme.sp4
                Layout.topMargin: Theme.sp4
                spacing: 16
                Text {
                    text: (i18n.language, i18n.t("nav_discover"))
                    color: Theme.t1; font.pixelSize: 20; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                Seg { label: (i18n.language, i18n.t("filter_all"));   value: "all";    onClicked: page.typeFilter = "all" }
                Seg { label: (i18n.language, i18n.t("cat_games"));    value: "game";   onClicked: page.typeFilter = "game" }
                Seg { label: (i18n.language, i18n.t("cat_movies"));   value: "movie";  onClicked: page.typeFilter = "movie" }
                Seg { label: (i18n.language, i18n.t("cat_series"));   value: "series"; onClicked: page.typeFilter = "series" }
                Item { Layout.fillWidth: true }
                Rectangle {
                    Layout.preferredWidth: 34; Layout.preferredHeight: 34; radius: 17
                    color: rfMa.containsMouse ? Theme.hover : "transparent"
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
                        id: rfMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: if (page.api) page.api.refresh()
                    }
                }
            }

            // ---------- hybrid hero (contained card) ----------
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: Math.max(340, Math.round(page.height * 0.40))
                Layout.leftMargin: Theme.sp5
                Layout.rightMargin: Theme.sp4
                visible: page.heroItem !== null

                Rectangle {
                    anchors.fill: parent
                    radius: 16
                    color: Theme.elev
                    border.color: Theme.hair; border.width: 1
                    clip: true

                    Item {
                        id: heroContent
                        anchors.fill: parent

                        // ambient background: backdrop, or the poster when the backdrop is bad/missing (games)
                        Image {
                            id: heroBg
                            anchors.fill: parent
                            source: page.heroItem
                                ? ((page.heroItem.backdrop && page.heroItem.backdrop.length > 0)
                                   ? page.heroItem.backdrop : (page.heroItem.poster || ""))
                                : ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true; cache: true
                            // decode small — it's blurred anyway, and blurring a full-res
                            // backdrop every frame is what made scroll crawl on Windows/D3D
                            sourceSize: Qt.size(640, 360)
                            visible: false
                        }
                        MultiEffect {
                            anchors.fill: heroBg
                            source: heroBg
                            blurEnabled: true
                            blur: 1.0
                            blurMax: 32
                            brightness: -0.25
                            saturation: -0.1
                        }
                        // horizontal scrim — heavy on the left for text legibility
                        Rectangle {
                            anchors.fill: parent
                            gradient: Gradient {
                                orientation: Gradient.Horizontal
                                GradientStop { position: 0.0;  color: "#f00b0b0d" }
                                GradientStop { position: 0.55; color: "#cc0b0b0d" }
                                GradientStop { position: 1.0;  color: "#700b0b0d" }
                            }
                        }

                        // right: crisp poster (rounded via mask)
                        Item {
                            id: heroPoster
                            anchors.right: parent.right; anchors.rightMargin: Theme.sp5
                            anchors.verticalCenter: parent.verticalCenter
                            height: Math.min(parent.height - Theme.sp5 * 2, 268)
                            width: height * 0.67
                            visible: page.heroItem && (page.heroItem.poster || "").length > 0
                            Rectangle {
                                id: hpImg
                                anchors.fill: parent; color: "#161618"; visible: false; layer.enabled: true
                                Image {
                                    anchors.fill: parent
                                    source: page.heroItem ? (page.heroItem.poster || "") : ""
                                    fillMode: Image.PreserveAspectCrop
                                    asynchronous: true; cache: true
                                    sourceSize: Qt.size(360, 540)
                                }
                            }
                            Rectangle { id: hpMask; anchors.fill: parent; radius: 12; color: "white"; visible: false; layer.enabled: true }
                            MultiEffect {
                                source: hpImg; anchors.fill: parent
                                maskEnabled: true; maskSource: hpMask
                                shadowEnabled: true; shadowBlur: 0.7; shadowColor: "#cc000000"; shadowVerticalOffset: 8
                            }
                        }

                        // left: text + CTAs
                        ColumnLayout {
                            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                            anchors.right: heroPoster.visible ? heroPoster.left : parent.right
                            anchors.leftMargin: Theme.sp5; anchors.topMargin: Theme.sp5
                            anchors.bottomMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
                            spacing: 9

                            Item { Layout.fillHeight: true }

                            Text {   // eyebrow: FEATURED · TYPE
                                text: "FEATURED   ·   " + (page.heroItem ? (page.heroItem.type || "").toUpperCase() : "")
                                color: "#5bc4d4"; font.pixelSize: 11; font.weight: Font.Bold
                                font.letterSpacing: 1.2; font.family: Theme.fontSans
                            }
                            Text {
                                text: page.heroItem ? page.heroItem.title : ""
                                color: "#ffffff"
                                font.pixelSize: 32; font.weight: Font.Bold; font.family: Theme.fontSans
                                Layout.fillWidth: true; elide: Text.ElideRight; maximumLineCount: 2; wrapMode: Text.WordWrap
                            }
                            RowLayout {   // year · ★rating · ●seeds
                                spacing: 12
                                Text {
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
                                Row {
                                    spacing: 6
                                    visible: page.heroSummary && page.heroSummary.maxSeeds > 0
                                    Rectangle { width: 7; height: 7; radius: 4; color: Theme.grn; anchors.verticalCenter: parent.verticalCenter }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: page.heroSummary ? (page.fmtCount(page.heroSummary.maxSeeds) + " seeds") : ""
                                        color: Theme.grn; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                    }
                                }
                            }
                            Text {
                                text: page.heroItem ? page.heroItem.overview : ""
                                color: "#d8d8dc"; font.pixelSize: 13; font.family: Theme.fontSans
                                Layout.fillWidth: true; Layout.maximumWidth: 620
                                wrapMode: Text.WordWrap; maximumLineCount: 3; elide: Text.ElideRight
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
                                Text {   // N sources · best X GB (when the lookup resolves)
                                    Layout.leftMargin: 4
                                    visible: page.heroSummary && page.heroSummary.count > 0
                                    text: {
                                        if (!page.heroSummary) return ""
                                        var s = page.heroSummary.count + " sources"
                                        if (page.heroSummary.bestSize > 0) s += "    ·    best " + page.fmtGB(page.heroSummary.bestSize)
                                        return s
                                    }
                                    color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontMono
                                }
                            }

                            Item { Layout.fillHeight: true }
                        }
                    }

                    // carousel dots — click to jump between featured items
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottom: parent.bottom
                        anchors.bottomMargin: Theme.sp4
                        spacing: 7
                        z: 5
                        visible: page.heroFiltered.length > 1
                        Repeater {
                            model: page.heroFiltered.length
                            delegate: Rectangle {
                                required property int index
                                width: index === page.shownIndex ? 20 : 7
                                height: 7; radius: 3.5
                                color: index === page.shownIndex ? Theme.accent : Qt.rgba(1, 1, 1, 0.35)
                                Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                                Behavior on color { ColorAnimation { duration: 200 } }
                                MouseArea {
                                    anchors.fill: parent; anchors.margins: -5
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: { page.heroIndex = index; heroTimer.restart() }
                                }
                            }
                        }
                    }
                }
            }

            // ---------- poster rows ----------
            Repeater {
                model: page.rowsFiltered
                delegate: ColumnLayout {
                    id: rowItem
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 10

                    readonly property bool inView: (y + height) > (flick.contentY - 400)
                                                   && y < (flick.contentY + flick.height + 400)
                    property bool everInView: false
                    onInViewChanged: if (inView) everInView = true
                    Component.onCompleted: if (inView) everInView = true

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.leftMargin: Theme.sp5
                        Layout.rightMargin: Theme.sp5
                        spacing: 10
                        Text {
                            text: rowItem.modelData.label
                            color: Theme.t1
                            font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                        }
                        Text {   // "See all" → filter to this row's type
                            visible: rowItem.modelData.items && rowItem.modelData.items.length > 0
                            text: (i18n.language, i18n.t("see_all"))
                            color: saMa.containsMouse ? Theme.t1 : Theme.t4
                            font.pixelSize: 13; font.weight: Font.Medium; font.family: Theme.fontSans
                            MouseArea {
                                id: saMa; anchors.fill: parent; anchors.margins: -4
                                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    var items = rowItem.modelData.items
                                    if (items && items.length > 0) page.typeFilter = items[0].type
                                }
                            }
                        }
                        Item { Layout.fillWidth: true }
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
        Item { Layout.fillWidth: true; Layout.preferredHeight: 52 }
        Rectangle {
            Layout.fillWidth: true; Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp4
            Layout.preferredHeight: Math.max(340, Math.round(page.height * 0.40)); radius: 16; color: Theme.elev
        }
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

    // empty / no-keys / empty-filter state (not loading)
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 14
        visible: page.rowsFiltered.length === 0 && !(page.api && page.api.loading)
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: page.api && page.api.statusText.length > 0 ? page.api.statusText : (i18n.language, i18n.t("discover_empty"))
            color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
        }
    }
}
