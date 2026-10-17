// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The VPN went down. Not a notice that fades: a band under the navigation
// until the tunnel is back, saying what happened to the transfers and
// offering the two things to do about it.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Rectangle {
    id: band
    signal settingsRequested()

    readonly property bool hasVpn: typeof vpn !== "undefined" && vpn !== null
    // dropped: the tunnel failed after being up (not a connect that never
    // worked). The window keeps it, so the nav chip says the same thing.
    property bool dropped: false
    readonly property bool killSwitch: typeof settings !== "undefined" && settings.getBool("killSwitchEnabled")

    visible: dropped
    implicitHeight: visible ? 46 : 0
    color: Theme.dangerBand
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.down }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16; anchors.rightMargin: 12
        spacing: 10
        Rectangle { width: 7; height: 7; radius: 3.5; color: Theme.down }
        Text {
            Layout.fillWidth: true
            textFormat: Text.StyledText
            elide: Text.ElideRight
            color: Theme.t2
            font.pixelSize: 13; font.family: Theme.fontSans
            text: (i18n.language, "<b><font color='" + Theme.t1 + "'>" + i18n.t(band.killSwitch ? "vpn_dropped_stopped" : "vpn_dropped_open") + "</font></b> "
                  + i18n.t(band.killSwitch ? "vpn_dropped_stopped_d" : "vpn_dropped_open_d")
                  + (band.hasVpn && vpn.retryIn > 0 ? " " + i18n.t("vpn_retry_in").arg(vpn.retryIn) : ""))
        }
        LightBtn {
            text: (i18n.language, i18n.t("vpn_reconnect"))
            onClicked: if (band.hasVpn) vpn.connectLastUsed()
        }
        BtnFlat { sm: true; text: (i18n.language, i18n.t("vpn_settings_btn")); onClicked: band.settingsRequested() }
    }
}
