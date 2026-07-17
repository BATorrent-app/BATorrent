// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent Remove Confirm.html + bat-dialog.css
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("remove_title2"))
    cardW: 440
    cardH: dlg.deleteFiles ? 432 : 360
    okText: (i18n.language, i18n.t("addon_remove"))

    // default OFF — "Remove" only un-lists the torrent; deleting the downloaded
    // files is an explicit opt-in (matches qBittorrent/Transmission convention).
    property bool deleteFiles: false
    property bool deletePermanently: false
    // never let a sticky "permanent" carry into the next removal
    onVisibleChanged: if (visible) { deleteFiles = false; deletePermanently = false }
    onDeleteFilesChanged: if (!deleteFiles) deletePermanently = false

    // .top — warn icon + title + description
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: Theme.sp2
        spacing: Theme.sp3

        // .warn-ic 40×40
        Rectangle {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignTop
            radius: 10
            color: Qt.rgba(229/255, 51/255, 43/255, 0.12)
            border.color: Qt.rgba(229/255, 51/255, 43/255, 0.32)
            border.width: 1
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/triangle-alert.svg"
                tint: Theme.accentText; s: 18
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: (i18n.language, i18n.t("remove_confirm_q"))
                color: Theme.t1
                font.pixelSize: 16
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: 320
                wrapMode: Text.WordWrap
                color: Theme.t3
                font.pixelSize: 12
                font.family: Theme.fontSans
                textFormat: Text.StyledText
                // escape the torrent name — it's StyledText and the name is
                // attacker-controlled (from the .torrent / magnet), so raw
                // markup would let a crafted name inject into the dialog.
                text: (i18n.language, i18n.t("remove_confirm_body"))
                    .arg("<font color='" + Theme.t2 + "'><b>"
                         + (typeof session !== "undefined" ? session.selectedName : "")
                             .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
                         + "</b></font>")
            }
        }
    }

    // .card — .opt clickable
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 64
        radius: 11
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: Theme.sp3

            TChk {
                id: chk
                Layout.alignment: Qt.AlignVCenter
                on: dlg.deleteFiles
                onToggled: function(v) { dlg.deleteFiles = v }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: (i18n.language, i18n.t("remove_also_files"))
                    color: Theme.t1
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                }
                Text {
                    text: (i18n.language, i18n.t("remove_delete_note"))
                    color: Theme.t4
                    font.pixelSize: 11
                    font.family: Theme.fontSans
                }
            }
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: { dlg.deleteFiles = !dlg.deleteFiles; chk.on = dlg.deleteFiles }
        }
    }

    // permanent option — only relevant once "delete files" is on
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 56
        visible: dlg.deleteFiles
        radius: 11
        color: Theme.panel
        border.color: dlg.deletePermanently ? Qt.rgba(229/255, 51/255, 43/255, 0.32) : Theme.hair
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: Theme.sp3
            TChk {
                id: permChk
                Layout.alignment: Qt.AlignVCenter
                on: dlg.deletePermanently
                onToggled: function(v) { dlg.deletePermanently = v }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    text: (i18n.language, i18n.t("remove_permanent"))
                    color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans
                }
                Text {
                    text: (i18n.language, i18n.t("remove_permanent_note"))
                    color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
                }
            }
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: { dlg.deletePermanently = !dlg.deletePermanently; permChk.on = dlg.deletePermanently }
        }
    }
}
