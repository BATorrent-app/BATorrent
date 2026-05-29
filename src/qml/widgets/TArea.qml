// Source: bat-dialog.css textarea.ta — multiline.
// min-height 88; padding 11; bg Theme.field; border 1px Theme.hair; radius 8.
// fontMono 11.5; line-height 1.5. :focus → border Theme.accent.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

Rectangle {
    id: ta
    property alias text: input.text
    property alias placeholder: input.placeholderText

    implicitWidth: 400
    implicitHeight: 88
    radius: 8
    color: Theme.field
    border.color: input.activeFocus ? Theme.accent : Theme.hair
    border.width: 1

    ScrollView {
        anchors.fill: parent
        anchors.margins: 11
        clip: true

        TextArea {
            id: input
            color: Theme.t1
            placeholderTextColor: Theme.t4
            font.pointSize: 11.5
            font.family: Theme.fontMono
            wrapMode: TextArea.Wrap
            background: null
            padding: 0
        }
    }
}
