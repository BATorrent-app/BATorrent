// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Disk add-guard: bridge blocks a too-big add and asks here first.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: root
    required property var sv
    anchors.fill: parent

    property int pendingFitIdx: -1
    property string pendingFitMsg: ""
    property double pendingFitShortfall: 0
    signal freeSpaceRequested(double targetBytes)

    Connections {
        target: root.sv.api
        ignoreUnknownSignals: true
        function onAddWontFit(index, name, needed, freeBytes) {
            root.pendingFitIdx = index
            root.pendingFitMsg = i18n.t("search_wontfit_body")
                .replace("%1", name)
                .replace("%2", root.sv.fmtSize(needed))
                .replace("%3", root.sv.fmtSize(freeBytes))
            root.pendingFitShortfall = Math.max(0, needed - freeBytes)
            fitDlg.open()
        }
    }
    BatDialog {
        id: fitDlg
        title: (i18n.language, i18n.t("search_wontfit_title"))
        cardW: 470; cardH: 280
        okText: (i18n.language, i18n.t("search_wontfit_ok"))
        cancelText: (i18n.language, i18n.t("btn_cancel"))
        onAccepted: if (root.sv.api && root.pendingFitIdx >= 0)
            root.sv.api.activateResult(root.pendingFitIdx, true)
        Text {
            Layout.fillWidth: true
            text: root.pendingFitMsg
            wrapMode: Text.WordWrap
            color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans; lineHeight: 1.35
        }
        BtnFlat {
            Layout.alignment: Qt.AlignLeft
            Layout.topMargin: 4
            text: (i18n.language, i18n.t("search_wontfit_freeup"))
            onClicked: {
                fitDlg.close()
                root.freeSpaceRequested(root.pendingFitShortfall)
            }
        }
    }
}
