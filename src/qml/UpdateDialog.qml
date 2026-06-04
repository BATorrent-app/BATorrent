// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Update available / download dialog, driven by the `updater` bridge.
// Only reachable in non-Store builds (the bridge is null otherwise).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    cardW: 460
    cardH: 240
    showOk: false
    cancelText: (i18n.language, i18n.t("release_notes_close"))

    property string version: ""
    property string url: ""
    property string assetName: ""
    property int pct: 0
    property bool downloading: false
    property string statusText: ""

    function showAvailable(v, u, a) {
        version = v; url = u; assetName = a
        downloading = false; pct = 0
        statusText = (i18n.language, i18n.t("update_available_msg")).arg(v)
        title = (i18n.language, i18n.t("update_available_title"))
        open()
    }
    function showNone() {
        downloading = false
        title = (i18n.language, i18n.t("update_title2"))
        statusText = (i18n.language, i18n.t("update_uptodate"))
        open()
    }
    function showError(msg) {
        downloading = false
        title = (i18n.language, i18n.t("update_title2"))
        statusText = (i18n.language, i18n.t("create_error_prefix")) + msg
        open()
    }

    Connections {
        target: typeof updater !== "undefined" ? updater : null
        ignoreUnknownSignals: true
        function onProgress(percent) { dlg.pct = percent }
        function onReady() { dlg.statusText = (i18n.language, i18n.t("update_restarting")) }
        function onFailed(message) { dlg.showError(message) }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp4

        Text {
            Layout.fillWidth: true
            text: dlg.statusText
            color: Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }

        // progress bar (only while downloading)
        Rectangle {
            visible: dlg.downloading
            Layout.fillWidth: true
            Layout.preferredHeight: 8
            radius: 4
            color: Theme.field
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: parent.width * (dlg.pct / 100)
                radius: 4
                color: Theme.accent
                Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.sp2
            spacing: Theme.sp3
            Item { Layout.fillWidth: true }
            BtnFlat {
                visible: dlg.version.length > 0 && !dlg.downloading
                text: (i18n.language, i18n.t("update_skip"))
                onClicked: { if (typeof updater !== "undefined" && updater) updater.skipVersion(dlg.version); dlg.close() }
            }
            BtnFlat {
                primary: true
                visible: dlg.version.length > 0
                enabled: !dlg.downloading
                text: dlg.downloading ? (i18n.language, i18n.t("update_downloading_pct")).arg(dlg.pct) : (i18n.language, i18n.t("update_install_btn"))
                onClicked: {
                    if (typeof updater === "undefined" || !updater) return
                    dlg.downloading = true
                    updater.downloadAndInstall(dlg.url, dlg.assetName)
                }
            }
        }
    }
}
