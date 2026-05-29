import QtQuick
import QtQuick.Layouts
import "."

Window {
    visible: true; width: 780; height: 560; color: Theme.bg; title: "Buscar torrents"
    readonly property string mono: "Menlo, 'SF Mono', monospace"
    ListModel { id: res
        ListElement { nm: "Forza.Horizon.6-CODEX"; prov: "The Pirate Bay"; sz: "92.4 GB"; se: "412"; le: "88"; ag: "3d" }
        ListElement { nm: "Forza Horizon 6 [FitGirl Repack]"; prov: "The Pirate Bay"; sz: "58.1 GB"; se: "389"; le: "120"; ag: "3d" }
        ListElement { nm: "Forza.Horizon.6.MULTi15-ElAmigos"; prov: "1337x"; sz: "71.9 GB"; se: "154"; le: "63"; ag: "2d" }
        ListElement { nm: "Forza.Horizon.6.Update.v1.2-CODEX"; prov: "The Pirate Bay"; sz: "2.1 GB"; se: "203"; le: "12"; ag: "1d" }
        ListElement { nm: "Forza Horizon 6 OST (FLAC)"; prov: "1337x"; sz: "480 MB"; se: "34"; le: "5"; ag: "6d" }
    }
    ColumnLayout { anchors.fill: parent; spacing: 0
        Rectangle { Layout.fillWidth: true; height: 36; color: Theme.elev; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            Text { anchors.centerIn: parent; text: "Buscar torrents"; color: Theme.t2; font.pixelSize: 12.5; font.bold: true } }
        RowLayout { Layout.fillWidth: true; Layout.margins: 24; spacing: 12
            Rectangle { Layout.fillWidth: true; height: 36; radius: 8; color: Theme.field; border.color: Theme.hair; border.width: 1
                Text { anchors.left: parent.left; anchors.leftMargin: 12; anchors.verticalCenter: parent.verticalCenter; text: "forza horizon 6"; color: Theme.t1; font.pixelSize: 12.5 } }
            Rectangle { width: 180; height: 36; radius: 8; color: "transparent"; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "Todos os provedores ⌄"; color: Theme.t2; font.pixelSize: 12 } } }
        Rectangle { Layout.fillWidth: true; height: 34; color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout { anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 24; spacing: 16
                Text { text: "NOME"; Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 10; font.bold: true }
                Text { text: "PROVEDOR"; Layout.preferredWidth: 120; color: Theme.t4; font.pixelSize: 10; font.bold: true }
                Text { text: "TAM."; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.bold: true }
                Text { text: "SEEDS"; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.bold: true }
                Text { text: "IDADE"; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.bold: true } } }
        ListView { Layout.fillWidth: true; Layout.fillHeight: true; model: res; interactive: false; clip: true
            delegate: Rectangle { width: ListView.view.width; height: 44; color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                RowLayout { anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 24; spacing: 16
                    Text { text: model.nm; Layout.fillWidth: true; color: Theme.t1; font.pixelSize: 12.5; elide: Text.ElideRight }
                    Rectangle { Layout.preferredWidth: 120; radius: 999; color: Theme.field; border.color: Theme.hair; border.width: 1; implicitHeight: 20; Text { anchors.centerIn: parent; text: model.prov; color: Theme.t3; font.pixelSize: 10; font.family: mono } }
                    Text { text: model.sz; Layout.preferredWidth: 70; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pixelSize: 12; font.family: mono }
                    Text { text: model.se; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.up; font.pixelSize: 12; font.family: mono }
                    Text { text: model.ag; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 11.5; font.family: mono } } }
        }
        Rectangle { Layout.fillWidth: true; height: 50; color: Theme.elev; Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            Text { anchors.left: parent.left; anchors.leftMargin: 24; anchors.verticalCenter: parent.verticalCenter; text: "42 resultados · 3 provedores"; color: Theme.t4; font.pixelSize: 10.5 } }
    }
}
