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
    // Read through `game` so a session ending (gamesChanged → a new item) refreshes it.
    readonly property var sessions: game && hub.api ? hub.api.gameSessions(game.infoHash, 6) : []
    visible: sessions.length > 0
    spacing: Theme.sp3

    Text {
        text: (i18n.language, i18n.t("game_activity"))
        color: Theme.t1
        font.pixelSize: 17
        font.weight: Font.Bold
        font.family: Theme.fontSans
    }

    Repeater {
        model: root.sessions
        delegate: RowLayout {
            required property var modelData
            width: Math.min(root.width, 420)
            spacing: Theme.sp5
            Text {
                Layout.fillWidth: true
                text: root.hub.fmtAgo(modelData.startMs)
                color: Theme.t2
                font.pixelSize: 13
                font.family: Theme.fontSans
            }
            Text {
                text: root.hub.fmtPlaytime(modelData.secs)
                color: Theme.t1
                font.pixelSize: 13
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
        }
    }
}
