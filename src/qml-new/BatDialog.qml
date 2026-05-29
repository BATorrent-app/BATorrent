import QtQuick
import QtQuick.Layouts
import "."

// Reusable dialog chrome (titlebar + body + footer) faithful to the HTML dialogs.
Window {
    id: dlg
    visible: true
    color: "transparent"
    flags: Qt.FramelessWindowHint
    property alias title: ttl.text
    property int cardW: 480
    property int cardH: 460
    width: cardW + 120; height: cardH + 120
    default property alias content: bodyCol.data
    property string footHint: ""
    signal accepted()
    signal rejected()
    property string okText: "OK"
    readonly property string mono: "Menlo, 'SF Mono', monospace"
    readonly property string sans: "-apple-system, 'SF Pro Text', 'Segoe UI', sans-serif"

    // dim backdrop
    Rectangle { anchors.fill: parent; color: Qt.rgba(0,0,0, Theme.isDark ? 0.5 : 0.32) }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: dlg.cardW; height: dlg.cardH; radius: 13; clip: true
        color: Theme.bg; border.color: Theme.isDark ? Qt.rgba(1,1,1,0.09) : Qt.rgba(0,0,0,0.14); border.width: 1

        ColumnLayout {
            anchors.fill: parent; spacing: 0
            // titlebar
            Rectangle {
                Layout.fillWidth: true; height: 36; color: Theme.elev
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                Row { anchors.verticalCenter: parent.verticalCenter; x: 14; spacing: 8
                    Rectangle { width: 12; height: 12; radius: 6; color: "#ff5f57" }
                    Rectangle { width: 12; height: 12; radius: 6; color: "#febc2e" }
                    Rectangle { width: 12; height: 12; radius: 6; color: "#28c840" }
                }
                Text { id: ttl; anchors.centerIn: parent; color: Theme.t2; font.pixelSize: 12.5; font.bold: true; font.family: dlg.sans }
            }
            // body
            ColumnLayout {
                id: bodyCol
                Layout.fillWidth: true; Layout.fillHeight: true
                Layout.margins: 24; spacing: 16
            }
            // footer
            Rectangle {
                Layout.fillWidth: true; height: 56; color: Theme.elev
                Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
                RowLayout {
                    anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 20
                    Text { text: dlg.footHint; color: Theme.t4; font.pixelSize: 10.5; font.family: dlg.sans }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        radius: 7; height: 33; implicitWidth: c.width + 28; color: "transparent"; border.color: Theme.hair; border.width: 1
                        Text { id: c; anchors.centerIn: parent; text: "Cancelar"; color: Theme.t2; font.pixelSize: 12; font.family: dlg.sans }
                        MouseArea { anchors.fill: parent; onClicked: dlg.rejected() }
                    }
                    Rectangle {
                        radius: 7; height: 33; implicitWidth: o.width + 36; color: Theme.accent; Layout.leftMargin: 8
                        Text { id: o; anchors.centerIn: parent; text: dlg.okText; color: "#fff"; font.pixelSize: 12; font.bold: true; font.family: dlg.sans }
                        MouseArea { anchors.fill: parent; onClicked: dlg.accepted() }
                    }
                }
            }
        }
    }
}
