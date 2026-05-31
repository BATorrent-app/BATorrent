// Keyboard shortcuts reference. Static data (ported from shortcutsdialog.cpp), no bridge.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 560
    height: 620
    minimumWidth: 520
    minimumHeight: 480
    color: Theme.bg
    title: "Atalhos de teclado"

    readonly property var groups: [
        { title: "Aplicativo", rows: [
            { k: "Ctrl+,", d: "Abrir Configurações" },
            { k: "Ctrl+Q", d: "Sair do BATorrent" },
            { k: "Ctrl+W", d: "Fechar janela" },
            { k: "F1", d: "Mostrar esta referência de atalhos" },
            { k: "Ctrl+Shift+L", d: "Abrir visualizador de logs" }
        ] },
        { title: "Torrents", rows: [
            { k: "Ctrl+O", d: "Adicionar torrent de arquivo" },
            { k: "Ctrl+U", d: "Adicionar torrent de magnet/URL" },
            { k: "Ctrl+N", d: "Criar novo torrent" },
            { k: "Delete", d: "Remover torrent selecionado" },
            { k: "Shift+Delete", d: "Remover torrent + dados" },
            { k: "Ctrl+A", d: "Selecionar todos os torrents" },
            { k: "Espaço", d: "Pausar/retomar selecionado" },
            { k: "Ctrl+R", d: "Forçar verificação do selecionado" },
            { k: "Ctrl+F", d: "Focar campo de busca/filtro" }
        ] },
        { title: "Fila", rows: [
            { k: "Ctrl+↑", d: "Subir na fila" },
            { k: "Ctrl+↓", d: "Descer na fila" },
            { k: "Ctrl+Home", d: "Mover para o topo da fila" },
            { k: "Ctrl+End", d: "Mover para o fim da fila" }
        ] },
        { title: "Visualização", rows: [
            { k: "Ctrl+1", d: "Mostrar todos" },
            { k: "Ctrl+2", d: "Mostrar baixando" },
            { k: "Ctrl+3", d: "Mostrar semeando" },
            { k: "Ctrl+4", d: "Mostrar concluídos" },
            { k: "Ctrl+5", d: "Mostrar ativos" },
            { k: "Ctrl+6", d: "Mostrar inativos" },
            { k: "F5", d: "Atualizar visualização" },
            { k: "Ctrl+Shift+T", d: "Alternar tema" }
        ] }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: "Atalhos de teclado"; color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: col.implicitHeight + 2 * Theme.sp5
            boundsBehavior: Flickable.StopAtBounds

            ColumnLayout {
                id: col
                x: Theme.sp5
                y: Theme.sp5
                width: parent.width - 2 * Theme.sp5
                spacing: Theme.sp5

                Repeater {
                    model: win.groups
                    delegate: ColumnLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        spacing: Theme.sp2

                        Text {
                            text: modelData.title.toUpperCase()
                            color: Theme.t4
                            font.pointSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.8
                            font.family: Theme.fontSans
                        }
                        Repeater {
                            model: modelData.rows
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: Theme.sp4
                                Rectangle {
                                    Layout.preferredWidth: 140
                                    implicitHeight: kc.implicitHeight + 8
                                    radius: 6
                                    color: Theme.field
                                    border.color: Theme.hair
                                    border.width: 1
                                    Text {
                                        id: kc
                                        anchors.centerIn: parent
                                        text: modelData.k
                                        color: Theme.t2
                                        font.pointSize: 11.5
                                        font.family: Theme.fontMono
                                    }
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.d
                                    color: Theme.t2
                                    font.pointSize: 12
                                    font.family: Theme.fontSans
                                    wrapMode: Text.WordWrap
                                }
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                Item { Layout.fillWidth: true }
                BtnFlat { primary: true; text: "Fechar"; onClicked: win.close() }
            }
        }
    }
}
