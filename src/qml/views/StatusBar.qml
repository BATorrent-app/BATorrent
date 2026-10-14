// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Bottom status strip of the Downloads page: version and counts, whether the
// port is reachable, the sharing notice, and the lifetime totals. Speeds,
// free space, alt speed and Donate live elsewhere now (toolbar, nav bar, app
// menu), so nothing here repeats what is already on screen.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"
import "../widgets"

Rectangle {
    id: status
    property var stats: null

    function stat(key, fallback) {
        return stats !== null && stats[key] !== undefined ? stats[key] : fallback
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 30
    color: Theme.panel
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

    component Note: Text {
        color: Theme.t4
        font.pixelSize: 12
        font.family: Theme.fontSans
        font.features: Theme.tnum
        Layout.alignment: Qt.AlignVCenter
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp4
        spacing: 10

        Note {
            readonly property string ver: typeof themeBridge !== "undefined" && themeBridge.appVersion.length > 0
                                          ? "v" + themeBridge.appVersion + " · " : ""
            text: ver + (i18n.language, i18n.t("status_torrents_active"))
                  .arg(status.stat("torrentCount", typeof session !== "undefined" ? session.torrentCount : 0))
                  .arg(status.stat("activeCount", typeof session !== "undefined" ? session.activeCount : 0))
        }

        // port reachability (UPnP/NAT-PMP + listen heuristic)
        Row {
            id: portInd
            Layout.alignment: Qt.AlignVCenter
            spacing: 6
            property int ps: status.stat("portStatus", typeof session !== "undefined" ? session.portStatus : 0)
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 8; height: 8; radius: 4
                color: portInd.ps === 1 ? Theme.grn
                     : portInd.ps === 2 ? Theme.warn
                     : portInd.ps === 3 ? Theme.accent : Theme.t4
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t(portInd.ps === 1 ? "status_port_open"
                     : portInd.ps === 2 ? "status_port_blocked"
                     : portInd.ps === 3 ? "status_port_closed" : "status_port_checking"))
                color: Theme.t4
                font.pixelSize: 12
                font.family: Theme.fontSans
            }
        }
        // Permanent and quiet. The wizard says it properly once; this is what
        // is always there, on the one page where transfers actually happen.
        Note {
            Layout.maximumWidth: 300
            text: "· " + (i18n.language, i18n.t("share_notice_short"))
            elide: Text.ElideRight
            ToolTip.visible: shareMa.containsMouse
            ToolTip.delay: 300
            ToolTip.text: (i18n.language, i18n.t("share_notice_body"))
            MouseArea { id: shareMa; anchors.fill: parent; hoverEnabled: true }
        }

        Item { Layout.fillWidth: true }

        Note {
            text: (i18n.language, i18n.t("status_totals2"))
                  .arg(status.stat("totalDownloaded", typeof session !== "undefined" ? session.totalDownloaded : "0 B"))
                  .arg(status.stat("totalUploaded", typeof session !== "undefined" ? session.totalUploaded : "0 B"))
                  .arg(status.stat("globalRatio", typeof session !== "undefined" ? session.globalRatio : "0.00"))
        }
    }
}
