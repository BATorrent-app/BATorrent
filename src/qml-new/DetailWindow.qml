import QtQuick
import QtQuick.Layouts
import "."

// Painel de detalhe com abas (Geral / Peers / Arquivos / Trackers / Pedaços)
Window {
    visible: true; width: 940; height: 460; color: Theme.bg; title: "Detalhe"
    readonly property string mono: "Menlo, 'SF Mono', monospace"
    property int tab: 1
    ListModel { id: peers
        ListElement { ip: "192.168.1.42"; cli: "qBittorrent 4.6"; pct: "98%"; dn: "1.2 MB/s"; up: "84 KB/s" }
        ListElement { ip: "10.0.4.18"; cli: "Transmission 4.0"; pct: "72%"; dn: "880 KB/s"; up: "—" }
        ListElement { ip: "89.34.12.7"; cli: "libtorrent 2.0"; pct: "54%"; dn: "640 KB/s"; up: "12 KB/s" }
        ListElement { ip: "201.17.88.3"; cli: "BATorrent 2.6"; pct: "100%"; dn: "—"; up: "220 KB/s" }
    }
    ColumnLayout { anchors.fill: parent; spacing: 0
        RowLayout { Layout.fillWidth: true; height: 44; Layout.leftMargin: 24; spacing: 24
            Repeater { model: ["Geral","Peers","Arquivos","Trackers","Pedaços"]
                delegate: Item { Layout.preferredHeight: 44; implicitWidth: tt.width
                    Text { id: tt; anchors.verticalCenter: parent.verticalCenter; text: modelData; color: index===tab ? Theme.t1 : Theme.t3; font.pixelSize: 12.5 }
                    Rectangle { visible: index===tab; anchors.bottom: parent.bottom; width: parent.width; height: 2; color: Theme.accent }
                    MouseArea { anchors.fill: parent; onClicked: tab = index } } }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hair }
        // Peers table (default)
        ListView { Layout.fillWidth: true; Layout.fillHeight: true; model: peers; interactive: false; clip: true
            header: Rectangle { width: ListView.view.width; height: 32; color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
                RowLayout { anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 24; spacing: 16
                    Text { text: "ENDEREÇO IP"; Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 10; font.bold: true }
                    Text { text: "CLIENTE"; Layout.preferredWidth: 150; color: Theme.t4; font.pixelSize: 10; font.bold: true }
                    Text { text: "%"; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.bold: true }
                    Text { text: "DOWN"; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.bold: true }
                    Text { text: "UP"; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.bold: true } } }
            delegate: Rectangle { width: ListView.view.width; height: 38; color: "transparent"; Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                RowLayout { anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 24; spacing: 16
                    Text { text: model.ip; Layout.fillWidth: true; color: Theme.t2; font.pixelSize: 11.5; font.family: mono }
                    Text { text: model.cli; Layout.preferredWidth: 150; color: Theme.t2; font.pixelSize: 12 }
                    Text { text: model.pct; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pixelSize: 11.5; font.family: mono }
                    Text { text: model.dn; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: model.dn==="—"?Theme.t4:Theme.accentText; font.pixelSize: 11.5; font.family: mono }
                    Text { text: model.up; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: model.up==="—"?Theme.t4:Theme.up; font.pixelSize: 11.5; font.family: mono } } }
        }
    }
}
