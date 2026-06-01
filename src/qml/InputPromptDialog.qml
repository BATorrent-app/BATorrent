// Reusable single-field prompt (rate limits, category, tag, tracker, …).
// Open via openWith(title, label, value, placeholder, callback); the trimmed
// text is passed to the callback when the user confirms.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    cardW: 440
    cardH: 220
    okText: (i18n.language, i18n.t("btn_ok"))

    property string promptLabel: ""
    property string promptPlaceholder: ""
    property var onAccept: null

    onAccepted: if (onAccept) onAccept(field.text.trim())

    function openWith(t, label, value, placeholder, cb) {
        dlg.title = t
        dlg.promptLabel = label || ""
        dlg.promptPlaceholder = placeholder || ""
        field.text = value || ""
        dlg.onAccept = cb
        dlg.open()
        field.forceActiveFocus()
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 8

        Text {
            text: dlg.promptLabel
            visible: dlg.promptLabel.length > 0
            color: Theme.t3
            font.pointSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
        }

        TFld {
            id: field
            Layout.fillWidth: true
            placeholder: dlg.promptPlaceholder
        }
    }
}
