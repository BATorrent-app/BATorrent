import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Item {
    id: tab
    property var pieces: []

    readonly property int total: pieces.length
    readonly property int done: {
        var n = 0
        for (var i = 0; i < pieces.length; ++i) if (pieces[i]) n++
        return n
    }
    readonly property real cellSize: {
        if (total <= 0) return 6
        var availW = width - 16
        var availH = height - 32
        if (availW <= 0 || availH <= 0) return 6
        var s = Math.floor(Math.sqrt((availW * availH) / total))
        return Math.max(3, Math.min(14, s))
    }
    readonly property int colsCount: total > 0 ? Math.max(1, Math.floor((width - 16) / cellSize)) : 0

    Column {
        anchors.fill: parent
        spacing: 6
        padding: 8

        Label {
            text: tab.total === 0
                ? qsTr("No piece data")
                : qsTr("%1 / %2 pieces (%3%)")
                    .arg(tab.done).arg(tab.total)
                    .arg((tab.total > 0 ? (tab.done / tab.total * 100) : 0).toFixed(1))
            color: Theme.muted
            font.pixelSize: Theme.fontCaption
        }

        Item {
            width: tab.width - 16
            height: tab.height - 32

            Grid {
                visible: tab.total > 0
                columns: tab.colsCount
                spacing: 1

                Repeater {
                    model: Math.min(tab.total, tab.colsCount * Math.floor((tab.height - 32) / (tab.cellSize + 1)) + tab.colsCount)
                    Rectangle {
                        width: tab.cellSize
                        height: tab.cellSize
                        color: tab.pieces[index] ? Theme.stateSeeding : Qt.rgba(Theme.border.r, Theme.border.g, Theme.border.b, 0.35)
                        radius: 1
                    }
                }
            }
        }
    }
}
