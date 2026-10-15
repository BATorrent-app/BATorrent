// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"

Column {
    id: root
    property var hub
    property var game
    visible: !!game
    spacing: 10

    // the setup files the game no longer needs, while they are still listed
    property int gen: 0
    readonly property var setup: { var _ = gen; return game && hub.api && hub.api.setupFilesOf ? hub.api.setupFilesOf(game.infoHash) : ({}) }
    Connections {
        target: root.hub.api
        ignoreUnknownSignals: true
        function onGamesChanged() { root.gen++ }
    }

    readonly property string folder: game && hub.api ? hub.api.gameFolder(game.infoHash) : ""
    readonly property string exe: game && hub.api ? hub.api.gameExe(game.infoHash) : ""
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

    Item { width: 1; height: 14; visible: !!root.setup.size }

    Rectangle {
        visible: !!root.setup.size
        width: root.width
        height: setupCol.implicitHeight + 32
        radius: Theme.rad(10)
        color: "transparent"
        border.color: Theme.stageEdge; border.width: 1
        Column {
            id: setupCol
            x: 16; y: 16
            width: parent.width - 32
            spacing: 6
            Text {
                text: (i18n.language, i18n.t("game_setup_files")).arg(root.setup.size || "")
                color: Theme.stageT1
                font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Text {
                width: parent.width
                wrapMode: Text.WordWrap
                text: {
                    i18n.language
                    var p = root.setup.peers || 0
                    return p > 0 ? i18n.t(p === 1 ? "game_setup_seeding_1" : "game_setup_seeding").arg(p)
                                 : i18n.t("game_setup_idle")
                }
                color: Theme.stageMuted
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Row {
                topPadding: 6
                spacing: 8
                Rectangle {
                    width: rmTxt.implicitWidth + 24; height: 32
                    radius: Theme.rad(6)
                    color: rmMa.containsMouse ? "#ffffff" : Theme.stageT1
                    Text { id: rmTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t((root.setup.peers || 0) > 0 ? "game_setup_remove_seeding" : "game_setup_remove")); color: Theme.stageBg; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    MouseArea { id: rmMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.hub.api && root.game) { root.hub.api.removeTorrentByHash(root.game.infoHash, true, false); root.gen++ } }
                }
                Rectangle {
                    width: kpTxt.implicitWidth + 24; height: 32
                    radius: Theme.rad(6)
                    color: kpMa.containsMouse ? Theme.stageHover : "transparent"
                    border.color: Theme.stageEdge; border.width: 1
                    Text { id: kpTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t("game_setup_keep")); color: Theme.stageT1; font.pixelSize: 13; font.family: Theme.fontSans }
                    MouseArea { id: kpMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.hub.api && root.game) root.hub.api.keepSetupFiles(root.game.infoHash) }
                }
            }
        }
    }
}
