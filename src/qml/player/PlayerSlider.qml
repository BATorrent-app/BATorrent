// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The seek bar: a 4px track, what is on disk ahead of the playhead in a
// lighter grey, the watched part in the accent, a white thumb, and the
// opening and ending as brackets over the track (the ending only has its
// start: it runs to the end).
import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import "../theme"

Slider {
    id: sl
    property real buffered: 0       // 0..1 end of the run on disk
    property real bufferedFrom: 0   // 0..1 where that run starts (the playback position)
    property var marks: []          // 0..1 chapter starts without a kind: a notch in the track
    property var spans: []          // [{ from, to, open }] opening/ending, 0..1
    readonly property bool active: sl.pressed || sl.hovered
    implicitHeight: 18
    background: Item {
        x: sl.leftPadding; y: sl.topPadding + sl.availableHeight / 2 - height / 2
        width: sl.availableWidth
        height: 4
        Rectangle {
            anchors.fill: parent
            radius: 2
            color: Qt.rgba(1, 1, 1, 0.22)
        }
        Rectangle {
            x: Math.max(0, Math.min(1, sl.bufferedFrom)) * parent.width
            width: Math.max(0, Math.min(1, sl.buffered) - Math.max(0, sl.bufferedFrom)) * parent.width
            height: parent.height; radius: 2; color: Qt.rgba(1, 1, 1, 0.35)
        }
        Repeater {
            model: sl.marks
            Rectangle {
                required property real modelData
                x: modelData * parent.width - 1
                width: 2; height: parent.height
                color: Qt.rgba(0, 0, 0, 0.8)
            }
        }
        Rectangle {
            width: sl.visualPosition * parent.width; height: parent.height; radius: 2
            color: Theme.accent
        }
        // opening: both edges; ending: where it starts
        Repeater {
            model: sl.spans
            Item {
                required property var modelData
                x: modelData.from * parent.width
                width: Math.max(4, (modelData.to - modelData.from) * parent.width)
                y: -3
                height: 10
                Rectangle { width: 2; height: parent.height; color: Qt.rgba(1, 1, 1, 0.7) }
                Rectangle { visible: modelData.open; anchors.right: parent.right; width: 2; height: parent.height; color: Qt.rgba(1, 1, 1, 0.7) }
            }
        }
    }
    handle: Item {
        implicitWidth: sl.active ? 16 : 14
        implicitHeight: implicitWidth
        x: sl.leftPadding + sl.visualPosition * (sl.availableWidth - width)
        y: sl.topPadding + sl.availableHeight / 2 - height / 2
        Behavior on implicitWidth { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutBack } }
        MultiEffect {
            source: disc
            anchors.fill: disc
            shadowEnabled: true
            shadowBlur: 1.0
            blurMax: 10
            shadowColor: "#80000000"
        }
        Rectangle { id: disc; anchors.fill: parent; radius: width / 2; color: "#ffffff" }
    }
}
