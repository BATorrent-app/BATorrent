// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Search content page (4.0 step ②). Rich search: cover thumbnails, client-side
// filters (quality/source/repacker/seeders/sort) and a slide-in detail drawer.
// Wired to QmlSearchBridge (`search`).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "theme"
import "widgets"

Rectangle {
    id: page
    color: Theme.bg

    readonly property var api: typeof search !== "undefined" ? search : null
    readonly property string sourceKey: {
        if (!api) return "stremio"
        var s = api.sources
        return (srcSel.currentIndex >= 0 && srcSel.currentIndex < s.length) ? s[srcSel.currentIndex].key : "stremio"
    }
    readonly property bool isLegacy: sourceKey === "legacy"
    readonly property bool isGames: sourceKey === "games" || sourceKey === "all"
    readonly property bool isCatalog: api && api.mode === "catalog"
    readonly property bool isTitles: api && api.mode === "titles"
    // a single picked title's torrents (or Stremio streams) don't need per-row covers
    readonly property bool showRowThumbs: !(api && api.singleTitleView)
    readonly property bool isFlatList: api && (api.mode === "torrent" || api.mode === "games"
                                              || api.mode === "all" || api.mode === "streams")
    property bool showGameMgr: false
    property var gameList: []

    // ---- client-side filter/sort state ----
    property string qualityFilter: ""
    property string sourceFilter: ""
    property string repackerFilter: ""
    property string providerFilter: ""
    property string langFilter: ""
    property int minSeeds: 0
    property string sortKey: ""        // "" = provider order (relevance)

    // ---- detail drawer state ----
    property var selected: null
    property int selectedIdx: -1
    property bool detailOpen: false
    property string detailPoster: ""

    function reloadGames() { gameList = (api ? api.gameSources() : []) }

    function typeLabel(t) {
        if (t === "movie") return i18n.t("search_type_movie")
        if (t === "series") return i18n.t("search_type_series")
        if (t === "game") return i18n.t("search_type_game")
        return ""
    }

    // Local paths (resolver covers) need a file: URL; http/qrc pass through.
    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        if (p.indexOf("http") === 0 || p.indexOf("qrc") === 0) return p
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }

    function distinctTokens(role, order) {
        if (!api) return []
        var seen = {}, out = []
        var res = api.results
        for (var i = 0; i < res.length; i++) {
            var v = res[i][role]
            if (v && !seen[v]) { seen[v] = true; out.push(v) }
        }
        if (order && order.length > 0) {
            out.sort(function (a, b) {
                var ia = order.indexOf(a), ib = order.indexOf(b)
                if (ia < 0) ia = 99; if (ib < 0) ib = 99
                return ia - ib
            })
        } else {
            out.sort()
        }
        return out
    }

    readonly property var qualityOptions: distinctTokens("quality", ["4K", "1080p", "720p", "480p"])
    readonly property var sourceOptions: distinctTokens("source", ["Remux", "BluRay", "WEB", "HDTV", "DVD", "CAM"])
    readonly property var repackerOptions: distinctTokens("repacker", [])
    readonly property var providerOptions: distinctTokens("provider", [])

    // languages present across results (each result's `langs` is a list)
    readonly property var langOptions: {
        var seen = {}, out = []
        var rs = api ? api.results : []
        for (var i = 0; i < rs.length; i++) {
            var ls = rs[i].langs || []
            for (var j = 0; j < ls.length; j++)
                if (ls[j] && !seen[ls[j]]) { seen[ls[j]] = true; out.push(ls[j]) }
        }
        out.sort()
        return out
    }
    readonly property var langNames: ({
        "PT": "Português", "EN": "English", "ES": "Español", "DE": "Deutsch",
        "IT": "Italiano", "FR": "Français", "RU": "Русский", "JA": "日本語",
        "UK": "Українська", "ZH": "中文", "KO": "한국어", "HI": "हिन्दी", "MULTI": "Multi"
    })
    function langName(c) { return langNames[c] || c }

    function computeView() {
        if (!api) return []
        var arr = []
        var res = api.results
        // Relevance scoring lives in the tested C++ SearchRanker (via the bridge),
        // not here — the view only reads the scores to sort by.
        var qwords = api.queryWords()
        for (var i = 0; i < res.length; i++) {
            var o = res[i]; o._idx = i
            o._rel = api.relevance(o.name, qwords)
            arr.push(o)
        }
        if (qualityFilter !== "") arr = arr.filter(function (r) { return r.quality === qualityFilter })
        if (sourceFilter !== "") arr = arr.filter(function (r) { return r.source === sourceFilter })
        if (repackerFilter !== "") arr = arr.filter(function (r) { return r.repacker === repackerFilter })
        if (providerFilter !== "") arr = arr.filter(function (r) { return r.provider === providerFilter })
        if (langFilter !== "") arr = arr.filter(function (r) { return (r.langs || []).indexOf(langFilter) !== -1 })
        if (minSeeds > 0) arr = arr.filter(function (r) { return (r.seedsN || 0) >= minSeeds })
        if (sortKey === "seeders") arr.sort(function (a, b) { return (b.seedsN || 0) - (a.seedsN || 0) })
        else if (sortKey === "size") arr.sort(function (a, b) { return (b.sizeBytes || 0) - (a.sizeBytes || 0) })
        else if (sortKey === "name") arr.sort(function (a, b) { return (a.name || "").localeCompare(b.name || "") })
        // default = Relevance. When viewing one picked title's releases (the watch
        // context) and "prefer my language" is on, the user's own language leads —
        // grouped above quality/seeders, Torrentio-style — so a viewer who needs a
        // dub/sub in their language finds it first.
        else {
            var nativeFirst = page.api && page.api.singleTitleView
                              && (typeof settings === "undefined" || settings.get("preferNativeLang") !== false)
            arr.sort(function (a, b) {
                if (nativeFirst) {
                    var n0 = (b.native ? 1 : 0) - (a.native ? 1 : 0); if (n0) return n0
                }
                var rd = (b._rel || 0) - (a._rel || 0); if (rd) return rd
                var nd = (b.native ? 1 : 0) - (a.native ? 1 : 0); if (nd) return nd
                return a._idx - b._idx
            })
        }
        return arr
    }

    // re-evaluates automatically: computeView() reads api.results and every
    // filter/sort property, so QML captures them all as binding dependencies.
    property var viewModel: computeView()

    function openDetail(item) {
        if (!item) return
        selected = item
        selectedIdx = item._idx
        detailPoster = fileUrl(item.poster || "")
        detailOpen = true
        if ((!item.poster || item.poster === "") && item.coverHash && api)
            api.resolveCover(item._idx)
    }

    function clearFilters() {
        qualityFilter = ""; sourceFilter = ""; repackerFilter = ""; providerFilter = ""; langFilter = ""; minSeeds = 0; sortKey = ""
        qualSel.currentIndex = 0; srcFiltSel.currentIndex = 0; provSel.currentIndex = 0
        repSel.currentIndex = 0; langSel.currentIndex = 0; seedSel.currentIndex = 0; sortSel.currentIndex = 0
    }

    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onGameSourcesChanged() { page.reloadGames() }
        function onResultsChanged() { page.detailOpen = false }
        function onCoverReady(infoHash, path) {
            if (page.selected && infoHash !== "" && infoHash === (page.selected.coverHash || ""))
                page.detailPoster = page.fileUrl(path)
        }
    }

    onShowGameMgrChanged: if (showGameMgr) reloadGames()
    onVisibleChanged: if (visible) {
        if (api) api.refreshSources()
        Qt.callLater(function() { queryFld.field.forceActiveFocus() })   // land ready to type
    }

    // ---- recent searches (persisted) ----
    property var recentList: []
    Component.onCompleted: {
        if (typeof settings !== "undefined") {
            try { recentList = JSON.parse(settings.get("searchRecent") || "[]") } catch (e) { recentList = [] }
        }
    }
    function pushRecent(q) {
        q = (q || "").trim()
        if (q.length === 0) return
        var list = recentList.filter(function (x) { return x.toLowerCase() !== q.toLowerCase() })
        list.unshift(q)
        recentList = list.slice(0, 8)
        if (typeof settings !== "undefined") settings.set("searchRecent", JSON.stringify(recentList))
    }

    function runSearch() {
        if (!api) return
        clearFilters()
        var cat = (isLegacy && catSel.currentIndex >= 0) ? api.categories[catSel.currentIndex].code : 0
        api.search(page.sourceKey, queryFld.text, cat)
    }
    // Committed search (Enter or a Discover/recent pick) — also records it as recent.
    function commitSearch() { runSearch(); pushRecent(queryFld.text) }

    // Debounced live search while typing (title-first hits TMDB+IGDB, so debounce).
    Timer {
        id: searchDebounce
        interval: 450; repeat: false
        onTriggered: page.runSearch()
    }

    // Auto-pick the "best" release of a picked title and add it. Delegates to the
    // bridge's ReleasePick-backed ranking (same logic as one-click Get & Watch, so
    // it honors the user's quality/size/language preferences) instead of a second
    // scoring formula here.
    function pickBest() {
        if (!api) return
        var i = api.bestResultIndex()
        if (i >= 0) api.activateResult(i)
    }

    // External entry (e.g. clicking a Discover poster): reflect the query in the
    // bar instead of running a "ghost" search, then run it title-first ("Tudo").
    function runQuery(text) {
        queryFld.text = text
        srcSel.currentIndex = 0
        commitSearch()
    }

    // ---- seed-health bar + disk-fit awareness ----
    function seedColor(n) {
        return n >= 50 ? Theme.grn : (n >= 10 ? "#e0a533" : (n >= 1 ? "#d97640" : Theme.t4))
    }
    function seedFill(n) {   // log scale: a handful reads as a sliver, 500+ fills it
        return n <= 0 ? 0 : Math.min(1, Math.log(n + 1) / Math.log(500))
    }
    function fmtSize(b) {
        if (!b || b <= 0) return "0 B"
        var u = ["B", "KB", "MB", "GB", "TB"]
        var i = Math.min(u.length - 1, Math.floor(Math.log(b) / Math.log(1024)))
        return (b / Math.pow(1024, i)).toFixed(i >= 3 ? 1 : 0) + " " + u[i]
    }
    // free bytes on the default save volume (numeric — the string `free` was a fmt'd label)
    readonly property double saveFree: (typeof session !== "undefined" && session.diskVolumes
        && session.diskVolumes.length > 0) ? (session.diskVolumes[0].freeBytes || 0) : -1
    readonly property int wontFit: {
        if (saveFree < 0 || !api) return 0
        var n = 0, res = api.results
        for (var i = 0; i < res.length; ++i)
            if ((res[i].sizeBytes || 0) > saveFree) ++n
        return n
    }
    function fmtCount(n) { return n >= 1000 ? (n / 1000).toFixed(1).replace(/\.0$/, "") + "k" : String(n) }

    // ---- best-match hero on the title-disambiguation grid ----
    readonly property bool showBestMatch: isTitles && api && api.results.length > 0 && !api.searching
    readonly property var bestItem: showBestMatch ? api.results[0] : null
    property var bestSummary: null   // {count, bestSize, maxSeeds} for bestItem
    onBestItemChanged: {
        bestSummary = null
        if (bestItem && typeof search !== "undefined" && (bestItem.name || "").length > 0)
            search.summarizeSources(bestItem.name)
    }
    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onSourceSummary(title, count, bestSize, maxSeeds) {
            if (page.bestItem && page.bestItem.name === title)
                page.bestSummary = { count: count, bestSize: bestSize, maxSeeds: maxSeeds }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("search_heading")); color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // search bar
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 36 + 2 * Theme.sp4
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                anchors.topMargin: Theme.sp4
                anchors.bottomMargin: Theme.sp4
                spacing: Theme.sp3

                TFld {
                    id: queryFld
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    icon: "qrc:/icons/search.svg"
                    clearable: true
                    placeholder: (i18n.language, i18n.t("search_input"))
                    onTextChanged: {
                        if (text.trim().length >= 2) searchDebounce.restart()
                        else searchDebounce.stop()
                    }
                    // commit on Enter only — committing on focus-out re-ran the search
                    // when you clicked a filter dropdown, eating the first click
                    Connections {
                        target: queryFld.field
                        function onAccepted() { page.commitSearch() }
                    }
                }
                TSelect {
                    id: srcSel
                    Layout.preferredHeight: 36
                    Layout.preferredWidth: 180
                    model: page.api ? page.api.sources : []
                    textRole: "label"
                }
                TSelect {
                    id: catSel
                    visible: page.isLegacy
                    Layout.preferredHeight: 36
                    Layout.preferredWidth: 130
                    model: page.api ? page.api.categories : []
                    textRole: "label"
                }
                // catalog manager belongs next to the source it configures, not
                // floating in the footer
                BtnFlat {
                    visible: page.isGames
                    text: (i18n.language, i18n.t("game_sources_btn"))
                    onClicked: page.showGameMgr = true
                }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("empty_search_btn")); onClicked: page.commitSearch() }
            }
        }

        // recent searches — chips shown while the box is empty
        Flow {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; Layout.topMargin: 2
            spacing: 8
            visible: queryFld.text.length === 0 && page.recentList.length > 0
            Text {
                text: (i18n.language, i18n.t("search_recent")) + ":"
                color: Theme.t4; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
                height: 24; verticalAlignment: Text.AlignVCenter
            }
            Repeater {
                model: page.recentList
                delegate: Rectangle {
                    required property var modelData
                    radius: 12
                    color: rcMa.containsMouse ? Theme.hover : Theme.field
                    border.color: Theme.hair; border.width: 1
                    implicitWidth: rcTxt.implicitWidth + 22
                    implicitHeight: 24
                    Text {
                        id: rcTxt; anchors.centerIn: parent; text: modelData
                        color: Theme.t2; font.pixelSize: 11; font.family: Theme.fontSans
                    }
                    MouseArea {
                        id: rcMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: page.runQuery(modelData)
                    }
                }
            }
        }

        // filter bar — only for flat result lists with content
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            visible: page.isFlatList && page.api && page.api.results.length > 0
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp3

                // provider / origin
                TSelect {
                    id: provSel
                    visible: page.providerOptions.length > 1
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 150
                    model: [i18n.t("search_provider_all")].concat(page.providerOptions)
                    onActivated: page.providerFilter = currentIndex <= 0 ? "" : currentText
                }
                // quality (video modes only)
                TSelect {
                    id: qualSel
                    visible: !page.isGames && page.qualityOptions.length > 0
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 110
                    model: [i18n.t("search_filter_all")].concat(page.qualityOptions)
                    onActivated: page.qualityFilter = currentIndex <= 0 ? "" : currentText
                }
                // source (video modes only)
                TSelect {
                    id: srcFiltSel
                    visible: !page.isGames && page.sourceOptions.length > 0
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 120
                    model: [i18n.t("search_filter_all")].concat(page.sourceOptions)
                    onActivated: page.sourceFilter = currentIndex <= 0 ? "" : currentText
                }
                // repacker (any mode that has repacked releases)
                TSelect {
                    id: repSel
                    visible: page.repackerOptions.length > 0
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 140
                    model: [i18n.t("search_repacker_all")].concat(page.repackerOptions)
                    onActivated: page.repackerFilter = currentIndex <= 0 ? "" : currentText
                }
                // audio language (video modes that have tagged releases)
                TSelect {
                    id: langSel
                    visible: !page.isGames && page.langOptions.length > 0
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 130
                    model: [i18n.t("search_lang_all")].concat(page.langOptions.map(page.langName))
                    onActivated: page.langFilter = currentIndex <= 0 ? "" : page.langOptions[currentIndex - 1]
                }
                // min seeders (video modes)
                TSelect {
                    id: seedSel
                    visible: !page.isGames
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 120
                    property var vals: [0, 1, 10, 50, 100]
                    model: [i18n.t("search_seeds_any"), "1+", "10+", "50+", "100+"]
                    onActivated: page.minSeeds = vals[currentIndex]
                }
                // sort
                TSelect {
                    id: sortSel
                    Layout.preferredHeight: 30
                    Layout.preferredWidth: 150
                    property var keys: ["", "seeders", "size", "name"]
                    model: [i18n.t("search_sort_relevance"), i18n.t("search_sort_seeders"),
                            i18n.t("search_sort_size"), i18n.t("search_sort_name")]
                    onActivated: page.sortKey = keys[currentIndex]
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: page.viewModel.length + " / " + (page.api ? page.api.results.length : 0)
                    color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono
                }
                BtnFlat {
                    // one-click "best" release for a picked title (quality + seeders + your language)
                    // secondary styling so it doesn't fight the red "Search" stacked above it
                    visible: page.api && page.api.singleTitleView && page.api.results.length > 0
                    primary: false
                    text: "★  " + (i18n.language, i18n.t("search_get_best"))
                    onClicked: page.pickBest()
                }
                BtnFlat {
                    visible: page.qualityFilter !== "" || page.sourceFilter !== "" || page.repackerFilter !== ""
                             || page.providerFilter !== "" || page.minSeeds > 0 || page.sortKey !== ""
                    text: (i18n.language, i18n.t("search_filter_clear"))
                    onClicked: page.clearFilters()
                }
            }
        }

        // results header (columns adapt to mode) — flat list only
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            visible: !page.isTitles
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp4
                property bool torrentish: page.api && (page.api.mode === "torrent" || page.api.mode === "games" || page.api.mode === "all")
                Item { Layout.preferredWidth: 46; visible: page.showRowThumbs }   // thumb column
                Text { text: (i18n.language, i18n.t("search_col_name2")); Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { text: (i18n.language, i18n.t("search_col_size2")); Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { visible: parent.torrentish; text: (i18n.language, i18n.t("search_col_seeds2")); Layout.preferredWidth: 100; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { visible: parent.torrentish; text: (i18n.language, i18n.t("search_col_leech")); Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Item { Layout.preferredWidth: 36 }
            }
        }

        // results
        ListView {
            id: resultsView
            visible: !page.isTitles
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: page.isTitles ? [] : page.viewModel
            boundsBehavior: Flickable.StopAtBounds
            cacheBuffer: 240

            // empty / loading state
            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 80, 360)
                spacing: 10
                visible: resultsView.count === 0
                Spinner {
                    Layout.alignment: Qt.AlignHCenter
                    visible: page.api && page.api.searching
                    s: 30
                }
                IconImg {
                    Layout.alignment: Qt.AlignHCenter
                    visible: !(page.api && page.api.searching)
                    src: "qrc:/icons/search.svg"
                    tint: Theme.t4
                    s: 38
                    opacity: 0.7
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: page.api && page.api.searching ? (i18n.language, i18n.t("search_searching2"))
                         : (page.api && page.api.statusText.length > 0 && page.api.results.length === 0 ? page.api.statusText
                            : (i18n.language, i18n.t("search_prompt")))
                    color: Theme.t2
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.fillWidth: true
                    visible: !(page.api && page.api.searching)
                    text: page.api && page.api.statusText.length > 0 && page.api.results.length === 0
                          ? (i18n.language, i18n.t("search_empty_hint"))
                          : (i18n.language, i18n.t("search_prompt_sub"))
                    color: Theme.t4
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                }
            }

            delegate: SearchResultRow {
                sv: page
                onMenuRequested: function (i) { rowMenu.openFor(i) }
            }
        }

        // best-match hero — explicitly surfaces the top title (not just first in grid)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 142
            Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; Layout.topMargin: Theme.sp4
            visible: page.showBestMatch
            radius: 14
            color: Theme.elev
            border.color: Theme.hair; border.width: 1
            RowLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 16
                PosterThumb {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 82; implicitHeight: 110
                    posterUrl: page.bestItem ? (page.bestItem.poster || "") : ""
                    label: page.bestItem ? (page.bestItem.name || "") : ""
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 5
                    Text {
                        text: (i18n.language, i18n.t("search_best_match"))
                        color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
                        font.letterSpacing: 1.2; font.family: Theme.fontSans
                    }
                    Text {
                        text: page.bestItem ? (page.bestItem.name || "") : ""
                        color: Theme.t1; font.pixelSize: 22; font.weight: Font.Bold; font.family: Theme.fontSans
                        Layout.fillWidth: true; elide: Text.ElideRight
                    }
                    Text {
                        text: {
                            if (!page.bestItem) return ""
                            var parts = []
                            var t = page.typeLabel(page.bestItem.type || "")
                            if ((page.bestItem.year || "").length > 0) parts.push(page.bestItem.year)
                            if (t.length > 0) parts.push(t)
                            if ((page.bestItem.rating || 0) > 0) parts.push("★ " + page.bestItem.rating.toFixed(1))
                            return parts.join("    ·    ")
                        }
                        color: Theme.t3; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                    Row {
                        spacing: 7
                        visible: page.bestSummary && page.bestSummary.count > 0
                        Rectangle { width: 7; height: 7; radius: 4; color: Theme.grn; anchors.verticalCenter: parent.verticalCenter }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: {
                                if (!page.bestSummary) return ""
                                var s = page.bestSummary.count + " torrents"
                                if (page.bestSummary.maxSeeds > 0) s += "    ·    best " + page.fmtCount(page.bestSummary.maxSeeds) + " seeds"
                                return s
                            }
                            color: Theme.grn; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontMono
                        }
                    }
                }
                BtnFlat {
                    Layout.alignment: Qt.AlignVCenter
                    primary: true
                    text: (i18n.language, i18n.t("search_see_torrents"))
                    onClicked: if (page.api) page.api.activateResult(0)
                }
            }
        }
        Text {
            visible: page.showBestMatch && page.api && page.api.results.length > 1
            text: (i18n.language, i18n.t("search_other_matches"))
            Layout.leftMargin: Theme.sp5; Layout.topMargin: 8
            color: Theme.t4; font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 0.8; font.family: Theme.fontSans
        }

        // titles grid (step 1: pick the actual movie/series/game, one cover each)
        GridView {
            id: titlesView
            visible: page.isTitles
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: page.isTitles ? (page.showBestMatch ? page.api.results.slice(1) : (page.api ? page.api.results : [])) : []
            cellWidth: 174
            cellHeight: 286
            leftMargin: Theme.sp5; rightMargin: Theme.sp5; topMargin: Theme.sp4; bottomMargin: Theme.sp4
            boundsBehavior: Flickable.StopAtBounds
            cacheBuffer: 600

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(parent.width - 80, 360)
                spacing: 10
                visible: titlesView.count === 0 && !page.showBestMatch
                Spinner {
                    Layout.alignment: Qt.AlignHCenter
                    visible: page.api && page.api.searching
                    s: 30
                }
                IconImg {
                    Layout.alignment: Qt.AlignHCenter
                    visible: !(page.api && page.api.searching)
                    src: "qrc:/icons/search.svg"; tint: Theme.t4; s: 38
                    opacity: 0.7
                }
                Text {
                    Layout.fillWidth: true
                    text: page.api && page.api.searching ? (i18n.language, i18n.t("search_searching2"))
                         : (page.api && page.api.statusText.length > 0 ? page.api.statusText
                            : (i18n.language, i18n.t("search_prompt")))
                    color: Theme.t2; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    visible: !(page.api && page.api.searching)
                    text: page.api && page.api.statusText.length > 0 && page.api.results.length === 0
                          ? (i18n.language, i18n.t("search_empty_hint"))
                          : (i18n.language, i18n.t("search_prompt_sub"))
                    color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans
                    horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap
                }
            }

            delegate: Item {
                required property var modelData
                required property int index
                width: titlesView.cellWidth
                height: titlesView.cellHeight
                PosterCard {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    posterW: 150
                    title: modelData.name || ""
                    poster: modelData.poster || ""
                    year: {
                        var y = modelData.year || ""
                        var t = page.typeLabel(modelData.type || "")
                        return t.length > 0 ? (y.length > 0 ? y + "  ·  " + t : t) : y
                    }
                    rating: modelData.rating || 0
                    type: modelData.type || ""
                    synopsis: modelData.overview || ""
                    watchlistEnabled: typeof session !== "undefined"
                    saved: typeof session !== "undefined"
                           && (session.watchlist, session.inWatchlist(modelData.name || "", modelData.type || ""))
                    onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                        title: modelData.name || "", type: modelData.type || "",
                        poster: modelData.poster || "", year: modelData.year || "" })
                    onActivated: if (page.api) page.api.activateResult(page.showBestMatch ? index + 1 : index)
                    onGetWatch: if (page.api) page.api.getAndWatch(modelData.name || "",
                                                                   modelData.year || "",
                                                                   modelData.type || "movie")
                }
            }
        }

        // footer
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                spacing: Theme.sp2
                BtnFlat { visible: page.api && page.api.canGoBack; text: (i18n.language, i18n.t("search_back2")); onClicked: if (page.api) page.api.back() }
                Text { text: page.api ? page.api.statusText : ""; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                Row {
                    spacing: 7
                    visible: page.isFlatList && page.wontFit > 0 && page.saveFree >= 0
                    Rectangle { width: 7; height: 7; radius: 4; color: "#e0a533"; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (i18n.language, i18n.t("search_disk_warn")).arg(page.fmtSize(page.saveFree)).arg(page.wontFit)
                        color: "#e0a533"; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                }
                BtnFlat {
                    visible: page.isTitles && page.api && !page.api.searching
                    text: (i18n.language, i18n.t("search_raw_results"))
                    onClicked: if (page.api) page.api.searchRaw()
                }
            }
        }
    }

    // ===== detail drawer =====
    SearchDetailDrawer { anchors.fill: parent; sv: page }

    // per-result right-click menu
    BatMenu {
        id: rowMenu
        property int idx: -1
        function openFor(i) { idx = i; popup() }
        implicitWidth: 190
        BatMenuItem {
            text: page.isCatalog ? (i18n.language, i18n.t("search_view_streams")) : (i18n.language, i18n.t("search_add"))
            onTriggered: if (page.api && rowMenu.idx >= 0) page.api.activateResult(rowMenu.idx)
        }
        BatMenuItem {
            text: (i18n.language, i18n.t("search_copy_magnet"))
            onTriggered: if (page.api && rowMenu.idx >= 0) page.api.copyMagnet(rowMenu.idx)
        }
        BatMenuItem {
            visible: !page.isCatalog && typeof debrid !== "undefined" && debrid.hasToken
            height: visible ? implicitHeight : 0
            text: (i18n.language, i18n.t("debrid_stream")).arg(debrid.providerName)
            onTriggered: {
                if (!page.api || rowMenu.idx < 0) return
                var m = page.api.magnetAt(rowMenu.idx)
                if (m) debrid.streamMagnet(m)
            }
        }
    }

    SearchGameMgr { sv: page }

    // disk add-guard: the bridge blocks a too-big add and asks here first
    property int pendingFitIdx: -1
    property string pendingFitMsg: ""
    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onAddWontFit(index, name, needed, freeBytes) {
            page.pendingFitIdx = index
            page.pendingFitMsg = i18n.t("search_wontfit_body")
                .replace("%1", name)
                .replace("%2", page.fmtSize(needed))
                .replace("%3", page.fmtSize(freeBytes))
            fitDlg.open()
        }
    }
    BatDialog {
        id: fitDlg
        title: (i18n.language, i18n.t("search_wontfit_title"))
        cardW: 470; cardH: 240
        okText: (i18n.language, i18n.t("search_wontfit_ok"))
        cancelText: (i18n.language, i18n.t("btn_cancel"))
        onAccepted: if (page.api && page.pendingFitIdx >= 0) page.api.activateResult(page.pendingFitIdx, true)
        Text {
            Layout.fillWidth: true
            text: page.pendingFitMsg
            wrapMode: Text.WordWrap
            color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans; lineHeight: 1.35
        }
    }
}
