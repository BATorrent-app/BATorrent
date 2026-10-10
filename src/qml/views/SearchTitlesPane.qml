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

    readonly property var titles: sv.api && sv.isTitles ? sv.api.results : []
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
                height: 300
                radius: 8
                color: Theme.stageBg
                clip: true
                visible: pane.bestItem !== null
                TitleBanner {
                    anchors.fill: parent
                    item: pane.bestItem
                    centered: true
                    showRating: false
                    textWidth: 480
                    logoWidth: 300
                    logoHeight: 96
                    kicker: (i18n.language, i18n.t("search_best_match"))
                    kickerColor: Theme.accentText
                    extraFacts: pane.bestItem && (pane.bestItem.maker || "").length > 0 ? [pane.bestItem.maker] : []
                    status: pane.bestSummary && pane.bestSummary.count > 0
                            ? (i18n.language, i18n.t("find_n_releases_best"))
                                  .arg(pane.bestSummary.count).arg(pane.sv.fmtCount(pane.bestSummary.maxSeeds))
                            : ""
                    primaryLabel: (i18n.language, i18n.t("find_see_releases"))
                    onPrimaryClicked: if (pane.sv.api) pane.sv.api.activateResult(0)
                }
                MouseArea {
                    anchors.fill: parent
                    z: -1
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (pane.sv.api) pane.sv.api.activateResult(0)
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
                        text: (i18n.language, i18n.t("search_titles_n")).arg(pane.others.length)
                        color: Theme.t4
                        font.pixelSize: 12; font.family: Theme.fontSans
                    }
                }
                Grid {
                    id: grid
                    readonly property int cols: Math.max(2, Math.min(6, Math.floor((col.width + 10) / 250)))
                    readonly property real cellW: (col.width - (cols - 1) * columnSpacing) / cols
                    columns: cols
                    columnSpacing: 10
                    rowSpacing: 24
                    Repeater {
                        model: pane.others
                        TitleTile {
                            required property var modelData
                            required property int index
                            width: grid.cellW
                            item: modelData
                            saved: pane.savedOf(modelData)
                            onWatchlistToggle: pane.toggleSaved(modelData)
                            onActivated: if (pane.sv.api) pane.sv.api.activateResult(pane.showBest ? index + 1 : index)
                        }
                    }
                }
            }
        }
    }
}
