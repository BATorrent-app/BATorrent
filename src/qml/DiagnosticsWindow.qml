// Network diagnostics snapshot. session.diagnostics() (QmlSessionBridge).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 560
    height: 460
    minimumWidth: 480
    minimumHeight: 380
    color: Theme.bg
    title: "Diagnóstico de rede"

    readonly property var sess: typeof session !== "undefined" ? session : null
    property var diag: ({})
    function run() { if (sess) diag = sess.diagnostics() }
    onVisibleChanged: if (visible) run()

    component Row4: RowLayout {
        property string label: ""
        property bool ok: false
        property string value: ""
        Layout.fillWidth: true
        spacing: 12
        Rectangle {
            Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
            color: ok ? Theme.grn : Theme.t4
        }
        Text { Layout.preferredWidth: 150; text: label; color: Theme.t3; font.pointSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        Text { Layout.fillWidth: true; text: value; color: ok ? Theme.t1 : Theme.t3; font.pointSize: 12; font.family: Theme.fontSans }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: "Diagnóstico de rede"; color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.sp6
            spacing: Theme.sp4

            Eyebrow { text: "REDE"; red: true }
            Text {
                text: "Status da conexão"
                color: Theme.t1
                font.pointSize: 18; font.weight: Font.Bold; font.letterSpacing: -0.3
                font.family: Theme.fontSans
                Layout.bottomMargin: Theme.sp2
            }

            Rectangle {
                Layout.fillWidth: true
                radius: 11
                color: Theme.panel
                border.color: Theme.hair
                border.width: 1
                implicitHeight: rows.implicitHeight + 2 * Theme.sp4
                ColumnLayout {
                    id: rows
                    anchors.left: parent.left; anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.leftMargin: Theme.sp4; anchors.rightMargin: Theme.sp4
                    anchors.topMargin: Theme.sp4
                    spacing: Theme.sp3
                    Row4 { label: "Escuta"; ok: win.diag.listenOk === true; value: win.diag.listenText || "—" }
                    Row4 { label: "DHT"; ok: win.diag.dhtOk === true; value: win.diag.dhtText || "—" }
                    Row4 { label: "NAT / Entrada"; ok: win.diag.natOk === true; value: win.diag.natText || "—" }
                    Row4 { label: "Alcance da porta"; ok: win.diag.portOk === true; value: win.diag.portText || "—" }
                }
            }

            Item { Layout.fillHeight: true }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                BtnFlat { text: "Executar testes"; onClicked: win.run() }
                Item { Layout.fillWidth: true }
                BtnFlat { primary: true; text: "Fechar"; onClicked: win.close() }
            }
        }
    }
}
