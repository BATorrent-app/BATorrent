// Source: BATorrent List Thumbs.html .thumb — poster thumbnail for list rows.
// 30×40 (3:4), radius 5, object-fit cover. Fallback: initial letter centered.
import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    id: thumb
    property string posterUrl: ""
    property string letter: ""
    implicitWidth: 30
    implicitHeight: 40

    // base (also the placeholder bg)
    Rectangle {
        anchors.fill: parent
        radius: 5
        color: "#161618"
        border.color: Theme.hair
        border.width: 1
        Image {
            anchors.centerIn: parent
            visible: thumb.posterUrl === ""
            width: parent.width * 0.55
            height: width
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(width * 2, width * 2)
            fillMode: Image.PreserveAspectFit
            opacity: 0.5
        }
    }

    // poster image (masked rounded)
    Rectangle {
        id: imgContent
        anchors.fill: parent
        color: "#161618"
        visible: false
        layer.enabled: true
        Image {
            anchors.fill: parent
            source: thumb.posterUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
        }
    }
    Rectangle {
        id: imgMask
        anchors.fill: parent
        radius: 5
        color: "white"
        visible: false
        layer.enabled: true
    }
    MultiEffect {
        source: imgContent
        anchors.fill: parent
        maskEnabled: true
        maskSource: imgMask
        visible: thumb.posterUrl !== ""
    }
}
