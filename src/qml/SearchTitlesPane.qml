// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Title-disambiguation surface: the best-match hero + the covers grid where
// the user picks the actual movie/series/game before drilling into releases.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

ColumnLayout {
    id: pane
    property var sv
    spacing: 0
    visible: sv.isTitles && !sv.browse

    // ---- best-match hero state ----
    readonly property bool showBestMatch: sv.isTitles && sv.api && sv.api.results.length > 0 && !sv.api.searching
    readonly property var bestItem: showBestMatch ? sv.api.results[0] : null
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

    // best-match hero — explicitly surfaces the top title (not just first in grid)
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 142
        Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; Layout.topMargin: Theme.sp4
        visible: pane.showBestMatch
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
                posterUrl: pane.bestItem ? (pane.bestItem.poster || "") : ""
                label: pane.bestItem ? (pane.bestItem.name || "") : ""
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
                    text: pane.bestItem ? (pane.bestItem.name || "") : ""
                    color: Theme.t1; font.pixelSize: 22; font.weight: Font.Bold; font.family: Theme.fontSans
                    Layout.fillWidth: true; elide: Text.ElideRight
                }
                Text {
                    text: {
                        if (!pane.bestItem) return ""
                        var parts = []
                        var t = pane.sv.typeLabel(pane.bestItem.type || "")
                        if ((pane.bestItem.year || "").length > 0) parts.push(pane.bestItem.year)
                        if (t.length > 0) parts.push(t)
                        if ((pane.bestItem.rating || 0) > 0) parts.push("★ " + pane.bestItem.rating.toFixed(1))
                        return parts.join("    ·    ")
                    }
                    color: Theme.t3; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
                Row {
                    spacing: 7
                    visible: pane.bestSummary && pane.bestSummary.count > 0
                    Rectangle { width: 7; height: 7; radius: 4; color: Theme.grn; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: {
                            if (!pane.bestSummary) return ""
                            var s = pane.bestSummary.count + " torrents"
                            if (pane.bestSummary.maxSeeds > 0) s += "    ·    best " + pane.sv.fmtCount(pane.bestSummary.maxSeeds) + " seeds"
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
                onClicked: if (pane.sv.api) pane.sv.api.activateResult(0)
            }
        }
    }
    Text {
        visible: pane.showBestMatch && pane.sv.api && pane.sv.api.results.length > 1
        text: (i18n.language, i18n.t("search_other_matches"))
        Layout.leftMargin: Theme.sp5; Layout.topMargin: 8
        color: Theme.t4; font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 0.8; font.family: Theme.fontSans
    }

    // titles grid (step 1: pick the actual movie/series/game, one cover each)
    GridView {
        id: titlesView
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: pane.sv.isTitles ? (pane.showBestMatch ? pane.sv.api.results.slice(1) : (pane.sv.api ? pane.sv.api.results : [])) : []
        cellWidth: 174
        cellHeight: 286
        leftMargin: Theme.sp5; rightMargin: Theme.sp5; topMargin: Theme.sp4; bottomMargin: Theme.sp4
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: 600

        SearchEmptyState {
            anchors.fill: parent
            sv: pane.sv
            visible: titlesView.count === 0 && !pane.showBestMatch
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
                    var t = pane.sv.typeLabel(modelData.type || "")
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
                onActivated: if (pane.sv.api) pane.sv.api.activateResult(pane.showBestMatch ? index + 1 : index)
                onGetWatch: if (pane.sv.api) pane.sv.api.getAndWatch(modelData.name || "",
                                                                     modelData.year || "",
                                                                     modelData.type || "movie")
            }
        }
    }
}
