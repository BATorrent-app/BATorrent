// Source: bat-dialog.css .eyebrow — small UPPERCASE caption.
// 9px / weight 700 / letter-spacing 1.8 / Theme.t4 / UPPERCASE.
// .eyebrow.red → color Theme.accent (mais saturado que t4).
import QtQuick
import "../theme"

Text {
    property bool red: false
    color: red ? Theme.accent : Theme.t4
    font.pointSize: 9
    font.weight: Font.Bold
    font.letterSpacing: 1.8
    font.family: Theme.fontSans
    font.capitalization: Font.AllUppercase
}
