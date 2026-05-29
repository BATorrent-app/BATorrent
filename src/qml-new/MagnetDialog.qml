import QtQuick
import QtQuick.Layouts
import "."

BatDialog {
    title: "Adicionar link magnet"; cardW: 480; cardH: 470
    okText: "Adicionar"; footHint: "Aceita vários links, um por linha"
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
        Column { spacing: 4
            Text { text: "ADICIONAR"; color: Theme.accent; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.8 }
            Text { text: "Link magnet"; color: Theme.t1; font.pixelSize: 19; font.bold: true; font.family: sans }
        }
        Column { Layout.fillWidth: true; spacing: 7
            Text { text: "Cole o link magnet"; color: Theme.t3; font.pixelSize: 11; font.bold: true }
            Rectangle { width: parent.width; height: 88; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                Text { anchors.fill: parent; anchors.margins: 11; wrapMode: Text.WrapAnywhere; color: Theme.t1; font.pixelSize: 11.5; font.family: mono
                    text: "magnet:?xt=urn:btih:657c6a58e3b1f24c9a7d42e7&dn=Forza.Horizon.6-CODEX" } }
        }
        Rectangle { Layout.fillWidth: true; radius: 9; color: Theme.panel; border.color: Theme.hair; border.width: 1; implicitHeight: 44
            Text { anchors.fill: parent; anchors.margins: 11; verticalAlignment: Text.AlignVCenter; color: Theme.t3; font.pixelSize: 11; wrapMode: Text.WordWrap
                text: "Os metadados (nome e arquivos) serão baixados da rede após adicionar." } }
        Column { Layout.fillWidth: true; spacing: 7
            Text { text: "Salvar em"; color: Theme.t3; font.pixelSize: 11; font.bold: true }
            RowLayout { width: parent.width; spacing: 8
                Rectangle { Layout.fillWidth: true; height: 34; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                    Text { anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter; text: "/Users/voce/Downloads"; color: Theme.t1; font.pixelSize: 11.5; font.family: mono } }
                Rectangle { width: 92; height: 34; radius: 7; color: Theme.field; border.color: Theme.hair; border.width: 1
                    Text { anchors.centerIn: parent; text: "Procurar…"; color: Theme.t2; font.pixelSize: 12 } } }
        }
        RowLayout { Layout.fillWidth: true
            Text { text: "Iniciar imediatamente"; color: Theme.t1; font.pixelSize: 12.5 }
            Item { Layout.fillWidth: true }
            Rectangle { width: 38; height: 21; radius: 11; color: Theme.accent
                Rectangle { width: 15; height: 15; radius: 8; color: "#fff"; x: 21; anchors.verticalCenter: parent.verticalCenter } }
        }
        Item { Layout.fillHeight: true }
    }
}
