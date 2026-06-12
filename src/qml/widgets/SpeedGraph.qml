// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Filled dual-curve speed graph (download accent / upload amber), same scaling
// rules as the Main graph panel: auto-scale to the visible peak, no floor.
import QtQuick
import QtQuick.Shapes
import "../theme"

Item {
    id: g
    property var dl: []
    property var ul: []
    readonly property int slots: 60

    readonly property int scaledMax: {
        var m = 1
        for (var i = 0; i < dl.length; ++i) if (dl[i] > m) m = dl[i]
        for (var j = 0; j < ul.length; ++j) if (ul[j] > m) m = ul[j]
        return Math.round(m * 1.15)
    }

    function scaleText(b) {
        if (b >= 1024 * 1024) return (b / (1024 * 1024)).toFixed(1) + " MB/s"
        return Math.round(b / 1024) + " KB/s"
    }
    function areaPath(arr, h) {
        if (!arr || arr.length === 0) return ""
        var n = arr.length
        var step = shape.width / (slots - 1)
        var off = (slots - n) * step
        function yAt(v) { return h - (v / g.scaledMax) * (h - 2) }
        var s = "M " + off.toFixed(1) + "," + h.toFixed(1)
        s += " L " + off.toFixed(1) + "," + yAt(arr[0]).toFixed(1)
        for (var i = 1; i < n; ++i) {
            var px = off + (i - 1) * step, py = yAt(arr[i - 1])
            var x = off + i * step, y = yAt(arr[i])
            var cx = (px + x) / 2
            s += " C " + cx.toFixed(1) + "," + py.toFixed(1) + " " + cx.toFixed(1) + "," + y.toFixed(1) + " " + x.toFixed(1) + "," + y.toFixed(1)
        }
        s += " L " + (off + (n - 1) * step).toFixed(1) + "," + h.toFixed(1) + " Z"
        return s
    }
    function linePath(arr, h) {
        if (!arr || arr.length === 0) return ""
        var n = arr.length
        var step = shape.width / (slots - 1)
        var off = (slots - n) * step
        function yAt(v) { return h - (v / g.scaledMax) * (h - 2) }
        var s = "M " + off.toFixed(1) + "," + yAt(arr[0]).toFixed(1)
        for (var i = 1; i < n; ++i) {
            var px = off + (i - 1) * step, py = yAt(arr[i - 1])
            var x = off + i * step, y = yAt(arr[i])
            var cx = (px + x) / 2
            s += " C " + cx.toFixed(1) + "," + py.toFixed(1) + " " + cx.toFixed(1) + "," + y.toFixed(1) + " " + x.toFixed(1) + "," + y.toFixed(1)
        }
        return s
    }

    Text {
        anchors.top: parent.top
        anchors.left: parent.left
        text: g.scaleText(g.scaledMax)
        color: Theme.t4
        font.pixelSize: 10
        font.family: Theme.fontSans
        z: 1
    }

    Shape {
        id: shape
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.bottomMargin: 2
        preferredRendererType: Shape.CurveRenderer
        antialiasing: true

        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            fillGradient: LinearGradient {
                x1: 0; y1: 0; x2: 0; y2: shape.height
                GradientStop { position: 0.0; color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.09) }
                GradientStop { position: 1.0; color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.0) }
            }
            PathSvg { path: g.areaPath(g.ul, shape.height) }
        }
        ShapePath {
            strokeColor: "transparent"
            strokeWidth: 0
            fillGradient: LinearGradient {
                x1: 0; y1: 0; x2: 0; y2: shape.height
                GradientStop { position: 0.0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.09) }
                GradientStop { position: 1.0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.0) }
            }
            PathSvg { path: g.areaPath(g.dl, shape.height) }
        }
        ShapePath {
            strokeColor: Theme.amber
            strokeWidth: 1.5
            fillColor: "transparent"
            PathSvg { path: g.linePath(g.ul, shape.height) }
        }
        ShapePath {
            strokeColor: Theme.accent
            strokeWidth: 1.5
            fillColor: "transparent"
            PathSvg { path: g.linePath(g.dl, shape.height) }
        }
    }
}
