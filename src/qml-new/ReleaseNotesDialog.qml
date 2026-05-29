import QtQuick
import QtQuick.Layouts
import "."

BatDialog {
    title: "Notas de Versão"; cardW: 540; cardH: 560; okText: "Fechar"; footHint: "CHANGELOG.md · fonte de verdade"
    ColumnLayout {
        Layout.fillWidth: true; Layout.fillHeight: true; spacing: 14
        RowLayout { Layout.fillWidth: true; spacing: 14
            Rectangle { width: 44; height: 44; radius: 10; color: Theme.panel; border.color: Theme.hair; border.width: 1; Text { anchors.centerIn: parent; text: "★"; color: Theme.accentText; font.pixelSize: 20 } }
            Column { spacing: 4
                Row { spacing: 9
                    Text { text: "NOVIDADES"; color: Theme.accent; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.8 }
                    Rectangle { radius: 999; color: Qt.rgba(Theme.accent.r,Theme.accent.g,Theme.accent.b,0.12); border.color: Qt.rgba(Theme.accent.r,Theme.accent.g,Theme.accent.b,0.3); border.width: 1; implicitWidth: vt.width+16; height: 18; Text { id: vt; anchors.centerIn: parent; text: "v2.6.1"; color: Theme.accentText; font.pixelSize: 9; font.bold: true; font.family: mono } } }
                Text { text: "Confira o que mudou"; color: Theme.t1; font.pixelSize: 19; font.bold: true; font.family: sans }
                Text { text: "28 mai 2026 · estável"; color: Theme.t4; font.pixelSize: 10.5; font.family: mono } }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
        Column { Layout.fillWidth: true; spacing: 11
            Row { spacing: 7; Rectangle { width: 6; height: 6; radius: 3; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter } Text { text: "CORREÇÃO CRÍTICA"; color: Theme.accentText; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.4 } }
            Text { width: 480; wrapMode: Text.WordWrap; color: Theme.t2; font.pixelSize: 11.5; lineHeight: 1.45
                text: "Auto-updater quebrado desde a v2.5.0 — o botão e a checagem na inicialização falhavam em silêncio. Corrigido desconectando handlers obsoletos antes de cada checagem." }
            Text { width: 480; wrapMode: Text.WordWrap; color: Theme.t2; font.pixelSize: 11.5
                text: "Adicionado timeout de 15s e política de redirecionamento nas requisições de atualização." }
        }
        Column { Layout.fillWidth: true; spacing: 11; Layout.topMargin: 6
            Row { spacing: 7; Rectangle { width: 6; height: 6; radius: 3; color: Theme.t4; anchors.verticalCenter: parent.verticalCenter } Text { text: "DA v2.6.0"; color: Theme.t3; font.pixelSize: 9; font.bold: true; font.letterSpacing: 1.4 } }
            Text { width: 480; wrapMode: Text.WordWrap; color: Theme.t2; font.pixelSize: 11.5
                text: "Plugins de busca (The Pirate Bay, Nyaa.si embutidos) e sistema de tradução reescrito — 683+ chaves × 7 idiomas migradas pra JSON." }
        }
        Item { Layout.fillHeight: true }
    }
}
