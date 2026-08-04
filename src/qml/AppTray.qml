// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import Qt.labs.platform as Platform

// System tray leaf. Emits signals instead of reaching into Main; keep id
// trayIcon on the instance so existing win/trayIcon call sites keep working.
Platform.SystemTrayIcon {
    id: root
    visible: true
    // The dot only exists once a VPN profile does. Sherwan asked for green when
    // bound and red when not; red on a machine that never had a VPN would be an
    // alarm about a feature the user does not use.
    readonly property bool hasVpn: typeof vpn !== "undefined" && vpn.profiles.length > 0
    readonly property string vpnTag: hasVpn ? (vpn.connState === 2 ? "&vpn=on" : "&vpn=off") : ""
    icon.source: ((typeof themeBridge !== "undefined" && themeBridge.osLight)
                  ? "image://applogo/dark?v=l" : "image://applogo/light?v=d") + vpnTag
    icon.mask: false
    tooltip: root.hasVpn
             ? "BATorrent  ·  " + (i18n.language, i18n.t(vpn.connState === 2 ? "vpn_hero_protected" : "vpn_hero_exposed"))
             : "BATorrent"

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
