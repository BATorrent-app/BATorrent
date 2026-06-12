// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Indeterminate spinner: accent arc, fast turn (~0.7s) so waits read shorter.
import QtQuick
import "../theme"

Item {
    id: sp
    property int s: 22
    property color tint: Theme.accent
    width: s
    height: s

    Canvas {
        id: cv
        anchors.fill: parent
        onPaint: {
            var c = getContext("2d")
            c.reset()
            var r = width / 2 - 2
            c.strokeStyle = String(sp.tint)
            c.lineWidth = 2.5
            c.lineCap = "round"
            c.beginPath()
            c.arc(width / 2, height / 2, r, 0, Math.PI * 1.45)
            c.stroke()
        }
        Connections { target: sp; function onTintChanged() { cv.requestPaint() } }
    }
    RotationAnimation on rotation {
        from: 0; to: 360
        duration: 700
        loops: Animation.Infinite
        running: sp.visible && sp.opacity > 0
    }
}
