// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A game's achievements: the newest unlocks first, the rest greyed in Steam's
// order, five at a time until asked for all. Locked secrets are one line at
// the end, as on Steam: a row each would only repeat that they are secret.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Column {
    id: root
    property var hub
    property var list: []
    property int unlocked: 0
    property int total: 0
    property bool showAll: false
    visible: list.length > 0
    spacing: 16

    readonly property var readable: list.filter(function (a) { return a.unlocked || !a.hidden })
    readonly property int secretsLeft: list.length - readable.length
    readonly property var shown: showAll ? readable : readable.slice(0, 5)
    readonly property bool showSecrets: secretsLeft > 0 && (showAll || readable.length <= 5)

    function rarity(label) {
        return label ? (i18n.language, i18n.t("game_ach_players")).arg(label) : ""
    }

    Item {
        width: root.width
        height: head.implicitHeight
        Text {
            id: head
            text: (i18n.language, i18n.t("game_achievements"))
            color: Theme.t1
            font.pixelSize: 18
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        Text {
            anchors.left: head.right
            anchors.leftMargin: 12
            anchors.baseline: head.baseline
            text: root.total > 0 ? (i18n.language, i18n.t("game_ach_of")).arg(root.unlocked).arg(root.total)
                                 : (i18n.language, i18n.t("game_ach_unlocked_n")).arg(root.unlocked)
            color: Theme.t4
            font.pixelSize: 13
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            anchors.right: parent.right
            anchors.baseline: head.baseline
            visible: root.readable.length > 5
            text: root.showAll ? (i18n.language, i18n.t("game_ach_less"))
                               : (i18n.language, i18n.t("game_ach_all")).arg(root.list.length)
            color: moreMa.containsMouse ? Theme.t1 : Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontSans
            MouseArea {
                id: moreMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.showAll = !root.showAll
            }
        }
    }

    Column {
        width: root.width
        spacing: 2
        Repeater {
            model: root.shown
            delegate: Rectangle {
                id: row
                required property var modelData
                width: root.width
                height: 68
                radius: 6
                color: rowMa.containsMouse ? Theme.stageHover : "transparent"
                opacity: modelData.unlocked ? 1 : 0.5
                MouseArea { id: rowMa; anchors.fill: parent; hoverEnabled: true }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    spacing: 16
                    Rectangle {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        radius: 6
                        color: "#1a1a1d"
                        clip: true
                        ChainImage {
                            anchors.fill: parent
                            sourceSize.width: 96
                            sources: [row.modelData.unlocked ? row.modelData.icon : row.modelData.iconGray]
                                     .filter(function (u) { return !!u })
                        }
                    }
                    Column {
                        Layout.fillWidth: true
                        spacing: 3
                        Text {
                            width: parent.width
                            text: row.modelData.name
                            color: Theme.t1
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            visible: text.length > 0
                            text: row.modelData.description
                            color: Theme.stageMuted
                            font.pixelSize: 12
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                        Row {
                            // A counting achievement shows how far along it is.
                            visible: !row.modelData.unlocked && (row.modelData.progressMax || 0) > 0
                            spacing: 10
                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 140
                                height: 3
                                radius: 2
                                color: Qt.rgba(1, 1, 1, 0.12)
                                Rectangle {
                                    width: parent.width * Math.min(1, (row.modelData.progress || 0)
                                                                      / Math.max(1, row.modelData.progressMax || 1))
                                    height: parent.height
                                    radius: 2
                                    color: Theme.t2
                                }
                            }
                            Text {
                                text: (row.modelData.progress || 0) + " / " + (row.modelData.progressMax || 0)
                                color: Theme.stageMuted
                                font.pixelSize: 11
                                font.family: Theme.fontSans
                                font.features: Theme.tnum
                            }
                        }
                    }
                    Column {
                        spacing: 3
                        Text {
                            anchors.right: parent.right
                            text: !row.modelData.unlocked ? (i18n.language, i18n.t("game_ach_locked"))
                                : row.modelData.unlockedAt > 0 ? root.hub.fmtAgo(row.modelData.unlockedAt * 1000)
                                : (i18n.language, i18n.t("game_ach_unlocked"))
                            color: row.modelData.unlocked ? Theme.t2 : Theme.stageDim
                            font.pixelSize: 12
                            font.family: Theme.fontSans
                        }
                        Text {
                            anchors.right: parent.right
                            visible: text.length > 0
                            text: root.rarity(row.modelData.percentLabel)
                            color: Theme.stageDim
                            font.pixelSize: 11
                            font.family: Theme.fontSans
                            font.features: Theme.tnum
                        }
                    }
                }
            }
        }

        Rectangle {
            visible: root.showSecrets
            width: root.width
            height: 68
            radius: 6
            color: "transparent"
            opacity: 0.5
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 16
                Rectangle {
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    radius: 6
                    color: "#1a1a1d"
                    Text {
                        anchors.centerIn: parent
                        text: "+" + root.secretsLeft
                        color: Theme.t2
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                        font.family: Theme.fontSans
                        font.features: Theme.tnum
                    }
                }
                Column {
                    Layout.fillWidth: true
                    spacing: 3
                    Text {
                        text: (i18n.language, i18n.t("game_ach_secrets_left")).arg(root.secretsLeft)
                        color: Theme.t1
                        font.pixelSize: 14
                        font.weight: Font.Medium
                        font.family: Theme.fontSans
                    }
                    Text {
                        text: (i18n.language, i18n.t("game_ach_hidden_desc"))
                        color: Theme.stageMuted
                        font.pixelSize: 12
                        font.family: Theme.fontSans
                    }
                }
            }
        }
    }
}
