// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Releases on a title's stage: a film's, a game's, or one episode's sources,
// with the filters that decide between them.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: list
    required property var sv
    required property var logic
    property var rows: []           // already narrowed to this title
    property string sortKey: "seeds"
    property bool isGame: false
    property bool forEpisode: false    // a season pack among one episode's sources says so

    property string quality: ""
    property bool hideCam: true
    property bool myLang: false
    property bool fits: false
    function reset() { quality = ""; hideCam = true; myLang = false; fits = false }

    spacing: 0

    readonly property var shown: logic.sortRows(
        logic.applyFilters(rows, { quality: quality, hideCam: hideCam, myLang: myLang,
                                   fits: fits, free: sv.saveFree }),
        sortKey, sv.api ? function (a, b) { return sv.api.compareBuildVersions(a, b) } : null)
    // results is read so the pick follows a new batch of releases
    readonly property int bestIdx: sv.api && sv.api.results.length >= 0 ? sv.api.bestResultIndex() : -1

    function tr(k) { return i18n.t(k) }

    StageFilterBar {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.bottomMargin: 10
        qualities: list.isGame ? [] : list.logic.qualityCounts(list.rows)
        quality: list.quality
        showCam: list.logic.hasCam(list.rows)
        hideCam: list.hideCam
        showLang: list.logic.hasNative(list.rows)
        myLang: list.myLang
        showFits: list.sv.saveFree >= 0
        fits: list.fits
        onQualityPicked: function (v) { list.quality = v }
        onCamToggled: list.hideCam = !list.hideCam
        onLangToggled: list.myLang = !list.myLang
        onFitsToggled: list.fits = !list.fits
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        color: Theme.stageHair
    }

    ListView {
        id: view
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        spacing: 4
        topMargin: 6
        bottomMargin: 20
        boundsBehavior: Flickable.StopAtBounds
        model: list.shown
        WheelScroller { flick: view }

        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            y: 40
            visible: view.count === 0
            horizontalAlignment: Text.AlignHCenter
            text: list.sv.api && list.sv.api.searching
                  ? (i18n.language, i18n.t("search_searching2"))
                  : (i18n.language, i18n.t(list.rows.length > 0 ? "find_no_match" : "find_no_releases"))
            color: Theme.stageT4
            font.pixelSize: 13
            font.family: Theme.fontSans
            MouseArea {
                anchors.fill: parent
                enabled: list.rows.length > 0
                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: list.reset()
            }
        }

        delegate: StageReleaseRow {
            required property var modelData
            width: view.width
            pill: list.logic.bigLabel(modelData, list.isGame)
            pillTone: list.logic.isCam(modelData) ? "cam" : (modelData.quality === "4K" ? "hi" : "")
            raw: modelData.name || ""
            meta: (i18n.language, list.logic.metaLine(modelData, list.tr))
                  + (list.forEpisode && !((modelData.episode || 0) > 0)
                     ? "  ·  " + i18n.t("find_season_pack") : "")
            warnBadge: (modelData.trustWhy || "").length > 0
                       ? (i18n.language, i18n.t(modelData.trustWhy)) : ""
            best: modelData._idx === list.bestIdx
            sizeText: modelData.sizeStr || ""
            sizeWarn: list.sv.saveFree >= 0 && (modelData.sizeBytes || 0) > list.sv.saveFree
            seedsN: modelData.seedsN || 0
            seedColor: list.sv.seedColor(modelData.seedsN || 0)
            seedFill: list.sv.seedFill(modelData.seedsN || 0)
            added: list.sv.wasAdded(modelData)
            canWatch: list.sv.canWatch(modelData)
            onAddRequested: if (list.sv.api) list.sv.api.activateResult(modelData._idx)
            onWatchRequested: if (list.sv.api) list.sv.api.addAndWatch(modelData._idx)
            onOpenRequested: list.sv.openDetail(modelData)
        }
    }
}
