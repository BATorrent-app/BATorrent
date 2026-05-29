// Source: BATorrent Welcome.html + bat-dialog.css + <style> inline
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Bem-vindo ao BATorrent"
    cardW: 560
    cardH: 470
    okText: "Começar"
    showCancel: false

    property bool dontShow: false

    // .hero (centralizado)
    ColumnLayout {
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignHCenter
        spacing: 8

        Image {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 52
            Layout.preferredHeight: 52
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(104, 104)
            fillMode: Image.PreserveAspectFit
        }
        Eyebrow { Layout.alignment: Qt.AlignHCenter; text: "BEM-VINDO"; red: true }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Pronto pra compartilhar."
            color: Theme.t1
            font.pointSize: 25
            font.weight: Font.Black
            font.family: Theme.fontSans
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 400
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            text: "BATorrent é um cliente BitTorrent leve e open-source. Funcional, escuro, focado."
            color: Theme.t2
            font.pointSize: 12.5
            font.family: Theme.fontSans
        }
    }

    // .cards grid 2×2
    GridLayout {
        Layout.fillWidth: true
        Layout.topMargin: 6
        columns: 2
        rowSpacing: 10
        columnSpacing: 10

        Repeater {
            model: [
                { icon: "qrc:/icons/open.svg",     t: "Abrir .torrent",  d: "Adicionar arquivo do disco" },
                { icon: "qrc:/icons/magnet.svg",   t: "Colar magnet",    d: "Da área de transferência" },
                { icon: "qrc:/icons/search.svg",   t: "Buscar",          d: "Stremio · Torrentio" },
                { icon: "qrc:/icons/rss.svg",      t: "Inscrever RSS",   d: "Auto-baixar novos" }
            ]
            delegate: Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                radius: 11
                color: Theme.panel
                border.color: cardMa.containsMouse ? Theme.accent : Theme.hair
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 13
                    anchors.rightMargin: 13
                    spacing: 11

                    Rectangle {
                        Layout.preferredWidth: 38
                        Layout.preferredHeight: 38
                        radius: 9
                        color: Theme.field
                        IconImg { anchors.centerIn: parent; src: modelData.icon; tint: Theme.accentText; s: 18 }
                    }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: modelData.t; color: Theme.t1; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        Text { text: modelData.d; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                    }
                }
                MouseArea { id: cardMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
            }
        }
    }

    // footer checkbox handled via footHint slot — but checklist puts checkbox at footer-left.
    // BatDialog footer only has hint+buttons; embed the "não mostrar" as last body row aligned bottom.
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: 4
        spacing: 8
        TChk { id: dsChk; on: dlg.dontShow; onToggled: function(v) { dlg.dontShow = v } }
        Text { text: "Não mostrar novamente"; color: Theme.t3; font.pointSize: 11.5; font.family: Theme.fontSans }
    }
}
