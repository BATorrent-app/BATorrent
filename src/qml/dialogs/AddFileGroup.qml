// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One group of the Add download window: game data, language packs, extras,
// a season. Ticked as a whole, or piece by piece through its chips.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: g
    property var group: ({ kind: "", label: "", files: [], chips: [], sizeBytes: 0, season: -1 })
    property var host: null
    property bool first: false

    readonly property int onCount: host.selRev >= 0 ? host.countOn(group.files) : 0
    readonly property bool allOn: onCount === group.files.length
    readonly property bool noneOn: onCount === 0
    readonly property var chipsOn: {
        host.selRev
        var out = []
        for (var i = 0; i < group.chips.length; ++i) {
            var c = group.chips[i]
            out.push(c.inBase || (c.files.length > 0 && host.countOn(c.files) === c.files.length))
        }
        return out
    }
    // the reader's own language first, then what comes in the game data
    readonly property var order: {
        var idx = []
        for (var i = 0; i < group.chips.length; ++i) idx.push(i)
        var want = host ? host.wantLang : "en"
        function rank(c) { return String(c.lang).split("-")[0] === want && !c.inBase ? 0 : c.inBase ? 1 : 2 }
        return idx.sort(function (a, b) { return rank(group.chips[a]) - rank(group.chips[b]) || a - b })
    }
    readonly property int pickable: { var n = 0; for (var i = 0; i < group.chips.length; ++i) if (!group.chips[i].inBase) ++n; return n }
    readonly property int picked: { var n = 0; for (var i = 0; i < group.chips.length; ++i) if (!group.chips[i].inBase && chipsOn[i]) ++n; return n }

    function title() {
        switch (group.kind) {
        case "game": return i18n.t("addw_grp_game")
        case "lang": return i18n.t("addw_grp_lang")
        case "extras": return i18n.t("addw_grp_extras")
        case "setup": return i18n.t("addw_grp_setup")
        case "subs": return i18n.t("addw_grp_subs")
        case "season": return i18n.t("addw_grp_season").arg(group.season)
        }
        return group.label
    }
    function sub() {
        switch (group.kind) {
        case "game": return i18n.t("addw_grp_game_sub").arg(group.files.length)
        case "lang": return pickable === picked ? i18n.t("addw_grp_lang_all").arg(picked).arg(pickable)
                                                 : i18n.t("addw_grp_lang_some").arg(picked).arg(pickable)
        case "extras": return group.chips.map(function (c) { return c.label }).join(", ")
        case "setup": return i18n.t("addw_grp_setup_sub")
        case "season": return i18n.t("addw_grp_episodes").arg(group.chips.length)
        }
        return i18n.t("addw_n_files").arg(group.files.length)
    }

    implicitHeight: inner.implicitHeight + 22
    height: implicitHeight

    Rectangle { visible: !g.first; width: parent.width; height: 1; color: Theme.hairSoft }

    ColumnLayout {
        id: inner
        x: 14; y: 11
        width: parent.width - 28
        spacing: 10
        RowLayout {
            Layout.fillWidth: true
            spacing: 12
            TChk {
                Layout.alignment: Qt.AlignVCenter
                on: g.allOn
                partial: !g.allOn && !g.noneOn
                onToggled: function (v) {
                    var turnOn = g.noneOn
                    g.host.setFiles(g.group.files, turnOn)
                    if (g.group.kind === "game") g.host.gameWarn = !turnOn
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: (i18n.language, g.title())
                    color: g.noneOn ? Theme.t3 : Theme.t1
                    font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, g.host.selRev, g.sub())
                    color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
            }
            Text {
                text: g.host.fmt(g.noneOn ? g.group.sizeBytes : g.host.bytesOn(g.group.files))
                color: g.noneOn ? Theme.t3 : Theme.t1
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
        Flow {
            Layout.fillWidth: true
            Layout.leftMargin: 30
            spacing: 6
            visible: g.group.kind !== "extras" && g.group.chips.length > 1
            Repeater {
                model: g.order
                delegate: Rectangle {
                    id: chip
                    readonly property var c: g.group.chips[modelData]
                    readonly property bool on: g.chipsOn[modelData] === true
                    width: cRow.implicitWidth + 20
                    height: 28
                    radius: 14
                    color: on ? Theme.hover : (cMa.containsMouse ? Qt.rgba(Theme.hover.r, Theme.hover.g, Theme.hover.b, 0.5) : "transparent")
                    border.width: 1
                    border.color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.3) : Theme.hair
                    Row {
                        id: cRow
                        anchors.centerIn: parent
                        spacing: 6
                        Text { text: chip.c.label; color: chip.on ? Theme.t1 : Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                        Text {
                            text: chip.c.inBase ? (i18n.language, i18n.t("addw_in_game_data")) : chip.c.size
                            color: chip.on ? Theme.t2 : Theme.t4
                            font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
                            anchors.baseline: parent.children[0].baseline
                        }
                    }
                    MouseArea {
                        id: cMa
                        anchors.fill: parent
                        enabled: !chip.c.inBase
                        hoverEnabled: true
                        cursorShape: chip.c.inBase ? Qt.ArrowCursor : Qt.PointingHandCursor
                        onClicked: g.host.setFiles(chip.c.files, !chip.on)
                    }
                }
            }
        }
    }
}
