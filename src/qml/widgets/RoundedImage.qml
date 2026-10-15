// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    id: root
    property var sources: []
    property real radius: Theme.rad(6)
    property color fill: Theme.elev
    property int sourceWidth: 0
    property real imageScale: 1

    Rectangle {
        id: box
        anchors.fill: parent
        color: root.fill
        visible: false
        layer.enabled: true
        ChainImage {
            anchors.fill: parent
            sources: root.sources
            sourceSize.width: root.sourceWidth
            scale: root.imageScale
            Behavior on scale { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        }
    }
    Rectangle { id: mask; anchors.fill: parent; radius: root.radius; visible: false; layer.enabled: true }
    MultiEffect { source: box; anchors.fill: parent; maskEnabled: true; maskSource: mask }
}
