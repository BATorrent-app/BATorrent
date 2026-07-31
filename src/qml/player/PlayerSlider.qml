// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import "theme"

Slider {
    id: sl
    property real buffered: 0   // 0..1 downloaded-from-start, drawn dim behind the fill
    readonly property bool active: sl.pressed || sl.hovered
    implicitHeight: 18
    background: Rectangle {
        x: sl.leftPadding; y: sl.topPadding + sl.availableHeight / 2 - height / 2
        width: sl.availableWidth
        height: sl.active ? 7 : 5; radius: height / 2
        color: "#26ffffff"
        Behavior on height { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Rectangle { width: Math.max(0, Math.min(1, sl.buffered)) * parent.width; height: parent.height; radius: parent.radius; color: "#4dffffff" }
        Rectangle {
            width: sl.visualPosition * parent.width; height: parent.height; radius: parent.radius
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.accent }
                GradientStop { position: 1.0; color: "#ff2e3d" }
            }
            layer.enabled: sl.active
            layer.effect: MultiEffect { blurEnabled: true; blur: 0.6; blurMax: 8 }
        }
    }
    handle: Item {
        implicitWidth: sl.active ? 15 : 0
        implicitHeight: implicitWidth
        x: sl.leftPadding + sl.visualPosition * (sl.availableWidth - width)
        y: sl.topPadding + sl.availableHeight / 2 - height / 2
        Behavior on implicitWidth { NumberAnimation { duration: 130; easing.type: Easing.OutBack } }
        MultiEffect {
            source: disc
            anchors.fill: disc
            shadowEnabled: true
            shadowBlur: 1.0
            blurMax: 10
            shadowColor: "#80000000"
            visible: parent.width > 0
        }
        Rectangle { id: disc; anchors.fill: parent; radius: width / 2; color: "#ffffff" }
    }
}
