// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Encontrar (Find) page — catalog browse (billboard + shelves via FindBrowse)
// that becomes rich search the moment you type. Owns search chrome state
// (filters/sort, detail drawer, recents) and composes Search* leaves.
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
        return (findBar.srcIndex >= 0 && findBar.srcIndex < s.length) ? s[findBar.srcIndex].key : "stremio"
    }
    readonly property bool isLegacy: sourceKey === "legacy"
    readonly property bool isGames: sourceKey === "games" || sourceKey === "all"
    readonly property bool isCatalog: api && api.mode === "catalog"
    readonly property bool isTitles: api && api.mode === "titles"
    readonly property bool isEpisodes: api && api.mode === "episodes"
    readonly property bool showRowThumbs: !(api && api.singleTitleView)
    readonly property bool isFlatList: api && (api.mode === "torrent" || api.mode === "games"
                                              || api.mode === "all" || api.mode === "streams")
    property bool showGameMgr: false
    property bool showSourcesMgr: false
    property var gameList: []
    property int gameCatalogGen: 0

    // ---- client-side filter/sort state ----
    property string qualityFilter: ""
    property string sourceFilter: ""
    property string groupFilter: ""
    property string providerFilter: ""
    property string langFilter: ""
    property string audioModeFilter: ""
    property int minSeeds: 0
    property string sortKey: ""

    property int seasonFilter: -2
    property int episodeFilter: -1
    readonly property bool isSeriesDrill: api && api.singleTitleView && !isEpisodes
                                          && api.workType === "series"
    readonly property bool showAudioModes: typeof i18n !== "undefined" && i18n.language !== 0

    readonly property bool catalogAvailable: !(typeof isStoreBuild !== "undefined" && isStoreBuild)
                                             && typeof discovery !== "undefined"
    property string typeFilter: "all"
    readonly property bool browse: findBar.text.trim().length === 0
    property bool showCatalogBrowse: false
    readonly property real browseScrollY: showCatalogBrowse ? catalogBrowsePane.scrollY : browsePane.scrollY
    readonly property bool docked: !browse || (catalogAvailable && browseScrollY > 36)

    property var selected: null
    property int selectedIdx: -1
    property bool detailOpen: false
    property string detailPoster: ""

    SearchFormat { id: fmt }
    SearchCompute { id: compute; sv: page }

    readonly property var seasonTabs: compute.seasonTabs
    readonly property int packCount: compute.packCount
    readonly property var episodeTabs: compute.episodeTabs
    readonly property var qualityOptions: compute.qualityOptions
    readonly property var sourceOptions: compute.sourceOptions
    readonly property var groupOptions: compute.groupOptions
    readonly property var providerOptions: compute.providerOptions
    readonly property bool hasVersions: compute.hasVersions
    readonly property var langOptions: compute.langOptions
    readonly property var viewModel: compute.viewModel
    readonly property double saveFree: compute.saveFree
    readonly property int wontFit: compute.wontFit

    function typeLabel(t) { return fmt.typeLabel(t) }
    function fileUrl(p) { return fmt.fileUrl(p) }
    function langName(c) { return fmt.langName(c) }
    function seedColor(n) { return fmt.seedColor(n) }
    function seedFill(n) { return fmt.seedFill(n) }
    function fmtSize(b) { return fmt.fmtSize(b) }
    function fmtCount(n) { return fmt.fmtCount(n) }

    onBrowseChanged: if (!browse) showCatalogBrowse = false
    onTypeFilterChanged: {
        if (typeFilter === "game")
            return
        showCatalogBrowse = false
        groupFilter = ""
        if (catalogBrowsePane) {
            catalogBrowsePane.group = ""
            catalogBrowsePane.pageIndex = 0
        }
    }

    function reloadGames() { gameList = (api ? api.gameSources() : []) }

    function openCatalogBrowse(group) {
        showCatalogBrowse = true
        if (catalogBrowsePane) {
            catalogBrowsePane.pageIndex = 0
            catalogBrowsePane.selectGroup(group || "")
        }
        groupFilter = group || ""
    }

    function closeCatalogBrowse() {
        showCatalogBrowse = false
        groupFilter = ""
        if (catalogBrowsePane) {
            catalogBrowsePane.group = ""
            catalogBrowsePane.pageIndex = 0
        }
    }

    function openDetail(item) {
        if (!item) return
        selected = item
        selectedIdx = item._idx
        detailPoster = fileUrl(item.poster || "")
        detailOpen = true
        if ((!item.poster || item.poster === "") && item.coverHash && api)
            api.resolveCover(item._idx)
        if (api) api.fetchWorkStills()
    }

    function clearFilters() {
        qualityFilter = ""; sourceFilter = ""; groupFilter = ""; providerFilter = ""; langFilter = ""; audioModeFilter = ""; minSeeds = 0; sortKey = ""
        seasonFilter = -2; episodeFilter = -1
        filtersRow.reset()
    }

    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onGameSourcesChanged() { page.reloadGames(); page.gameCatalogGen++ }
        function onResultsChanged() {
            page.detailOpen = false
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

    Connections {
        target: typeof addons !== "undefined" ? addons : null
        ignoreUnknownSignals: true
        function onChanged() { if (page.api) page.api.refreshSources() }
    }

    onShowGameMgrChanged: if (showGameMgr) reloadGames()
    onVisibleChanged: if (visible) {
        if (api) api.refreshSources()
        Qt.callLater(function() { findBar.focusInput() })
    }

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
    function commitSearch() { runSearch(); pushRecent(findBar.text) }

    function queryEdited(t) {
        if (t.trim().length >= 2) searchDebounce.restart()
        else searchDebounce.stop()
    }
    Timer {
        id: searchDebounce
        interval: 450; repeat: false
        onTriggered: page.runSearch()
    }

    function pickBest() {
        if (!api) return
        var i = api.bestResultIndex()
        if (i >= 0) api.activateResult(i)
    }

    function runQuery(text) {
        findBar.setText(text)
        findBar.resetSource()
        commitSearch()
    }

    signal freeSpaceRequested(double targetBytes)

    ColumnLayout {
        id: mainCol
        anchors.fill: parent
        spacing: 0

        Item { Layout.fillHeight: true; visible: page.browse && !page.catalogAvailable }

        SearchLanding {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 20
            visible: page.browse && !page.catalogAvailable
        }

        FindBar { id: findBar; sv: page }

        SearchRecents { sv: page }

        Item { Layout.fillHeight: true; visible: page.browse && !page.catalogAvailable }

        FindBrowse {
            id: browsePane
            Layout.fillWidth: true
            Layout.preferredHeight: mainCol.height - findBar.height
            visible: page.browse && page.catalogAvailable && !page.showCatalogBrowse
            typeFilter: page.typeFilter
            active: page.visible && page.browse && page.catalogAvailable && !page.showCatalogBrowse
            showCatalogEntry: {
                var _ = page.gameCatalogGen
                return page.api && page.api.gameSources().length > 0
            }
            onFindRequested: function(title) { page.runQuery(title) }
            onTypeFilterRequested: function(type) { page.typeFilter = type }
            onCatalogBrowseRequested: function(group) { page.openCatalogBrowse(group || "") }
        }

        FindCatalogBrowse {
            id: catalogBrowsePane
            sv: page
            Layout.fillWidth: true
            Layout.preferredHeight: mainCol.height - findBar.height
            visible: page.browse && page.showCatalogBrowse
            onBackRequested: page.closeCatalogBrowse()
        }

        SearchWorkHeader { sv: page }
        SearchFiltersRow { id: filtersRow; sv: page }
        SearchModeBars { sv: page }
        SearchListPane { sv: page; Layout.fillWidth: true; Layout.fillHeight: true }
        SearchTitlesPane { sv: page; Layout.fillWidth: true; Layout.fillHeight: true }
        SearchResultsFooter { sv: page }
    }

    SearchDetailDrawer { anchors.fill: parent; sv: page }
    SearchGameMgr { sv: page }
    SourcesManager { sv: page }
    SearchFitDialog {
        sv: page
        onFreeSpaceRequested: function(bytes) { page.freeSpaceRequested(bytes) }
    }
}
