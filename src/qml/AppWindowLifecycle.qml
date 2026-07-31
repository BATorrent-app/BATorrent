// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Window

Item {
    required property var host

    Timer {
        id: geomSave
        interval: 600; repeat: false
        onTriggered: {
            if (typeof settings === "undefined") return
            if (host.visibility !== Window.Windowed) return
            settings.set("winWidth", host.width)
            settings.set("winHeight", host.height)
        }
    }
    Connections {
        target: host
        function onWidthChanged() { if (host.visibility === Window.Windowed) geomSave.restart() }
        function onHeightChanged() { if (host.visibility === Window.Windowed) geomSave.restart() }
        function onActiveChanged() { if (host.active) clipMagnetDelay.restart() }
    }
    Timer { id: clipMagnetDelay; interval: 250; onTriggered: host.checkClipboardMagnet() }
}
