// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: pane

    property bool classic: false
    property bool navLeft: false
    property bool detailBottom: false
    property int step: 0
    property bool narrow: false
    property var uiPalette: Theme

    spacing: 0

    Item {
        Layout.fillWidth: true
        Layout.preferredHeight: pane.narrow ? 260 : 300

        OnboardingPreview {
            anchors.fill: parent
            classic: pane.classic
            navLeft: pane.navLeft
            detailBottom: pane.detailBottom
            focusArea: pane.step === 1 ? "theme"
                     : pane.step === 2 ? "view"
                     : pane.step === 3 ? "nav"
                     : pane.step === 4 ? "detail" : ""
        }

        Rectangle {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            z: 5
            radius: 7
            color: Qt.rgba(0, 0, 0, 0.62)
            border.width: 1
            border.color: Qt.rgba(1, 1, 1, 0.12)
            implicitWidth: demoLabel.implicitWidth + 16
            implicitHeight: 22

            Text {
                id: demoLabel
                anchors.centerIn: parent
                text: i18n.t("welcome_preview_demo")
                color: "#d0d0d4"
                font.pixelSize: 10
                font.family: pane.uiPalette.fontSans
            }
        }
    }
}
