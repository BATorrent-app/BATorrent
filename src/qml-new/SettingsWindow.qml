import QtQuick
import QtQuick.Layouts
import "."

Window {
    visible: true; width: 884; height: 624; color: Theme.bg; title: "Preferências"
    readonly property string mono: "Menlo, 'SF Mono', monospace"
    readonly property string sans: "-apple-system, 'SF Pro Text', 'Segoe UI', sans-serif"
    property int sec: 0
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Rectangle { Layout.fillWidth: true; height: 36; color: Theme.elev
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            Row { x: 14; anchors.verticalCenter: parent.verticalCenter; spacing: 8
                Rectangle { width: 12; height: 12; radius: 6; color: "#ff5f57" } Rectangle { width: 12; height: 12; radius: 6; color: "#febc2e" } Rectangle { width: 12; height: 12; radius: 6; color: "#28c840" } }
            Text { anchors.centerIn: parent; text: "Preferências"; color: Theme.t2; font.pixelSize: 12.5; font.bold: true } }
        RowLayout { Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
            // sidebar
            Rectangle { Layout.preferredWidth: 226; Layout.fillHeight: true; color: Theme.panel
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }
                ColumnLayout { anchors.fill: parent; anchors.margins: 12; spacing: 1
                    Rectangle { Layout.fillWidth: true; height: 32; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                        Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.verticalCenter: parent.verticalCenter; text: "Buscar configuração…"; color: Theme.t4; font.pixelSize: 12 } }
                    Item { height: 6 }
                    Repeater {
                        model: ["Geral","Velocidade","Rede","VPN / Kill Switch","Proxy & Filtro de IP","WebUI & Pareamento","Notificações","Addons & Mídia","Avançado"]
                        delegate: Rectangle { Layout.fillWidth: true; height: 34; radius: 7; color: index === sec ? Theme.sel : "transparent"
                            Rectangle { visible: index === sec; width: 2; height: 20; anchors.verticalCenter: parent.verticalCenter; x: -8; color: Theme.accent }
                            Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: index === sec ? Theme.t1 : Theme.t2; font.pixelSize: 12.5 }
                            MouseArea { anchors.fill: parent; onClicked: sec = index } }
                    }
                }
            }
            // content
            Item { Layout.fillWidth: true; Layout.fillHeight: true
                ColumnLayout { anchors.fill: parent; anchors.margins: 24; spacing: 0
                    Text { text: "PREFERÊNCIAS"; color: Theme.t4; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.8 }
                    Text { text: "Geral"; color: Theme.t1; font.pixelSize: 19; font.bold: true; Layout.topMargin: 6 }
                    Text { text: "Pastas de download, idioma, aparência e comportamento na bandeja."; color: Theme.t3; font.pixelSize: 12; Layout.topMargin: 8 }
                    Text { text: "DOWNLOADS"; color: Theme.t4; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8; Layout.topMargin: 24 }
                    Rectangle { Layout.fillWidth: true; Layout.topMargin: 8; radius: 11; color: Theme.panel; border.color: Theme.hair; border.width: 1; implicitHeight: col.height + 4
                        ColumnLayout { id: col; width: parent.width - 32; x: 16; y: 2; spacing: 0
                            Repeater {
                                model: [["Pasta padrão para salvar","/Users/voce/Downloads/BATorrent","path"],["Sempre usar pasta padrão","","on"],["Mover concluídos automaticamente","","off"]]
                                delegate: ColumnLayout { Layout.fillWidth: true; spacing: 0
                                    RowLayout { Layout.fillWidth: true; Layout.topMargin: 13; Layout.bottomMargin: 13; spacing: 16
                                        Text { text: modelData[0]; color: Theme.t1; font.pixelSize: 12.5; Layout.fillWidth: true }
                                        Loader { sourceComponent: modelData[2]==="path" ? pathField : (modelData[2]==="on" ? togOn : togOff) } }
                                    Rectangle { visible: index < 2; Layout.fillWidth: true; height: 1; color: Theme.hairSoft } }
                            }
                        }
                    }
                    Item { Layout.fillHeight: true }
                }
            }
        }
        // footer
        Rectangle { Layout.fillWidth: true; height: 56; color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout { anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 20
                Text { text: "As alterações são aplicadas ao confirmar"; color: Theme.t4; font.pixelSize: 10.5 }
                Item { Layout.fillWidth: true }
                Rectangle { radius: 7; height: 33; implicitWidth: 80; color: "transparent"; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "Cancelar"; color: Theme.t2; font.pixelSize: 12 } }
                Rectangle { radius: 7; height: 33; implicitWidth: 64; color: Theme.accent; Layout.leftMargin: 8; Text { anchors.centerIn: parent; text: "OK"; color: "#fff"; font.pixelSize: 12; font.bold: true } } }
        }
    }
    Component { id: pathField; RowLayout { spacing: 8
        Rectangle { Layout.preferredWidth: 210; height: 30; radius: 7; color: Theme.field; border.color: Theme.hair; border.width: 1; Text { anchors.left: parent.left; anchors.leftMargin: 10; anchors.verticalCenter: parent.verticalCenter; text: "/Users/voce/Downloads/BATorrent"; color: Theme.t1; font.pixelSize: 10.5; font.family: mono; elide: Text.ElideMiddle; width: 188 } }
        Rectangle { width: 78; height: 30; radius: 7; color: Theme.field; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "Procurar…"; color: Theme.t2; font.pixelSize: 11.5 } } } }
    Component { id: togOn; Rectangle { width: 38; height: 21; radius: 11; color: Theme.accent; Rectangle { width: 15; height: 15; radius: 8; color: "#fff"; x: 21; anchors.verticalCenter: parent.verticalCenter } } }
    Component { id: togOff; Rectangle { width: 38; height: 21; radius: 11; color: Theme.isDark ? Theme.field : "#d4d6dc"; border.color: Theme.hair; border.width: 1; Rectangle { width: 15; height: 15; radius: 8; color: Theme.isDark ? "#8c8884" : "#fff"; x: 3; anchors.verticalCenter: parent.verticalCenter } } }
}
