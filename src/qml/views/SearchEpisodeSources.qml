// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One episode's sources: its own releases first, then the season packs that
// hold it. Where they come from is said once at the top, with the person's
// language; "Only what fits" is the one filter.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: src
    required property var sv
    required property var logic
    property var index
    property int season
    property int episode
    property string title
    signal backRequested()

    property bool onlyFits: false
    onEpisodeChanged: onlyFits = false
    spacing: 0

    readonly property var rows: index ? logic.episodeReleases(index, episode) : []
    readonly property var kept: onlyFits ? rows.filter(function (r) { return logic.fitOf(r, sv.saveFree).fits }) : rows
    readonly property var own: kept.filter(function (r) { return (r.episode || 0) > 0 })
    readonly property var packs: kept.filter(function (r) { return !((r.episode || 0) > 0) })
    // results is read so the pick follows a new batch of releases
    readonly property int bestIdx: logic.recommendedIdx(own.concat(packs),
                                                        sv.api && sv.results.length >= 0 ? sv.api.bestResultIndex() : -1,
                                                        sv.saveFree)

    // "PT-BR", the tag the language line uses for the person's own language
    readonly property string myTag: {
        var n = String(i18n.localeName || "")
        return n.length > 0 ? n.replace("_", "-").toUpperCase() : String(i18n.code || "").toUpperCase()
    }
    readonly property string myLangName: {
        var l = Qt.locale(i18n.localeName || "")
        var n = l.nativeLanguageName || ""
        n = n.length > 0 ? n.charAt(0).toUpperCase() + n.slice(1) : ""
        var t = String(i18n.localeName || "").split("_")[1] || ""
        return t.length > 0 ? n + " (" + t + ")" : n
    }
    readonly property string providerLine: {
        i18n.language
        var seen = ({}), list = []
        for (var i = 0; i < rows.length; i++) {
            var p = rows[i].provider || rows[i].sub || ""
            if (p.length > 0 && !seen[p]) { seen[p] = true; list.push(p) }
        }
        var bits = [i18n.t(rows.length === 1 ? "find_rel_n_1" : "find_rel_n").arg(rows.length)]
        if (list.length === 1) bits.push(i18n.t("find_rel_all_from").arg(list[0].replace(/\s*\(.*\)$/, "")))
        else if (list.length > 1) bits.push(i18n.t("find_rel_from_n").arg(list.length))
        if (myLangName.length > 0) bits.push(i18n.t("find_rel_yours").arg(myLangName))
        return bits.join(" · ")
    }

    // ---- header: back, the episode, where its releases come from, the one filter ----
    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8; Layout.rightMargin: 8; Layout.bottomMargin: 14
        spacing: 12
        Rectangle {
            Layout.preferredWidth: 32; Layout.preferredHeight: 32
            Layout.alignment: Qt.AlignTop
            radius: Theme.rad(6)
            color: backMa.containsMouse ? Theme.stageHover : "transparent"
            border.width: 1; border.color: Theme.stageEdge
            IconImg { anchors.centerIn: parent; src: "qrc:/icons/arrow-left.svg"; tint: Theme.stageT1; s: 14 }
            MouseArea { id: backMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: src.backRequested() }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            Text {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                text: (i18n.language, i18n.t("search_season_abbr")).arg(src.season) + " "
                      + (i18n.language, i18n.t("search_episode_abbr")).arg(src.episode)
                      + (src.title.length > 0 ? " · " + src.title : "")
                color: Theme.stageT1
                font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                text: src.providerLine
                color: Theme.stageMuted
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                elide: Text.ElideRight
            }
        }
        Row {
            visible: src.sv.saveFree >= 0
            Layout.alignment: Qt.AlignTop
            spacing: 10
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t("find_rel_only_fits"))
                color: Theme.stageT1; font.pixelSize: 14; font.family: Theme.fontSans
            }
            TToggle {
                anchors.verticalCenter: parent.verticalCenter
                on: src.onlyFits
                onToggled: function (v) { src.onlyFits = v }
            }
        }
    }

    Flickable {
        id: flick
        Layout.fillWidth: true
        Layout.fillHeight: true
        clip: true
        contentHeight: groups.height + 24
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
        WheelScroller { flick: flick }

        Column {
            id: groups
            width: flick.width
            spacing: 18

            Group {
                label: (i18n.language, i18n.t("find_rel_just_this"))
                count: String(src.own.length)
                items: src.own
            }
            Group {
                label: (i18n.language, i18n.t("find_rel_packs_with"))
                count: (i18n.language, i18n.t("find_rel_whole_season")).arg(src.packs.length)
                items: src.packs
            }

            Text {
                visible: src.kept.length === 0
                width: groups.width
                topPadding: 30
                horizontalAlignment: Text.AlignHCenter
                text: src.sv.api && src.sv.api.searching
                      ? (i18n.language, i18n.t("search_searching2"))
                      : (i18n.language, i18n.t(src.rows.length > 0 ? "find_no_match" : "find_no_releases"))
                color: Theme.stageMuted
                font.pixelSize: 13; font.family: Theme.fontSans
            }
        }
    }

    component Group: Column {
        id: grp
        property string label
        property string count
        property var items: []
        visible: items.length > 0
        width: groups.width
        spacing: 4
        RowLayout {
            width: parent.width
            Text {
                Layout.leftMargin: 8
                Layout.fillWidth: true
                text: grp.label
                color: Theme.stageMuted
                font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9
                font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
            }
            Text {
                Layout.rightMargin: 8
                text: grp.count
                color: Theme.stageMuted
                font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
        Repeater {
            model: grp.items
            ReleaseRow {
                required property var modelData
                readonly property var fit: src.logic.fitOf(modelData, src.sv.saveFree)
                width: grp.width
                quality: modelData.quality || "?"
                name: src.logic.readableName(modelData, i18n.t)
                raw: modelData.name || ""
                recommended: modelData._idx === src.bestIdx
                langBits: (i18n.language, src.logic.langBits(modelData, src.myTag, i18n.t))
                sizeText: modelData.sizeStr || ""
                seedsN: modelData.seedsN || 0
                seedFill: src.logic.seedFillOf(modelData.seedsN || 0)
                fewSeeds: src.logic.hasFewSeeds(modelData)
                shortBy: fit.fits ? "" : src.sv.fmtSize(fit.short)
                canWatch: src.sv.canWatch(modelData)
                added: src.sv.wasAdded(modelData)
                onAddRequested: if (src.sv.api) src.sv.api.activateResult(modelData._idx)
                onWatchRequested: if (src.sv.api) src.sv.api.addAndWatchEpisode(modelData._idx, src.season, src.episode)
                onOpenRequested: src.sv.openDetail(modelData)
            }
        }
    }
}
