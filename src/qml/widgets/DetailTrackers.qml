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
    spacing: 0

    Rectangle {
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
        Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 18
        text: (i18n.language, i18n.t("inspector_no_trackers2")); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
    }
    ListView {
        Layout.fillWidth: true; Layout.fillHeight: true; clip: true; model: pane.trackers
        visible: pane.trackers.length > 0
        boundsBehavior: Flickable.StopAtBounds
        delegate: Rectangle {
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
}
