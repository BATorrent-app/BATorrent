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
}
