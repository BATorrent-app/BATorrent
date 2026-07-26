// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: bat-dialog.css / batorrent-home.css — pequenos ícones SVG tingidos.
import QtQuick
import QtQuick.Window
import QtQuick.Effects

Item {
    id: ico
    property string src
    property color tint: "#9ca3af"
    property int s: 16
    implicitWidth: s
    implicitHeight: s

    // Rasterize at the display's real density. A fixed 2x was fine on Retina but
    // got upscaled — visibly soft — on 3x screens (Windows at 300%, some 4K).
    readonly property int rasterPx: Math.max(2, Math.ceil(Screen.devicePixelRatio)) * ico.s

    Image {
        id: imgSrc
        anchors.fill: parent
        source: ico.src
        sourceSize: Qt.size(ico.rasterPx, ico.rasterPx)
        fillMode: Image.PreserveAspectFit
        visible: false
        layer.enabled: true
    }
    MultiEffect {
        source: imgSrc
        anchors.fill: imgSrc
        colorization: 1.0
        colorizationColor: ico.tint
    }
}
