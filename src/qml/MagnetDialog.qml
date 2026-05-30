// Source: BATorrent Magnet.html + bat-dialog.css
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Adicionar link magnet"
    cardW: 480
    cardH: 470
    footHint: "Aceita vários links, um por linha"
    okText: "Adicionar"

    property alias magnetText: magnetArea.text
    onOpenedChanged: if (opened) magnetArea.text = ""

    // ----- col 1: eyebrow + title -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: "ADICIONAR"; red: true }
        Text {
            text: "Link magnet"
            color: Theme.t1
            font.pointSize: 19
            font.weight: Font.DemiBold
            font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }
    }

    // ----- col 2: label + textarea -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text {
            text: "Cole o link magnet"
            color: Theme.t3
            font.pointSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        TArea {
            id: magnetArea
            Layout.fillWidth: true
            Layout.preferredHeight: 88
            placeholder: "magnet:?xt=urn:btih:..."
        }
    }

    // ----- col 3: meta-note card -----
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 60
        radius: 9
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            spacing: 10

            // info icon (i circular) — desenhado inline já que não temos info.svg
            Rectangle {
                Layout.alignment: Qt.AlignTop
                width: 15; height: 15
                radius: 7.5
                color: "transparent"
                border.color: Theme.t3
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "i"
                    color: Theme.t3
                    font.pointSize: 9
                    font.weight: Font.Bold
                    font.family: Theme.fontSans
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: "Os metadados (nome e lista de arquivos) serão baixados da rede após adicionar."
                color: Theme.t3
                font.pointSize: 11
                font.family: Theme.fontSans
                lineHeight: 1.4
            }
        }
    }

    // ----- col 4: salvar em + path -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text {
            text: "Salvar em"
            color: Theme.t3
            font.pointSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        PathFld {
            Layout.fillWidth: true
            text: "/Users/voce/Downloads"
        }
    }

    // ----- col 5: togrow Iniciar imediatamente -----
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Text {
            text: "Iniciar imediatamente"
            color: Theme.t2
            font.pointSize: 12
            font.family: Theme.fontSans
        }
        Item { Layout.fillWidth: true }
        TToggle { on: true }
    }
}
