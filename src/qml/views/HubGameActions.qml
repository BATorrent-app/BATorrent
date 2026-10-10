// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The band under a game's hero: the one thing to do next, then how much of
// it you have already done.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: root
    property var hub
    property var game
    implicitHeight: row.implicitHeight
    height: implicitHeight

    readonly property bool busy: !root.hub.gameStateActionable(root.game)
    readonly property bool played: !!root.game && (root.game.lastPlayed || 0) > 0

    RowLayout {
        id: row
        width: parent.width
        spacing: Theme.sp6

        BtnFlat {
            Layout.preferredWidth: 180
            enabled: !root.busy
            primary: !root.busy
            icon: "qrc:/icons/play.svg"
            text: root.hub.gameStateLabel(root.game)
            onClicked: if (root.game) root.hub.gamePrimary(root.game)
        }

        Repeater {
            model: root.played
                ? [ { label: "game_playtime",
                      value: root.hub.fmtPlaytime(root.game.playSeconds) || "—" },
                    { label: "game_last_played",
                      value: root.game.playing ? i18n.t("hub_playing_now")
                                               : root.hub.fmtAgo(root.game.lastPlayed) } ]
                : []
            delegate: Column {
                required property var modelData
                spacing: 2
                Text {
                    text: (i18n.language, i18n.t(modelData.label))
                    color: Theme.t4
                    font.pixelSize: 11
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    font.capitalization: Font.AllUppercase
                    font.letterSpacing: 0.6
                }
                Text {
                    text: modelData.value
                    color: Theme.t1
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
        }

        Text {
            visible: !root.played
            text: (i18n.language, i18n.t("game_never_played"))
            color: Theme.t3
            font.pixelSize: 13
            font.family: Theme.fontSans
        }

        Item { Layout.fillWidth: true }
    }
}
