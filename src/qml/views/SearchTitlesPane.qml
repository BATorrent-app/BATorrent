// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Title-disambiguation surface: the best match as a wide banner, the other
// candidates as tiles, before drilling into one title's releases.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: pane
    property var sv
    visible: sv.isTitles && !sv.browse

    // the type chips filter what the catalogs found
    readonly property var titles: {
        var all = sv.api && sv.isTitles ? sv.results : []
        var t = sv.typeFilter
        return t === "all" ? all : all.filter(function (w) { return w.type === t })
    }
    readonly property bool showBest: titles.length > 0 && sv.api && !sv.api.searching
    readonly property var bestItem: showBest ? titles[0] : null
    readonly property var others: showBest ? titles.slice(1) : titles

    property var bestSummary: null   // {count, bestSize, maxSeeds} for bestItem
    onBestItemChanged: {
        bestSummary = null
        if (bestItem && sv.api && (bestItem.name || "").length > 0)
            sv.api.summarizeSources(bestItem.name)
    }
    Connections {
        target: pane.sv.api
        ignoreUnknownSignals: true
        function onSourceSummary(title, count, bestSize, maxSeeds) {
            if (pane.bestItem && pane.bestItem.name === title)
                pane.bestSummary = { count: count, bestSize: bestSize, maxSeeds: maxSeeds }
        }
    }

    // the type chips filter the list: open by the row's place in the full results
    function activate(it) {
        if (!sv.api || !it) return
        var i = sv.results.indexOf(it)
        if (i >= 0) sv.api.activateResult(i)
    }
    function savedOf(it) {
        return typeof session !== "undefined"
               && (session.watchlist, session.inWatchlist(it.name || "", it.type || ""))
    }
    function toggleSaved(it) {
        if (typeof session !== "undefined") session.toggleWatchlist({
            title: it.name || "", type: it.type || "", poster: it.poster || "", year: it.year || "" })
    }

    // loading and not-found, centred: this pane owns the page while titles resolve
    SearchEmptyState {
        anchors.fill: parent
        sv: pane.sv
        visible: pane.titles.length === 0
    }

    Flickable {
        id: flick
        anchors.fill: parent
        visible: pane.titles.length > 0
        contentHeight: col.height + 32
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        WheelScroller { flick: flick }

        Column {
            id: col
            x: Theme.sp5
            y: 20
            width: flick.width - 2 * Theme.sp5
            spacing: 32

            Rectangle {
                width: col.width
                height: 330
                radius: Theme.rad(8)
                color: Theme.stageBg
                clip: true
                visible: pane.bestItem !== null
                TitleBanner {
                    anchors.fill: parent
                    item: pane.bestItem
                    centered: true
                    textWidth: 480
                    logoWidth: 300
                    logoHeight: 96
                    kicker: pane.bestItem ? (i18n.language, i18n.t("search_best_match")).toUpperCase() + "  ·  "
                                            + typeLabel(pane.bestItem.type || "").toUpperCase() : ""
                    showType: false
                    extraFacts: {
                        var b = pane.bestItem
                        if (!b) return []
                        var f = []
                        if ((b.maker || "").length > 0) f.push(b.maker)
                        var g = b.genres || []
                        if (g.length > 0) f.push(g.slice(0, 2).join(", "))
                        return f
                    }
                    // what the person already has of it, then how many releases there are
                    statusDot: false
                    status: {
                        i18n.language
                        var bits = []
                        var o = pane.sv.owned ? pane.sv.owned.stateOf(pane.bestItem) : null
                        if (o) {
                            bits.push(i18n.t("find_in_library"))
                            if (o.next) bits.push(i18n.t("find_next_ep").arg(o.next.season).arg(o.next.episode))
                        }
                        if (pane.bestSummary && pane.bestSummary.count > 0)
                            bits.push(pane.bestSummary.count === 1 ? i18n.t("find_n_releases_1")
                                                                   : i18n.t("find_n_releases").arg(pane.bestSummary.count))
                        return bits.join(" · ")
                    }
                    primaryLabel: {
                        i18n.language
                        var t = pane.bestItem ? pane.bestItem.type : ""
                        return i18n.t(t === "series" ? "find_open_series" : t === "game" ? "find_see_builds" : "find_see_releases")
                    }
                    onPrimaryClicked: pane.activate(pane.bestItem)
                }
                MouseArea {
                    anchors.fill: parent
                    z: -1
                    cursorShape: Qt.PointingHandCursor
                    onClicked: pane.activate(pane.bestItem)
                }
            }

            Column {
                width: col.width
                spacing: 14
                visible: pane.others.length > 0
                Row {
                    spacing: 12
                    Text {
                        id: othersTitle
                        text: (i18n.language, i18n.t("find_other_matches"))
                        color: Theme.t1
                        font.pixelSize: 18; font.weight: Font.DemiBold
                        font.letterSpacing: -0.2; font.family: Theme.fontSans
                    }
                    Text {
                        anchors.baseline: othersTitle.baseline
                        text: (i18n.language, i18n.t("find_n_titles_relevance")).arg(Math.min(pane.others.length, grid.cols))
                        color: Theme.t4
                        font.pixelSize: 12; font.family: Theme.fontSans
                    }
                }
                Grid {
                    id: grid
                    // one row: the next few by relevance, the rest is the raw list's job
                    readonly property int cols: Math.max(2, Math.min(6, Math.floor((col.width + 12) / 212)))
                    readonly property real cellW: (col.width - (cols - 1) * columnSpacing) / cols
                    columns: cols
                    columnSpacing: 10
                    rowSpacing: 24
                    Repeater {
                        model: pane.others.slice(0, grid.cols)
                        TitleTile {
                            required property var modelData
                            width: grid.cellW
                            item: modelData
                            saved: pane.savedOf(modelData)
                            owned: pane.sv.owned ? pane.sv.owned.stateOf(modelData) : null
                            onWatchlistToggle: pane.toggleSaved(modelData)
                            onActivated: pane.activate(modelData)
                        }
                    }
                }
            }
        }
    }
}
