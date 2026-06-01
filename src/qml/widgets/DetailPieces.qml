// Pieces pane for the detail panel. Data: session.selectedPieces (array of bool)
import QtQuick
import QtQuick.Layouts
import "../theme"

ColumnLayout {
    id: pane
    property var pieces: []
    spacing: Theme.sp3

    readonly property int total: pieces.length
    readonly property int doneCount: {
        var n = 0
        for (var i = 0; i < pieces.length; ++i) if (pieces[i]) n++
        return n
    }

    Text {
        Layout.topMargin: Theme.sp4
        Layout.leftMargin: Theme.sp5
        text: pane.total === 0
              ? (i18n.language, i18n.t("detailpieces_empty"))
              : pane.doneCount + " / " + pane.total + " pedaços (" + (pane.total > 0 ? Math.round(pane.doneCount / pane.total * 100) : 0) + "%)"
        color: Theme.t3
        font.pointSize: 11.5
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
                model: Math.min(pane.total, 2000)
                delegate: Rectangle {
                    width: (pieceGrid.width - (pieceGrid.columns - 1) * 2) / pieceGrid.columns
                    height: width
                    radius: 2
                    color: pane.pieces[index] ? Theme.accent : Theme.field
                }
            }
        }
    }
}
