// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Two weeks of play as bars, one per day, today in the accent.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Column {
    id: root
    property var hub
    property var game
    // Read through `game` so a session ending (gamesChanged → a new item) refreshes it.
    readonly property var days: game && hub.api ? hub.api.gameActivity(game.infoHash, 14) : []
    readonly property real maxSecs: Math.max.apply(null, [1].concat(days))
    readonly property real totalSecs: days.reduce(function (a, b) { return a + b }, 0)
    visible: !!game && (game.lastPlayed || 0) > 0
    spacing: 16

    Item {
        width: root.width
        height: head.implicitHeight
        Text {
            id: head
            text: (i18n.language, i18n.t("game_activity"))
            color: Theme.t1
            font.pixelSize: 18
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        Text {
            anchors.left: head.right
            anchors.leftMargin: 12
            anchors.baseline: head.baseline
            text: (i18n.language, i18n.t("game_activity_window")).arg(root.hub.hours(root.totalSecs))
            color: Theme.t4
            font.pixelSize: 13
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
    }

    Row {
        id: bars
        width: root.width
        height: 90
        spacing: 6
        readonly property real barW: root.days.length > 0
            ? (width - spacing * (root.days.length - 1)) / root.days.length : 0
        Repeater {
            model: root.days
            delegate: Item {
                required property var modelData
                required property int index
                width: bars.barW
                height: bars.height
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width
                    height: modelData > 0 ? Math.max(4, Math.round(modelData / root.maxSecs * 86)) : 3
                    radius: 2
                    color: modelData <= 0 ? Qt.rgba(1, 1, 1, 0.08)
                         : (index === root.days.length - 1 ? Theme.accent : Qt.rgba(1, 1, 1, 0.32))
                    Behavior on height { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
                }
                MouseArea { id: tipMa; anchors.fill: parent; hoverEnabled: true }
                ToolTip.visible: tipMa.containsMouse && modelData > 0
                ToolTip.text: root.hub.hours(modelData)
                ToolTip.delay: 300
            }
        }
    }

    Item {
        width: root.width
        height: weeksTxt.implicitHeight
        Text {
            id: weeksTxt
            text: (i18n.language, i18n.t("game_two_weeks_ago"))
            color: Theme.stageDim
            font.pixelSize: 11
            font.family: Theme.fontSans
        }
        Text {
            anchors.right: parent.right
            text: (i18n.language, i18n.t("hub_today"))
            color: Theme.stageDim
            font.pixelSize: 11
            font.family: Theme.fontSans
        }
    }
}
