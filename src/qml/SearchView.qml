// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Encontrar (Find) page — catalog browse (billboard + shelves via FindBrowse)
// that becomes rich search the moment you type. Owns ALL search state
// (filters/sort, series grouping, detail drawer, recents, disk-fit) and
// composes the extracted chrome: FindBar, SearchFiltersRow, SearchModeBars,
// SearchListPane, SearchTitlesPane. Wired to QmlSearchBridge (`search`).
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
        return (findBar.srcIndex >= 0 && findBar.srcIndex < s.length) ? s[findBar.srcIndex].key : "stremio"
    }
    readonly property bool isLegacy: sourceKey === "legacy"
    readonly property bool isGames: sourceKey === "games" || sourceKey === "all"
    readonly property bool isCatalog: api && api.mode === "catalog"
    readonly property bool isTitles: api && api.mode === "titles"
    readonly property bool isEpisodes: api && api.mode === "episodes"
    // a single picked title's torrents (or Stremio streams) don't need per-row covers
    readonly property bool showRowThumbs: !(api && api.singleTitleView)
    readonly property bool isFlatList: api && (api.mode === "torrent" || api.mode === "games"
                                              || api.mode === "all" || api.mode === "streams")
    property bool showGameMgr: false
    property bool showSourcesMgr: false
    property var gameList: []

    // ---- client-side filter/sort state ----
    property string qualityFilter: ""
    property string sourceFilter: ""
    property string repackerFilter: ""
    property string providerFilter: ""
    property string langFilter: ""
    property string audioModeFilter: ""   // "" = all | "dub" | "sub" | "original"
    property int minSeeds: 0
    property string sortKey: ""        // "" = provider order (relevance)

    // ---- series grouping (a picked series' releases, or the episode picker) ----
    property int seasonFilter: -2      // -2 = all, -1 = packs only, >0 = that season
    property int episodeFilter: -1     // -1 = all episodes of the season
    // releases of one picked series → group the flat list by parsed season/episode
    readonly property bool isSeriesDrill: api && api.singleTitleView && !isEpisodes
                                          && api.workType === "series"
    readonly property var seasonTabs: {
        if (!api || !(isSeriesDrill || isEpisodes)) return []
        var seen = {}, out = []
        var res = api.results
        for (var i = 0; i < res.length; i++) {
            var s = res[i].season
            if (s > 0 && !seen[s]) { seen[s] = true; out.push(s) }
        }
        out.sort(function (a, b) { return a - b })
        return out
    }
    readonly property int packCount: {
        if (!api || !isSeriesDrill) return 0
        var n = 0, res = api.results
        for (var i = 0; i < res.length; i++)
            if (res[i].pack === true) n++
        return n
    }
    readonly property var episodeTabs: {
        if (!api || !isSeriesDrill || seasonFilter <= 0) return []
        var seen = {}, out = []
        var res = api.results
        for (var i = 0; i < res.length; i++) {
            var r = res[i]
            if (r.season === seasonFilter && (r.episode || 0) > 0 && !seen[r.episode]) {
                seen[r.episode] = true; out.push(r.episode)
            }
        }
        out.sort(function (a, b) { return a - b })
        return out
    }

    // the dub/sub axis only exists for non-English UIs (English content has no
    // "dubbed in my language" question) — hide the segmented control otherwise
    readonly property bool showAudioModes: typeof i18n !== "undefined" && i18n.language !== 0

    // Browse ⇄ Results: empty query = browse (catalog billboard + shelves, or
    // the branded landing on store builds); anything typed = the search chrome.
    // The bar docks when searching OR once the browse content scrolls.
    readonly property bool catalogAvailable: !(typeof isStoreBuild !== "undefined" && isStoreBuild)
                                             && typeof discovery !== "undefined"
    property string typeFilter: "all"   // all | game | movie | series (browse pills)
    readonly property bool browse: findBar.text.trim().length === 0
    readonly property bool docked: !browse || (catalogAvailable && browsePane.scrollY > 36)

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
        // episode picker: air order, season tab only — release filters don't apply
        if (isEpisodes) {
            if (seasonFilter > 0) arr = arr.filter(function (r) { return r.season === seasonFilter })
            arr.sort(function (a, b) { return a._idx - b._idx })
            return arr
        }
        // series drill-down: a season tab keeps its episodes AND the packs that
        // contain them (season packs of that season + whole-series packs)
        if (isSeriesDrill) {
            if (seasonFilter === -1) arr = arr.filter(function (r) { return r.pack === true })
            else if (seasonFilter > 0) arr = arr.filter(function (r) {
                if (r.pack === true) return r.season === seasonFilter || (r.season || -1) < 0
                if (episodeFilter > 0) return r.season === seasonFilter && r.episode === episodeFilter
                return r.season === seasonFilter
            })
        }
        if (qualityFilter !== "") arr = arr.filter(function (r) { return r.quality === qualityFilter })
        if (sourceFilter !== "") arr = arr.filter(function (r) { return r.source === sourceFilter })
        if (repackerFilter !== "") arr = arr.filter(function (r) { return r.repacker === repackerFilter })
        if (providerFilter !== "") arr = arr.filter(function (r) { return r.provider === providerFilter })
        if (langFilter !== "") arr = arr.filter(function (r) { return (r.langs || []).indexOf(langFilter) !== -1 })
        if (audioModeFilter !== "") arr = arr.filter(function (r) { return (r.audioMode || "original") === audioModeFilter })
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
                // equal relevance (one title's releases all match) → healthiest first
                var sd = (b.seedsN || 0) - (a.seedsN || 0); if (sd) return sd
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
        if (api) api.fetchWorkStills()   // lazy backdrops for the drawer strip
    }

    function clearFilters() {
        qualityFilter = ""; sourceFilter = ""; repackerFilter = ""; providerFilter = ""; langFilter = ""; audioModeFilter = ""; minSeeds = 0; sortKey = ""
        seasonFilter = -2; episodeFilter = -1
        filtersRow.reset()
    }

    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onGameSourcesChanged() { page.reloadGames() }
        function onResultsChanged() {
            page.detailOpen = false
            // episode picker: land on the first season, not a 300-row flat list
            if (page.isEpisodes && page.seasonFilter === -2 && page.seasonTabs.length > 0)
                page.seasonFilter = page.seasonTabs[0]
        }
        function onWorkChanged() { page.seasonFilter = -2; page.episodeFilter = -1 }
        function onModeChanged() {
            if (page.api.mode === "catalog" || page.api.mode === "titles") {
                page.seasonFilter = -2; page.episodeFilter = -1
            }
        }
        function onCoverReady(infoHash, path) {
            if (page.selected && infoHash !== "" && infoHash === (page.selected.coverHash || ""))
                page.detailPoster = page.fileUrl(path)
        }
    }

    // provider toggles/adds change which sources the dropdown offers
    Connections {
        target: typeof addons !== "undefined" ? addons : null
        ignoreUnknownSignals: true
        function onChanged() { if (page.api) page.api.refreshSources() }
    }

    onShowGameMgrChanged: if (showGameMgr) reloadGames()
    onVisibleChanged: if (visible) {
        if (api) api.refreshSources()
        Qt.callLater(function() { findBar.focusInput() })   // land ready to type
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
        var cat = (isLegacy && findBar.catIndex >= 0) ? api.categories[findBar.catIndex].code : 0
        api.search(page.sourceKey, findBar.text, cat)
    }
    // Committed search (Enter or a Discover/recent pick) — also records it as recent.
    function commitSearch() { runSearch(); pushRecent(findBar.text) }

    // Debounced live search while typing (title-first hits TMDB+IGDB, so debounce).
    function queryEdited(t) {
        if (t.trim().length >= 2) searchDebounce.restart()
        else searchDebounce.stop()
    }
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
        findBar.setText(text)
        findBar.resetSource()
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

    ColumnLayout {
        id: mainCol
        anchors.fill: parent
        spacing: 0

        // top spacer — centres the branded landing on store builds (no catalog)
        Item { Layout.fillHeight: true; visible: page.browse && !page.catalogAvailable }

        // branding hero — store-build fallback for the browse state
        SearchLanding {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 20
            visible: page.browse && !page.catalogAvailable
        }

        FindBar { id: findBar; sv: page }

        // recent searches — store-fallback chips while the box is empty
        Flow {
            Layout.fillWidth: false
            Layout.alignment: Qt.AlignHCenter
            Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; Layout.topMargin: 2
            spacing: 8
            visible: page.browse && !page.catalogAvailable && page.recentList.length > 0
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

        // bottom spacer — balances the top spacer so the hero stays centred
        Item { Layout.fillHeight: true; visible: page.browse && !page.catalogAvailable }

        // catalog browse — billboard + shelves; hidden (state preserved) once
        // typing. Sized explicitly to the space under the bar: fillHeight kept a
        // stale height for this child (same layout misbehavior that collapsed
        // the shelves), and "everything under the bar" is the design anyway.
        FindBrowse {
            id: browsePane
            Layout.fillWidth: true
            Layout.preferredHeight: mainCol.height - findBar.height
            visible: page.browse && page.catalogAvailable
            typeFilter: page.typeFilter
            active: page.visible && page.browse && page.catalogAvailable
            onFindRequested: function(title) { page.runQuery(title) }
            onTypeFilterRequested: function(type) { page.typeFilter = type }
        }

        // picked-work header — says WHICH title (and type) these releases belong
        // to; without it a game and a series with the same name are indistinguishable
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 62
            visible: !page.browse && page.api && page.api.singleTitleView && !page.isEpisodes
                     && (page.api.workTitle || "").length > 0
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp4
                PosterThumb {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 34; implicitHeight: 46
                    posterUrl: page.fileUrl(page.api.workPoster || "")
                    label: page.api.workTitle
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    spacing: 2
                    Text {
                        Layout.fillWidth: true
                        text: page.api.workTitle
                        color: Theme.t1; font.pixelSize: 15; font.weight: Font.Bold; font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                    Text {
                        text: {
                            var parts = []
                            if ((page.api.workYear || "").length > 0) parts.push(page.api.workYear)
                            var t = page.typeLabel(page.api.workType || "")
                            if (t.length > 0) parts.push(t)
                            return parts.join("  ·  ")
                        }
                        color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                }
            }
        }

        SearchFiltersRow { id: filtersRow; sv: page }
        SearchModeBars { sv: page }
        SearchListPane { sv: page; Layout.fillWidth: true; Layout.fillHeight: true }
        SearchTitlesPane { sv: page; Layout.fillWidth: true; Layout.fillHeight: true }

        // footer — search chrome only
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            visible: !page.browse
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

    SearchGameMgr { sv: page }
    SourcesManager { sv: page }

    // disk add-guard: the bridge blocks a too-big add and asks here first
    property int pendingFitIdx: -1
    property string pendingFitMsg: ""
    property double pendingFitShortfall: 0   // needed - free, for the "free up space" handoff
    signal freeSpaceRequested(double targetBytes)
    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onAddWontFit(index, name, needed, freeBytes) {
            page.pendingFitIdx = index
            page.pendingFitMsg = i18n.t("search_wontfit_body")
                .replace("%1", name)
                .replace("%2", page.fmtSize(needed))
                .replace("%3", page.fmtSize(freeBytes))
            page.pendingFitShortfall = Math.max(0, needed - freeBytes)
            fitDlg.open()
        }
    }
    BatDialog {
        id: fitDlg
        title: (i18n.language, i18n.t("search_wontfit_title"))
        cardW: 470; cardH: 280
        okText: (i18n.language, i18n.t("search_wontfit_ok"))
        cancelText: (i18n.language, i18n.t("btn_cancel"))
        onAccepted: if (page.api && page.pendingFitIdx >= 0) page.api.activateResult(page.pendingFitIdx, true)
        Text {
            Layout.fillWidth: true
            text: page.pendingFitMsg
            wrapMode: Text.WordWrap
            color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans; lineHeight: 1.35
        }
        BtnFlat {
            Layout.alignment: Qt.AlignLeft
            Layout.topMargin: 4
            text: (i18n.language, i18n.t("search_wontfit_freeup"))
            onClicked: {
                fitDlg.close()
                page.freeSpaceRequested(page.pendingFitShortfall)
            }
        }
    }
}
