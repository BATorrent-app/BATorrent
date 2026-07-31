// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "theme"
import "widgets"

// Full-screen acknowledgement for a manual Refresh. Sibling backdrop + glyph
// so the icon isn't dimmed by the parent's opacity.
Item {
    id: root
    anchors.fill: parent
    z: 9999
    property real amt: 0
    visible: amt > 0.01

    function flash() {
        icon.rotation = 0
        spin.restart()
        anim.restart()
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: root.amt * 0.78
    }
    IconImg {
        id: icon
        anchors.centerIn: parent
        src: "qrc:/icons/refresh.svg"
        tint: Theme.accent
        s: 76
        opacity: root.amt
        scale: Theme.reduceMotion ? 1 : (0.88 + 0.12 * root.amt)
        RotationAnimation on rotation {
            id: spin
            running: false
            from: 0
            to: 360
            duration: 380
            loops: 1
        }
    }
    SequentialAnimation {
        id: anim
        NumberAnimation { target: root; property: "amt"; to: 1.0; duration: 120; easing.type: Easing.OutCubic }
        PauseAnimation { duration: 300 }
        NumberAnimation { target: root; property: "amt"; to: 0.0; duration: 280; easing.type: Easing.OutCubic }
    }
    MouseArea { anchors.fill: parent; enabled: root.amt > 0.01 }
}
