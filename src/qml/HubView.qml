// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// HUB page (4.0 step ⑤) — your library of watchable video torrents. "Continue
// watching" (resume from where you stopped, with a watched-% bar on the poster)
// plus a grid of every movie; click → embedded player. Built on session.movieLibrary().
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "theme"
import "widgets"

Item {
    id: page
    signal openSearch(string query)
    property var api: typeof session !== "undefined" ? session : null
    property var library: []
    property var gameItems: []
    // most-recent first, capped — the two "continue" rails at the top
    readonly property var continueItems: library.filter(function (i) { return (i.resumeMs || 0) > 0 })
        .sort(function (a, b) { return (b.resumeAt || 0) - (a.resumeAt || 0) }).slice(0, 3)
    readonly property var continuePlaying: gameItems.filter(function (i) { return (i.lastPlayed || 0) > 0 })
        .sort(function (a, b) { return (b.lastPlayed || 0) - (a.lastPlayed || 0) }).slice(0, 3)
    readonly property bool empty: library.length === 0 && gameItems.length === 0

    // continue rails are sized to hold exactly 3 cards each
    readonly property int railCardW: 134
    readonly property int railSpacing: 16
    readonly property int railW: 3 * railCardW + 2 * railSpacing

    // library search + sort (applies to the Movies/Games grids, not the rails)
    property string librarySearch: ""
    property string librarySort: "recent"   // recent | name

    function refresh() {
        library = api ? api.movieLibrary() : []
        gameItems = api ? api.gameLibrary() : []
    }
    onVisibleChanged: if (visible) refresh()
    Component.onCompleted: refresh()

    // live: a download finishing while the HUB is open shows up without re-entering
    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onTorrentFinished(name, hash) { if (page.visible) page.refresh() }
        function onGamesChanged() { if (page.visible) page.gameItems = page.api ? page.api.gameLibrary() : [] }
    }

    // human "12h 30m" / "45m" from seconds
    function fmtPlaytime(secs) {
        if (!secs || secs <= 0) return ""
        var h = Math.floor(secs / 3600), m = Math.floor((secs % 3600) / 60)
        return h > 0 ? (h + "h " + m + "m") : (m + "m")
    }

    function applyView(list) {
        var q = librarySearch.trim().toLowerCase()
        var arr = q.length > 0
            ? list.filter(function (i) { return (i.title || "").toLowerCase().indexOf(q) >= 0 })
            : list.slice()
        if (librarySort === "name")
            arr.sort(function (a, b) { return (a.title || "").localeCompare(b.title || "") })
        return arr
    }
    function fmtTime(ms) {
        if (!ms || ms <= 0) return ""
        var s = Math.floor(ms / 1000), h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), ss = s % 60
        function pad(n) { return (n < 10 ? "0" : "") + n }
        return (h > 0 ? h + ":" + pad(m) : m + "") + ":" + pad(ss)
    }

    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        if (p.indexOf("file:") === 0) return p
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }

    // Play a movie: single video → play; multiple (a series) → pick an episode.
    function playMovie(item) {
        if (!api) return
        if (item.videos && item.videos.length > 1) episodeMenu.openFor(item)
        else api.playByHash(item.infoHash)
    }
    // Play a game: launchGame uses the manual exe if set, else auto-detects one,
    // else opens the folder. "Set executable…" (right-click) is the override.
    function playGame(hash) {
        if (api) api.launchGame(hash)
    }
    function fmtAgo(ms) {
        if (!ms || ms <= 0) return ""
        var d = Math.floor((Date.now() - ms) / 86400000)
        if (d <= 0) return i18n.t("hub_today")
        if (d === 1) return i18n.t("hub_yesterday")
        return i18n.t("hub_days_ago").replace("%1", d)
    }
    function openExePicker(hash, launchAfter) {
        exePicker.pendingHash = hash
        exePicker.launchAfter = launchAfter
        var folder = api ? api.gameFolder(hash) : ""
        if (folder.length > 0) exePicker.currentFolder = page.fileUrl(folder)
        exePicker.open()
    }

    Rectangle { anchors.fill: parent; color: Theme.bg }

    // The hub always shows its structure (greeting + the two continue rails) so
    // a fresh, empty library reads as onboarding rather than a dead screen.
    Flickable {
        anchors.fill: parent
        contentHeight: col.implicitHeight + 32
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ColumnLayout {
            id: col
            width: parent.width
            spacing: 22

            // greeting
            Text {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; Layout.topMargin: Theme.sp5 + 4
                text: (i18n.language, i18n.t("hub_greeting"))
                color: Theme.t1; font.pixelSize: 25; font.weight: Font.Bold; font.family: Theme.fontSans
            }

            // library search + sort
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: Theme.sp3
                visible: page.library.length > 0 || page.gameItems.length > 0
                TFld {
                    Layout.preferredWidth: 260; Layout.preferredHeight: 32
                    icon: "qrc:/icons/search.svg"; clearable: true
                    placeholder: (i18n.language, i18n.t("hub_search_lib"))
                    onTextChanged: page.librarySearch = text
                }
                Item { Layout.fillWidth: true }
                TSelect {
                    Layout.preferredWidth: 150; Layout.preferredHeight: 32
                    property var keys: ["recent", "name"]
                    model: [i18n.t("hub_sort_recent"), i18n.t("hub_sort_name")]
                    onActivated: page.librarySort = keys[currentIndex]
                }
            }

            // Continue watching | Continue playing — side by side, ≤3 each, with a
            // placeholder prompt when nothing has been watched/played yet.
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 32

                // each rail is sized to hold exactly 3 cards (cards row or placeholder)
                ColumnLayout {
                    Layout.preferredWidth: page.railW; Layout.alignment: Qt.AlignTop
                    spacing: 12
                    Text {
                        text: (i18n.language, i18n.t("hub_continue"))
                        color: Theme.t1; font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                    Row {
                        spacing: page.railSpacing
                        visible: page.continueItems.length > 0
                        Repeater {
                            model: page.continueItems
                            delegate: HubCard {
                                cardW: page.railCardW; item: modelData
                                onPlay: page.playMovie(modelData)
                                onContext: continueMenu.openFor(modelData.infoHash, modelData.fileIndex)
                            }
                        }
                    }
                    RailPlaceholder { visible: page.continueItems.length === 0; text: (i18n.language, i18n.t("hub_watch_placeholder")) }
                }

                ColumnLayout {
                    Layout.preferredWidth: page.railW; Layout.alignment: Qt.AlignTop
                    spacing: 12
                    Text {
                        text: (i18n.language, i18n.t("hub_continue_playing"))
                        color: Theme.t1; font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                    Row {
                        spacing: page.railSpacing
                        visible: page.continuePlaying.length > 0
                        Repeater {
                            model: page.continuePlaying
                            delegate: HubCard {
                                cardW: page.railCardW; item: modelData; requireDoubleClick: true
                                onPlay: page.playGame(modelData.infoHash)
                                onContext: gameMenu.openFor(modelData.infoHash)
                            }
                        }
                    }
                    RailPlaceholder { visible: page.continuePlaying.length === 0; text: (i18n.language, i18n.t("hub_play_placeholder")) }
                }

                Item { Layout.fillWidth: true }   // keep both rails left-aligned at their 3-card width
            }

            // My List (saved titles) — click to find it in Search; heart to remove
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 12
                visible: page.api && page.api.watchlist.length > 0
                Text {
                    text: (i18n.language, i18n.t("hub_mylist"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 268
                    orientation: ListView.Horizontal
                    spacing: 16
                    clip: true
                    model: page.api ? page.api.watchlist : []
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: PosterCard {
                        required property var modelData
                        posterW: 150
                        title: modelData.title || ""
                        poster: modelData.poster || ""
                        year: modelData.year || ""
                        type: modelData.type || ""
                        watchlistEnabled: true
                        saved: true
                        onWatchlistToggle: if (page.api) page.api.toggleWatchlist(modelData)
                        onActivated: page.openSearch(modelData.title || "")
                    }
                }
            }

            // Your games
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 12
                visible: page.gameItems.length > 0
                Text {
                    text: (i18n.language, i18n.t("hub_games"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                GridLayout {
                    Layout.fillWidth: true
                    columnSpacing: 18
                    rowSpacing: 20
                    columns: Math.max(1, Math.floor((page.width - 2 * Theme.sp5 + columnSpacing) / (150 + columnSpacing)))
                    Repeater {
                        model: page.applyView(page.gameItems)
                        delegate: HubCard {
                            item: modelData
                            requireDoubleClick: true
                            onPlay: page.playGame(modelData.infoHash)
                            onContext: gameMenu.openFor(modelData.infoHash)
                        }
                    }
                }
            }

            // Your movies
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                Layout.bottomMargin: Theme.sp5
                spacing: 12
                visible: page.library.length > 0
                Text {
                    text: (i18n.language, i18n.t("hub_movies"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                GridLayout {
                    Layout.fillWidth: true
                    columnSpacing: 18
                    rowSpacing: 20
                    columns: Math.max(1, Math.floor((page.width - 2 * Theme.sp5 + columnSpacing) / (150 + columnSpacing)))
                    Repeater {
                        model: page.applyView(page.library)
                        delegate: HubCard { item: modelData; onPlay: page.playMovie(modelData) }
                    }
                }
            }
        }
    }

    // dashed-feel prompt shown in a "continue" rail when it has nothing yet
    component RailPlaceholder: Rectangle {
        property alias text: ph.text
        Layout.fillWidth: true
        Layout.preferredHeight: Math.round(page.railCardW * 1.5)   // a poster-sized empty slot
        radius: 10
        color: "transparent"
        border.color: Theme.hairSoft; border.width: 1
        Text {
            id: ph
            anchors.centerIn: parent
            width: parent.width - 32
            color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
        }
    }

    // 2:3 poster card with hover play, a download badge (while incomplete) and a
    // watched-% bar along the bottom. Emits play().
    component HubCard: Item {
        id: card
        property var item
        property int cardW: 150
        property bool requireDoubleClick: false
        readonly property int cardH: Math.round(cardW * 1.5)
        signal play()
        signal context()
        implicitWidth: cardW
        implicitHeight: cardH + 38

        Item {
            id: art
            width: card.cardW; height: card.cardH
            scale: ma.containsMouse ? 1.04 : 1.0
            Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

            Rectangle {
                anchors.fill: parent; radius: 10; color: "#161618"; clip: true
                border.color: ma.containsMouse ? Theme.accent : Theme.hair; border.width: 1
                opacity: img.status === Image.Ready ? 0 : 1
                Behavior on opacity { NumberAnimation { duration: 250 } }
                Behavior on border.color { ColorAnimation { duration: 140 } }
                Image {
                    anchors.centerIn: parent; width: parent.width * 0.4; height: width
                    source: "qrc:/images/logo.svg"; sourceSize: Qt.size(width * 2, width * 2)
                    fillMode: Image.PreserveAspectFit; opacity: 0.3
                }
            }
            Image {
                id: img; anchors.fill: parent
                source: card.item.poster || ""
                fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true
                visible: false; sourceSize: Qt.size(card.cardW * 2, card.cardH * 2)
            }
            Rectangle { id: mask; anchors.fill: parent; radius: 10; visible: false; layer.enabled: true }
            MultiEffect {
                anchors.fill: parent; source: img; maskEnabled: true; maskSource: mask
                opacity: img.status === Image.Ready ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
            }

            // hover play overlay
            Rectangle {
                anchors.fill: parent; radius: 10; color: "#66000000"
                opacity: ma.containsMouse ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 140 } }
                IconImg { anchors.centerIn: parent; src: "qrc:/icons/play.svg"; tint: "white"; s: 40 }
            }

            // "playing now" badge
            Rectangle {
                visible: card.item.playing === true
                anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6
                radius: 6; color: Theme.accent
                implicitWidth: pnl.width + 12; implicitHeight: 18
                Text {
                    id: pnl; anchors.centerIn: parent
                    text: (i18n.language, i18n.t("hub_playing_now"))
                    color: Theme.accentText; font.pixelSize: 9; font.weight: Font.Bold; font.family: Theme.fontSans
                }
            }

            // download badge (still downloading)
            Rectangle {
                visible: !card.item.completed
                anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 6
                radius: 6; color: "#cc000000"
                implicitWidth: dlt.width + 12; implicitHeight: 18
                Text {
                    id: dlt; anchors.centerIn: parent
                    text: "↓ " + Math.round((card.item.progress || 0) * 100) + "%"
                    color: Theme.accent; font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
            }

            // elapsed / total time (resume)
            Rectangle {
                visible: (card.item.watchedPct || 0) > 0 && (card.item.durMs || 0) > 0
                anchors.bottom: parent.bottom; anchors.right: parent.right
                anchors.bottomMargin: 12; anchors.rightMargin: 6
                radius: 5; color: "#cc000000"
                implicitWidth: tlabel.width + 12; implicitHeight: 17
                Text {
                    id: tlabel; anchors.centerIn: parent
                    text: page.fmtTime(card.item.resumeMs) + " / " + page.fmtTime(card.item.durMs)
                    color: "#e8e8ec"; font.pixelSize: 9; font.weight: Font.DemiBold; font.family: Theme.fontMono
                }
            }

            // watched-% bar
            Rectangle {
                visible: (card.item.watchedPct || 0) > 0
                anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
                anchors.margins: 5
                height: 4; radius: 2; color: "#99000000"
                Rectangle {
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: parent.width * Math.min(1, card.item.watchedPct)
                    radius: 2; color: Theme.accent
                }
            }
        }

        Text {
            id: titleLabel
            anchors.top: art.bottom; anchors.topMargin: 8
            width: card.cardW
            text: card.item.title || ""
            color: ma.containsMouse ? Theme.t1 : Theme.t2
            font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
            elide: Text.ElideRight; maximumLineCount: 1
            Behavior on color { ColorAnimation { duration: 140 } }
        }
        Text {
            anchors.top: titleLabel.bottom; anchors.topMargin: 2
            width: card.cardW
            visible: text.length > 0
            text: card.item.playing === true ? (i18n.language, i18n.t("hub_playing_now"))
                : (card.item.playSeconds || 0) > 0 ? page.fmtPlaytime(card.item.playSeconds) + " " + i18n.t("hub_played")
                : (card.item.lastPlayed || 0) > 0 ? page.fmtAgo(card.item.lastPlayed)
                : ""
            color: card.item.playing === true ? Theme.accent : Theme.t4
            font.pixelSize: 10; font.family: Theme.fontSans
            elide: Text.ElideRight
        }

        MouseArea {
            id: ma; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: function (mouse) {
                if (mouse.button === Qt.RightButton) { card.context(); return }
                if (!card.requireDoubleClick) card.play()
            }
            onDoubleClicked: if (card.requireDoubleClick) card.play()
        }

        // rich hover card: genres + description (games carry IGDB metadata)
        ToolTip {
            id: richTip
            parent: card
            visible: ma.containsMouse && (card.item.description || "").length > 0
            delay: 600
            width: 300
            padding: 12
            x: card.cardW + 10
            y: (card.cardH - height) / 2
            contentItem: ColumnLayout {
                spacing: 6
                Text {
                    visible: text.length > 0
                    text: ((card.item.genres && card.item.genres.length > 0) ? card.item.genres.join("  ·  ") : "")
                          + (((card.item.rating || 0) > 0) ? ((card.item.genres && card.item.genres.length > 0 ? "   " : "") + "★ " + Number(card.item.rating).toFixed(1)) : "")
                    color: Theme.t4; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    Layout.maximumWidth: 276; wrapMode: Text.WordWrap
                }
                Text {
                    text: card.item.description || ""
                    width: 276; Layout.maximumWidth: 276
                    wrapMode: Text.WordWrap; maximumLineCount: 8; elide: Text.ElideRight
                    color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans; lineHeight: 1.35
                }
            }
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 9 }
        }
    }

    // ---- games: manual executable picker + per-game context menu ----
    FileDialog {
        id: exePicker
        property string pendingHash: ""
        property bool launchAfter: true
        title: (i18n.language, i18n.t("hub_set_exe"))
        onAccepted: {
            if (!page.api || pendingHash.length === 0) return
            page.api.setGameExe(pendingHash, selectedFile.toString())
            page.refresh()
            if (launchAfter) page.api.launchGame(pendingHash)
        }
    }

    BatMenu {
        id: gameMenu
        property string hash: ""
        function openFor(h) { hash = h; popup() }
        implicitWidth: 200
        BatMenuItem {
            text: (i18n.language, i18n.t("hub_set_exe"))
            onTriggered: page.openExePicker(gameMenu.hash, false)
        }
        BatMenuItem {
            text: (i18n.language, i18n.t("hub_open_folder"))
            onTriggered: if (page.api) Qt.openUrlExternally(page.fileUrl(page.api.gameFolder(gameMenu.hash)))
        }
    }

    BatMenu {
        id: continueMenu
        property string hash: ""
        property int fileIdx: 0
        function openFor(h, f) { hash = h; fileIdx = f || 0; popup() }
        implicitWidth: 210
        BatMenuItem {
            text: (i18n.language, i18n.t("hub_remove_continue"))
            onTriggered: {
                if (page.api) page.api.clearResume(continueMenu.hash, continueMenu.fileIdx)
                page.refresh()
            }
        }
    }

    // episode picker for multi-video torrents (series). Pulls real episode
    // titles from TMDB per season and merges them in live (falls back to file
    // names if there's no metadata).
    BatMenu {
        id: episodeMenu
        property string hash: ""
        property var videos: []
        property int tmdbId: 0
        property var titles: ({})      // "season_episode" → title
        function openFor(item) {
            hash = item.infoHash
            videos = item.videos || []
            tmdbId = item.tmdbId || 0
            titles = ({})
            if (tmdbId > 0 && page.api) {
                var seen = ({})
                for (var i = 0; i < videos.length; i++) {
                    var sn = videos[i].season
                    if (sn >= 0 && !seen[sn]) { seen[sn] = true; page.api.fetchEpisodes(tmdbId, sn) }
                }
            }
            popup()
        }
        implicitWidth: 380
        Connections {
            target: page.api
            ignoreUnknownSignals: true
            function onEpisodesReady(tmdbId, season, episodes) {
                if (tmdbId !== episodeMenu.tmdbId) return
                var t = Object.assign({}, episodeMenu.titles)
                for (var i = 0; i < episodes.length; i++)
                    t[season + "_" + episodes[i].episode] = episodes[i].name
                episodeMenu.titles = t
            }
        }
        Repeater {
            model: episodeMenu.videos
            BatMenuItem {
                id: epItem
                required property var modelData
                text: {
                    var m = epItem.modelData
                    var check = m.watched ? "✓  " : ""
                    if (m.season >= 0 && m.episode >= 0) {
                        var title = episodeMenu.titles[m.season + "_" + m.episode] || m.name
                        return check + "S" + m.season + "·E" + (m.episode < 10 ? "0" + m.episode : m.episode) + "  —  " + title
                    }
                    return check + m.name
                }
                elideMode: Text.ElideMiddle
                onTriggered: if (page.api) page.api.playFile(episodeMenu.hash, epItem.modelData.idx)
            }
        }
    }
}
