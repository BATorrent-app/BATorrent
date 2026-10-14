// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Eyebrow-titled key/value column (Storage/Transfer/Health in the detail
// surfaces). model: [{ kk: i18nKey, v: value }].
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"

ColumnLayout {
    id: sec
    property string title
    property var model: []
    property int valueMaxWidth: 110   // unused: values take the column and elide
    // the bottom panel: 13px rows with more air; the inspector: 12px, tight
    property bool roomy: false
    spacing: roomy ? 9 : 5

    Text {
        text: sec.title
        color: Theme.t4
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.letterSpacing: 1.0
        font.capitalization: Font.AllUppercase
        font.family: Theme.fontSans
    }
    Repeater {
        model: sec.model
        delegate: RowLayout {
            Layout.fillWidth: true
            spacing: sec.roomy ? 16 : 12
            Text { text: (i18n.language, i18n.t(modelData.kk)); color: Theme.t3; font.pixelSize: sec.roomy ? 13 : 13; font.family: Theme.fontSans }
            Text {
                Layout.fillWidth: true
                text: modelData.v
                color: Theme.t1
                font.pixelSize: 13
                font.family: Theme.fontSans
                font.features: Theme.tnum
                elide: Text.ElideRight
                horizontalAlignment: Text.AlignRight
                MouseArea { id: vMa; anchors.fill: parent; acceptedButtons: Qt.NoButton; hoverEnabled: parent.truncated }
                ToolTip.visible: vMa.containsMouse
                ToolTip.text: modelData.v
                ToolTip.delay: 400
            }
        }
    }
}
