import QtQuick
import QtQuick.Layouts
import "."

BatDialog {
    title: "Sobre o BATorrent"; cardW: 460; cardH: 540; okText: "Doar"; footHint: "© 2026 · open-source"
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
        Column { Layout.alignment: Qt.AlignHCenter; spacing: 10
            Image { anchors.horizontalCenter: parent.horizontalCenter; width: 58; height: 58; source: "images/logo.svg"; fillMode: Image.PreserveAspectFit
                layer.enabled: !Theme.isDark }
            Row { anchors.horizontalCenter: parent.horizontalCenter; spacing: 0
                Text { text: "BAT"; color: Theme.accent; font.pixelSize: 22; font.bold: true; font.letterSpacing: 1 }
                Text { text: "orrent"; color: Theme.t1; font.pixelSize: 22; font.bold: true; font.letterSpacing: 1 } }
            Rectangle { anchors.horizontalCenter: parent.horizontalCenter; radius: 999; color: Qt.rgba(Theme.accent.r,Theme.accent.g,Theme.accent.b,0.12)
                border.color: Qt.rgba(Theme.accent.r,Theme.accent.g,Theme.accent.b,0.32); border.width: 1; implicitWidth: vp.width + 18; height: 22
                Text { id: vp; anchors.centerIn: parent; text: "v2.6.1"; color: Theme.accentText; font.pixelSize: 10; font.bold: true; font.family: mono } }
            Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Um cliente BitTorrent leve e de código aberto."; color: Theme.t2; font.pixelSize: 11.5 }
        }
        Rectangle { Layout.fillWidth: true; Layout.topMargin: 22; Layout.bottomMargin: 22; radius: 12; color: Theme.panel; border.color: Theme.hair; border.width: 1; implicitHeight: 64
            Text { anchors.fill: parent; anchors.margins: 13; wrapMode: Text.WordWrap; verticalAlignment: Text.AlignVCenter; color: Theme.t3; font.pixelSize: 10.5
                text: "Sem telemetria, sem analytics. A única saída automática é a checagem de release no GitHub — desativável." } }
        Text { text: "BIBLIOTECAS"; color: Theme.t4; font.pixelSize: 10; font.bold: true; font.letterSpacing: 0.8 }
        Repeater {
            model: [["Qt","6.10.2"],["libtorrent-rasterbar","2.0.11"],["OpenSSL","3.6.1"],["Boost","1.86"]]
            delegate: RowLayout { Layout.fillWidth: true; Layout.topMargin: 6
                Text { text: modelData[0]; color: Theme.t1; font.pixelSize: 12 }
                Item { Layout.fillWidth: true }
                Text { text: modelData[1]; color: Theme.t4; font.pixelSize: 11; font.family: mono } }
        }
        Item { Layout.fillHeight: true }
    }
}
