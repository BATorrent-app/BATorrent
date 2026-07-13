// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Flat-results surface: mode-adaptive column headers + the results list with
// its empty/loading state and the per-row context menu.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

ColumnLayout {
    id: pane
    property var sv
    spacing: 0
    visible: !sv.isTitles && !sv.browse

    // results header (columns adapt to mode)
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 30
        color: "transparent"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp5
            anchors.rightMargin: Theme.sp5
            spacing: Theme.sp4
            property bool torrentish: pane.sv.api && (pane.sv.api.mode === "torrent" || pane.sv.api.mode === "games" || pane.sv.api.mode === "all")
            Item { Layout.preferredWidth: 46; visible: pane.sv.showRowThumbs }   // thumb column
            Text { text: (i18n.language, i18n.t("search_col_name2")); Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t(pane.sv.isEpisodes ? "search_col_aired" : "search_col_size2")); Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { visible: parent.torrentish; text: (i18n.language, i18n.t("search_col_seeds2")); Layout.preferredWidth: 100; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { visible: parent.torrentish; text: (i18n.language, i18n.t("search_col_leech")); Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Item { Layout.preferredWidth: 36 }
        }
    }

    ListView {
        id: resultsView
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        model: pane.sv.isTitles ? [] : pane.sv.viewModel
        boundsBehavior: Flickable.StopAtBounds
        cacheBuffer: 240

        // empty / loading / welcome state
        SearchEmptyState {
            anchors.fill: parent
            sv: pane.sv
            visible: resultsView.count === 0
        }

        delegate: SearchResultRow {
            sv: pane.sv
            onMenuRequested: function (i) { rowMenu.openFor(i) }
        }
    }

    // per-result right-click menu
    BatMenu {
        id: rowMenu
        property int idx: -1
        function openFor(i) { idx = i; popup() }
        implicitWidth: 190
        BatMenuItem {
            text: pane.sv.isCatalog ? (i18n.language, i18n.t("search_view_streams")) : (i18n.language, i18n.t("search_add"))
            onTriggered: if (pane.sv.api && rowMenu.idx >= 0) pane.sv.api.activateResult(rowMenu.idx)
        }
        BatMenuItem {
            text: (i18n.language, i18n.t("search_copy_magnet"))
            onTriggered: if (pane.sv.api && rowMenu.idx >= 0) pane.sv.api.copyMagnet(rowMenu.idx)
        }
        BatMenuItem {
            visible: !pane.sv.isCatalog && typeof debrid !== "undefined" && debrid.hasToken
            height: visible ? implicitHeight : 0
            text: (i18n.language, i18n.t("debrid_stream")).arg(debrid.providerName)
            onTriggered: {
                if (!pane.sv.api || rowMenu.idx < 0) return
                var m = pane.sv.api.magnetAt(rowMenu.idx)
                if (m) debrid.streamMagnet(m)
            }
        }
    }
}
