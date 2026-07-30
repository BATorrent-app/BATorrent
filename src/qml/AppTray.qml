// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import Qt.labs.platform as Platform

// System tray leaf. Emits signals instead of reaching into Main; keep id
// trayIcon on the instance so existing win/trayIcon call sites keep working.
Platform.SystemTrayIcon {
    id: root
    visible: true
    icon.source: (typeof themeBridge !== "undefined" && themeBridge.osLight)
                 ? "image://applogo/dark?v=l" : "image://applogo/light?v=d"
    icon.mask: false
    tooltip: "BATorrent"

    signal restoreRequested()
    signal contextRequested(var geometry)

    onActivated: function(reason) {
        if (reason === Platform.SystemTrayIcon.Trigger
            || reason === Platform.SystemTrayIcon.DoubleClick) {
            root.restoreRequested()
        } else if (reason === Platform.SystemTrayIcon.Context) {
            root.contextRequested(root.geometry)
        }
    }
}
