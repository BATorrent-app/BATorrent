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
    signal goDiscover()
    property var api: typeof session !== "undefined" ? session : null
    property var library: []
    property var gameItems: []
    // most-recent first, capped — the two "continue" rails at the top
    readonly property var continueItems: library.filter(function (i) { return (i.resumeMs || 0) > 0 })
        .sort(function (a, b) { return (b.resumeAt || 0) - (a.resumeAt || 0) }).slice(0, 3)
    readonly property var continuePlaying: gameItems.filter(function (i) { return (i.lastPlayed || 0) > 0 })
        .sort(function (a, b) { return (b.lastPlayed || 0) - (a.lastPlayed || 0) }).slice(0, 3)
    // nothing played yet → suggest a ready-to-play game so the hero isn't a dead placeholder
    readonly property var suggestedGame: {
        for (var i = 0; i < gameItems.length; i++)
            if (gameItems[i].installState === 4) return gameItems[i]
        return null
    }
    readonly property bool empty: library.length === 0 && gameItems.length === 0
    // newest in your library (movies + games), front and centre — Plex/Netflix style
    readonly property var recentlyAdded: {
        var all = library.concat(gameItems)
        all.sort(function (a, b) { return (b.addedTime || 0) - (a.addedTime || 0) })
        return all.slice(0, 12)
    }

    // ---- "Recommended for you": match your library's top genre to a discovery
    // genre-shelf. Game genres (IGDB) are English-stable; movie genres handle EN/PT. ----
    function genreKey(name) {
        var s = (name || "").toLowerCase()
        if (s.indexOf("rpg") >= 0 || s.indexOf("role") >= 0) return "rpg"
        if (s.indexOf("shoot") >= 0 || s.indexOf("tiro") >= 0 || s.indexOf("fps") >= 0) return "shooter"
        if (s.indexOf("strateg") >= 0 || s.indexOf("estrat") >= 0) return "strategy"
        if (s.indexOf("indie") >= 0) return "indie"
        if (s.indexOf("sci") >= 0 || s.indexOf("ficç") >= 0 || s.indexOf("cient") >= 0) return "scifi"
        if (s.indexOf("horror") >= 0 || s.indexOf("terror") >= 0) return "horror"
        if (s.indexOf("action") >= 0 || s.indexOf("ação") >= 0 || s.indexOf("acao") >= 0) return "action"
        return ""
    }
    readonly property string topGenre: {
        var counts = ({})
        function tally(arr) {
            if (!arr) return
            for (var i = 0; i < arr.length; i++) {
                var k = page.genreKey(arr[i])
                if (k.length > 0) counts[k] = (counts[k] || 0) + 1
            }
        }
        for (var g = 0; g < gameItems.length; g++) tally(gameItems[g].genres)
        for (var m = 0; m < library.length; m++) tally(library[m].genres)
        var best = "", bestN = 0
        for (var key in counts) if (counts[key] > bestN) { bestN = counts[key]; best = key }
        return best
    }
    readonly property var recommendations: {
        if (topGenre.length === 0 || !disco) return []
        var rows = disco.rows || []
        var have = ({})
        for (var i = 0; i < library.length; i++) have[(library[i].title || "").toLowerCase()] = true
        for (var j = 0; j < gameItems.length; j++) have[(gameItems[j].title || "").toLowerCase()] = true
        var out = []
        for (var r = 0; r < rows.length && out.length < 12; r++) {
            if (rows[r].genre !== topGenre) continue
            var items = rows[r].items || []
            for (var k = 0; k < items.length && out.length < 12; k++) {
                var it = items[k]
                if (have[(it.title || "").toLowerCase()]) continue
                out.push(it)
            }
        }
        return out
    }

    // "Because you watched {X}" — TMDB per-title recommendations for your latest watch
    readonly property var recSeed: {
        for (var i = 0; i < continueItems.length; i++)
            if ((continueItems[i].tmdbId || 0) > 0) return continueItems[i]
        return null
    }
    property var perTitleRecs: []
    onRecSeedChanged: {
        perTitleRecs = []
        if (recSeed && disco) disco.fetchRecommendations(recSeed.tmdbId, recSeed.isSeries ? "series" : "movie")
    }
    Connections {
        target: page.disco
        ignoreUnknownSignals: true
        function onRecommendationsReady(tmdbId, items) {
            if (!page.recSeed || page.recSeed.tmdbId !== tmdbId) return
            var have = ({})
            for (var i = 0; i < page.library.length; i++) have[(page.library[i].title || "").toLowerCase()] = true
            var out = []
            for (var k = 0; k < items.length && out.length < 12; k++) {
                if (have[(items[k].title || "").toLowerCase()]) continue
                out.push(items[k])
            }
            page.perTitleRecs = out
        }
        function onGameRecommendationsReady(gameName, items) {
            if (!page.gameSeed || (page.gameSeed.title || "") !== gameName) return
            var have = ({})
            for (var i = 0; i < page.gameItems.length; i++) have[(page.gameItems[i].title || "").toLowerCase()] = true
            var out = []
            for (var k = 0; k < items.length && out.length < 12; k++) {
                if (have[(items[k].title || "").toLowerCase()]) continue
                out.push(items[k])
            }
            page.gameRecs = out
        }
    }
    // "Because you played {X}" — IGDB similar games for your latest played game
    readonly property var gameSeed: continuePlaying.length > 0 ? continuePlaying[0] : null
    property var gameRecs: []
    onGameSeedChanged: {
        gameRecs = []
        if (gameSeed && disco) disco.fetchGameRecommendations(gameSeed.title || "")
    }

    // continue rails are sized to hold exactly 3 cards each
    readonly property int railCardW: 134
    readonly property int railSpacing: 16
    readonly property int railW: 3 * railCardW + 2 * railSpacing

    // library search + sort (applies to the Movies/Games grids, not the rails)
    property string librarySearch: ""
    property string librarySort: "recent"   // recent | name

    // discovery feed (shared with the Discover page) — source for recommendations
    readonly property var disco: typeof discovery !== "undefined" ? discovery : null
    function refresh() {
        library = api ? api.movieLibrary() : []
        gameItems = api ? api.gameLibrary() : []
        if (disco) disco.load()   // ensure rows exist so "Recommended for you" can populate
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
    function fmtLeft(ms) {
        if (!ms || ms <= 0) return ""
        var s = Math.floor(ms / 1000), h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60)
        return i18n.t("hub_time_left").replace("%1", h > 0 ? (h + "h " + m + "m") : (m + "m"))
    }
    // S2E05 for the episode you're mid-way through (continue-watching a series)
    function episodeLabel(item) {
        if (!item || !item.isSeries || !item.videos) return ""
        for (var i = 0; i < item.videos.length; i++) {
            var v = item.videos[i]
            if (v.idx === item.fileIndex && (v.season > 0 || v.episode > 0))
                return "S" + v.season + "E" + (v.episode < 10 ? "0" + v.episode : v.episode)
        }
        return ""
    }
    function fmtSize(b) {
        if (!b || b <= 0) return ""
        var u = ["B", "KB", "MB", "GB", "TB"]
        var i = Math.min(u.length - 1, Math.floor(Math.log(b) / Math.log(1024)))
        return (b / Math.pow(1024, i)).toFixed(i >= 3 ? 1 : 0) + " " + u[i]
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
    // installState ints mirror QmlSessionBridge::GameInstallState:
    //   0 Downloading · 1 ReadyToInstall · 2 Extracting · 3 Installing
    //   4 Ready · 5 Playing · 6 NeedsSetup · 7 Failed
    // The card's primary gesture is state-driven (Steam model): one labelled action,
    // never a blind "open folder".
    function gamePrimary(item) {
        if (!api || !item) return
        switch (item.installState) {
        case 4: api.launchGame(item.infoHash); break          // Ready → Play
        case 1: case 7: api.installGame(item.infoHash); break // Install / Retry → run the chain
        case 6: page.openExePicker(item.infoHash, true); break // NeedsSetup → pick exe then launch
        case 3: page.gameMenuOpenFolder(item.infoHash); break  // Installing → reopen the setup folder
        // 0 Downloading / 2 Extracting / 5 Playing → busy, no-op
        default: break
        }
    }
    function gameStateLabel(item) {
        if (!item) return ""
        switch (item.installState) {
        case 0: return "↓ " + Math.round((item.progress || 0) * 100) + "%"
        case 1: return i18n.t("hub_gs_install")
        case 2: return i18n.t("hub_gs_extracting")
        case 3: return i18n.t("hub_gs_finish_setup")
        case 4: return i18n.t("hub_gs_play")
        case 5: return i18n.t("hub_gs_playing")
        case 6: return i18n.t("hub_gs_setup")
        case 7: return i18n.t("hub_gs_retry")
        }
        return ""
    }
    // actionable (accent) vs busy (muted) — drives the state button's colour
    function gameStateActionable(item) {
        if (!item) return false
        var s = item.installState
        return s === 1 || s === 4 || s === 6 || s === 7
    }
    // card footer line: state + size ("Ready to play · 9 GB", "↓ 64% · 12 GB", …)
    function cardStatus(item, isGame) {
        if (!item) return ""
        if (item.playing === true) return i18n.t("hub_playing_now")
        var sz = page.fmtSize(item.size || 0)
        var dot = sz ? ("  ·  " + sz) : ""
        if (isGame) {
            switch (item.installState) {
            case 0: return "↓ " + Math.round((item.progress || 0) * 100) + "%" + dot
            case 4: return i18n.t("hub_ready_to_play") + dot
            case 1: return i18n.t("hub_installed") + dot
            default: return page.gameStateLabel(item) + dot
            }
        }
        return item.completed ? (i18n.t("hub_installed") + dot)
                              : ("↓ " + Math.round((item.progress || 0) * 100) + "%" + dot)
    }
    function gameMenuOpenFolder(hash) {
        if (!api) return
        var folder = api.gameFolder(hash)
        if (folder && folder.length > 0) Qt.openUrlExternally(page.fileUrl(folder))
    }
    function fmtAgo(ms) {
        if (!ms || ms <= 0) return ""
        var h = Math.floor((Date.now() - ms) / 3600000)
        if (h < 24) return h < 1 ? i18n.t("hub_today") : i18n.t("hub_hours_ago").replace("%1", h)
        var d = Math.floor(h / 24)
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

    // ---- detail drawer state ----
    property var detailItem: null
    property bool detailIsGame: false
    property bool detailOpen: false
    function openDetail(item, isGame) { detailItem = item; detailIsGame = isGame === true; detailOpen = true }

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

            // empty library → guide the user to get content (first-run onboarding)
            ColumnLayout {
                Layout.fillWidth: true
                Layout.topMargin: 48
                spacing: 14
                visible: page.empty
                IconImg { Layout.alignment: Qt.AlignHCenter; src: "qrc:/icons/hub.svg"; tint: Theme.t4; s: 46; opacity: 0.7 }
                Text { Layout.alignment: Qt.AlignHCenter; text: (i18n.language, i18n.t("hub_empty_title")); color: Theme.t1; font.pixelSize: 18; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                Text { Layout.alignment: Qt.AlignHCenter; text: (i18n.language, i18n.t("hub_empty_sub")); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
                RowLayout {
                    Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 4; spacing: 12
                    BtnFlat { primary: true; icon: "qrc:/icons/discover.svg"; text: (i18n.language, i18n.t("nav_discover")); onClicked: page.goDiscover() }
                    BtnFlat { icon: "qrc:/icons/search.svg"; text: (i18n.language, i18n.t("nav_search")); onClicked: page.openSearch("") }
                }
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

            // Continue watching + Continue playing — the latest of each as a large
            // hero with one-click Resume + rich metadata.
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                visible: !page.empty
                spacing: 20

                // ---------- CONTINUE WATCHING ----------
                Rectangle {
                    id: cwHero
                    Layout.fillWidth: true
                    Layout.preferredHeight: 224
                    radius: 16; color: Theme.elev
                    border.color: Theme.hair; border.width: 1
                    clip: true
                    readonly property var it: page.continueItems.length > 0 ? page.continueItems[0] : null

                    Image { id: cwBg; anchors.fill: parent; visible: false; asynchronous: true; cache: true
                            source: cwHero.it ? (cwHero.it.poster || "") : ""; fillMode: Image.PreserveAspectCrop }
                    MultiEffect { anchors.fill: parent; source: cwBg; blurEnabled: true; blur: 1.0; blurMax: 40
                                  brightness: -0.35; saturation: -0.1; opacity: cwHero.it ? 0.5 : 0 }
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient { orientation: Gradient.Horizontal
                            GradientStop { position: 0.0; color: "#f00e0e10" }
                            GradientStop { position: 0.62; color: "#cc0e0e10" }
                            GradientStop { position: 1.0; color: "#660e0e10" }
                        }
                    }
                    RailPlaceholder { anchors.centerIn: parent; visible: cwHero.it === null; text: (i18n.language, i18n.t("hub_watch_placeholder")) }

                    Item {
                        anchors.fill: parent; anchors.margins: 24
                        visible: cwHero.it !== null

                        Item {   // crisp poster (right)
                            id: cwPoster
                            anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                            height: parent.height; width: height * 0.7
                            Rectangle { id: cwPC; anchors.fill: parent; color: "#161618"; visible: false; layer.enabled: true
                                Image { anchors.fill: parent; source: cwHero.it ? (cwHero.it.poster || "") : ""; fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true } }
                            Rectangle { id: cwPM; anchors.fill: parent; radius: 14; color: "white"; visible: false; layer.enabled: true }
                            MultiEffect { source: cwPC; anchors.fill: parent; maskEnabled: true; maskSource: cwPM }
                            Rectangle { anchors.fill: parent; radius: 14; color: "transparent"; border.color: "#33ffffff"; border.width: 1 }
                        }

                        ColumnLayout {
                            anchors.left: parent.left; anchors.right: cwPoster.left; anchors.rightMargin: 22
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 7
                            Text { text: (i18n.language, i18n.t("hub_continue")).toUpperCase(); color: Theme.accent; font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans }
                            Text { Layout.fillWidth: true; text: cwHero.it ? (cwHero.it.title || "") : ""; color: "#fff"; font.pixelSize: 27; font.weight: Font.Bold; font.family: Theme.fontSans; elide: Text.ElideRight; maximumLineCount: 2; wrapMode: Text.WordWrap }
                            Text {
                                color: Theme.t3; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                text: {
                                    if (!cwHero.it) return ""
                                    var parts = []
                                    var ep = page.episodeLabel(cwHero.it)
                                    if (ep.length > 0) parts.push(ep)
                                    else if ((cwHero.it.year || "").length > 0) parts.push(cwHero.it.year)
                                    parts.push(cwHero.it.isSeries ? i18n.t("hub_series") : i18n.t("hub_movie"))
                                    return parts.join("  ·  ")
                                }
                            }
                            RowLayout {
                                Layout.fillWidth: true; Layout.topMargin: 5; spacing: 12
                                Rectangle {
                                    Layout.fillWidth: true; Layout.preferredHeight: 4; Layout.alignment: Qt.AlignVCenter
                                    radius: 2; color: "#40ffffff"
                                    Rectangle { width: parent.width * Math.min(1, cwHero.it ? (cwHero.it.watchedPct || 0) : 0); height: parent.height; radius: 2; color: Theme.accent }
                                }
                                Text { text: cwHero.it ? (page.fmtTime(cwHero.it.resumeMs) + " / " + page.fmtTime(cwHero.it.durMs)) : ""; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono }
                            }
                            RowLayout {
                                Layout.topMargin: 6; spacing: 14
                                BtnFlat { primary: true; icon: "qrc:/icons/play.svg"; text: (i18n.language, i18n.t("hub_resume"));
                                          onClicked: if (cwHero.it && api) api.playByHashFile(cwHero.it.infoHash, cwHero.it.fileIndex) }
                                Text { text: cwHero.it ? page.fmtLeft((cwHero.it.durMs || 0) - (cwHero.it.resumeMs || 0)) : ""; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                            }
                        }
                    }
                }

                // ---------- CONTINUE PLAYING ----------
                Rectangle {
                    id: cpHero
                    Layout.fillWidth: true
                    Layout.preferredHeight: 224
                    readonly property bool sug: page.continuePlaying.length === 0 && it !== null
                    radius: 16; color: Theme.elev
                    border.color: Theme.hair; border.width: 1
                    clip: true
                    readonly property var it: page.continuePlaying.length > 0 ? page.continuePlaying[0] : page.suggestedGame

                    Image { id: cpBg; anchors.fill: parent; visible: false; asynchronous: true; cache: true
                            source: cpHero.it ? (cpHero.it.poster || "") : ""; fillMode: Image.PreserveAspectCrop }
                    MultiEffect { anchors.fill: parent; source: cpBg; blurEnabled: true; blur: 1.0; blurMax: 44
                                  brightness: -0.4; saturation: -0.1; opacity: cpHero.it ? 0.5 : 0 }
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient
                            { GradientStop { position: 0.0; color: "#e00e0e10" } GradientStop { position: 1.0; color: "#aa0e0e10" } }
                    }
                    Text {
                        anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 20
                        text: (i18n.language, (cpHero.sug ? i18n.t("hub_ready_to_play") : i18n.t("hub_continue_playing"))).toUpperCase()
                        color: Theme.accent; font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans
                        z: 2
                    }
                    RailPlaceholder { anchors.centerIn: parent; visible: cpHero.it === null; text: (i18n.language, i18n.t("hub_play_placeholder")) }

                    RowLayout {
                        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                        anchors.margins: 20
                        spacing: 14
                        visible: cpHero.it !== null
                        Item {   // small cover
                            Layout.preferredWidth: 78; Layout.preferredHeight: 104; Layout.alignment: Qt.AlignBottom
                            Rectangle { id: cpPC; anchors.fill: parent; color: "#161618"; visible: false; layer.enabled: true
                                Image { anchors.fill: parent; source: cpHero.it ? (cpHero.it.poster || "") : ""; fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true } }
                            Rectangle { id: cpPM; anchors.fill: parent; radius: 12; color: "white"; visible: false; layer.enabled: true }
                            MultiEffect { source: cpPC; anchors.fill: parent; maskEnabled: true; maskSource: cpPM }
                            Rectangle { anchors.fill: parent; radius: 12; color: "transparent"; border.color: "#33ffffff"; border.width: 1 }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.alignment: Qt.AlignBottom
                            spacing: 6
                            Text { Layout.fillWidth: true; text: cpHero.it ? (cpHero.it.title || "") : ""; color: "#fff"; font.pixelSize: 19; font.weight: Font.Bold; font.family: Theme.fontSans; elide: Text.ElideRight; maximumLineCount: 2; wrapMode: Text.WordWrap }
                            Text {
                                Layout.fillWidth: true; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight
                                text: {
                                    if (!cpHero.it) return ""
                                    if (cpHero.sug) return i18n.t("hub_ready_to_play")
                                    var parts = []
                                    if ((cpHero.it.playSeconds || 0) > 0) parts.push(page.fmtPlaytime(cpHero.it.playSeconds) + " " + i18n.t("hub_played"))
                                    var ago = page.fmtAgo(cpHero.it.lastPlayed)
                                    if (ago.length > 0) parts.push(ago)
                                    return parts.join("  ·  ")
                                }
                            }
                            BtnFlat { Layout.topMargin: 4; primary: true; icon: "qrc:/icons/play.svg"; text: (i18n.language, cpHero.sug ? i18n.t("hub_gs_play") : i18n.t("hub_resume")); onClicked: if (cpHero.it) page.gamePrimary(cpHero.it) }
                        }
                    }
                }
            }

            // Recently added — the newest in your library, front and centre
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 12
                visible: page.recentlyAdded.length > 0
                Text {
                    text: (i18n.language, i18n.t("hub_recent"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 264
                    orientation: ListView.Horizontal
                    spacing: 18
                    clip: true
                    model: page.recentlyAdded
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: HubCard {
                        required property var modelData
                        item: modelData
                        isGame: modelData.installState !== undefined
                        requireDoubleClick: isGame
                        onShowDetail: page.openDetail(modelData, isGame)
                        onPlay: isGame ? page.gamePrimary(modelData) : page.playMovie(modelData)
                        onContext: isGame ? gameMenu.openFor(modelData.infoHash)
                                          : continueMenu.openFor(modelData.infoHash, modelData.fileIndex)
                    }
                }
            }

            // Recommended for you — discovery picks matching your library's top genre
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 12
                visible: page.recommendations.length > 0
                Text {
                    text: (i18n.language, i18n.t("hub_recommended"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 268
                    orientation: ListView.Horizontal
                    spacing: 16
                    clip: true
                    model: page.recommendations
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: PosterCard {
                        required property var modelData
                        posterW: 150
                        title: modelData.title || ""
                        poster: modelData.poster || ""
                        year: modelData.year || ""
                        rating: modelData.rating || 0
                        type: modelData.type || ""
                        synopsis: modelData.overview || ""
                        watchlistEnabled: typeof session !== "undefined"
                        saved: typeof session !== "undefined"
                               && (session.watchlist, session.inWatchlist(modelData.title, modelData.type))
                        onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                            title: modelData.title, type: modelData.type, poster: modelData.poster, year: modelData.year })
                        onActivated: page.openSearch(modelData.title || "")
                        onGetWatch: if (typeof search !== "undefined")
                                        search.getAndWatch(modelData.title || "", modelData.year || "", modelData.type || "movie")
                    }
                }
            }

            // Because you watched {X} — per-title TMDB recommendations
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 12
                visible: page.perTitleRecs.length > 0 && page.recSeed !== null
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, i18n.t("hub_because_watched")).arg(page.recSeed ? (page.recSeed.title || "") : "")
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans; elide: Text.ElideRight
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 268
                    orientation: ListView.Horizontal
                    spacing: 16
                    clip: true
                    model: page.perTitleRecs
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: PosterCard {
                        required property var modelData
                        posterW: 150
                        title: modelData.title || ""
                        poster: modelData.poster || ""
                        year: modelData.year || ""
                        rating: modelData.rating || 0
                        type: modelData.type || ""
                        synopsis: modelData.overview || ""
                        watchlistEnabled: typeof session !== "undefined"
                        saved: typeof session !== "undefined"
                               && (session.watchlist, session.inWatchlist(modelData.title, modelData.type))
                        onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                            title: modelData.title, type: modelData.type, poster: modelData.poster, year: modelData.year })
                        onActivated: page.openSearch(modelData.title || "")
                        onGetWatch: if (typeof search !== "undefined")
                                        search.getAndWatch(modelData.title || "", modelData.year || "", modelData.type || "movie")
                    }
                }
            }

            // Because you played {X} — IGDB similar games
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5
                spacing: 12
                visible: page.gameRecs.length > 0 && page.gameSeed !== null
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, i18n.t("hub_because_played")).arg(page.gameSeed ? (page.gameSeed.title || "") : "")
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans; elide: Text.ElideRight
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 268
                    orientation: ListView.Horizontal
                    spacing: 16
                    clip: true
                    model: page.gameRecs
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: PosterCard {
                        required property var modelData
                        posterW: 150
                        title: modelData.title || ""
                        poster: modelData.poster || ""
                        year: modelData.year || ""
                        rating: modelData.rating || 0
                        type: modelData.type || "game"
                        synopsis: modelData.overview || ""
                        watchlistEnabled: typeof session !== "undefined"
                        saved: typeof session !== "undefined"
                               && (session.watchlist, session.inWatchlist(modelData.title, "game"))
                        onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                            title: modelData.title, type: "game", poster: modelData.poster, year: modelData.year })
                        onActivated: page.openSearch(modelData.title || "")
                    }
                }
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
                        synopsis: modelData.overview || ""
                        onWatchlistToggle: if (page.api) page.api.toggleWatchlist(modelData)
                        onActivated: page.openSearch(modelData.title || "")
                        // saved titles are now actionable: ▶ fetches + watches in one click
                        onGetWatch: if (typeof search !== "undefined")
                                        search.getAndWatch(modelData.title || "", modelData.year || "", modelData.type || "movie")
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
                            isGame: true
                            requireDoubleClick: true
                            onShowDetail: page.openDetail(modelData, true)
                            onPlay: page.gamePrimary(modelData)
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
                        delegate: HubCard { item: modelData; onShowDetail: page.openDetail(modelData, false); onPlay: page.playMovie(modelData); onContext: continueMenu.openFor(modelData.infoHash, modelData.fileIndex) }
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
        property bool isGame: false
        readonly property int cardH: Math.round(cardW * 1.5)
        signal play()
        signal context()
        signal showDetail()
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

            // hover play overlay (movies: a play glyph; games use the state button below)
            Rectangle {
                anchors.fill: parent; radius: 10; color: "#66000000"
                opacity: ma.containsMouse ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 140 } }
                IconImg {
                    anchors.centerIn: parent; src: "qrc:/icons/play.svg"; tint: "white"; s: 40
                    visible: !card.isGame
                }
            }

            // games: state-driven primary action (Install / Extracting / Play / …).
            // The label IS the differentiation — never a blind double-click.
            Rectangle {
                id: stateBtn
                visible: card.isGame && card.item.installState !== 0 && card.item.installState !== 5
                anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
                anchors.margins: 6
                height: 26; radius: 7
                readonly property bool actionable: page.gameStateActionable(card.item)
                color: actionable ? (sbma.containsMouse ? Qt.lighter(Theme.accent, 1.12) : Theme.accent)
                                  : "#cc1b1b1f"
                border.color: actionable ? "transparent" : Theme.hair
                border.width: actionable ? 0 : 1
                opacity: (ma.containsMouse || actionable) ? 1 : 0.92
                Behavior on opacity { NumberAnimation { duration: 140 } }
                Row {
                    anchors.centerIn: parent; spacing: 5
                    Spinner {
                        visible: card.item.installState === 2 || card.item.installState === 3
                        s: 13; tint: Theme.t2
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: page.gameStateLabel(card.item)
                        color: stateBtn.actionable ? Theme.accentText : Theme.t2
                        font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.fontSans
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
                MouseArea {
                    id: sbma; anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: stateBtn.actionable ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onClicked: page.gamePrimary(card.item)
                }
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

            // status pill: Installed (green) / ↓X% (downloading)
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 6
                radius: 6; color: "#cc000000"
                implicitWidth: dlRow.width + 12; implicitHeight: 18
                Row {
                    id: dlRow; anchors.centerIn: parent; spacing: 5
                    Rectangle { visible: card.item.completed; width: 6; height: 6; radius: 3; color: Theme.grn; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: card.item.completed ? i18n.t("hub_installed") : ("↓ " + Math.round((card.item.progress || 0) * 100) + "%")
                        color: card.item.completed ? Theme.grn : Theme.accent
                        font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
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
            text: (i18n.language, page.cardStatus(card.item, card.isGame))
            color: card.item.playing === true ? Theme.accent : Theme.t4
            font.pixelSize: 10; font.family: Theme.fontSans
            elide: Text.ElideRight
        }

        MouseArea {
            id: ma; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            // single click → detail drawer; double click → quick-play
            onClicked: function (mouse) {
                if (mouse.button === Qt.RightButton) { card.context(); return }
                card.showDetail()
            }
            onDoubleClicked: card.play()
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
            text: (i18n.language, i18n.t("hub_gs_install"))
            onTriggered: if (page.api) page.api.installGame(gameMenu.hash)
        }
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

    // ===== detail drawer (click a library card) =====
    Rectangle {
        anchors.fill: parent
        visible: page.detailOpen || detailDrawer.x < page.width
        color: "#88000000"
        opacity: page.detailOpen ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }
        MouseArea { anchors.fill: parent; onClicked: page.detailOpen = false }
    }
    Rectangle {
        id: detailDrawer
        width: Math.min(420, page.width)
        height: parent.height
        x: page.detailOpen ? parent.width - width : parent.width
        color: Theme.elev
        Behavior on x { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.hair }
        MouseArea { anchors.fill: parent }   // swallow clicks so the scrim doesn't close it

        readonly property var it: page.detailItem
        readonly property bool isGame: page.detailIsGame

        Flickable {
            anchors.fill: parent
            anchors.margins: 24
            contentHeight: dcol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            ColumnLayout {
                id: dcol
                width: parent.width
                spacing: 16
                visible: detailDrawer.it !== null

                // close
                Item {
                    Layout.fillWidth: true; Layout.preferredHeight: 28
                    Rectangle {
                        anchors.right: parent.right; width: 28; height: 28; radius: 14
                        color: dClose.containsMouse ? Theme.hover : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; color: Theme.t2; font.pixelSize: 14 }
                        MouseArea { id: dClose; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: page.detailOpen = false }
                    }
                }

                // cover (rounded mask, no shadow)
                Item {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 168; Layout.preferredHeight: 224
                    Rectangle { id: dPC; anchors.fill: parent; color: "#161618"; visible: false; layer.enabled: true
                        Image { anchors.fill: parent; source: detailDrawer.it ? (detailDrawer.it.poster || "") : ""; fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true } }
                    Rectangle { id: dPM; anchors.fill: parent; radius: 14; color: "white"; visible: false; layer.enabled: true }
                    MultiEffect { source: dPC; anchors.fill: parent; maskEnabled: true; maskSource: dPM }
                    Rectangle { anchors.fill: parent; radius: 14; color: "transparent"; border.color: "#33ffffff"; border.width: 1 }
                }

                Text {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignHCenter
                    text: detailDrawer.it ? (detailDrawer.it.title || "") : ""
                    color: Theme.t1; font.pixelSize: 20; font.weight: Font.Bold; font.family: Theme.fontSans
                    horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignHCenter
                    visible: text.length > 0
                    color: Theme.t3; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight
                    text: {
                        var it = detailDrawer.it; if (!it) return ""
                        var parts = []
                        if ((it.year || "").length > 0) parts.push(it.year)
                        if (it.genres && it.genres.length > 0) parts.push(it.genres.slice(0, 2).join(" · "))
                        if ((it.rating || 0) > 0) parts.push("★ " + Number(it.rating).toFixed(1))
                        return parts.join("   ·   ")
                    }
                }
                Text {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignHCenter
                    visible: text.length > 0
                    color: detailDrawer.it && detailDrawer.it.playing ? Theme.accent : Theme.t4
                    font.pixelSize: 12; font.family: Theme.fontMono; horizontalAlignment: Text.AlignHCenter
                    text: {
                        var it = detailDrawer.it; if (!it) return ""
                        if (detailDrawer.isGame) return page.cardStatus(it, true)
                        var p = []
                        if ((it.watchedPct || 0) > 0) p.push(Math.round(it.watchedPct * 100) + "% " + i18n.t("hub_watched"))
                        if ((it.durMs || 0) > 0) p.push(page.fmtTime(it.durMs))
                        if ((it.size || 0) > 0) p.push(page.fmtSize(it.size))
                        return p.join("   ·   ")
                    }
                }

                // actions
                BtnFlat {
                    Layout.fillWidth: true; Layout.topMargin: 4
                    primary: true; icon: "qrc:/icons/play.svg"
                    text: detailDrawer.isGame ? (page.gameStateActionable(detailDrawer.it) ? page.gameStateLabel(detailDrawer.it) : i18n.t("hub_gs_play"))
                          : ((detailDrawer.it && (detailDrawer.it.watchedPct || 0) > 0) ? i18n.t("hub_resume") : i18n.t("hub_gs_play"))
                    onClicked: {
                        if (!detailDrawer.it) return
                        if (detailDrawer.isGame) page.gamePrimary(detailDrawer.it)
                        else if (api) api.playByHashFile(detailDrawer.it.infoHash, detailDrawer.it.fileIndex)
                        page.detailOpen = false
                    }
                }
                BtnFlat {
                    Layout.fillWidth: true
                    icon: "qrc:/icons/open.svg"
                    text: (i18n.language, i18n.t("hub_open_folder"))
                    onClicked: if (api && detailDrawer.it) {
                        var f = api.gameFolder(detailDrawer.it.infoHash)
                        if (f && f.length > 0) Qt.openUrlExternally(page.fileUrl(f))
                    }
                }
                BtnFlat {
                    Layout.fillWidth: true
                    visible: detailDrawer.isGame
                    icon: "qrc:/icons/settings.svg"
                    text: (i18n.language, i18n.t("hub_set_exe"))
                    onClicked: if (detailDrawer.it) { page.openExePicker(detailDrawer.it.infoHash, false); page.detailOpen = false }
                }

                Text {
                    Layout.fillWidth: true; Layout.topMargin: 4
                    visible: detailDrawer.it && (detailDrawer.it.description || detailDrawer.it.overview || "").length > 0
                    text: detailDrawer.it ? (detailDrawer.it.description || detailDrawer.it.overview || "") : ""
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                    wrapMode: Text.WordWrap; lineHeight: 1.35
                }
            }
        }
    }
}
