import QtQuick
import QtQuick.Layouts
import "."

BatDialog {
    title: "Bem-vindo ao BATorrent"; cardW: 560; cardH: 470; okText: "Começar"; footHint: "Não mostrar novamente"
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
        Column { Layout.alignment: Qt.AlignHCenter; spacing: 9
            Image { anchors.horizontalCenter: parent.horizontalCenter; width: 52; height: 52; source: "images/logo.svg"; fillMode: Image.PreserveAspectFit }
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "BEM-VINDO"; color: Theme.accent; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.8 }
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Pronto pra compartilhar."; color: Theme.t1; font.pixelSize: 25; font.bold: true; font.family: sans }
            Text { width: 400; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: Theme.t2; font.pixelSize: 12.5
                text: "BATorrent é um cliente BitTorrent leve e open-source. Funcional, escuro, focado." }
        }
        GridLayout { Layout.fillWidth: true; Layout.topMargin: 22; columns: 2; columnSpacing: 10; rowSpacing: 10
            Repeater {
                model: [["Abrir .torrent","Adicionar arquivo do disco"],["Colar magnet","Da área de transferência"],["Buscar","Stremio · Torrentio"],["Inscrever RSS","Auto-baixar novos"]]
                delegate: Rectangle { Layout.fillWidth: true; implicitHeight: 64; radius: 11; color: Theme.panel; border.color: Theme.hair; border.width: 1
                    RowLayout { anchors.fill: parent; anchors.margins: 14; spacing: 12
                        Rectangle { width: 38; height: 38; radius: 9; color: Theme.field; border.color: Theme.hair; border.width: 1
                            Text { anchors.centerIn: parent; text: "◆"; color: Theme.accentText; font.pixelSize: 16 } }
                        Column { spacing: 2; Text { text: modelData[0]; color: Theme.t1; font.pixelSize: 12.5; font.bold: true } Text { text: modelData[1]; color: Theme.t4; font.pixelSize: 10.5 } } } }
            }
        }
        Item { Layout.fillHeight: true }
    }
}
