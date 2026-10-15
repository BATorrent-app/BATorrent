// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// An achievement over the game: its own window in the screen's bottom right,
// never taking focus or clicks, five seconds, no sound of its own. It says
// what was done (the achievement's description, not the game's name: the
// person is in it) and how rare it is, on a line that never cuts. Several at
// once become one card with a count.
import QtQuick
import QtQuick.Window
import "../theme"
import "../widgets"

Window {
    id: toast
    width: 360
    height: card.implicitHeight
    color: "transparent"
    // Not a Tool window on macOS: that is an NSPanel, hidden whenever another app (the game) is in front.
    flags: (Qt.platform.os === "osx" ? Qt.Window : Qt.Tool) | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
           | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput
    visible: false

    // the card on screen, and how many more came in with it
    property var current: null
    property int extra: 0
    // a: { name, icon, percentLabel, rare, description, unlocked, total }
    function push(a) {
        if (current && card.shown) {
            // in a burst: the first one stays, the count grows, the clock restarts
            extra++
            holdTimer.restart()
            return
        }
        current = a
        extra = 0
        var area = typeof themeBridge !== "undefined" ? themeBridge.noticeArea()
                                                       : Qt.rect(0, 0, Screen.width, Screen.height)
        x = area.x + area.width - width - 18
        y = area.y + area.height - height - 18
        visible = true
        card.shown = true
        holdTimer.restart()
    }

    Timer { id: holdTimer; interval: 5000; onTriggered: { card.shown = false; hideTimer.restart() } }
    Timer { id: hideTimer; interval: Theme.durSlow; onTriggered: { toast.visible = false; toast.current = null } }

    Rectangle {
        id: card
        property bool shown: false
        width: parent.width
        implicitHeight: content.implicitHeight + 32
        radius: Theme.rad(10)
        color: Theme.elev
        border.width: 1
        border.color: Theme.hair
        clip: true
        opacity: shown ? 1 : 0
        x: shown ? 0 : 24
        Behavior on opacity { NumberAnimation { duration: card.shown ? Theme.durSlow : Theme.durBase } }
        Behavior on x { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }

        Row {
            id: content
            x: 16; y: 16
            width: parent.width - 32
            spacing: 14
            Item {
                width: 64; height: 64
                RoundedImage {
                    anchors.fill: parent
                    radius: Theme.rad(8)
                    fill: Theme.hover
                    sourceWidth: 128
                    sources: toast.current && toast.current.icon ? [toast.current.icon] : []
                }
                Rectangle {
                    visible: toast.extra > 0
                    anchors.right: parent.right; anchors.bottom: parent.bottom
                    anchors.rightMargin: -4; anchors.bottomMargin: -4
                    width: plus.implicitWidth + 10; height: 20
                    radius: 10
                    color: Theme.t1
                    Text { id: plus; anchors.centerIn: parent; text: "+" + toast.extra; color: Theme.bg; font.pixelSize: 12; font.weight: Font.Bold; font.family: Theme.fontSans }
                }
            }
            Column {
                width: parent.width - 64 - 14
                spacing: 3
                Text {
                    text: (i18n.language, toast.extra > 0 ? i18n.t("ach_unlocked_n").arg(toast.extra + 1) : i18n.t("toast_achievement_kicker"))
                    color: Theme.t3
                    font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9
                    font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                }
                Text {
                    width: parent.width
                    text: toast.current ? toast.current.name : ""
                    color: Theme.t1; font.pixelSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    visible: text !== ""
                    text: (i18n.language, !toast.current ? "" : toast.extra > 0 ? i18n.t("ach_and_more").arg(toast.extra)
                                                                   : (toast.current.description || ""))
                    color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
                    wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                }
                // rarity and progress: one line, never cut
                Text {
                    width: parent.width
                    visible: !!toast.current
                    textFormat: Text.StyledText
                    font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                    color: Theme.t3
                    text: {
                        i18n.language
                        var c = toast.current
                        if (!c) return ""
                        var bits = []
                        bits.push("<font color='" + (c.rare ? Theme.t1 : Theme.t3) + "'><b>" + i18n.t(c.rare ? "toast_achievement_rare" : "ach_common") + "</b></font>")
                        if (c.percentLabel) bits.push(i18n.t("game_ach_players").arg(c.percentLabel))
                        if (c.total > 0) bits.push(i18n.t("ach_progress").arg(c.unlocked).arg(c.total))
                        return bits.join("  ·  ")
                    }
                }
            }
        }
        // how far along the game's list, as a 2px line at the foot
        Rectangle {
            anchors.bottom: parent.bottom
            height: 2
            width: toast.current && toast.current.total > 0 ? parent.width * toast.current.unlocked / toast.current.total : 0
            color: Theme.t1
        }
    }
}
