import QtQuick
import QtQuick.Layouts
import "."

BatDialog {
    title: "Adicionar addon"; cardW: 560; cardH: 540; okText: "Fechar"; footHint: "Instale apenas addons confiáveis"
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 16
        Column { spacing: 4
            Text { text: "ADDONS"; color: Theme.accent; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.8 }
            Text { text: "Adicionar addon"; color: Theme.t1; font.pixelSize: 19; font.bold: true; font.family: sans } }
        RowLayout { Layout.fillWidth: true; spacing: 8
            Rectangle { Layout.fillWidth: true; height: 34; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                Text { anchors.left: parent.left; anchors.leftMargin: 11; anchors.verticalCenter: parent.verticalCenter; text: "https://addon.exemplo.com/manifest.json"; color: Theme.t4; font.pixelSize: 11.5; font.family: mono } }
            Rectangle { width: 80; height: 34; radius: 7; color: Theme.accent; Text { anchors.centerIn: parent; text: "Instalar"; color: "#fff"; font.pixelSize: 12; font.bold: true } } }
        Text { text: "DA COMUNIDADE"; color: Theme.t4; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8 }
        Rectangle { Layout.fillWidth: true; Layout.fillHeight: true; radius: 12; color: Theme.panel; border.color: Theme.hair; border.width: 1
            ColumnLayout { anchors.fill: parent; anchors.leftMargin: 15; anchors.rightMargin: 15; spacing: 0
                Repeater {
                    model: [["Torrentio","Streams de filmes e séries",true],["Stremio Catalogs","Catálogos populares",false],["The Pirate Bay","Provedor de busca",false],["Nyaa.si","Busca para anime",false]]
                    delegate: ColumnLayout { Layout.fillWidth: true; spacing: 0
                        RowLayout { Layout.fillWidth: true; Layout.topMargin: 12; Layout.bottomMargin: 12; spacing: 12
                            Rectangle { width: 36; height: 36; radius: 9; color: Theme.field; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "▶"; color: Theme.accentText; font.pixelSize: 13 } }
                            Column { Layout.fillWidth: true; spacing: 2; Text { text: modelData[0]; color: Theme.t1; font.pixelSize: 12.5; font.bold: true } Text { text: modelData[1]; color: Theme.t4; font.pixelSize: 10.5 } }
                            Item { Layout.fillWidth: true }
                            Loader { sourceComponent: modelData[2] ? installed : installBtn } }
                        Rectangle { visible: index < 3; Layout.fillWidth: true; height: 1; color: Theme.hairSoft } }
                }
            }
        }
        Component { id: installed; Row { spacing: 6; Text { text: "✓"; color: Theme.up; font.pixelSize: 13 } Text { text: "Instalado"; color: Theme.up; font.pixelSize: 11 } } }
        Component { id: installBtn; Rectangle { width: 70; height: 28; radius: 6; color: Theme.field; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "Instalar"; color: Theme.t2; font.pixelSize: 11.5 } } }
    }
}
