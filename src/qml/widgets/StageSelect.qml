// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A dropdown on a title's stage: a dark box, or bare text when it sits inside
// a sentence ("2014 · 8 episodes   4K ▾").
import QtQuick
import "../theme"

Item {
    id: sel
    // [{ value, label }]
    property var options: []
    property var value
    property bool bare: false
    property int fontSize: 12
    signal picked(var value)

    readonly property string label: {
        for (var i = 0; i < options.length; i++)
            if (options[i].value === value) return options[i].label
        return ""
    }

    implicitHeight: bare ? txt.implicitHeight : 30
    implicitWidth: txt.implicitWidth + (bare ? 14 : 36)

    Rectangle {
        anchors.fill: parent
        visible: !sel.bare
        radius: 4
        color: Theme.stageField
        border.width: 1
        border.color: ma.containsMouse ? Theme.stageGhost : Theme.stageEdge
        Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
    }
    Text {
        id: txt
        anchors.left: parent.left
        anchors.leftMargin: sel.bare ? 0 : 10
        anchors.verticalCenter: parent.verticalCenter
        text: sel.label
        color: sel.bare ? (ma.containsMouse ? Theme.stageT1 : Theme.stageT2) : Theme.stageT1
        font.pixelSize: sel.fontSize
        font.weight: sel.bare ? Font.Normal : Font.Medium
        font.family: Theme.fontSans
    }
    IconImg {
        anchors.right: parent.right
        anchors.rightMargin: sel.bare ? 0 : 9
        anchors.verticalCenter: parent.verticalCenter
        src: "qrc:/icons/chevron.svg"
        tint: Theme.stageT2
        s: 10
    }
    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: menu.popup(sel, 0, sel.height + 4)
    }
    BatMenu {
        id: menu
        implicitWidth: Math.max(160, sel.width)
        Repeater {
            model: sel.options
            BatMenuItem {
                required property var modelData
                text: modelData.label
                onTriggered: sel.picked(modelData.value)
            }
        }
    }
}
