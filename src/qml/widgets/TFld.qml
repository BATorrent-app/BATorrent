// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Single-line text input.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: fld
    property string icon: ""
    property alias text: input.text
    property alias placeholder: input.placeholderText
    property alias field: input   // inner TextField, so callers can focus it directly
    property bool mono: false
    property bool readonly: false
    property bool password: false
    property bool clearable: false   // shows an ✕ to empty the field when it has text
    property string hint: ""         // a key to press, in grey before the ✕ ("Enter")
    // optional appearance overrides: let a hero/landing field scale up without a
    // second component (defaults preserve every existing call site)
    property real fontSize: mono ? 12 : 13
    property real iconSize: 14
    signal edited(string text)

    implicitWidth: 240
    implicitHeight: 34
    radius: Theme.rad(8)
    color: Theme.field
    // focus is a neutral ring: red is for acting
    border.color: input.activeFocus ? Theme.fieldFocus : Theme.hair
    border.width: input.activeFocus ? 1.5 : 1

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
            s: fld.iconSize
        }
        TextField {
            id: input
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.t1
            placeholderTextColor: Theme.t4
            font.pixelSize: fld.fontSize
            font.family: fld.mono ? Theme.fontMono : Theme.fontSans
            readOnly: fld.readonly
            echoMode: fld.password ? TextInput.Password : TextInput.Normal
            verticalAlignment: TextInput.AlignVCenter
            background: null
            padding: 0
            onEditingFinished: fld.edited(text)
        }
        Text {
            visible: fld.hint !== ""
            Layout.alignment: Qt.AlignVCenter
            text: fld.hint
            color: Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans
        }
        IconImg {
            visible: fld.clearable && input.text.length > 0
            Layout.alignment: Qt.AlignVCenter
            src: "qrc:/icons/close.svg"
            tint: clearMa.containsMouse ? Theme.t1 : Theme.t4
            s: 13
            MouseArea {
                id: clearMa
                anchors.fill: parent
                anchors.margins: -5
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: { input.text = ""; input.forceActiveFocus(); fld.edited("") }
            }
        }
    }
}
