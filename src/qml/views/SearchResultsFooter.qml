// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Search chrome footer: back, status, disk-fit warn, raw-results escape hatch.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

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
            visible: root.sv.api && root.sv.api.canGoBack && !Theme.titleStripNav
            primary: true
            text: (i18n.language, i18n.t("search_back2"))
            // From the full table, back is the title's own page, not the grid.
            onClicked: {
                if (root.sv.showAdvanced && root.sv.titlePicked) root.sv.showAdvanced = false
                else if (root.sv.api) root.sv.leaveTitle()
            }
        }
        Text {
            textFormat: Text.PlainText
            text: {
                i18n.language
                var api = root.sv.api
                if (!api) return ""
                // the titles a search found: how many, for what, from where
                if (root.sv.isTitles && !api.searching && root.sv.results.length > 0)
                    return i18n.t("find_results_from").arg(root.sv.results.length).arg("“" + api.activeQuery + "”")
                return api.statusText
            }
            color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        Item { Layout.fillWidth: true }
        Row {
            spacing: 7
            visible: root.sv.isFlatList && root.sv.wontFit > 0 && root.sv.saveFree >= 0
            Rectangle {
                width: 7; height: 7; radius: Theme.rad(4); color: Theme.warn
                anchors.verticalCenter: parent.verticalCenter
            }
            // which disk, how much is free, and the way to make room
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: {
                    i18n.language
                    return i18n.t(root.sv.wontFit === 1 ? "find_disk_wont_fit_1" : "find_disk_wont_fit")
                               .arg(root.sv.wontFit).arg(root.sv.saveDiskName).arg(root.sv.fmtSize(root.sv.saveFree)) + " · "
                }
                color: Theme.warn; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t("search_wontfit_freeup"))
                color: Theme.warn; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                font.underline: fuMa.containsMouse
                MouseArea { id: fuMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: root.sv.freeSpaceRequested(0) }
            }
        }
        BtnFlat {
            visible: root.sv.isTitles && root.sv.api && !root.sv.api.searching && root.sv.results.length > 0
            text: (i18n.language, i18n.t("search_raw_results"))
            onClicked: if (root.sv.api) root.sv.api.searchRaw()
        }
    }
}
