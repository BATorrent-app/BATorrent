// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// VPN status pill for the top nav bar (bound-interface signal + cockpit door).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "theme"
import "widgets"

Rectangle {
    id: root
    required property var bar

    visible: typeof vpn !== "undefined"
    Layout.alignment: Qt.AlignVCenter
    Layout.preferredHeight: 34
    Layout.preferredWidth: vpnRow.implicitWidth + 22
    radius: 9

    property bool bound: false
    function refreshBound() {
        bound = (typeof settings !== "undefined") && settings.get("outgoingInterface") !== ""
    }
    Component.onCompleted: refreshBound()
    Connections {
        target: (typeof settings !== "undefined") ? settings : null
        function onChanged() { root.refreshBound() }
    }
    readonly property int st: bound ? 2 : 0
    readonly property color stColor: st === 2 ? Theme.grn
                                   : st === 1 ? Theme.amber
                                   : st === 3 ? Theme.accent : Theme.t4
    color: vpnMa.containsMouse ? Theme.hover : "transparent"
    Behavior on color { ColorAnimation { duration: 130 } }
    RowLayout {
        id: vpnRow
        anchors.centerIn: parent
        spacing: 8
        Item {
            Layout.preferredWidth: 16; Layout.preferredHeight: 16
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/set-vpn.svg"
                tint: root.stColor; s: 15
            }
            Rectangle {
                width: 7; height: 7; radius: 3.5
                anchors.right: parent.right; anchors.bottom: parent.bottom
                anchors.rightMargin: -1; anchors.bottomMargin: -1
                color: root.stColor
                SequentialAnimation on opacity {
                    running: root.st === 1
                    loops: Animation.Infinite
                    NumberAnimation { from: 1; to: 0.3; duration: 600 }
                    NumberAnimation { from: 0.3; to: 1; duration: 600 }
                }
            }
        }
        Text {
            visible: !root.bar.tightChip
            text: "VPN"
            color: root.st === 2 ? Theme.t2 : Theme.t3
            font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
        }
    }
    MouseArea {
        id: vpnMa
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.bar.vpnClicked()
    }
    ToolTip.visible: vpnMa.containsMouse
    ToolTip.delay: 400
    ToolTip.text: (i18n.language, "VPN — " +
        (root.st === 2 ? i18n.t("vpn_state_on")
         : root.st === 1 ? i18n.t("vpn_state_connecting")
         : root.st === 3 ? i18n.t("vpn_state_failed")
         : i18n.t("vpn_state_off")))
}
