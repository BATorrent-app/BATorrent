// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A title's logo, drawn white only when it is mostly dark; hidden until its tone is known so it never flashes.
import QtQuick
import QtQuick.Effects
import "../theme"

Image {
    id: logo
    property bool dark: false
    property bool toneKnown: false
    readonly property var disco: typeof discovery !== "undefined" ? discovery : null

    fillMode: Image.PreserveAspectFit
    asynchronous: true
    cache: true
    smooth: true
    opacity: status === Image.Ready && toneKnown ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.durSlow } }

    function ask() {
        dark = false
        toneKnown = !disco || String(source).indexOf("http") !== 0
        if (!toneKnown) disco.fetchLogoTone(String(source))
    }
    onSourceChanged: ask()
    Component.onCompleted: ask()
    Connections {
        target: logo.disco
        ignoreUnknownSignals: true
        function onLogoToneReady(url, isDark) {
            if (url !== String(logo.source)) return
            logo.dark = isDark
            logo.toneKnown = true
        }
    }

    layer.enabled: dark
    layer.effect: MultiEffect { brightness: 1.0 }
}
