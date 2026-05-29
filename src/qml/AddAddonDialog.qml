// Source: BATorrent Add Addon.html + bat-dialog.css
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Adicionar addon"
    cardW: 560
    cardH: 540
    okText: "Fechar"
    showCancel: false
    footHint: "Instale apenas addons em que você confia"

    // 1. eyebrow + title + sub
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: "ADDONS"; red: true }
        Text {
            text: "Adicionar addon"
            color: Theme.t1
            font.pointSize: 19
            font.weight: Font.DemiBold
            font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }
        Text {
            Layout.fillWidth: true
            Layout.maximumWidth: 460
            wrapMode: Text.WordWrap
            text: "Estenda o BATorrent com extensões da comunidade para catálogos, streams e busca."
            color: Theme.t3
            font.pointSize: 12
            font.family: Theme.fontSans
        }
    }

    // 2. URL do manifesto + install button
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text { text: "URL do manifesto"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            TFld {
                Layout.fillWidth: true
                mono: true
                placeholder: "https://addon.exemplo.com/manifest.json"
            }
            BtnFlat { primary: true; text: "Instalar" }
        }
    }

    // 3. community list
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp2
        Text {
            text: "DA COMUNIDADE"
            color: Theme.t4
            font.pointSize: 10
            font.weight: Font.Bold
            font.letterSpacing: 0.8
            font.family: Theme.fontSans
            font.capitalization: Font.AllUppercase
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 11
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            implicitHeight: addCol.implicitHeight

            ColumnLayout {
                id: addCol
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                spacing: 0

                Repeater {
                    model: [
                        { nm: "Torrentio", d: "Streams de torrent para filmes e séries", installed: true },
                        { nm: "Stremio Catalogs", d: "Catálogos populares e listas em alta", installed: false },
                        { nm: "The Pirate Bay", d: "Provedor de busca (apibay)", installed: false },
                        { nm: "Nyaa.si", d: "Provedor de busca para anime", installed: false }
                    ]
                    delegate: ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 12
                            Layout.bottomMargin: 12
                            spacing: 12

                            // .ic 36×36
                            Rectangle {
                                Layout.preferredWidth: 36
                                Layout.preferredHeight: 36
                                radius: 9
                                color: Theme.field
                                border.color: Theme.hair
                                border.width: 1
                                IconImg {
                                    anchors.centerIn: parent
                                    src: "qrc:/icons/rss.svg"
                                    tint: Theme.accentText
                                    s: 18
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: modelData.nm; color: Theme.t1; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                                Text { text: modelData.d; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                            }
                            // action: installed badge or install button
                            Row {
                                visible: modelData.installed
                                spacing: 6
                                IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/play.svg"; tint: Theme.up; s: 14 }
                                Text { anchors.verticalCenter: parent.verticalCenter; text: "Instalado"; color: Theme.up; font.pointSize: 11; font.family: Theme.fontSans }
                            }
                            BtnFlat { visible: !modelData.installed; sm: true; text: "Instalar" }
                        }
                        Rectangle { visible: index < 3; Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
                    }
                }
            }
        }
    }
}
