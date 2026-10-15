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
    signal moreRequested(var anchor)   // the split button's chevron: drop the game menu under it
    implicitHeight: 84

    // One half of the split button. Each half paints its own fill, round on the
    // outside and square at the seam, so a hover lights only its half (clip
    // would not round the corners).
    component Half: Item {
        id: half
        property color fill
        property bool leftSide: true
        Rectangle {
            anchors.fill: parent
            radius: Theme.rad(6)
            color: half.fill
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
        }
        Rectangle {
            x: half.leftSide ? half.width - 6 : 0
            width: 6
            height: half.height
            color: half.fill
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
        }
    }

    readonly property bool actionable: root.hub.gameStateActionable(root.game)
    readonly property bool played: !!root.game && (root.game.lastPlayed || 0) > 0


    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 48
        anchors.rightMargin: 48
        spacing: 40

        // A split button: the one next step, and behind the chevron every other
        // way in (install with a Defender exclusion, pick the exe, open the
        // folder). The chevron stays live when the main action is not, so a
        // game stuck on a state still has a way out.
        Item {
            id: split
            readonly property real playW: Math.max(150, playRow.implicitWidth + 52)
            readonly property real chevronW: 42
            readonly property color base: root.actionable ? Theme.accent : Theme.stageIdleFill
            readonly property color lit: root.actionable ? Qt.lighter(Theme.accent, 1.08) : Theme.stageIdleFillHover
            readonly property bool menuOpen: !!root.hub.gameMenu && root.hub.gameMenu.opened
            Layout.preferredHeight: 48
            Layout.preferredWidth: playW + chevronW
            scale: playMa.pressed || moreMa.pressed ? Theme.pressScale : 1
            Behavior on scale { NumberAnimation { duration: Theme.durFast } }

            Half {
                width: split.playW
                height: split.height
                fill: playMa.containsMouse && root.actionable ? split.lit : split.base
            }
            Half {
                x: split.playW
                width: split.chevronW
                height: split.height
                leftSide: false
                fill: moreMa.containsMouse || split.menuOpen ? split.lit : split.base
            }
            Rectangle {   // the seam
                x: split.playW
                y: 12
                width: 1
                height: split.height - 24
                color: Qt.rgba(1, 1, 1, 0.28)
            }

            Row {
                id: playRow
                x: (split.playW - width) / 2
                anchors.verticalCenter: parent.verticalCenter
                spacing: 10
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.actionable
                    src: "qrc:/icons/play.svg"
                    tint: Theme.inkOnAccent
                    s: 15
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.hub.gameStateLabel(root.game)
                    color: Theme.inkOnAccent
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
            IconImg {
                x: split.playW + (split.chevronW - width) / 2
                anchors.verticalCenter: parent.verticalCenter
                src: "qrc:/icons/chevron.svg"
                tint: Theme.inkOnAccent
                s: 16
                rotation: split.menuOpen ? 180 : 0
                Behavior on rotation { NumberAnimation { duration: Theme.durBase; easing.type: Theme.easeOut } }
            }

            MouseArea {
                id: playMa
                width: split.playW
                height: split.height
                enabled: root.actionable
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (root.game) root.hub.gamePrimary(root.game)
            }
            MouseArea {
                id: moreMa
                x: split.playW
                width: split.chevronW
                height: split.height
                enabled: !!root.game
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.moreRequested(split)
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
                    radius: Theme.rad(2)
                    color: Qt.rgba(1, 1, 1, 0.1)
                    Rectangle {
                        width: parent.width * Math.max(0, Math.min(1, modelData.bar || 0))
                        height: parent.height
                        radius: Theme.rad(2)
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
