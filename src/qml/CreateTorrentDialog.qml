// Source: BATorrent Create Torrent.html + bat-dialog.css
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Criar torrent"
    cardW: 560
    cardH: 560
    okText: "Criar"
    footHint: "Salva um arquivo .torrent"

    // 1. eyebrow + title
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: "NOVO"; red: true }
        Text {
            text: "Criar torrent"
            color: Theme.t1
            font.pointSize: 19
            font.weight: Font.DemiBold
            font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }
    }

    // 2. origem + path
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text { text: "Origem (arquivo ou pasta)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        PathFld { Layout.fillWidth: true; text: "/Users/voce/Projetos/ubuntu-remix" }
    }

    // 3. trackers textarea
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text { text: "Trackers (um por linha)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        TArea {
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            text: "udp://tracker.opentrackr.org:1337/announce\nudp://open.bittorrent.com:6969/announce\nhttps://tracker.example.org/announce"
        }
    }

    // 4. grid2 — piece size + comment
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.sp4

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 7
            Text { text: "Tamanho de peça"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            TSelect {
                Layout.fillWidth: true
                model: ["Automático", "256 KB", "512 KB", "1 MB", "2 MB", "4 MB", "8 MB"]
                currentIndex: 3
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 7
            Text { text: "Comentário (opcional)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            TFld { Layout.fillWidth: true; placeholder: "ex: build noturna" }
        }
    }

    // 5. card — 2 toggle rows
    Rectangle {
        Layout.fillWidth: true
        radius: 11
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        implicitHeight: cardCol.implicitHeight

        ColumnLayout {
            id: cardCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            spacing: 0

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "Torrent privado"; color: Theme.t1; font.pointSize: 12.5; font.family: Theme.fontSans }
                    Text { text: "Desativa DHT, PEX e LSD — para trackers privados"; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                }
                TToggle { on: false }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 12
                Layout.bottomMargin: 12
                spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "Iniciar seeding após criar"; color: Theme.t1; font.pointSize: 12.5; font.family: Theme.fontSans }
                    Text { text: "Adiciona o torrent e começa a semear"; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                }
                TToggle { on: true }
            }
        }
    }
}
