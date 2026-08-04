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

    readonly property int rasterPx: Math.max(2, Math.ceil(Screen.devicePixelRatio)) * ico.s
    // Dev only: redirect qrc:/icons/x.svg to the working tree so an SVG edit
    // lands like a QML edit. themeBridge.devIconDir is empty in a release build,
    // and the cache buster is what makes a *re-saved* file actually reload —
    // QQuickImage keys its cache on the URL alone.
    readonly property string devIcons: (typeof themeBridge !== "undefined" && themeBridge.devIconDir)
                                       ? themeBridge.devIconDir : ""
    readonly property int iconEpoch: (typeof themeBridge !== "undefined" && ico.devIcons.length > 0)
                                     ? themeBridge.iconEpoch : 0
    readonly property string resolvedSrc: {
        if (ico.devIcons.length === 0 || ico.src.indexOf("qrc:/icons/") !== 0) return ico.src
        return ico.devIcons + ico.src.substring("qrc:/icons".length) + "?v=" + ico.iconEpoch
    }
    // Software RHI + MultiEffect = blank icons on the gray-screen path.
    readonly property bool soft: typeof themeBridge !== "undefined" && themeBridge.softwareRenderer

    Image {
        id: imgSrc
        anchors.fill: parent
        source: ico.resolvedSrc
        sourceSize: Qt.size(ico.rasterPx, ico.rasterPx)
        fillMode: Image.PreserveAspectFit
        visible: ico.soft
        layer.enabled: !ico.soft
    }
    MultiEffect {
        visible: !ico.soft
        source: imgSrc
        anchors.fill: imgSrc
        colorization: 1.0
        colorizationColor: ico.tint
    }
}
