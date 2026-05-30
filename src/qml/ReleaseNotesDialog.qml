// Source: BATorrent Release Notes.html + bat-dialog.css + <style> inline
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Notas de versão"
    cardW: 540
    cardH: 560
    okText: "Fechar"
    showCancel: false
    footHint: "CHANGELOG.md · fonte de verdade"

    // .nhead
    RowLayout {
        Layout.fillWidth: true
        Layout.bottomMargin: Theme.sp1
        spacing: 14

        // .badge 44×44 ★
        Rectangle {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44
            Layout.alignment: Qt.AlignTop
            radius: 10
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            Text { anchors.centerIn: parent; text: "★"; color: Theme.accentText; font.pointSize: 22 }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            RowLayout {
                spacing: 8
                Eyebrow { text: "NOVIDADES"; red: true }
                TChip { text: "v2.6.1" }
            }
            Text {
                text: "Confira o que mudou"
                color: Theme.t1
                font.pointSize: 19
                font.weight: Font.DemiBold
                font.letterSpacing: -0.3
                font.family: Theme.fontSans
            }
            Text {
                text: "28 mai 2026 · estável"
                color: Theme.t4
                font.pointSize: 10.5
                font.family: Theme.fontMono
            }
        }
    }
    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

    // changelog groups
    component ClGroup: ColumnLayout {
        id: clg
        property string tag
        property color tagColor: Theme.t4
        property var items: []
        Layout.fillWidth: true
        spacing: 8
        RowLayout {
            spacing: 7
            Rectangle { Layout.alignment: Qt.AlignVCenter; width: 7; height: 7; radius: 3.5; color: clg.tagColor }
            Text {
                text: clg.tag
                color: clg.tagColor
                font.pointSize: 9
                font.weight: Font.Bold
                font.letterSpacing: 1.4
                font.family: Theme.fontSans
                font.capitalization: Font.AllUppercase
            }
        }
        Repeater {
            model: clg.items
            delegate: RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 2
                spacing: 8
                Text { text: "•"; color: Theme.t4; font.pointSize: 13; Layout.alignment: Qt.AlignTop }
                Text {
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    textFormat: Text.StyledText
                    text: modelData
                    color: Theme.t2
                    font.pointSize: 11.5
                    font.family: Theme.fontSans
                    lineHeight: 1.45
                }
            }
        }
    }

    ClGroup {
        tag: "Correção crítica"
        tagColor: Theme.accentText
        items: [
            "Corrigido o <b><font color='" + Theme.t1 + "'>auto-updater</font></b> quebrado desde a v2.5.0.",
            "Timeout de rede reduzido para 15s nas checagens de release.",
            "Atualização manual da 2.5 → 2.6 documentada no README."
        ]
    }

    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
    Text {
        text: "DA v2.6.0"
        color: Theme.t4
        font.pointSize: 10
        font.weight: Font.Bold
        font.letterSpacing: 0.8
        font.family: Theme.fontSans
    }

    ClGroup {
        tag: "Plugins de busca"
        items: [ "The Pirate Bay e Nyaa.si agora vêm embutidos como provedores de busca." ]
    }
    ClGroup {
        tag: "Tradução & Categorias"
        items: [
            "683+ chaves traduzidas em 7 idiomas, migradas para JSON (<code>translator.cpp</code>).",
            "Caminhos de salvamento temporários por categoria."
        ]
    }
}
