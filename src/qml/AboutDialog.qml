// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent About.html + bat-dialog.css + <style> inline
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("about_title"))
    cardW: 460
    cardH: 540
    okText: (i18n.language, i18n.t("action_donate"))
    showCancel: false
    footHint: "© 2026 · open-source"

    // .ahero
    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        spacing: 8

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 58
            Layout.preferredHeight: 58
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(116, 116)
            fillMode: Image.PreserveAspectFit
        }
        // wordmark BAT + orrent
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 0
            Text { text: "BAT"; color: Theme.accent; font.pixelSize: 22; font.weight: Font.Black; font.letterSpacing: 1; font.family: Theme.fontSans }
            Text { text: "orrent"; color: Theme.t1; font.pixelSize: 22; font.weight: Font.Black; font.letterSpacing: 1; font.family: Theme.fontSans }
        }
        // .vrow
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: 8
            TChip { text: (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? ("v" + themeBridge.appVersion) : "" }
            Text { text: (i18n.language, i18n.t("about_build_stable")); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: (i18n.language, i18n.t("about_description"))
            color: Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontSans
        }
    }

    // .privacy card
    Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: Theme.sp3
        Layout.bottomMargin: Theme.sp3
        radius: 12
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        implicitHeight: privRow.implicitHeight + 26

        RowLayout {
            id: privRow
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 13
            anchors.rightMargin: 13
            spacing: 11

            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: (i18n.language, i18n.t("about_no_telemetry_body"))
                color: Theme.t3
                font.pixelSize: 11
                font.family: Theme.fontSans
                lineHeight: 1.45
            }
        }
    }

    // .glabel BIBLIOTECAS
    Text {
        text: (i18n.language, i18n.t("about_libraries"))
        color: Theme.t4
        font.pixelSize: 10
        font.weight: Font.Bold
        font.letterSpacing: 0.8
        font.family: Theme.fontSans
    }
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 0
        Repeater {
            id: libsRep
            model: (typeof themeBridge !== "undefined") ? themeBridge.libraries() : []
            delegate: ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 8
                    Layout.bottomMargin: 8
                    Text { text: modelData.nm; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans }
                    Item { Layout.fillWidth: true }
                    Text { text: modelData.v; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono }
                }
                Rectangle { visible: index < libsRep.count - 1; Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }
            }
        }
    }

    // translation credits — community contributors
    Text {
        text: (i18n.language, i18n.t("about_translations"))
        color: Theme.t4
        font.pixelSize: 10
        font.weight: Font.Bold
        font.letterSpacing: 0.8
        font.family: Theme.fontSans
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 8
        Layout.bottomMargin: 8
        Text { text: "🇺🇦 Українська"; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans }
        Item { Layout.fillWidth: true }
        Text {
            text: "<a href=\"https://github.com/dkindratyuk-web\">dkindratyuk-web</a>"
            textFormat: Text.RichText
            linkColor: Theme.accentText
            color: Theme.t2; font.pixelSize: 11; font.family: Theme.fontMono
            onLinkActivated: function(link) { Qt.openUrlExternally(link) }
        }
    }

    // .license
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: Theme.sp1
        Text { text: (i18n.language, i18n.t("about_license")); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
        Item { Layout.fillWidth: true }
        Text { text: "MIT"; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono }
    }
}
