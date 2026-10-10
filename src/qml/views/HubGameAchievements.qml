// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A game's achievements: the newest unlocks first, the rest greyed in Steam's
// order, five at a time until asked for all. A hidden one stays hidden until
// it is earned, as it does on Steam.
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

    readonly property var shown: showAll ? list : list.slice(0, 5)

    function rarity(p) {
        if (p < 0) return ""
        return (i18n.language, i18n.t("game_ach_players")).arg(p < 1 ? p.toFixed(1) : Math.round(p))
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
            visible: root.list.length > 5
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
                readonly property bool secret: modelData.hidden && !modelData.unlocked
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
                            text: row.secret ? (i18n.language, i18n.t("game_ach_hidden")) : row.modelData.name
                            color: Theme.t1
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            visible: text.length > 0
                            text: row.secret ? (i18n.language, i18n.t("game_ach_hidden_desc")) : row.modelData.description
                            color: Theme.stageMuted
                            font.pixelSize: 12
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
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
                            text: root.rarity(row.modelData.percent)
                            color: Theme.stageDim
                            font.pixelSize: 11
                            font.family: Theme.fontSans
                            font.features: Theme.tnum
                        }
                    }
                }
            }
        }
    }
}
