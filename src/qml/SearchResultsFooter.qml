// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Search chrome footer — back, status, disk-fit warn, raw-results escape hatch.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: root
    required property var sv

    Layout.fillWidth: true
    Layout.preferredHeight: 56
    visible: !sv.browse
    color: Theme.elev
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp5
        anchors.rightMargin: 20
        spacing: Theme.sp2
        BtnFlat {
            visible: root.sv.api && root.sv.api.canGoBack
            text: (i18n.language, i18n.t("search_back2"))
            onClicked: if (root.sv.api) root.sv.api.back()
        }
        Text {
            text: root.sv.api ? root.sv.api.statusText : ""
            color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
        }
        Item { Layout.fillWidth: true }
        Row {
            spacing: 7
            visible: root.sv.isFlatList && root.sv.wontFit > 0 && root.sv.saveFree >= 0
            Rectangle {
                width: 7; height: 7; radius: 4; color: "#e0a533"
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t("search_disk_warn"))
                    .arg(root.sv.fmtSize(root.sv.saveFree)).arg(root.sv.wontFit)
                color: "#e0a533"; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
        }
        BtnFlat {
            visible: root.sv.isTitles && root.sv.api && !root.sv.api.searching
            text: (i18n.language, i18n.t("search_raw_results"))
            onClicked: if (root.sv.api) root.sv.api.searchRaw()
        }
    }
}
