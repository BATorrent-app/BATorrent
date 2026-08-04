// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "../theme"

Grid {
    id: picker
    property var uiPalette: Theme

    columns: 2
    spacing: 10

    Repeater {
        model: Theme.swatches
        delegate: Rectangle {
            id: swatch

            required property var modelData
            readonly property bool selected: Theme.name === modelData.key

            width: Math.max(132, (picker.width - picker.spacing) / 2)
            height: 62
            radius: 10
            color: modelData.bg
            border.width: selected || activeFocus ? 2 : 1
            border.color: selected ? picker.uiPalette.accent
                         : activeFocus ? picker.uiPalette.focusRing
                         : mouse.containsMouse ? picker.uiPalette.t4 : picker.uiPalette.hair
            activeFocusOnTab: true
            Accessible.role: Accessible.RadioButton
            Accessible.name: themeLabel.text
            Accessible.checked: selected

            Behavior on border.color {
                ColorAnimation { duration: Theme.reduceMotion ? 0 : 140 }
            }

            function pick() {
                forceActiveFocus()
                Theme.setName(modelData.key)
            }

            Rectangle {
                x: 10
                y: 10
                width: 44
                height: 42
                radius: 6
                color: swatch.modelData.panel
                Rectangle {
                    x: 7
                    y: 8
                    width: 22
                    height: 4
                    radius: 2
                    color: swatch.modelData.accent
                }
                Rectangle {
                    x: 7
                    y: 18
                    width: 30
                    height: 3
                    radius: 1.5
                    color: swatch.modelData.accent
                    opacity: 0.35
                }
            }

            Text {
                id: themeLabel
                x: 64
                anchors.verticalCenter: parent.verticalCenter
                text: swatch.modelData.key === "dark" ? i18n.t("set_theme_dark")
                    : swatch.modelData.key === "light" ? i18n.t("set_theme_light")
                    : swatch.modelData.key === "midnight" ? "Midnight"
                    : swatch.modelData.key === "sakura" ? "Sakura"
                    : swatch.modelData.key === "darkstar" ? "Dark Star" : "Matrix"
                color: swatch.modelData.key === "light" || swatch.modelData.key === "sakura"
                       ? "#16171a" : "#f3f3f4"
                font.pixelSize: 13
                font.weight: swatch.selected ? Font.DemiBold : Font.Normal
                font.family: picker.uiPalette.fontSans
            }

            MouseArea {
                id: mouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: swatch.pick()
            }

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    swatch.pick()
                    event.accepted = true
                }
            }
        }
    }
}
