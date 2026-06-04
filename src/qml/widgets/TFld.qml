// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: bat-dialog.css .field — input row.
// height 34; padding 0 11; gap 8; bg Theme.field; border 1px Theme.hair; radius 8.
// :focus-within → border Theme.accent. ícone 14px Theme.t4; input 12.5 Theme.t1; placeholder Theme.t4.
// .mono → fontMono 11.5.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: fld
    property string icon: ""
    property alias text: input.text
    property alias placeholder: input.placeholderText
    property bool mono: false
    property bool readonly: false
    property bool password: false
    signal edited(string text)

    implicitWidth: 240
    implicitHeight: 34
    radius: 8
    color: Theme.field
    border.color: input.activeFocus ? Theme.accent : Theme.hair
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 11
        anchors.rightMargin: 11
        spacing: 8

        IconImg {
            visible: fld.icon !== ""
            Layout.alignment: Qt.AlignVCenter
            src: fld.icon
            tint: Theme.t4
            s: 14
        }
        TextField {
            id: input
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.t1
            placeholderTextColor: Theme.t4
            font.pixelSize: fld.mono ? 12 : 13
            font.family: fld.mono ? Theme.fontMono : Theme.fontSans
            readOnly: fld.readonly
            echoMode: fld.password ? TextInput.Password : TextInput.Normal
            verticalAlignment: TextInput.AlignVCenter
            background: null
            padding: 0
            onEditingFinished: fld.edited(text)
        }
    }
}
