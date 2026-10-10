// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Column {
    id: root
    property var hub
    property var game
    visible: !!game
    spacing: Theme.sp3

    readonly property string folder: game && hub.api ? hub.api.gameFolder(game.infoHash) : ""
    readonly property string exe: game && hub.api ? hub.api.gameExe(game.infoHash) : ""

    Text {
        text: (i18n.language, i18n.t("game_files"))
        color: Theme.t1
        font.pixelSize: 17
        font.weight: Font.Bold
        font.family: Theme.fontSans
    }

    Repeater {
        model: [
            { label: "game_folder", value: root.folder },
            // No saved exe means launch autodetects one each time.
            { label: "game_exe", value: root.exe.length > 0 ? root.exe : (i18n.language, i18n.t("game_exe_auto")) },
            { label: "game_size", value: root.game ? root.hub.fmtSize(root.game.size) : "" }
        ]
        delegate: RowLayout {
            required property var modelData
            width: root.width
            spacing: Theme.sp5
            Text {
                Layout.preferredWidth: 110
                text: (i18n.language, i18n.t(modelData.label))
                color: Theme.t4
                font.pixelSize: 12
                font.family: Theme.fontSans
            }
            Text {
                Layout.fillWidth: true
                text: modelData.value
                color: Theme.t2
                font.pixelSize: 12
                font.family: Theme.fontSans
                font.features: Theme.tnum
                elide: Text.ElideMiddle
            }
        }
    }

    Row {
        spacing: Theme.sp3
        BtnFlat {
            sm: true
            icon: "qrc:/icons/open.svg"
            text: (i18n.language, i18n.t("hub_open_folder"))
            onClicked: if (root.game) root.hub.gameMenuOpenFolder(root.game.infoHash)
        }
        BtnFlat {
            sm: true
            icon: "qrc:/icons/settings.svg"
            text: (i18n.language, i18n.t("hub_set_exe"))
            onClicked: if (root.game) root.hub.openExePicker(root.game.infoHash, false)
        }
    }
}
