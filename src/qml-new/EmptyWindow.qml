import QtQuick
import QtQuick.Layouts
import "."

// Estado vazio — janela principal sem torrents
Window {
    visible: true; width: 1100; height: 680; color: Theme.bg; title: "BATorrent"
    ColumnLayout { anchors.fill: parent; spacing: 0
        Rectangle { Layout.fillWidth: true; height: 66; color: Theme.elev; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout { anchors.fill: parent; anchors.leftMargin: 16; spacing: 10
                Rectangle { width: 30; height: 30; radius: 8; color: Theme.accent; Image { anchors.centerIn: parent; width: 18; height: 18; source: "images/logo.svg"; fillMode: Image.PreserveAspectFit } }
                Item { Layout.fillWidth: true } } }
        Item { Layout.fillWidth: true; Layout.fillHeight: true
            ColumnLayout { anchors.centerIn: parent; spacing: 0
                Rectangle { Layout.alignment: Qt.AlignHCenter; width: 86; height: 86; radius: 22; color: Theme.panel; border.color: Theme.hair; border.width: 1
                    Image { anchors.centerIn: parent; width: 44; height: 44; source: "images/logo.svg"; opacity: 0.5; fillMode: Image.PreserveAspectFit } }
                Text { Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 22; text: "Nenhum torrent ainda"; color: Theme.t1; font.pixelSize: 19; font.bold: true }
                Text { Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 9; width: 360; horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; color: Theme.t3; font.pixelSize: 12.5
                    text: "Adicione um .torrent, cole um link magnet ou busque — seus downloads aparecem aqui." }
                RowLayout { Layout.alignment: Qt.AlignHCenter; Layout.topMargin: 24; spacing: 10
                    Rectangle { width: 150; height: 36; radius: 8; color: Theme.accent; Text { anchors.centerIn: parent; text: "Abrir .torrent"; color: "#fff"; font.pixelSize: 12.5; font.bold: true } }
                    Rectangle { width: 140; height: 36; radius: 8; color: Theme.panel; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "Colar magnet"; color: Theme.t1; font.pixelSize: 12.5; font.bold: true } } }
            }
        }
        Rectangle { Layout.fillWidth: true; height: 30; color: Theme.elev; Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            Text { anchors.left: parent.left; anchors.leftMargin: 16; anchors.verticalCenter: parent.verticalCenter; text: "0 torrents"; color: Theme.t4; font.pixelSize: 11.5 } }
    }
}
