// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Bottom status strip of the Downloads page: version, torrent/active counts,
// alt-speed turtle, port reachability, global speeds and totals. Carved out
// of Main.qml verbatim; version moved here from the nav rail.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    Layout.fillWidth: true
    Layout.preferredHeight: 30
    color: Theme.panel
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp4
        spacing: Theme.sp2

        Text {
            visible: typeof themeBridge !== "undefined" && themeBridge.appVersion.length > 0
            text: typeof themeBridge !== "undefined" ? ("v" + themeBridge.appVersion + "  ·") : ""
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            text: typeof session !== "undefined"
                  ? (i18n.language, i18n.t("status_torrents_active")).arg(session.torrentCount).arg(session.activeCount)
                  : "0 torrents"
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
        }
        // alt-speed (turtle): a mode toggle lives with the other status
        // signals, not among the toolbar actions
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: Theme.sp2
            width: 26; height: 22; radius: 6
            readonly property bool on: typeof session !== "undefined" && session.altSpeedsActive
            color: on ? Theme.accentTint : (turtleMa.containsMouse ? Theme.hover : "transparent")
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/turtle.svg"
                tint: parent.on ? Theme.accentText : Theme.t4
                s: 15
            }
            MouseArea {
                id: turtleMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof session !== "undefined") session.setAltSpeedsActive(!session.altSpeedsActive)
            }
            ToolTip.text: (i18n.language, i18n.t("tb_alt_speed"))
            ToolTip.visible: turtleMa.containsMouse
            ToolTip.delay: 400
        }

        // port reachability (UPnP/NAT-PMP + listen heuristic)
        Row {
            id: portInd
            Layout.alignment: Qt.AlignVCenter
            Layout.leftMargin: Theme.sp2
            spacing: 6
            property int ps: typeof session !== "undefined" ? session.portStatus : 0
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 8; height: 8; radius: 4
                color: portInd.ps === 1 ? Theme.grn
                     : portInd.ps === 2 ? Theme.amber
                     : portInd.ps === 3 ? Theme.accent : Theme.t4
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t("tb_port") + ": " + (portInd.ps === 1 ? i18n.t("port_open")
                     : portInd.ps === 2 ? i18n.t("port_firewalled")
                     : portInd.ps === 3 ? i18n.t("port_closed") : i18n.t("port_checking")))
                color: Theme.t4
                font.pixelSize: 12
                font.family: Theme.fontSans
            }
        }
        Item { Layout.fillWidth: true }
        Text { text: "↓"; color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans }
        Text { text: typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s"; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum }
        Text { text: "↑"; color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; Layout.leftMargin: Theme.sp1 }
        Text { text: typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s"; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum }
        Text {
            text: typeof session !== "undefined"
                  ? "·  " + (i18n.language, i18n.t("status_totals")).arg(session.totalDownloaded).arg(session.totalUploaded).arg(session.globalRatio)
                  : ""
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            visible: typeof session !== "undefined" && session.freeDiskSpace.length > 0
            text: typeof session !== "undefined"
                  ? "·  " + (i18n.language, i18n.t("status_free_space")).arg(session.freeDiskSpace)
                  : ""
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
        }
    }
}
