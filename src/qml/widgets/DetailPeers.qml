// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Peers pane for the detail panel. Data: session.selectedPeerList
// rows: { ip, port, client, downSpeed, upSpeed, progress }
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: pane
    property var peers: []
    property bool loading: false   // building the peer list (deferred off the click)
    property bool compact: false   // 2-line rows for the 340px side inspector
    spacing: 0

    Rectangle {
        visible: !pane.compact
        Layout.fillWidth: true
        Layout.preferredHeight: 30
        color: "transparent"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
        RowLayout {
            anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
            Text { text: (i18n.language, i18n.t("detailpeers_country")); Layout.preferredWidth: 56; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_ip")); Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_client")); Layout.preferredWidth: 150; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_progr")); Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_down")); Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailpeers_up")); Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
        }
    }
    Text {
        visible: pane.peers.length === 0 && !pane.loading
        Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 18
        text: (i18n.language, i18n.t("detailpeers_empty")); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
    }
    // loading placeholder — skeleton rows while the peer list is being built
    ColumnLayout {
        visible: pane.loading && pane.peers.length === 0
        Layout.fillWidth: true; Layout.topMargin: 6; spacing: 0
        Repeater {
            model: 8
            delegate: RowLayout {
                Layout.fillWidth: true; Layout.preferredHeight: 32
                Layout.leftMargin: Theme.sp5; Layout.rightMargin: Theme.sp5; spacing: Theme.sp4
                Rectangle { Layout.preferredWidth: 22; Layout.preferredHeight: 12; radius: 6; color: Theme.field }
                Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 12; radius: 6; color: Theme.field; opacity: 0.6 }
                Rectangle { Layout.preferredWidth: 60; Layout.preferredHeight: 12; radius: 6; color: Theme.field; opacity: 0.45 }
            }
        }
    }
    ListView {
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: pane.peers
        visible: pane.peers.length > 0
        boundsBehavior: Flickable.StopAtBounds
        delegate: pane.compact ? compactRow : wideRow
    }

    Component {
        id: wideRow
        Rectangle {
            width: ListView.view.width; height: 32; color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5
                Text { text: modelData.flag || ""; Layout.preferredWidth: 56; font.pixelSize: 13; font.family: Theme.fontSans }
                Text { text: modelData.ip; Layout.fillWidth: true; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontMono; elide: Text.ElideRight }
                Text { text: modelData.client; Layout.preferredWidth: 150; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                Text { text: Math.floor((modelData.progress || 0) * 100) + "%"; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono }
                Text { text: modelData.downSpeed; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.accentText; font.pixelSize: 12; font.family: Theme.fontMono }
                Text { text: modelData.upSpeed; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.up; font.pixelSize: 12; font.family: Theme.fontMono }
            }
        }
    }
    // 2 lines: flag + ip ·· progress / client ·· ↓ ↑ — same fields, sidebar width
    Component {
        id: compactRow
        Rectangle {
            width: ListView.view.width; height: 46; color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4; anchors.rightMargin: Theme.sp4
                anchors.topMargin: 7; anchors.bottomMargin: 7
                spacing: 3
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 7
                    Text { text: modelData.flag || ""; font.pixelSize: 12; font.family: Theme.fontSans }
                    Text { text: modelData.ip; Layout.fillWidth: true; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontMono; elide: Text.ElideRight }
                    Text { text: Math.floor((modelData.progress || 0) * 100) + "%"; color: Theme.t2; font.pixelSize: 11; font.family: Theme.fontMono }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: modelData.client; Layout.fillWidth: true; color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans; elide: Text.ElideRight }
                    Text { text: "↓ " + modelData.downSpeed; color: Theme.accentText; font.pixelSize: 11; font.family: Theme.fontMono }
                    Text { text: "↑ " + modelData.upSpeed; color: Theme.up; font.pixelSize: 11; font.family: Theme.fontMono }
                }
            }
        }
    }
}
