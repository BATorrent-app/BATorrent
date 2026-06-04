// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Live log tail with level filter, clear, export. Wired to QmlLogBridge (`logs`).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "theme"
import "widgets"

Window {
    id: win
    width: 900
    height: 600
    minimumWidth: 640
    minimumHeight: 420
    color: Theme.bg
    title: (i18n.language, i18n.t("logviewer_title2"))

    readonly property var api: typeof logs !== "undefined" ? logs : null
    onVisibleChanged: {
        if (!api) return
        if (visible) api.start(); else api.stop()
    }

    FileDialog {
        id: saveDlg
        title: (i18n.language, i18n.t("logviewer_save_logs"))
        fileMode: FileDialog.SaveFile
        nameFilters: [(i18n.language, i18n.t("filter_text_files"))]
        onAccepted: api.exportLogs(saveDlg.selectedFile.toString())
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // titlebar with level filter
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: Theme.elev
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp3
                Text { text: (i18n.language, i18n.t("logviewer_title2")); color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                Text { text: (i18n.language, i18n.t("logviewer_level2")); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                TSelect {
                    Layout.preferredWidth: 130
                    model: win.api ? win.api.levelNames : []
                    currentIndex: win.api ? win.api.level : 0
                    onActivated: function(i) { if (win.api) win.api.level = i }
                }
            }
        }

        // log body
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.field

            ScrollView {
                id: scroll
                anchors.fill: parent
                anchors.margins: Theme.sp3
                clip: true

                TextArea {
                    id: logText
                    readOnly: true
                    text: win.api ? win.api.text : ""
                    color: Theme.t2
                    font.pixelSize: 11
                    font.family: Theme.fontMono
                    wrapMode: TextArea.NoWrap
                    background: null
                    // sticky auto-scroll: jump to bottom when new text arrives if already near bottom
                    property bool stick: true
                    onTextChanged: if (stick) cursorPosition = length
                }
            }
            Connections {
                target: scroll.ScrollBar ? scroll.ScrollBar.vertical : null
                ignoreUnknownSignals: true
            }
        }

        // footer actions
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                spacing: Theme.sp2
                BtnFlat { text: (i18n.language, i18n.t("logviewer_open_folder2")); onClicked: if (win.api) win.api.openLogsFolder() }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("logviewer_save2")); onClicked: { if (win.api) saveDlg.currentFile = (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(win.api.defaultExportName()); saveDlg.open() } }
                BtnFlat { text: (i18n.language, i18n.t("logviewer_clear")); onClicked: if (win.api) win.api.clearLog() }
                Item { Layout.fillWidth: true }
                BtnFlat { text: (i18n.language, i18n.t("release_notes_close")); onClicked: win.close() }
            }
        }
    }
}
