// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: bat-dialog.css .chip — pill pequena.
// font 10 mono / Theme.t3; bg field; border 1 hair; padding 2 8; radius 999.
// .chip.red → color Theme.accentText; bg Theme.sel; border rgba(229,51,43,0.3).
import QtQuick
import "../theme"

Rectangle {
    id: chip
    property string text
    property bool red: false

    implicitWidth: lbl.implicitWidth + 16
    implicitHeight: 18
    radius: 999
    color: red ? Theme.sel : Theme.field
    border.color: red ? Qt.rgba(229/255, 51/255, 43/255, 0.3) : Theme.hair
    border.width: 1

    Text {
        id: lbl
        anchors.centerIn: parent
        text: chip.text
        color: chip.red ? Theme.accentText : Theme.t3
        font.pixelSize: 10
        font.family: Theme.fontMono
    }
}
