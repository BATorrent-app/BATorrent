// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../views"

Rectangle {
    id: preview
    property bool classic: false
    property bool navLeft: false
    property bool detailBottom: false
    property string focusArea: ""

    readonly property real logicalWidth: 1280
    readonly property real logicalHeight: 720
    readonly property real previewScale: Math.min(width / logicalWidth, height / logicalHeight)
    readonly property var demoCounts: ({
        all: 5, active: 3, downloading: 2, seeding: 1,
        paused: 1, queued: 0, completed: 1
    })
    readonly property var demoStats: ({
        torrentCount: 5,
        activeCount: 3,
        altSpeedsActive: false,
        portStatus: 1,
        totalDownSpeed: "8.2 MB/s",
        totalUpSpeed: "1.1 MB/s",
        totalDownloaded: "21.4 GB",
        totalUploaded: "8.7 GB",
        globalRatio: "0.41",
        freeDiskSpace: "184 GB"
    })
    implicitWidth: 520
    implicitHeight: 300
    radius: 14
    color: Theme.bg
    border.width: focusArea === "theme" ? 2 : 1
    border.color: focusArea === "theme" ? Theme.accent : Theme.hair
    clip: true
    Accessible.ignored: true

    Behavior on border.color {
        ColorAnimation { duration: Theme.reduceMotion ? 0 : Theme.durFast }
    }
    ListModel { id: demoModel }

    function populateDemoModel() {
        demoModel.clear()
        demoModel.append({
            torrentName: "Nightfall.2026.1080p", metaTitle: "Nightfall",
            stateKey: "downloading", progress: 0.72, posterPath: "qrc:/images/007.jpg",
            stateString: i18n.t("state_downloading"), stateDetail: "", fileKind: "MKV",
            category: "Movies", size: "8.4 GB", downSpeed: "6.8 MB/s",
            upSpeed: "420 KB/s", downRate: 7130317, upRate: 430080,
            sizeBytes: 9019431322, infoHash: "demo-nightfall", playable: true,
            downloaded: "6.1 GB", year: 2026, genres: "Thriller · Drama",
            queuePos: 0, numPeers: 18, numSeeds: 42, ratio: 0.64,
            availability: 4.2, eta: "14m"
        })
        demoModel.append({
            torrentName: "Forza.Horizon.Collection", metaTitle: "Forza Horizon",
            stateKey: "seeding", progress: 1.0, posterPath: "qrc:/images/forza.png",
            stateString: i18n.t("state_seeding"), stateDetail: "", fileKind: "EXE",
            category: "Games", size: "67.2 GB", downSpeed: "0 KB/s",
            upSpeed: "860 KB/s", downRate: 0, upRate: 880640,
            sizeBytes: 72155450572, infoHash: "demo-forza", playable: false,
            downloaded: "67.2 GB", year: 2025, genres: "Racing",
            queuePos: 0, numPeers: 7, numSeeds: 31, ratio: 1.84,
            availability: 8.1, eta: ""
        })
        demoModel.append({
            torrentName: "Hollow.Knight.Soundtrack", metaTitle: "Hollow Knight",
            stateKey: "finished", progress: 1.0, posterPath: "qrc:/images/hollow.webp",
            stateString: i18n.t("state_finished"), stateDetail: "", fileKind: "FLAC",
            category: "Audio", size: "1.3 GB", downSpeed: "0 KB/s",
            upSpeed: "0 KB/s", downRate: 0, upRate: 0,
            sizeBytes: 1395864371, infoHash: "demo-hollow", playable: true,
            downloaded: "1.3 GB", year: 2017, genres: "Soundtrack",
            queuePos: 0, numPeers: 0, numSeeds: 18, ratio: 0.92,
            availability: 3.7, eta: ""
        })
        demoModel.append({
            torrentName: "Open.Skies.S01E04", metaTitle: "Open Skies",
            stateKey: "downloading", progress: 0.38, posterPath: "",
            stateString: i18n.t("state_downloading"), stateDetail: "", fileKind: "MP4",
            category: "Series", size: "3.1 GB", downSpeed: "1.4 MB/s",
            upSpeed: "96 KB/s", downRate: 1468006, upRate: 98304,
            sizeBytes: 3328599654, infoHash: "demo-skies", playable: true,
            downloaded: "1.2 GB", year: 2026, genres: "Documentary",
            queuePos: 0, numPeers: 9, numSeeds: 24, ratio: 0.18,
            availability: 2.8, eta: "23m"
        })
        demoModel.append({
            torrentName: "Creative.Tools.Bundle", metaTitle: "Creative Tools",
            stateKey: "missing", progress: 0.54, posterPath: "",
            stateString: i18n.t("state_paused"), stateDetail: "", fileKind: "ZIP",
            category: "Apps", size: "4.8 GB", downSpeed: "0 KB/s",
            upSpeed: "0 KB/s", downRate: 0, upRate: 0,
            sizeBytes: 5153960755, infoHash: "demo-tools", playable: false,
            downloaded: "2.6 GB", year: 2026, genres: "Software",
            queuePos: 0, numPeers: 0, numSeeds: 16, ratio: 0.0,
            availability: 1.9, eta: ""
        })
    }
    Component.onCompleted: populateDemoModel()
    Connections {
        target: i18n
        function onLanguageChanged() { preview.populateDemoModel() }
    }
    QtObject {
        id: previewHost

        readonly property var model: demoModel
        readonly property bool hasSel: true
        readonly property bool detailBottom: preview.detailBottom
        property int detailTab: 0
        property bool detailsLocked: false
        property bool detailsCollapsed: false
        readonly property bool detailsShownCollapsed: false

        // Delegates like the real window does — the preview must not carry its
        // own copy of the state language, or the two drift and the wizard shows
        // a colour the app no longer uses.
        function fillFor(key) { return Theme.fillFor(key) }
        function textFor(key) {
            if (key === "finished" || key === "completed") return Theme.grn
            if (key === "seeding") return Theme.up
            if (key === "paused" || key === "queued") return Theme.t3
            return Theme.accentText
        }
        function dotFor(key) {
            if (key === "finished" || key === "completed") return Theme.grn
            if (key === "seeding") return Theme.amber
            if (key === "paused" || key === "queued") return Theme.t4
            return Theme.accent
        }
        function fmtEta(seconds) {
            return i18n.t("eta_left").arg(Math.max(1, Math.round(seconds / 60)) + "m")
        }
        function fileUrl(path) { return path || "" }
        function catLabel(value) {
            if (value === "Apps") return i18n.t("cat_apps")
            if (value === "Games") return i18n.t("cat_games")
            if (value === "Movies") return i18n.t("cat_movies")
            if (value === "Series") return i18n.t("cat_series")
            return value
        }
        function customCategories() { return [] }
        function clearFilterFocus() {}
        function openContext() {}
        function flashRefresh() {}
        function toggleDetailsCollapsed() {}
        function toggleDetailsLocked() {}
    }

    QtObject {
        id: previewController

        property bool gridView: !preview.classic
        property bool classicMode: preview.classic
        property string activeFilter: "all"
        property string catFilter: ""
        property string sortColumn: "name"
        property bool sortAsc: true
        property var selectedRows: [0]
        property int selected: 0

        function isRowSelected(row) { return row === 0 }
        function selectRow() {}
        function setFilter(value) { activeFilter = value }
        function applyCatFilter(value) { catFilter = value }
        function toggleSort(value) { sortColumn = value }
    }

    Item {
        id: scaledFrame
        width: preview.logicalWidth
        height: preview.logicalHeight
        x: (preview.width - width * scale) / 2
        y: (preview.height - height * scale) / 2
        scale: preview.previewScale
        transformOrigin: Item.TopLeft
        enabled: false

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            NavBar {
                id: topNavigation
                visible: !preview.navLeft
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 54 : 0
                currentIndex: 0
                showDownloadChip: false
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                NavRail {
                    id: sideNavigation
                    visible: preview.navLeft
                    Layout.preferredWidth: visible ? 188 : 0
                    Layout.fillHeight: true
                    currentIndex: 0
                    collapsed: false
                    persistCollapsedState: false
                    showDownloadChip: false
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    Toolbar {
                        win: previewHost
                        downSpeedOverride: preview.demoStats.totalDownSpeed
                        upSpeedOverride: preview.demoStats.totalUpSpeed
                    }

                    FilterBar {
                        id: previewFilter
                        win: previewHost
                        controller: previewController
                        countOverride: preview.demoCounts
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 0

                        OnboardingPreviewLibrary {
                            id: library
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            host: previewHost
                            controller: previewController
                            demoModel: demoModel
                        }

                        OnboardingPreviewDetail {
                            id: sideDetail
                            // Honor the detail-position choice alone. Coupling to
                            // classicMode made both tiles look identical after the
                            // user picked the list view (classic always docks bottom).
                            visible: preview.focusArea === "detail" && !preview.detailBottom
                            Layout.preferredWidth: visible ? 340 : 0
                            Layout.fillHeight: true
                            vertical: true
                        }
                    }

                    OnboardingPreviewDetail {
                        id: bottomDetail
                        visible: preview.focusArea === "detail" && preview.detailBottom
                        Layout.fillWidth: true
                        Layout.preferredHeight: visible ? 270 : 0
                        vertical: false
                    }

                    StatusBar { stats: preview.demoStats }
                }
            }
        }

        OnboardingPreviewFocus {
            targetItem: preview.navLeft ? sideNavigation : topNavigation
            visible: preview.focusArea === "nav"
        }
        OnboardingPreviewFocus {
            targetItem: preview.detailBottom ? bottomDetail : sideDetail
            visible: preview.focusArea === "detail"
        }
        OnboardingPreviewFocus {
            targetItem: library
            visible: preview.focusArea === "view"
        }
    }
}
