// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: bat-dialog.css .path — Row gap 8: .field (flex 1, mono) + browse button.
import QtQuick
import QtQuick.Layouts
import "../theme"

RowLayout {
    id: pth
    property alias text: fld.text
    property alias placeholder: fld.placeholder
    property string browseLabel: (i18n.language, i18n.t("pathfld_browse"))
    signal browseClicked()

    spacing: Theme.sp2

    TFld {
        id: fld
        Layout.fillWidth: true
        mono: true
    }
    BtnFlat {
        text: pth.browseLabel
        onClicked: pth.browseClicked()
    }
}
