import QtQuick
import QtQuick.Layouts
import "."

BatDialog {
    title: "Remover torrent"; cardW: 440; cardH: 360; okText: "Remover"
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
        RowLayout { Layout.fillWidth: true; spacing: 12
            Rectangle { width: 40; height: 40; radius: 10; color: Qt.rgba(0.9,0.2,0.17,0.12); border.color: Qt.rgba(0.9,0.2,0.17,0.32); border.width: 1
                Text { anchors.centerIn: parent; text: "⚠"; color: Theme.accentText; font.pixelSize: 18 } }
            Column { Layout.fillWidth: true; spacing: 6
                Text { text: "Remover este torrent?"; color: Theme.t1; font.pixelSize: 16; font.bold: true; font.family: sans }
                Text { width: 320; wrapMode: Text.WordWrap; color: Theme.t3; font.pixelSize: 12; font.family: sans
                    text: "Você vai remover Forza.Horizon.6-CODEX da lista. Esta ação não pode ser desfeita." } }
        }
        Rectangle { Layout.fillWidth: true; radius: 11; color: Theme.panel; border.color: Theme.hair; border.width: 1; implicitHeight: 56
            RowLayout { anchors.fill: parent; anchors.margins: 14; spacing: 12
                Rectangle { width: 17; height: 17; radius: 5; color: Theme.accent
                    Text { anchors.centerIn: parent; text: "✓"; color: "#fff"; font.pixelSize: 11; font.bold: true } }
                Column { spacing: 2
                    Text { text: "Também excluir os arquivos do disco"; color: Theme.t1; font.pixelSize: 12.5 }
                    Text { text: "Apaga 62.1 GB já baixados permanentemente"; color: Theme.t4; font.pixelSize: 10.5 } } }
        }
        Item { Layout.fillHeight: true }
    }
}
