// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent Empty.html (.empty block) — centered empty state for the Main content area.
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: empty
    signal openClicked()
    signal magnetClicked()
    signal linkClicked()

    spacing: 0

    // .mk 86×86
    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        Layout.bottomMargin: 22
        implicitWidth: 86
        implicitHeight: 86
        radius: 22
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        Image {
            anchors.centerIn: parent
            width: 44; height: 44
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(88, 88)
            fillMode: Image.PreserveAspectFit
            opacity: 0.5
        }
    }

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: (i18n.language, i18n.t("empty_title"))
        color: Theme.t1
        font.pixelSize: 19
        font.weight: Font.DemiBold
        font.letterSpacing: -0.3
        font.family: Theme.fontSans
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 9
        Layout.maximumWidth: 360
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: (i18n.language, i18n.t("empty_body2"))
        color: Theme.t3
        font.pixelSize: 13
        font.family: Theme.fontSans
        lineHeight: 1.55
    }

    // .cta
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 24
        spacing: 10

        Rectangle {
            implicitWidth: openRow.implicitWidth + 32
            implicitHeight: 36
            radius: 8
            color: openMa.containsMouse ? Theme.accentDark : Theme.accent
            Row {
                id: openRow
                anchors.centerIn: parent
                spacing: 8
                IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/open.svg"; tint: "#ffffff"; s: 16 }
                Text { anchors.verticalCenter: parent.verticalCenter; text: (i18n.language, i18n.t("welcome_card_open_title")); color: "#ffffff"; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            }
            MouseArea { id: openMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: empty.openClicked() }
        }
        Rectangle {
            implicitWidth: magRow.implicitWidth + 32
            implicitHeight: 36
            radius: 8
            color: Theme.panel
            border.color: magMa.containsMouse ? (Theme.isDark ? Qt.rgba(1,1,1,0.2) : Qt.rgba(0,0,0,0.22)) : Theme.hair
            border.width: 1
            Row {
                id: magRow
                anchors.centerIn: parent
                spacing: 8
                IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/magnet.svg"; tint: Theme.t1; s: 16 }
                Text { anchors.verticalCenter: parent.verticalCenter; text: (i18n.language, i18n.t("empty_paste_btn")); color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            }
            MouseArea { id: magMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: empty.magnetClicked() }
        }
        Rectangle {
            implicitWidth: linkRow.implicitWidth + 32
            implicitHeight: 36
            radius: 8
            color: Theme.panel
            border.color: linkMa.containsMouse ? (Theme.isDark ? Qt.rgba(1,1,1,0.2) : Qt.rgba(0,0,0,0.22)) : Theme.hair
            border.width: 1
            Row {
                id: linkRow
                anchors.centerIn: parent
                spacing: 8
                IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/download.svg"; tint: Theme.t1; s: 16 }
                Text { anchors.verticalCenter: parent.verticalCenter; text: (i18n.language, i18n.t("empty_link_btn")); color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            }
            MouseArea { id: linkMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: empty.linkClicked() }
        }
    }

    // .hintdrop
    RowLayout {
        Layout.alignment: Qt.AlignHCenter
        Layout.topMargin: 26
        spacing: 8
        IconImg { src: "qrc:/icons/upload.svg"; tint: Theme.t4; s: 14 }
        Text { text: (i18n.language, i18n.t("empty_drag_hint")); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
    }
}
