// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Multi-line text field.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Rectangle {
    id: ta
    property alias text: input.text
    property alias placeholder: input.placeholderText

    implicitWidth: 400
    implicitHeight: 88
    radius: 8
    color: Theme.field
    border.color: input.activeFocus ? Theme.accent : Theme.hair
    border.width: 1

    ScrollView {
        anchors.fill: parent
        anchors.margins: 11
        clip: true

        TextArea {
            id: input
            color: Theme.t1
            placeholderTextColor: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontMono
            wrapMode: TextArea.Wrap
            background: null
            padding: 0
        }
    }
}
