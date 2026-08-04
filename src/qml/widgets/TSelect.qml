// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: batorrent-settings.css .select / bat-dialog .select
// height 30, appearance none, padding 0 30 0 10, bg Theme.field, border 1px Theme.hair, radius 7.
// caret triângulo Theme.t3 à direita. :focus → border Theme.accent.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

ComboBox {
    id: cb
    property var uiPalette: Theme
    implicitHeight: 30
    font.pixelSize: 12
    font.family: uiPalette.fontSans

    // optional per-item leading icon (parallel to model; e.g. language flags).
    // empty array → plain text select, unchanged.
    property var icons: []

    contentItem: Row {
        leftPadding: 10
        spacing: 7
        Image {
            anchors.verticalCenter: parent.verticalCenter
            visible: source != ""
            source: (cb.currentIndex >= 0 && cb.currentIndex < cb.icons.length) ? cb.icons[cb.currentIndex] : ""
            width: 20; height: 14; sourceSize.height: 28; fillMode: Image.PreserveAspectFit; smooth: true
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: cb.displayText
            color: cb.uiPalette.t1
            font: cb.font
            rightPadding: 30
            elide: Text.ElideRight
        }
    }

    background: Rectangle {
        radius: 7
        color: cb.uiPalette.field
        border.color: cb.activeFocus ? cb.uiPalette.accent : cb.uiPalette.hair
        border.width: 1
    }

    indicator: Canvas {
        x: cb.width - 20
        y: cb.height / 2 - 3
        width: 10
        height: 6
        onPaint: {
            var c = getContext("2d")
            c.reset()
            c.fillStyle = cb.uiPalette.t3
            c.beginPath()
            c.moveTo(0, 0); c.lineTo(width, 0); c.lineTo(width / 2, height); c.closePath()
            c.fill()
        }
    }

    popup: Popup {
        y: cb.height + 4
        width: cb.width
        padding: 4
        background: Rectangle {
            radius: 8
            color: cb.uiPalette.elev
            border.color: cb.uiPalette.hair
            border.width: 1
        }
        contentItem: ListView {
            implicitHeight: contentHeight
            model: cb.popup.visible ? cb.delegateModel : null
            clip: true
            currentIndex: cb.highlightedIndex
        }
    }

    delegate: ItemDelegate {
        width: cb.width - 8
        height: 28
        contentItem: Row {
            leftPadding: 8
            spacing: 7
            Image {
                anchors.verticalCenter: parent.verticalCenter
                visible: source != ""
                source: (index < cb.icons.length) ? cb.icons[index] : ""
                width: 20; height: 14; sourceSize.height: 28; fillMode: Image.PreserveAspectFit; smooth: true
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                // honor textRole for object models ({key,label}); plain string
                // models (no textRole) fall back to modelData unchanged.
                text: (cb.textRole.length > 0 && modelData && modelData[cb.textRole] !== undefined)
                      ? modelData[cb.textRole] : modelData
                color: cb.uiPalette.t1
                font.pixelSize: 12
                font.family: cb.uiPalette.fontSans
            }
        }
        background: Rectangle {
            radius: 5
            color: highlighted ? cb.uiPalette.hover : "transparent"
        }
    }
}
