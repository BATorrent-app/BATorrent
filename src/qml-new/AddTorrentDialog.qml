import QtQuick
import QtQuick.Layouts
import "."

BatDialog {
    title: "Adicionar torrent"; cardW: 580; cardH: 600; okText: "Adicionar"; footHint: "5 de 7 arquivos · 92.1 GB"
    ListModel {
        id: files
        ListElement { nm: "Forza Horizon 6"; sz: "92.4 GB"; pct: 0; dir: true; on: true; ind: 0 }
        ListElement { nm: "Forza.Horizon.6-CODEX.bin"; sz: "78.0 GB"; pct: 100; dir: false; on: true; ind: 1 }
        ListElement { nm: "data1.pak"; sz: "8.2 GB"; pct: 82; dir: false; on: true; ind: 1 }
        ListElement { nm: "data2.pak"; sz: "5.9 GB"; pct: 40; dir: false; on: true; ind: 1 }
        ListElement { nm: "setup.exe"; sz: "18 MB"; pct: 100; dir: false; on: true; ind: 1 }
        ListElement { nm: "soundtrack.flac"; sz: "492 MB"; pct: 0; dir: false; on: false; ind: 1 }
    }
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
        RowLayout { Layout.fillWidth: true; spacing: 16
            Rectangle { width: 44; height: 44; radius: 10; color: "#161618"; clip: true; border.color: Theme.hair; border.width: 1; Image { anchors.fill: parent; source: "images/forza.png"; fillMode: Image.PreserveAspectCrop } }
            Column { spacing: 2
                Text { text: "CONFIRMAR DOWNLOAD"; color: Theme.accent; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.6 }
                Text { text: "Forza Horizon 6"; color: Theme.t1; font.pixelSize: 19; font.bold: true; font.family: sans }
                Text { text: "92.4 GB · 6 itens"; color: Theme.t3; font.pixelSize: 11.5; font.family: mono } }
        }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; radius: 11; color: Theme.panel; border.color: Theme.hair; border.width: 1; clip: true
            ListView { anchors.fill: parent; model: files; interactive: false
                delegate: Rectangle { width: ListView.view.width; height: 40; color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                    RowLayout { anchors.fill: parent; anchors.leftMargin: 14 + model.ind*18; anchors.rightMargin: 14; spacing: 10
                        Rectangle { width: 16; height: 16; radius: 5; color: model.on ? Theme.accent : Theme.field; border.color: model.on ? "transparent" : Theme.hair; border.width: 1
                            Text { visible: model.on; anchors.centerIn: parent; text: "✓"; color: "#fff"; font.pixelSize: 10; font.bold: true } }
                        Text { text: model.dir ? "📁" : "📄"; font.pixelSize: 13; color: Theme.t4 }
                        Text { text: model.nm; color: Theme.t1; font.pixelSize: 12; font.bold: model.dir; Layout.fillWidth: true; elide: Text.ElideRight }
                        Text { visible: !model.dir; text: model.sz; color: Theme.t4; font.pixelSize: 11; font.family: mono } } }
            }
        }
        Column { Layout.fillWidth: true; spacing: 7
            Text { text: "Salvar em"; color: Theme.t3; font.pixelSize: 11; font.bold: true }
            RowLayout { width: parent.width; spacing: 8
                Rectangle { Layout.fillWidth: true; height: 34; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                    Text { anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter; text: "/Users/voce/Downloads"; color: Theme.t1; font.pixelSize: 11.5; font.family: mono } }
                Rectangle { width: 92; height: 34; radius: 7; color: Theme.field; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "Procurar…"; color: Theme.t2; font.pixelSize: 12 } } } }
    }
}
