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
    // room for the axis labels; the plot starts after it
    readonly property int gutter: 42
    readonly property int divisions: 4

    readonly property real peak: {
        var m = 1
        for (var i = 0; i < dl.length; ++i) if (dl[i] > m) m = dl[i]
        for (var j = 0; j < ul.length; ++j) if (ul[j] > m) m = ul[j]
        return m
    }

    // Round the STEP, not the max: peak×1.15 gave arbitrary ceilings ("7 KB/s")
    // that no gridline could label sensibly. Snapping the step to 1-2-5×10ⁿ makes
    // every tick a number a person reads without decoding — 0 5 10 15 20.
    readonly property real unitBytes: peak >= 1024 * 1024 ? 1024 * 1024 : 1024
    readonly property real step: {
        var raw = (peak / unitBytes) / divisions
        if (raw <= 0) return 1
        var e = Math.pow(10, Math.floor(Math.log(raw) / Math.LN10))
        var f = raw / e
        return (f <= 1 ? 1 : f <= 2 ? 2 : f <= 5 ? 5 : 10) * e
    }
    readonly property real scaledMax: step * divisions * unitBytes

    // unit on the top tick only — repeating it down the axis is redundant ink
    function tickText(v, withUnit) {
        var u = v / g.unitBytes
        var s = (g.step < 1 ? u.toFixed(1) : String(Math.round(u)))
        return withUnit ? s + (g.unitBytes > 1024 ? " MB/s" : " KB/s") : s
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

    // Y axis: recessive gridlines with the value on the left. Same geometry as
    // the Shape below, so a curve touching a line really is at that value.
    Item {
        id: axis
        anchors.fill: parent
        anchors.leftMargin: g.gutter
        anchors.topMargin: 16
        anchors.bottomMargin: 2

        Repeater {
            model: g.divisions + 1
            delegate: Item {
                required property int index
                readonly property real value: g.step * index * g.unitBytes
                // mirrors yAt() in the path builders
                y: axis.height - (value / g.scaledMax) * (axis.height - 2)
                width: axis.width
                height: 1

                Rectangle {
                    anchors.fill: parent
                    color: index === 0 ? Theme.hair : Theme.hairSoft
                }
                Text {
                    anchors.right: parent.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    text: g.tickText(parent.value, index === g.divisions)
                    color: Theme.t4
                    font.pixelSize: 9
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
        }
    }

    Shape {
        id: shape
        anchors.fill: parent
        anchors.leftMargin: g.gutter
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
