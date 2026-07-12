// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Eyebrow-titled key/value column (Storage/Transfer/Health in the detail
// surfaces). model: [{ kk: i18nKey, v: value }].
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: sec
    property string title
    property var model: []
    property int valueMaxWidth: 110
    spacing: 5

    Text {
        text: sec.title
        color: Theme.t4
        font.pixelSize: 10
        font.weight: Font.Bold
        font.letterSpacing: 0.8
        font.capitalization: Font.AllUppercase
        font.family: Theme.fontSans
        Layout.bottomMargin: Theme.sp2
    }
    Repeater {
        model: sec.model
        delegate: RowLayout {
            Layout.fillWidth: true
            Text { text: (i18n.language, i18n.t(modelData.kk)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
            Item { Layout.fillWidth: true }
            Text {
                text: modelData.v
                color: Theme.t1
                font.pixelSize: 12
                font.family: Theme.fontSans
                font.features: Theme.tnum
                elide: Text.ElideMiddle
                Layout.maximumWidth: sec.valueMaxWidth
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}
