// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The client-side filter/sort row of the search results (provider, quality,
// source, repacker, language, seeds, sort + count / best / clear). Writes the
// page's filter properties; the page's computeView() consumes them.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: row
    property var sv
    // clearFilters() resets the page-side properties; this puts the selects back
    function reset() {
        qualSel.currentIndex = 0; srcFiltSel.currentIndex = 0; provSel.currentIndex = 0
        repSel.currentIndex = 0; langSel.currentIndex = 0; seedSel.currentIndex = 0; sortSel.currentIndex = 0
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 44
    visible: sv.isFlatList && sv.api && sv.api.results.length > 0
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
            visible: row.sv.providerOptions.length > 1
            Layout.preferredHeight: 30
            Layout.preferredWidth: 150
            model: [i18n.t("search_provider_all")].concat(row.sv.providerOptions)
            onActivated: row.sv.providerFilter = currentIndex <= 0 ? "" : currentText
        }
        // quality (video modes only)
        TSelect {
            id: qualSel
            visible: !row.sv.isGames && row.sv.qualityOptions.length > 0
            Layout.preferredHeight: 30
            Layout.preferredWidth: 110
            model: [i18n.t("search_filter_all")].concat(row.sv.qualityOptions)
            onActivated: row.sv.qualityFilter = currentIndex <= 0 ? "" : currentText
        }
        // source (video modes only)
        TSelect {
            id: srcFiltSel
            visible: !row.sv.isGames && row.sv.sourceOptions.length > 0
            Layout.preferredHeight: 30
            Layout.preferredWidth: 120
            model: [i18n.t("search_filter_all")].concat(row.sv.sourceOptions)
            onActivated: row.sv.sourceFilter = currentIndex <= 0 ? "" : currentText
        }
        // who released it — the axis users trust, and the one they asked for
        TSelect {
            id: repSel
            visible: row.sv.groupOptions.length > 0
            Layout.preferredHeight: 30
            Layout.preferredWidth: 140
            model: [i18n.t("search_group_all")].concat(row.sv.groupOptions)
            onActivated: row.sv.groupFilter = currentIndex <= 0 ? "" : currentText
        }
        // audio language (video modes that have tagged releases)
        TSelect {
            id: langSel
            visible: !row.sv.isGames && row.sv.langOptions.length > 0
            Layout.preferredHeight: 30
            Layout.preferredWidth: 130
            model: [i18n.t("search_lang_all")].concat(row.sv.langOptions.map(row.sv.langName))
            onActivated: row.sv.langFilter = currentIndex <= 0 ? "" : row.sv.langOptions[currentIndex - 1]
        }
        // min seeders (video modes)
        TSelect {
            id: seedSel
            visible: !row.sv.isGames
            Layout.preferredHeight: 30
            Layout.preferredWidth: 120
            property var vals: [0, 1, 10, 50, 100]
            model: [i18n.t("search_seeds_any"), "1+", "10+", "50+", "100+"]
            onActivated: row.sv.minSeeds = vals[currentIndex]
        }
        // sort
        TSelect {
            id: sortSel
            Layout.preferredHeight: 30
            Layout.preferredWidth: 150
            property var keys: row.sv.hasVersions
                               ? ["", "seeders", "size", "name", "version"]
                               : ["", "seeders", "size", "name"]
            model: {
                var base = [i18n.t("search_sort_relevance"), i18n.t("search_sort_seeders"),
                            i18n.t("search_sort_size"), i18n.t("search_sort_name")]
                if (row.sv.hasVersions) base.push(i18n.t("search_sort_version"))
                return (i18n.language, base)
            }
            onActivated: row.sv.sortKey = keys[currentIndex]
        }

        Item { Layout.fillWidth: true }

        Text {
            text: row.sv.viewModel.length + " / " + (row.sv.api ? row.sv.api.results.length : 0)
            color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        BtnFlat {
            // one-click "best" release for a picked title (quality + seeders + your language)
            // secondary styling so it doesn't fight the red "Search" stacked above it
            visible: row.sv.api && row.sv.api.singleTitleView && row.sv.api.results.length > 0
            primary: false
            text: "★  " + (i18n.language, i18n.t("search_get_best"))
            onClicked: row.sv.pickBest()
        }
        BtnFlat {
            visible: row.sv.qualityFilter !== "" || row.sv.sourceFilter !== "" || row.sv.groupFilter !== ""
                     || row.sv.providerFilter !== "" || row.sv.minSeeds > 0 || row.sv.sortKey !== ""
            text: (i18n.language, i18n.t("search_filter_clear"))
            onClicked: row.sv.clearFilters()
        }
    }
}
