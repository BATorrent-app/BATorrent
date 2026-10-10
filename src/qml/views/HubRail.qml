// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "../theme"
import "../widgets"

Column {
    id: rail
    property var hub
    property string title
    property string kind
    property var items: []
    signal activated(var item)
    signal context(var item)
    visible: items.length > 0
    spacing: 14
    topPadding: 24

    Item {
        x: 24
        width: rail.width - 48
        height: head.implicitHeight
        Text {
            id: head
            text: rail.title
            color: Theme.t1
            font.pixelSize: 17
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        Text {
            anchors.left: head.right
            anchors.leftMargin: 12
            anchors.baseline: head.baseline
            text: (i18n.language, i18n.t("hub_n_titles")).arg(rail.items.length)
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
    }

    ListView {
        id: list
        width: rail.width
        height: 252 * 9 / 16 + 9 + 18 + 16
        orientation: ListView.Horizontal
        spacing: 10
        leftMargin: 24
        rightMargin: 24
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: rail.items
        delegate: HubTile {
            required property var modelData
            width: 252
            hub: rail.hub
            item: modelData
            kind: rail.kind
            onActivated: rail.activated(modelData)
            onContext: rail.context(modelData)
        }
    }
}
