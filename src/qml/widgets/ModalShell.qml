// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The frame every modal of the redesign shares: the window dimmed behind it,
// one card in the middle, Esc and a click outside to leave, and its place on
// the DialogStack. What goes in the card is the dialog's own. A dialog that
// has to tidy up on the way out (drop a waiting magnet) overrides dismiss().
import QtQuick
import "../theme"
import "."

Item {
    id: shell
    anchors.fill: parent
    z: 200
    visible: opened || anim > 0

    property bool opened: false
    property real anim: 0
    Behavior on anim { NumberAnimation { duration: Theme.durBase; easing.type: Theme.easeOut } }
    onOpenedChanged: {
        anim = opened ? 1 : 0
        if (opened) DialogStack.push(shell); else DialogStack.pop(shell)
    }
    Component.onDestruction: DialogStack.pop(shell)

    // the card: as wide as asked, as tall as its content says (cardHeight),
    // never past the window
    property int cardWidth: 560
    property real cardHeight: 400
    property int cardRadius: 12
    // Esc and a click on the dim leave; a dialog mid-install can say no
    property bool dismissable: true
    readonly property alias card: card
    default property alias cardContent: card.data

    function open() { opened = true }
    function close() { opened = false }
    function dismiss() { close() }

    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: shell.opened && shell.dismissable && DialogStack.topItem === shell
        onActivated: shell.dismiss()
    }

    Rectangle {
        anchors.fill: parent
        opacity: shell.anim
        color: Theme.scrim
        MouseArea {
            anchors.fill: parent
            onClicked: if (shell.dismissable) shell.dismiss()
            onWheel: function (w) { w.accepted = true }
        }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(shell.cardWidth, shell.width - 32)
        height: Math.min(shell.cardHeight, shell.height - 48)
        opacity: shell.anim
        scale: Theme.reduceMotion ? 1 : (0.97 + 0.03 * shell.anim)
        radius: Theme.rad(shell.cardRadius)
        color: Theme.elev
        border.color: Theme.hair
        border.width: 1
        clip: true
        // clicks and wheel on the card stay on the card
        MouseArea { anchors.fill: parent; onWheel: function (w) { w.accepted = true } }
    }
}
