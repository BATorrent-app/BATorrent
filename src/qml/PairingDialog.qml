// WebUI pairing: shows LAN URL + QR for the configured port. `pairing` bridge.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Parear celular"
    cardW: 460
    cardH: 560
    okText: "Fechar"
    showCancel: false

    readonly property var api: typeof pairing !== "undefined" ? pairing : null
    property string copyLabel: "Copiar URL"
    property var rows: api ? api.qrRows() : []

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: "PAREAMENTO"; red: true }
        Text {
            text: "Parear celular"
            color: Theme.t1
            font.pointSize: 19; font.weight: Font.DemiBold; font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }
        Text {
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: "Abra a câmera do celular e aponte para o QR, ou digite a URL no navegador. O aparelho precisa estar na mesma rede."
            color: Theme.t3
            font.pointSize: 12
            font.family: Theme.fontSans
        }
    }

    // no LAN
    Text {
        visible: !dlg.api || !dlg.api.available
        Layout.fillWidth: true
        wrapMode: Text.WordWrap
        text: "Nenhuma interface de rede ativa encontrada. Conecte-se ao Wi-Fi ou à rede cabeada."
        color: Theme.accentText
        font.pointSize: 12
        font.family: Theme.fontSans
    }

    // QR (rendered from bridge matrix rows)
    Rectangle {
        visible: dlg.api && dlg.api.available && dlg.rows.length > 0
        Layout.alignment: Qt.AlignHCenter
        width: 232; height: 232
        radius: 8
        color: "white"
        Column {
            anchors.centerIn: parent
            property int mods: dlg.rows.length
            property real cell: mods > 0 ? 200 / mods : 0
            Repeater {
                model: dlg.rows
                delegate: Row {
                    required property var modelData
                    Repeater {
                        model: modelData.length
                        delegate: Rectangle {
                            required property int index
                            width: parent.parent.cell
                            height: parent.parent.cell
                            color: modelData.charAt(index) === "1" ? "black" : "white"
                        }
                    }
                }
            }
        }
    }

    // URL box
    Rectangle {
        visible: dlg.api && dlg.api.available
        Layout.fillWidth: true
        implicitHeight: 56
        radius: 8
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        Text {
            anchors.centerIn: parent
            text: dlg.api ? dlg.api.url : ""
            color: Theme.t1
            font.pointSize: 15; font.weight: Font.Bold
            font.family: Theme.fontMono
        }
    }

    RowLayout {
        visible: dlg.api && dlg.api.available
        Layout.fillWidth: true
        spacing: Theme.sp2
        BtnFlat { text: dlg.copyLabel; onClicked: { if (dlg.api) { dlg.api.copyUrl(); dlg.copyLabel = "Copiado!" } } }
        Item { Layout.fillWidth: true }
        BtnFlat { primary: true; text: "Abrir no navegador"; onClicked: if (dlg.api) dlg.api.openBrowser() }
    }
}
