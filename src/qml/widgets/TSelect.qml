// Source: batorrent-settings.css .select / bat-dialog .select
// height 30, appearance none, padding 0 30 0 10, bg Theme.field, border 1px Theme.hair, radius 7.
// caret triângulo Theme.t3 à direita. :focus → border Theme.accent.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

ComboBox {
    id: cb
    implicitHeight: 30
    font.pointSize: 12
    font.family: Theme.fontSans

    contentItem: Text {
        text: cb.displayText
        color: Theme.t1
        font: cb.font
        verticalAlignment: Text.AlignVCenter
        leftPadding: 10
        rightPadding: 30
        elide: Text.ElideRight
    }

    background: Rectangle {
        radius: 7
        color: Theme.field
        border.color: cb.activeFocus ? Theme.accent : Theme.hair
        border.width: 1
    }

    indicator: Canvas {
        x: cb.width - 20
        y: cb.height / 2 - 3
        width: 10
        height: 6
        onPaint: {
            var c = getContext("2d")
            c.reset()
            c.fillStyle = Theme.t3
            c.beginPath()
            c.moveTo(0, 0); c.lineTo(width, 0); c.lineTo(width / 2, height); c.closePath()
            c.fill()
        }
    }

    popup: Popup {
        y: cb.height + 4
        width: cb.width
        padding: 4
        background: Rectangle {
            radius: 8
            color: Theme.elev
            border.color: Theme.hair
            border.width: 1
        }
        contentItem: ListView {
            implicitHeight: contentHeight
            model: cb.popup.visible ? cb.delegateModel : null
            clip: true
            currentIndex: cb.highlightedIndex
        }
    }

    delegate: ItemDelegate {
        width: cb.width - 8
        height: 28
        contentItem: Text {
            text: modelData
            color: Theme.t1
            font.pointSize: 12
            font.family: Theme.fontSans
            verticalAlignment: Text.AlignVCenter
            leftPadding: 8
        }
        background: Rectangle {
            radius: 5
            color: highlighted ? Theme.hover : "transparent"
        }
    }
}
