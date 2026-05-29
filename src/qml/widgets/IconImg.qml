// Source: bat-dialog.css / batorrent-home.css — pequenos ícones SVG tingidos.
import QtQuick
import QtQuick.Effects

Item {
    id: ico
    property string src
    property color tint: "#9ca3af"
    property int s: 16
    implicitWidth: s
    implicitHeight: s

    Image {
        id: imgSrc
        anchors.fill: parent
        source: ico.src
        sourceSize: Qt.size(ico.s * 2, ico.s * 2)
        fillMode: Image.PreserveAspectFit
        visible: false
        layer.enabled: true
    }
    MultiEffect {
        source: imgSrc
        anchors.fill: imgSrc
        colorization: 1.0
        colorizationColor: ico.tint
    }
}
