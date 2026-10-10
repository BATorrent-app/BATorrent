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
    property int achUnlocked: 0
    property int achTotal: 0
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
            model: {
                var out = [
                    { label: "game_playtime", value: root.played ? root.hub.hours(root.game.playSeconds) : "—" },
                    { label: "game_last_played",
                      value: !root.played ? "—"
                             : (root.game.playing ? i18n.t("hub_playing_now") : root.hub.fmtAgo(root.game.lastPlayed)) }
                ]
                if (root.achTotal > 0 || root.achUnlocked > 0)
                    out.push({ label: "game_achievements",
                               value: root.achTotal > 0 ? root.achUnlocked + " / " + root.achTotal : String(root.achUnlocked),
                               bar: root.achTotal > 0 ? root.achUnlocked / root.achTotal : -1 })
                out.push({ label: "game_on_disk", value: root.game ? (root.hub.fmtSize(root.game.size) || "—") : "—" })
                return out
            }
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
                Rectangle {
                    visible: modelData.bar !== undefined && modelData.bar >= 0
                    width: 120
                    height: 3
                    radius: 2
                    color: Qt.rgba(1, 1, 1, 0.1)
                    Rectangle {
                        width: parent.width * Math.max(0, Math.min(1, modelData.bar || 0))
                        height: parent.height
                        radius: 2
                        color: Theme.accent
                        Behavior on width { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
                    }
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
