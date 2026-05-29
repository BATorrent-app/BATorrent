// Source: bat-dialog.css .chk — checkbox 17×17.
// off: bg field; border 1 hair; check oculto.
// on:  bg accent; border transparent; check svg 11px #fff opacity 1.
// partial: bg field; border Theme.accent; traço central 8×2 Theme.accent.
import QtQuick
import "../theme"

Rectangle {
    id: cb
    property bool on: false
    property bool partial: false
    signal toggled(bool on)

    implicitWidth: 17
    implicitHeight: 17
    radius: 5
    color: on ? Theme.accent : Theme.field
    border.color: partial ? Theme.accent : (on ? "transparent" : Theme.hair)
    border.width: 1

    // partial bar
    Rectangle {
        visible: cb.partial && !cb.on
        anchors.centerIn: parent
        width: 8
        height: 2
        color: Theme.accent
    }

    // check (path "M20 6 9 17l-5-5") — desenhamos como duas linhas via Canvas pra ficar fiel
    Canvas {
        anchors.fill: parent
        anchors.margins: 3
        visible: cb.on
        onPaint: {
            var c = getContext("2d")
            c.reset()
            c.strokeStyle = "#ffffff"
            c.lineWidth = 2.2
            c.lineCap = "round"
            c.lineJoin = "round"
            c.beginPath()
            c.moveTo(width * 0.18, height * 0.55)
            c.lineTo(width * 0.42, height * 0.78)
            c.lineTo(width * 0.86, height * 0.28)
            c.stroke()
        }
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: { cb.partial = false; cb.on = !cb.on; cb.toggled(cb.on) }
    }
}
