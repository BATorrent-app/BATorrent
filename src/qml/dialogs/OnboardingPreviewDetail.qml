// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Rectangle {
    id: detail

    property bool vertical: true

    color: Theme.panel
    border.width: 1
    border.color: Theme.hair
    clip: true
    Accessible.ignored: true

    component PreviewIdentity: RowLayout {
        spacing: Theme.sp4

        Rectangle {
            Layout.preferredWidth: detail.vertical ? 104 : 116
            Layout.preferredHeight: detail.vertical ? 146 : 154
            radius: 9
            color: Theme.field
            clip: true
            Image {
                anchors.fill: parent
                source: "qrc:/images/007.jpg"
                sourceSize: Qt.size(232, 308)
                fillMode: Image.PreserveAspectCrop
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 9
                color: "#cc000000"
                Rectangle {
                    width: parent.width * 0.72
                    height: parent.height
                    color: Theme.accent
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            spacing: Theme.sp2

            Text {
                Layout.fillWidth: true
                text: "Nightfall"
                color: Theme.t1
                font.pixelSize: detail.vertical ? 18 : 20
                font.weight: Font.Bold
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Text {
                text: i18n.t("state_downloading")
                color: Theme.accentText
                font.pixelSize: 12
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 8
                radius: 4
                color: Theme.track
                Rectangle {
                    width: parent.width * 0.72
                    height: parent.height
                    radius: parent.radius
                    color: Theme.accent
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: "72%"
                    color: Theme.t2
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "6.8 MB/s"
                    color: Theme.t2
                    font.pixelSize: 12
                    font.family: Theme.fontMono
                    font.features: Theme.tnum
                }
            }
            Text {
                text: "6.1 GB " + i18n.t("word_of") + " 8.4 GB"
                color: Theme.t4
                font.pixelSize: 11
                font.family: Theme.fontSans
            }
        }
    }

    component PreviewGeneral: GridLayout {
        columns: detail.vertical ? 2 : 4
        columnSpacing: Theme.sp5
        rowSpacing: Theme.sp3

        Text { text: i18n.t("detail_kv_size"); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
        Text { text: "8.4 GB"; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
        Text { text: i18n.t("detail_kv_ratio"); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
        Text { text: "0.64"; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
        Text { text: i18n.t("detail_kv_peers"); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
        Text { text: "18 (42)"; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
        Text { text: i18n.t("detail_kv_eta"); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
        Text { text: "14m"; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            visible: detail.vertical
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 45 : 0

            Text {
                anchors.left: parent.left
                anchors.leftMargin: Theme.sp4
                anchors.verticalCenter: parent.verticalCenter
                text: i18n.t("detail_selected_torrent")
                color: Theme.t4
                font.pixelSize: 10
                font.weight: Font.Bold
                font.letterSpacing: 1
                font.capitalization: Font.AllUppercase
                font.family: Theme.fontSans
            }
            Row {
                anchors.right: parent.right
                anchors.rightMargin: Theme.sp3
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4
                IconImg { src: "qrc:/icons/lock-open-solid.svg"; tint: Theme.t3; s: 17 }
                IconImg { src: "qrc:/icons/close-bold.svg"; tint: Theme.t3; s: 17 }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.hairSoft
            }
        }

        Rectangle {
            visible: !detail.vertical
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 42 : 0
            color: "transparent"

            DetailTabs {
                anchors.left: parent.left
                anchors.leftMargin: Theme.sp5
                anchors.right: headerActions.left
                anchors.rightMargin: Theme.sp3
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                current: 0
                counts: ["", "42", "7", "12", "", ""]
            }
            Row {
                id: headerActions
                anchors.right: parent.right
                anchors.rightMargin: Theme.sp4
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                IconImg { src: "qrc:/icons/lock-open-solid.svg"; tint: Theme.t3; s: 17 }
                Text { text: "⌄"; color: Theme.t2; font.pixelSize: 18; font.bold: true; font.family: Theme.fontSans }
            }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 1
                color: Theme.hair
            }
        }

        PreviewIdentity {
            visible: detail.vertical
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sp4
            Layout.rightMargin: Theme.sp4
            Layout.topMargin: Theme.sp4
            Layout.bottomMargin: Theme.sp4
        }

        DetailTabs {
            visible: detail.vertical
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? 36 : 0
            Layout.leftMargin: Theme.sp4
            Layout.rightMargin: Theme.sp3
            compact: true
            current: 0
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.hair
        }

        PreviewGeneral {
            visible: detail.vertical
            Layout.fillWidth: true
            Layout.leftMargin: Theme.sp4
            Layout.rightMargin: Theme.sp4
            Layout.topMargin: Theme.sp4
        }

        RowLayout {
            visible: !detail.vertical
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Theme.sp5
            Layout.rightMargin: Theme.sp5
            Layout.topMargin: Theme.sp4
            Layout.bottomMargin: Theme.sp4
            spacing: Theme.sp6

            PreviewIdentity {
                Layout.preferredWidth: 460
                Layout.fillHeight: true
            }
            Rectangle {
                Layout.preferredWidth: 1
                Layout.fillHeight: true
                color: Theme.hair
            }
            PreviewGeneral {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
            }
        }

        Item { visible: detail.vertical; Layout.fillHeight: true }
    }
}
