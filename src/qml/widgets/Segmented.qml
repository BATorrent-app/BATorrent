// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A segmented choice: a field-coloured track, the chosen option lit. Options
// are { value, label, disabled, tip }; a label may carry a little styled text
// (a count in a dimmer colour). A disabled option says why in its tooltip.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Rectangle {
    id: seg
    property var options: []
    property var current
    property int optionHeight: 28
    property int fontSize: 13
    signal picked(var value)

    implicitWidth: row.implicitWidth + 6
    implicitHeight: optionHeight + 6
    radius: Theme.rad(8)
    color: Theme.field
    border.color: Theme.hair
    border.width: 1

    Row {
        id: row
        x: 3; y: 3
        spacing: 2
        Repeater {
            model: seg.options
            delegate: Rectangle {
                id: opt
                required property var modelData
                readonly property bool on: seg.current === modelData.value && !modelData.disabled
                width: lbl.implicitWidth + 22
                height: seg.optionHeight
                radius: Math.max(0, Theme.rad(8) - 2)
                color: on ? Theme.segOn : "transparent"
                opacity: modelData.disabled ? 0.4 : 1
                Behavior on color { ColorAnimation { duration: Theme.durBase } }
                Text {
                    id: lbl
                    anchors.centerIn: parent
                    text: opt.modelData.label
                    textFormat: Text.StyledText
                    color: opt.on ? Theme.t1 : Theme.t3
                    font.pixelSize: seg.fontSize
                    font.weight: opt.on ? Font.DemiBold : Font.Normal
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
                MouseArea {
                    id: ma
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: opt.modelData.disabled ? Qt.ArrowCursor : Qt.PointingHandCursor
                    onClicked: if (!opt.modelData.disabled) seg.picked(opt.modelData.value)
                }
                ToolTip.visible: !!opt.modelData.tip && ma.containsMouse
                ToolTip.text: opt.modelData.tip || ""
            }
        }
    }
}
