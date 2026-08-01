// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Meta column under a Library PosterTile: state/speed line + downloaded-of-total.
import QtQuick
import "../theme"

Column {
    id: root
    required property var tile
    required property var win

    spacing: 2

    Item {
        width: root.width
        height: 16
        Row {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            Rectangle {
                width: 7
                height: 7
                radius: 3.5
                anchors.verticalCenter: parent.verticalCenter
                color: (tile.isDownloading && tile.stateDetail.length > 0) ? Theme.amber : win.dotFor(tile.stateKey)
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: tile.isDownloading ? ("↓ " + tile.downSpeed)
                      : (tile.hasBadge && tile.metaLine.length > 0) ? tile.metaLine
                      : (tile.stateKey === "seeding"
                         ? ((i18n.language, i18n.t("state_seeding")) + " · ↑ " + tile.upSpeed)
                         : (tile.progress >= 0.999 && tile.stateKey === "paused")
                         ? (i18n.language, i18n.t("state_paused"))
                         : tile.stateString)
                color: (tile.isDownloading && tile.stateDetail.length > 0) ? Theme.amber
                       : (tile.hasBadge && tile.metaLine.length > 0) ? Theme.t4
                       : win.textFor(tile.stateKey)
                font.pixelSize: 13
                // Medium is a bundled IBM Plex face, not a synthesised weight —
                // it buys legibility at this size without another pixel of line
                // height, which the tile has no room for.
                font.weight: Font.Medium
                font.family: Theme.fontSans
                width: Math.min(implicitWidth, root.width - 12 - rightTxt.width - 10)
                elide: Text.ElideRight
            }
        }
        Text {
            id: rightTxt
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: tile.isDownloading ? (tile.etaSec >= 0 ? win.fmtEta(tile.etaSec) : "") : tile.size
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
    }
    Text {
        width: root.width
        horizontalAlignment: Text.AlignRight
        visible: tile.isDownloading
        text: tile.downloaded + " " + (i18n.language, i18n.t("word_of")) + " " + tile.size
        color: Theme.t4
        font.pixelSize: 11
        font.family: Theme.fontSans
        font.features: Theme.tnum
    }
}
