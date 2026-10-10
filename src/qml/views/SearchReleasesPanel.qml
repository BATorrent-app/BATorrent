// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A film's or a game's releases, beside its art. A film's are ordered by who
// is sharing them; a game's by build, because the newest patch is the one
// worth having. A game also says up front whether the disk can take it.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: panel
    required property var sv
    required property var logic
    property bool isGame: false

    spacing: 0

    property string sortKey: isGame ? "newest" : "seeds"
    readonly property var rows: {
        if (!sv.api) return []
        var api = sv.api
        var qsets = api.queryWordSets()
        return logic.onTitle(api.results, function (n) { return api.relevanceMulti(n, qsets) })
    }
    readonly property var sortOptions: (isGame ? [{ value: "newest", label: (i18n.language, i18n.t("find_newest_first")) }] : [])
        .concat([{ value: "seeds", label: (i18n.language, i18n.t("find_most_seeded")) },
                 { value: "size_desc", label: (i18n.language, i18n.t("find_largest_first")) },
                 { value: "size_asc", label: (i18n.language, i18n.t("find_smallest_first")) }])
    // Size of the release the list leads with: the one most people will take.
    readonly property double need: list.shown.length > 0 ? (list.shown[0].sizeBytes || 0) : 0
    readonly property var volume: (typeof session !== "undefined" && session.diskVolumes
                                   && session.diskVolumes.length > 0) ? session.diskVolumes[0] : null

    StageHeading {
        Layout.fillWidth: true
        title: (i18n.language, i18n.t("find_releases"))
        linkText: (i18n.language, i18n.t("find_all_releases"))
        onLinkClicked: panel.sv.showAdvanced = true
        noteLead: StageSelect {
            bare: true
            options: panel.sortOptions
            value: panel.sortKey
            onPicked: function (v) { panel.sortKey = v }
        }
    }

    ColumnLayout {
        id: disk
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.bottomMargin: 16
        visible: panel.isGame && panel.need > 0 && panel.volume !== null
        spacing: 8
        readonly property bool fits: panel.volume ? panel.volume.freeBytes >= panel.need : true
        RowLayout {
            Layout.fillWidth: true
            Text {
                Layout.fillWidth: true
                text: (i18n.language, i18n.t("find_needs")).arg(list.shown.length > 0 ? list.shown[0].sizeStr : "")
                color: Theme.stageMuted
                font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Text {
                text: panel.volume ? (i18n.language, i18n.t("find_free_on"))
                                        .arg(panel.volume.free).arg(panel.volume.name) : ""
                color: disk.fits ? Theme.stageMuted : Theme.warn
                font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 3
            radius: 2
            color: Qt.rgba(1, 1, 1, 0.08)
            clip: true
            Rectangle {
                height: parent.height
                width: parent.width * panel.logic.fitFraction(
                           panel.need, panel.volume ? panel.volume.freeBytes : -1)
                color: disk.fits ? Theme.grn : Theme.warn
                Behavior on width { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
            }
        }
    }

    SearchReleaseList {
        id: list
        Layout.fillWidth: true
        Layout.fillHeight: true
        sv: panel.sv
        logic: panel.logic
        isGame: panel.isGame
        rows: panel.rows
        sortKey: panel.sortKey
    }
}
