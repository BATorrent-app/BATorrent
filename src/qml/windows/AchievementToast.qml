// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The unlock card over the game: its own window, never taking focus or clicks, one notice at a time.
import QtQuick
import QtQuick.Window
import "../theme"
import "../widgets"

Window {
    id: toast
    width: 380
    height: 104
    color: "transparent"
    // Not a Tool window on macOS: that is an NSPanel, hidden whenever another app (the game) is in front.
    flags: (Qt.platform.os === "osx" ? Qt.Window : Qt.Tool) | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
           | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput
    visible: false

    property var queue: []
    property var current: null
    readonly property bool rare: !!current && current.rare === true

    function push(game, name, icon, percentLabel, rare) {
        var q = queue
        q.push({ game: game, name: name, icon: icon, percentLabel: percentLabel || "", rare: rare === true })
        queue = q
        if (!current) next()
    }
    function next() {
        if (queue.length === 0) { current = null; card.shown = false; hideTimer.restart(); return }
        var q = queue
        current = q.shift()
        queue = q
        // The corner of the monitor the game is on, asked for each card: the
        // Screen attached property spans every monitor and put the card off
        // screen on a two-monitor desk.
        var area = typeof themeBridge !== "undefined" ? themeBridge.noticeArea()
                                                       : Qt.rect(0, 0, Screen.width, Screen.height)
        x = area.x + area.width - width - 24
        y = area.y + area.height - height - 24
        visible = true
        card.shown = true
        holdTimer.restart()
    }

    Timer { id: holdTimer; interval: 5000; onTriggered: { card.shown = false; gapTimer.restart() } }
    Timer { id: gapTimer; interval: 320; onTriggered: toast.next() }
    Timer { id: hideTimer; interval: 320; onTriggered: if (!toast.current) toast.visible = false }

    Rectangle {
        id: card
        property bool shown: false
        width: parent.width
        height: parent.height
        radius: Theme.rad(10)
        color: "#141416"
        border.width: 1
        border.color: toast.rare ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.55)
                                 : Qt.rgba(1, 1, 1, 0.09)
        opacity: shown ? 1 : 0
        x: shown ? 0 : 28
        Behavior on opacity { NumberAnimation { duration: card.shown ? Theme.durSlow : Theme.durBase } }
        Behavior on x { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }

        Rectangle {
            visible: toast.rare
            x: 0; y: 10
            width: 3
            height: parent.height - 20
            radius: Theme.rad(2)
            color: Theme.accent
        }

        Row {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 14

            RoundedImage {
                width: 72
                height: 72
                radius: Theme.rad(8)
                fill: "#1d1d20"
                sourceWidth: 144
                sources: toast.current && toast.current.icon ? [toast.current.icon] : []
            }

            Column {
                width: parent.width - 72 - 14
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                Row {
                    spacing: 8
                    Text {
                        text: (i18n.language, i18n.t("toast_achievement_kicker"))
                        color: toast.rare ? Theme.accentText : Theme.t4
                        font.pixelSize: 11
                        font.weight: Font.DemiBold
                        font.letterSpacing: 1.1
                        font.capitalization: Font.AllUppercase
                        font.family: Theme.fontSans
                    }
                    Rectangle {
                        visible: toast.rare
                        anchors.verticalCenter: parent.verticalCenter
                        width: rareTxt.implicitWidth + 12
                        height: 16
                        radius: Theme.rad(8)
                        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.16)
                        Text {
                            id: rareTxt
                            anchors.centerIn: parent
                            text: (i18n.language, i18n.t("toast_achievement_rare"))
                            color: Theme.accentText
                            font.pixelSize: 10
                            font.weight: Font.DemiBold
                            font.family: Theme.fontSans
                        }
                    }
                }
                Text {
                    width: parent.width
                    text: toast.current ? toast.current.name : ""
                    color: Theme.t1
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    text: {
                        if (!toast.current) return ""
                        var parts = []
                        if (toast.current.game) parts.push(toast.current.game)
                        if (toast.current.percentLabel)
                            parts.push((i18n.language, i18n.t("game_ach_players")).arg(toast.current.percentLabel))
                        return parts.join("  ·  ")
                    }
                    color: Theme.t3
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                    elide: Text.ElideRight
                }
            }
        }
    }
}
