// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A game page's right column: its files, then a card for each thing worth
// doing about them, shown only when it applies. The setup files it no longer
// needs; its saves (and a copy of them); the languages and extras the repack
// shipped; a newer build in the game catalogs. Buttons are light or outlined,
// never red: nothing here is the page's one action.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"
import "../widgets/TimeAgo.js" as TimeAgo

Column {
    id: root
    property var hub
    property var game
    visible: !!game
    spacing: 10

    readonly property var api: hub ? hub.api : null
    readonly property string hash: game ? game.infoHash : ""
    // asked again when the library changes (an item removed, a recheck done)
    property int gen: 0
    Connections {
        target: root.api
        ignoreUnknownSignals: true
        function onGamesChanged() { root.gen++ }
    }
    onHashChanged: { gen++; backedUpTo = "" }

    function ask(fn) { var _ = gen; return hash && api && api[fn] ? api[fn](hash) : ({}) }
    readonly property var setup: ask("setupFilesOf")
    readonly property var saves: ask("gameSaves")
    readonly property var extras: ask("gameExtras")
    readonly property var newer: {
        var _ = gen
        return game && typeof search !== "undefined" && search ? search.newerBuild(game.title || "", game.release || "") : ({})
    }
    property string backedUpTo: ""

    readonly property string folder: game && api ? api.gameFolder(game.infoHash) : ""
    readonly property string exe: game && api ? api.gameExe(game.infoHash) : ""
    readonly property var rows: {
        if (!game) return []
        var out = []
        if (game.group) out.push({ label: "game_build", value: game.group })
        if (game.addedTime > 0)
            out.push({ label: "game_installed", value: (i18n.language, i18n.date(game.addedTime)) })
        // Shown the way the system writes it (D:\Downloads\...); the path
        // itself keeps Qt's slashes for opening the folder.
        out.push({ label: "game_folder",
                   value: Qt.platform.os === "windows" ? folder.replace(/\//g, "\\") : folder })
        // No saved exe means launch autodetects one each time.
        out.push({ label: "game_exe",
                   value: exe.length > 0 ? exe.split(/[\\/]/).pop() : (i18n.language, i18n.t("game_exe_auto")) })
        return out
    }
    function names(list, on) {
        return (list || []).filter(function (p) { return p.on === on }).map(function (p) { return p.name })
    }
    readonly property bool partsLeftOut: names(extras.languages, false).length + names(extras.extras, false).length > 0

    Text {
        text: (i18n.language, i18n.t("game_files"))
        color: Theme.t4
        font.pixelSize: 13
        font.weight: Font.DemiBold
        font.family: Theme.fontSans
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 0.6
    }

    Repeater {
        model: root.rows
        delegate: RowLayout {
            required property var modelData
            width: root.width
            spacing: 12
            Text {
                Layout.preferredWidth: 96
                Layout.alignment: Qt.AlignTop
                text: (i18n.language, i18n.t(modelData.label))
                color: Theme.t4
                font.pixelSize: 13
                font.family: Theme.fontSans
            }
            Text {
                Layout.fillWidth: true
                text: modelData.value
                color: Theme.stageInk
                font.pixelSize: 13
                font.family: Theme.fontSans
                font.features: Theme.tnum
                wrapMode: Text.WrapAnywhere
            }
        }
    }

    Item { width: 1; height: 4 }

    // the setup files the game no longer needs, while they are still listed
    SideCard {
        visible: !!root.setup.size
        heading: (i18n.language, i18n.t("game_setup_files")).arg(root.setup.size || "")
        body: {
            i18n.language
            var p = root.setup.peers || 0
            return p > 0 ? i18n.t(p === 1 ? "game_setup_seeding_1" : "game_setup_seeding").arg(p) : i18n.t("game_setup_idle")
        }
        primary: (i18n.language, i18n.t((root.setup.peers || 0) > 0 ? "game_setup_remove_seeding" : "game_setup_remove"))
        secondary: (i18n.language, i18n.t("game_setup_keep"))
        onPrimaryClicked: if (root.api) { root.api.removeTorrentByHash(root.hash, true, false); root.gen++ }
        onSecondaryClicked: if (root.api) root.api.keepSetupFiles(root.hash)
    }

    // where it saves, and a dated copy kept out of the crack's reach
    SideCard {
        visible: !!root.saves.path
        heading: (i18n.language, i18n.t("game_saves"))
        body: {
            i18n.language
            var n = root.saves.count || 0
            var line = i18n.t(n === 1 ? "game_saves_n_1" : "game_saves_n").arg(n)
            if (root.saves.lastMs > 0) line += " · " + i18n.t("game_saves_last").arg(TimeAgo.ago(root.saves.lastMs / 1000, i18n.t).toLowerCase())
            return line
        }
        mono: root.saves.path || ""
        note: root.backedUpTo.length > 0 ? (i18n.language, i18n.t("game_saves_copied")) : ""
        onNoteClicked: if (root.api) root.api.openPath(root.backedUpTo)
        primary: (i18n.language, i18n.t("game_saves_backup"))
        secondary: (i18n.language, i18n.t("ctx_open_folder"))
        onPrimaryClicked: if (root.api) root.backedUpTo = root.api.backUpSaves(root.hash, root.game.title || "")
        onSecondaryClicked: if (root.api) root.api.openPath(root.saves.path)
    }

    // what the repack shipped, and what was left out of the download
    SideCard {
        visible: !!root.extras.languages
        heading: (i18n.language, i18n.t("game_included"))
        body: {
            i18n.language
            var bits = []
            var langs = root.names(root.extras.languages, true)
            if ((root.extras.languages || []).length > 0)
                bits.push(i18n.t("game_included_lang").arg(langs.length > 0 ? langs.join(", ") : i18n.t("game_included_default")))
            if ((root.extras.extras || []).length > 0) {
                var got = root.names(root.extras.extras, true)
                bits.push(i18n.t("game_included_extras").arg(got.length > 0 ? got.join(", ") : i18n.t("game_included_not_dl")))
            }
            return bits.join(" · ")
        }
        secondary: root.partsLeftOut ? (i18n.language, i18n.t("game_included_add")) : ""
        onSecondaryClicked: root.hub.torrentFilesRequested(root.hash)
    }

    // a newer build in the game catalogs
    SideCard {
        visible: !!root.newer.version
        heading: (i18n.language, i18n.t("game_newer"))
        body: (i18n.language, i18n.t("game_newer_body").arg(root.newer.version || "").arg(root.newer.size || ""))
        inlineAction: (i18n.language, i18n.t("game_newer_see"))
        onSecondaryClicked: root.hub.openTitle({ title: root.game.title || "", type: "game" })
    }

    component SideCard: Rectangle {
        id: card
        property string heading
        property string body
        property string mono        // a path, in the code font
        property string note        // a quiet clickable line under the buttons
        property string primary     // light button
        property string secondary   // outlined button
        property string inlineAction // an outlined button on the heading's row instead
        signal primaryClicked()
        signal secondaryClicked()
        signal noteClicked()
        width: root.width
        height: cardCol.implicitHeight + 32
        radius: Theme.rad(10)
        color: "transparent"
        border.color: Theme.stageEdge; border.width: 1
        ColumnLayout {
            id: cardCol
            x: 16; y: 16
            width: parent.width - 32
            spacing: 6
            RowLayout {
                Layout.fillWidth: true
                spacing: 10
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    Text {
                        Layout.fillWidth: true
                        text: card.heading
                        color: Theme.stageT1
                        font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                        elide: Text.ElideRight
                    }
                    Text {
                        Layout.fillWidth: true
                        visible: card.body.length > 0
                        wrapMode: Text.WordWrap
                        text: card.body
                        color: Theme.stageMuted
                        font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                }
                OutlineBtn {
                    visible: card.inlineAction.length > 0
                    Layout.alignment: Qt.AlignVCenter
                    text: card.inlineAction
                    onClicked: card.secondaryClicked()
                }
            }
            Text {
                Layout.fillWidth: true
                visible: card.mono.length > 0
                text: card.mono
                textFormat: Text.PlainText
                color: Theme.stageMuted
                font.pixelSize: 12; font.family: Theme.fontMono
                elide: Text.ElideMiddle
            }
            Row {
                Layout.topMargin: 6
                visible: card.primary.length > 0 || card.secondary.length > 0
                spacing: 8
                LightBtn {
                    visible: card.primary.length > 0
                    stage: true
                    height: 32
                    text: card.primary
                    onClicked: card.primaryClicked()
                }
                OutlineBtn {
                    visible: card.secondary.length > 0 && card.inlineAction.length === 0
                    text: card.secondary
                    onClicked: card.secondaryClicked()
                }
            }
            Text {
                visible: card.note.length > 0
                text: card.note
                color: noteMa.containsMouse ? Theme.stageT1 : Theme.stageMuted
                font.pixelSize: 12; font.family: Theme.fontSans; font.underline: noteMa.containsMouse
                MouseArea { id: noteMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: card.noteClicked() }
            }
        }
    }

    component OutlineBtn: Rectangle {
        id: ob
        property string text
        signal clicked()
        implicitWidth: obTxt.implicitWidth + 24; implicitHeight: 32
        width: implicitWidth; height: implicitHeight
        radius: Theme.rad(6)
        color: obMa.containsMouse ? Theme.stageHover : "transparent"
        border.color: Theme.stageEdge; border.width: 1
        Text { id: obTxt; anchors.centerIn: parent; text: ob.text; color: Theme.stageT1; font.pixelSize: 13; font.family: Theme.fontSans }
        MouseArea { id: obMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: ob.clicked() }
    }
}
