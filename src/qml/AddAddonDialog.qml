// Source: BATorrent Add Addon.html + bat-dialog.css — wired to QmlAddonBridge (`addons`).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Addons"
    cardW: 560
    cardH: 600
    okText: "Fechar"
    showCancel: false
    footHint: errorText.length > 0 ? errorText : "Instale apenas addons em que você confia"

    property string errorText: ""
    readonly property var addonsApi: typeof addons !== "undefined" ? addons : null

    Connections {
        target: dlg.addonsApi
        ignoreUnknownSignals: true
        function onError(message) { dlg.errorText = message }
    }

    // 1. eyebrow + title + sub
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: "ADDONS"; red: true }
        Text {
            text: "Addons & Extensões"
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

    // 2. install via manifest URL
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text { text: "URL do manifesto"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp2
            TFld {
                id: urlFld
                Layout.fillWidth: true
                mono: true
                placeholder: "https://addon.exemplo.com/manifest.json"
                onEdited: function(t) { dlg.installUrl(t) }
            }
            BtnFlat { primary: true; text: "Instalar"; onClicked: dlg.installUrl(urlFld.text) }
        }
    }

    function installUrl(u) {
        var url = (u || "").trim()
        if (url.length === 0 || !dlg.addonsApi) return
        dlg.errorText = ""
        dlg.addonsApi.addAddon(url)
        urlFld.text = ""
    }

    // 3. installed addons
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp2
        Text {
            text: "INSTALADOS"
            color: Theme.t4
            font.pointSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.8
            font.family: Theme.fontSans; font.capitalization: Font.AllUppercase
        }

        // empty state
        Rectangle {
            Layout.fillWidth: true
            visible: !dlg.addonsApi || dlg.addonsApi.installed.length === 0
            implicitHeight: 56
            radius: 11
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            Text {
                anchors.centerIn: parent
                text: "Nenhum addon instalado ainda."
                color: Theme.t4; font.pointSize: 11.5; font.family: Theme.fontSans
            }
        }

        Rectangle {
            Layout.fillWidth: true
            visible: dlg.addonsApi && dlg.addonsApi.installed.length > 0
            radius: 11
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            implicitHeight: instCol.implicitHeight

            ColumnLayout {
                id: instCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 14; anchors.rightMargin: 14
                spacing: 0

                Repeater {
                    model: dlg.addonsApi ? dlg.addonsApi.installed : []
                    delegate: ColumnLayout {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 12
                            Layout.bottomMargin: 12
                            spacing: 12

                            Rectangle {
                                Layout.preferredWidth: 36; Layout.preferredHeight: 36
                                radius: 9; color: Theme.field
                                border.color: Theme.hair; border.width: 1
                                IconImg { anchors.centerIn: parent; src: "qrc:/icons/rss.svg"; tint: Theme.accentText; s: 18 }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: modelData.name; color: Theme.t1; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans; elide: Text.ElideRight; Layout.fillWidth: true }
                                Text {
                                    visible: modelData.description.length > 0
                                    text: modelData.description; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans
                                    elide: Text.ElideRight; Layout.fillWidth: true
                                }
                                Text {
                                    visible: modelData.types.length > 0
                                    text: modelData.types; color: Theme.t4; font.pointSize: 9.5; font.family: Theme.fontMono
                                }
                            }
                            TToggle {
                                on: modelData.enabled
                                onToggled: function(v) { if (dlg.addonsApi) dlg.addonsApi.setEnabled(index, v) }
                            }
                            BtnFlat { sm: true; text: "Remover"; onClicked: if (dlg.addonsApi) dlg.addonsApi.removeAddon(index) }
                        }
                        Rectangle { visible: index < (dlg.addonsApi ? dlg.addonsApi.installed.length - 1 : 0); Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
                    }
                }
            }
        }
    }

    // 4. suggested (from bridge)
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp2
        Text {
            text: "SUGERIDOS"
            color: Theme.t4
            font.pointSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.8
            font.family: Theme.fontSans; font.capitalization: Font.AllUppercase
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 11
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            implicitHeight: sugCol.implicitHeight

            ColumnLayout {
                id: sugCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 14; anchors.rightMargin: 14
                spacing: 0

                Repeater {
                    model: dlg.addonsApi ? dlg.addonsApi.suggested : []
                    delegate: ColumnLayout {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        spacing: 0

                        RowLayout {
                            Layout.fillWidth: true
                            Layout.topMargin: 12; Layout.bottomMargin: 12
                            spacing: 12

                            Rectangle {
                                Layout.preferredWidth: 36; Layout.preferredHeight: 36
                                radius: 9; color: Theme.field
                                border.color: Theme.hair; border.width: 1
                                IconImg { anchors.centerIn: parent; src: "qrc:/icons/rss.svg"; tint: Theme.accentText; s: 18 }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text { text: modelData.name; color: Theme.t1; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                                Text { text: modelData.description; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans; elide: Text.ElideRight; Layout.fillWidth: true }
                            }
                            Row {
                                visible: modelData.installed
                                spacing: 6
                                IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/play.svg"; tint: Theme.up; s: 14 }
                                Text { anchors.verticalCenter: parent.verticalCenter; text: "Instalado"; color: Theme.up; font.pointSize: 11; font.family: Theme.fontSans }
                            }
                            BtnFlat { visible: !modelData.installed; sm: true; text: "Instalar"; onClicked: if (dlg.addonsApi) dlg.addonsApi.addAddon(modelData.url) }
                        }
                        Rectangle { visible: index < (dlg.addonsApi ? dlg.addonsApi.suggested.length - 1 : 0); Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
                    }
                }
            }
        }
    }

    // 5. auto-trackers + torrent search
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp2
        Text {
            text: "AUTOMÁTICO"
            color: Theme.t4
            font.pointSize: 10; font.weight: Font.Bold; font.letterSpacing: 0.8
            font.family: Theme.fontSans; font.capitalization: Font.AllUppercase
        }

        Rectangle {
            Layout.fillWidth: true
            radius: 11
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            implicitHeight: autoCol.implicitHeight

            ColumnLayout {
                id: autoCol
                anchors.left: parent.left; anchors.right: parent.right
                anchors.leftMargin: 14; anchors.rightMargin: 14
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 12; Layout.bottomMargin: 12
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "Adicionar trackers públicos automaticamente"; color: Theme.t1; font.pointSize: 12.5; font.family: Theme.fontSans }
                        Text { text: (dlg.addonsApi ? dlg.addonsApi.trackerCount : 0) + " trackers carregados"; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                    }
                    TToggle {
                        on: dlg.addonsApi ? dlg.addonsApi.autoTrackers : false
                        onToggled: function(v) { if (dlg.addonsApi) dlg.addonsApi.autoTrackers = v }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 12; Layout.bottomMargin: 12
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { text: "Habilitar busca de torrents"; color: Theme.t1; font.pointSize: 12.5; font.family: Theme.fontSans }
                        Text { text: "Permite a janela de busca usar a API configurada"; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                    }
                    TToggle {
                        on: dlg.addonsApi ? dlg.addonsApi.torrentSearchEnabled : false
                        onToggled: function(v) { if (dlg.addonsApi) dlg.addonsApi.torrentSearchEnabled = v }
                    }
                }
                Rectangle { visible: dlg.addonsApi && dlg.addonsApi.torrentSearchEnabled; Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

                TFld {
                    visible: dlg.addonsApi && dlg.addonsApi.torrentSearchEnabled
                    Layout.fillWidth: true
                    Layout.topMargin: 12; Layout.bottomMargin: 12
                    mono: true
                    text: dlg.addonsApi ? dlg.addonsApi.torrentSearchUrl : ""
                    placeholder: "URL da API de busca"
                    onEdited: function(t) { if (dlg.addonsApi) dlg.addonsApi.torrentSearchUrl = t }
                }
            }
        }
    }
}
