// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Pieces pane for the detail panel. Data: session.selectedPieces (array of bool)
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: pane
    // { total, done, cells: [0..1] } — bounded/downsampled by the bridge so a
    // huge torrent doesn't ship tens of thousands of entries (crashed on Windows)
    property var pieces: ({})
    spacing: Theme.sp3

    readonly property int total: (pieces && pieces.total) ? pieces.total : 0
    readonly property int doneCount: (pieces && pieces.done) ? pieces.done : 0
    readonly property var cells: (pieces && pieces.cells) ? pieces.cells : []

    Text {
        Layout.topMargin: Theme.sp4
        Layout.leftMargin: Theme.sp5
        text: pane.total === 0
              ? (i18n.language, i18n.t("detailpieces_empty"))
              : pane.doneCount + " / " + pane.total + " " + (i18n.language, i18n.t("word_pieces")) + " (" + (pane.total > 0 ? Math.round(pane.doneCount / pane.total * 100) : 0) + "%)"
        color: Theme.t3
        font.pixelSize: 12
        font.family: Theme.fontSans
    }

    Flickable {
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.leftMargin: Theme.sp5
        Layout.rightMargin: Theme.sp5
        Layout.bottomMargin: Theme.sp4
        clip: true
        contentHeight: pieceGrid.height
        boundsBehavior: Flickable.StopAtBounds

        Grid {
            id: pieceGrid
            width: parent.width
            columns: Math.max(20, Math.floor(width / 11))
            spacing: 2
            Repeater {
                model: pane.cells.length
                delegate: Rectangle {
                    width: (pieceGrid.width - (pieceGrid.columns - 1) * 2) / pieceGrid.columns
                    height: width
                    radius: 2
                    readonly property real f: pane.cells[index] || 0
                    color: f >= 0.999 ? Theme.accent
                         : f > 0 ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.3 + 0.65 * f)
                         : Theme.field
                }
            }
        }
    }
}
