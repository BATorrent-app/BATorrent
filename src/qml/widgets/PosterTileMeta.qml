// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Meta column under a Library PosterTile: what the release is, then how much of
// it there is. The state used to be repeated here as a dot plus a label while
// the badge over the artwork said the same thing — one status, one place, so
// the status now lives only in the badge and this line is free to carry what
// the tile could not show before.
//
// Genres in particular: they were gated behind hasBadge, so a film only
// admitted to being a thriller once it had finished downloading. They are known
// the moment metadata resolves and are shown from that moment on.
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
        Text {
            id: leftTxt
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            // A torrent that is stuck says why, in the short form written for
            // this width; the engine's sentence stays for the tooltip. Genres
            // are catalogue trivia next to "nobody is connected", so they only
            // get the line when nothing is wrong.
            readonly property bool hasTrouble: tile.stateDetailShort.length > 0
            readonly property bool hasGenres: tile.genres.length > 0
            text: hasTrouble ? tile.stateDetailShort
                  : hasGenres ? tile.genres
                  : tile.isDownloading ? ("↓ " + tile.downSpeed)
                  : tile.stateKey === "seeding" ? ("↑ " + tile.upSpeed)
                  : ""
            // Amber is for a transfer that wants to move and cannot. Nobody
            // asking for what you are seeding is a fact, not a fault, and a
            // magnet still looking is slow rather than broken.
            color: hasTrouble ? (tile.isDownloading ? Theme.amber : Theme.t3)
                   : (hasGenres ? Theme.t3 : Theme.t4)
            font.pixelSize: 13
            // Medium is a bundled IBM Plex face, not a synthesised weight —
            // it buys legibility at this size without another pixel of line
            // height, which the tile has no room for.
            font.weight: Font.Medium
            font.family: Theme.fontSans
            width: Math.min(implicitWidth, root.width - rightTxt.width - 10)
            elide: Text.ElideRight
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
}
