import QtQuick
import QtQuick.Layouts
import "."

BatDialog {
    title: "Criar torrent"; cardW: 560; cardH: 560; okText: "Criar"; footHint: "Salva um arquivo .torrent"
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
        Column { spacing: 4
            Text { text: "NOVO"; color: Theme.accent; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.8 }
            Text { text: "Criar torrent"; color: Theme.t1; font.pixelSize: 19; font.bold: true; font.family: sans } }
        Column { Layout.fillWidth: true; spacing: 7
            Text { text: "Origem (arquivo ou pasta)"; color: Theme.t3; font.pixelSize: 11; font.bold: true }
            RowLayout { width: parent.width; spacing: 8
                Rectangle { Layout.fillWidth: true; height: 34; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                    Text { anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter; text: "/Users/voce/Projetos/ubuntu-remix"; color: Theme.t1; font.pixelSize: 11.5; font.family: mono } }
                Rectangle { width: 92; height: 34; radius: 7; color: Theme.field; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "Procurar…"; color: Theme.t2; font.pixelSize: 12 } } } }
        Column { Layout.fillWidth: true; spacing: 7
            Text { text: "Trackers (um por linha)"; color: Theme.t3; font.pixelSize: 11; font.bold: true }
            Rectangle { width: parent.width; height: 84; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                Text { anchors.fill: parent; anchors.margins: 11; color: Theme.t1; font.pixelSize: 11.5; font.family: mono; lineHeight: 1.4
                    text: "udp://tracker.opentrackr.org:1337/announce\nudp://tracker.openbittorrent.com:6969/announce" } } }
        Rectangle { Layout.fillWidth: true; radius: 11; color: Theme.panel; border.color: Theme.hair; border.width: 1; implicitHeight: 96
            ColumnLayout { anchors.fill: parent; anchors.leftMargin: 14; anchors.rightMargin: 14; spacing: 0
                RowLayout { Layout.fillWidth: true; Layout.fillHeight: true
                    Column { spacing: 3; Text { text: "Torrent privado"; color: Theme.t1; font.pixelSize: 12.5 } Text { text: "Desativa DHT, PEX e LSD"; color: Theme.t4; font.pixelSize: 10.5 } }
                    Item { Layout.fillWidth: true }
                    Rectangle { width: 38; height: 21; radius: 11; color: Theme.field; border.color: Theme.hair; border.width: 1
                        Rectangle { width: 15; height: 15; radius: 8; color: "#8c8884"; x: 3; anchors.verticalCenter: parent.verticalCenter } } }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
                RowLayout { Layout.fillWidth: true; Layout.fillHeight: true
                    Column { spacing: 3; Text { text: "Iniciar seeding após criar"; color: Theme.t1; font.pixelSize: 12.5 } Text { text: "Adiciona e começa a semear"; color: Theme.t4; font.pixelSize: 10.5 } }
                    Item { Layout.fillWidth: true }
                    Rectangle { width: 38; height: 21; radius: 11; color: Theme.accent
                        Rectangle { width: 15; height: 15; radius: 8; color: "#fff"; x: 21; anchors.verticalCenter: parent.verticalCenter } } } }
        }
        Item { Layout.fillHeight: true }
    }
}
