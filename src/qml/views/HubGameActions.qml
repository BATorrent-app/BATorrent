// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The band under a game's banner: the one thing to do next, how much of it
// you have already done, and the folder and settings at the far end.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: root
    property var hub
    property var game
    signal settingsRequested()
    implicitHeight: 84

    readonly property bool actionable: root.hub.gameStateActionable(root.game)
    readonly property bool played: !!root.game && (root.game.lastPlayed || 0) > 0


    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 48
        anchors.rightMargin: 48
        spacing: 40

        Rectangle {
            id: play
            Layout.preferredHeight: 48
            Layout.preferredWidth: Math.max(150, playRow.implicitWidth + 52)
            radius: 6
            color: root.actionable ? (playMa.containsMouse ? Qt.lighter(Theme.accent, 1.08) : Theme.accent)
                                   : "#2b2b30"
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            scale: playMa.pressed ? Theme.pressScale : 1
            Behavior on scale { NumberAnimation { duration: Theme.durFast } }
            Row {
                id: playRow
                anchors.centerIn: parent
                spacing: 10
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.actionable
                    src: "qrc:/icons/play.svg"
                    tint: "#ffffff"
                    s: 15
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.hub.gameStateLabel(root.game)
                    color: "#ffffff"
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
            MouseArea {
                id: playMa
                anchors.fill: parent
                enabled: root.actionable
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (root.game) root.hub.gamePrimary(root.game)
            }
        }

        Repeater {
            model: [
                { label: "game_playtime", value: root.played ? root.hub.hours(root.game.playSeconds) : "—" },
                { label: "game_last_played",
                  value: !root.played ? "—"
                         : (root.game.playing ? i18n.t("hub_playing_now") : root.hub.fmtAgo(root.game.lastPlayed)) },
                { label: "game_on_disk", value: root.game ? (root.hub.fmtSize(root.game.size) || "—") : "—" }
            ]
            delegate: Column {
                required property var modelData
                spacing: 4
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
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
        }

        Item { Layout.fillWidth: true }

        StageRoundButton {
            size: 40
            icon: "qrc:/icons/folder.svg"
            onClicked: if (root.game) root.hub.gameMenuOpenFolder(root.game.infoHash)
        }
        StageRoundButton {
            size: 40
            icon: "qrc:/icons/settings.svg"
            onClicked: root.settingsRequested()
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Qt.rgba(1, 1, 1, 0.06)
    }
}
