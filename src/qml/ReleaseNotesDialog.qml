// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Header chrome from the QML redesign; changelog body + version come from C++
// (themeBridge) so nothing is hardcoded.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("release_notes_title2"))
    cardW: 540
    cardH: 560
    okText: (i18n.language, i18n.t("release_notes_close"))
    showCancel: false
    footHint: (i18n.language, i18n.t("rn_changelog_src"))

    readonly property string appVer: (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""

    // .nhead
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Theme.sp1
        spacing: 14

        // logo badge
        Image {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44
            Layout.alignment: Qt.AlignTop
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(88, 88)
            fillMode: Image.PreserveAspectFit
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            RowLayout {
                spacing: 8
                Eyebrow { text: (i18n.language, i18n.t("release_notes_eyebrow")); red: true }
                TChip { visible: dlg.appVer.length > 0; text: "v" + dlg.appVer }
            }
            Text {
                text: (i18n.language, i18n.t("release_notes_check"))
                color: Theme.t1
                font.pixelSize: 19
                font.weight: Font.DemiBold
                font.letterSpacing: -0.3
                font.family: Theme.fontSans
            }
        }
    }
    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

    // real changelog (HTML) rendered as rich text — scrolls inside the dialog body
    Text {
        Layout.fillWidth: true
        Layout.topMargin: Theme.sp2
        wrapMode: Text.WordWrap
        textFormat: Text.MarkdownText
        text: (typeof themeBridge !== "undefined") ? themeBridge.releaseNotes() : ""
        color: Theme.t2
        font.pixelSize: 12
        font.family: Theme.fontSans
        lineHeight: 1.4
        onLinkActivated: function(link) { Qt.openUrlExternally(link) }
    }
}
