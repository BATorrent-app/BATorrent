// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The one status mark. A state gets a symbol and a colour here and nowhere
// else, so the grid badge and the list row cannot end up saying the same fact
// with different glyphs — which is what happened when each view built its own
// arrow inline.
//
// The direction carries the meaning on its own: down is coming in, up is going
// out, a tick is finished. Colour only reinforces it, so the mark still reads
// for someone who cannot separate the reds from the greens.
import QtQuick
import "../theme"

Row {
    id: mark
    property string stateKey: ""
    property string label: ""
    // Grid draws over artwork and needs its own light-on-dark palette; the list
    // sits on the panel and uses the theme's state colours.
    property bool onArtwork: false
    property int symbolSize: 13
    property int labelSize: 10
    // 0 = take whatever the label needs. A grid badge shares the tile's top
    // edge with the year/category pill and has to give way instead of running
    // under it, so it caps this and lets the label elide.
    property int maxLabelWidth: 0
    // Nominally downloading, but nothing is arriving and the engine has a
    // reason (no peers, checking, awaiting metadata). The state is unchanged,
    // so the symbol is; only the colour drops to amber to say "stalled".
    property bool stalled: false

    readonly property string symbol:
          stateKey === "seeding"   ? "↑"
        : stateKey === "completed" ? "✓"
        : stateKey === "queued"    ? "⋯"
        : stateKey === "paused"    ? "‖"
        : stateKey === "missing"   ? "✗"
        : stateKey === "error"     ? "!"
        : "↓"

    readonly property color symbolColor:
          stalled                  ? Theme.amber
        : stateKey === "completed" ? Theme.grn
        : stateKey === "seeding"   ? Theme.amber
        : (stateKey === "paused" || stateKey === "queued")
                                   ? (onArtwork ? "#c9c9cf" : Theme.t3)
        : Theme.accent

    spacing: 5

    Text {
        anchors.verticalCenter: parent.verticalCenter
        text: mark.symbol
        color: mark.symbolColor
        font.pixelSize: mark.symbolSize
        font.weight: Font.Bold
        font.family: Theme.fontSans
    }
    Text {
        anchors.verticalCenter: parent.verticalCenter
        visible: mark.label.length > 0
        text: mark.label
        color: mark.onArtwork ? "#ffffff" : mark.symbolColor
        opacity: mark.onArtwork ? 0.92 : 1.0
        font.pixelSize: mark.labelSize
        font.weight: Font.Bold
        font.capitalization: Font.AllUppercase
        font.letterSpacing: 0.5
        font.family: Theme.fontSans
        width: mark.maxLabelWidth > 0
               ? Math.min(implicitWidth, mark.maxLabelWidth) : implicitWidth
        elide: Text.ElideRight
    }
}
