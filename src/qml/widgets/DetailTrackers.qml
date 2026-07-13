// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Trackers pane for the detail panel. Data: session.selectedTrackers
// rows: { url, tier, status }
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: pane
    property var trackers: []
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
            Text { text: (i18n.language, i18n.t("tracker_url_col")); Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailtrackers_tier")); Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
            Text { text: (i18n.language, i18n.t("detailtrackers_status")); Layout.preferredWidth: 200; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
        }
    }
    Text {
        visible: pane.trackers.length === 0
        Layout.fillWidth: true; Layout.topMargin: 18
        horizontalAlignment: Text.AlignHCenter
        text: (i18n.language, i18n.t("inspector_no_trackers2")); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
    }
    ListView {
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: pane.trackers
        visible: pane.trackers.length > 0
        boundsBehavior: Flickable.StopAtBounds
        delegate: pane.compact ? compactRow : wideRow
    }

    Component {
        id: wideRow
        Rectangle {
            width: ListView.view.width; height: 32
            color: rowMa.hovered ? Theme.hover : "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            HoverHandler { id: rowMa }
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: Theme.sp5; anchors.rightMargin: Theme.sp5; spacing: 8
                Text { text: modelData.url; Layout.fillWidth: true; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono; elide: Text.ElideRight }
                Text { text: modelData.tier; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono }
                Text { text: modelData.status; Layout.preferredWidth: 200; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                Text {
                    text: "✕"; Layout.preferredWidth: 16; horizontalAlignment: Text.AlignHCenter
                    visible: rowMa.hovered || rmMa.containsMouse
                    color: rmMa.containsMouse ? Theme.accentText : Theme.t4
                    font.pixelSize: 12; font.family: Theme.fontSans
                    MouseArea { id: rmMa; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: if (typeof session !== "undefined") session.removeTrackerFromSelected(modelData.url) }
                }
            }
        }
    }
    // 2 lines: url ·· ✕ / tier ·· status — same fields, sidebar width
    Component {
        id: compactRow
        Rectangle {
            width: ListView.view.width; height: 46
            color: rowMa.hovered ? Theme.hover : "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            HoverHandler { id: rowMa }
            ColumnLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4; anchors.rightMargin: Theme.sp4
                anchors.topMargin: 7; anchors.bottomMargin: 7
                spacing: 3
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: modelData.url; Layout.fillWidth: true; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono; elide: Text.ElideRight }
                    Text {
                        text: "✕"
                        visible: rowMa.hovered || rmMa.containsMouse
                        color: rmMa.containsMouse ? Theme.accentText : Theme.t4
                        font.pixelSize: 12; font.family: Theme.fontSans
                        MouseArea { id: rmMa; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: if (typeof session !== "undefined") session.removeTrackerFromSelected(modelData.url) }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text { text: (i18n.language, i18n.t("detailtrackers_tier")) + " " + modelData.tier; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono }
                    Item { Layout.fillWidth: true }
                    Text { text: modelData.status; Layout.maximumWidth: 200; color: Theme.t1; font.pixelSize: 11; font.family: Theme.fontSans; elide: Text.ElideRight }
                }
            }
        }
    }
}
