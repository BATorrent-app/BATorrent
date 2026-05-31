// Source: BATorrent RSS.html (bat-dialog.css + <style> inline)
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import "theme"
import "widgets"

Window {
    id: win
    width: 760
    height: 560
    minimumWidth: 620
    minimumHeight: 420
    color: Theme.bg
    title: "RSS"

    property int selectedFeed: 0

    readonly property var feedList: typeof rss !== "undefined" ? rss.feeds : []
    readonly property var itemList: (typeof rss !== "undefined" && feedList.length > selectedFeed && selectedFeed >= 0)
        ? rss.itemsForFeed(selectedFeed) : []
    readonly property var curFeed: (feedList.length > selectedFeed && selectedFeed >= 0) ? feedList[selectedFeed] : null

    // keep selection in range after a feed is removed
    onFeedListChanged: if (selectedFeed >= feedList.length) selectedFeed = Math.max(0, feedList.length - 1)

    // surface RSS errors in the title bar status line
    property string statusMsg: ""
    Connections {
        target: typeof rss !== "undefined" ? rss : null
        ignoreUnknownSignals: true
        function onErrorOccurred(message) { win.statusMsg = "Erro: " + message }
        function onAutoDownloaded(feedName, itemTitle) { win.statusMsg = "Auto-baixado: " + itemTitle }
    }

    // feed settings editor
    Rectangle {
        id: editOverlay
        anchors.fill: parent
        z: 110
        visible: false
        color: Qt.rgba(0, 0, 0, 0.5)
        property int feedIdx: -1

        function openFor(idx) {
            var f = win.feedList[idx]
            if (!f) return
            feedIdx = idx
            edFilter.text = f.filterPattern || ""
            edPath.text = f.savePath || ""
            edInterval.text = String(f.checkInterval || 30)
            edEnabled.on = f.enabled === true
            edAuto.on = f.autoDownload === true
            visible = true
        }

        MouseArea { anchors.fill: parent; onClicked: editOverlay.visible = false }
        Rectangle {
            anchors.centerIn: parent
            width: 460; height: 360; radius: 13
            color: Theme.bg; border.color: Theme.hair; border.width: 1
            MouseArea { anchors.fill: parent }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.sp5
                spacing: Theme.sp3
                Text { text: "Configurações do feed"; color: Theme.t1; font.pointSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans }

                Text { text: "Filtro (regex de auto-download)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                TFld { id: edFilter; Layout.fillWidth: true; mono: true; placeholder: "ex: 1080p|2160p" }

                Text { text: "Pasta de destino (vazio = padrão)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                PathFld { id: edPath; Layout.fillWidth: true; onBrowseClicked: edFolderDlg.open() }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp4
                    ColumnLayout {
                        spacing: 6
                        Text { text: "Intervalo (min)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        TFld { id: edInterval; Layout.preferredWidth: 100; placeholder: "30" }
                    }
                    Item { Layout.fillWidth: true }
                    ColumnLayout {
                        spacing: 6
                        RowLayout { spacing: 8; Text { text: "Habilitado"; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontSans } Item { Layout.fillWidth: true } TToggle { id: edEnabled } }
                        RowLayout { spacing: 8; Text { text: "Auto-baixar"; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontSans } Item { Layout.fillWidth: true } TToggle { id: edAuto } }
                    }
                }

                Item { Layout.fillHeight: true }
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    BtnFlat { text: "Cancelar"; onClicked: editOverlay.visible = false }
                    BtnFlat {
                        primary: true; text: "Salvar"
                        onClicked: {
                            if (typeof rss !== "undefined" && editOverlay.feedIdx >= 0)
                                rss.updateFeedSettings(editOverlay.feedIdx, edFilter.text, edPath.text,
                                    parseInt(edInterval.text) || 30, edEnabled.on, edAuto.on)
                            editOverlay.visible = false
                        }
                    }
                }
            }
        }
        FolderDialog {
            id: edFolderDlg
            onAccepted: edPath.text = edFolderDlg.selectedFolder.toString().replace(/^file:\/\//, "")
        }
    }

    // inline "add feed" prompt
    Rectangle {
        id: addOverlay
        anchors.fill: parent
        z: 100
        visible: false
        color: Qt.rgba(0, 0, 0, 0.5)
        MouseArea { anchors.fill: parent; onClicked: addOverlay.visible = false }
        Rectangle {
            anchors.centerIn: parent
            width: 440; height: 150; radius: 13
            color: Theme.bg; border.color: Theme.hair; border.width: 1
            MouseArea { anchors.fill: parent }
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Theme.sp5
                spacing: Theme.sp3
                Text { text: "Adicionar feed RSS"; color: Theme.t1; font.pointSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                TFld { id: addUrl; Layout.fillWidth: true; mono: true; placeholder: "https://exemplo.com/rss.xml" }
                Item { Layout.fillHeight: true }
                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    BtnFlat { text: "Cancelar"; onClicked: { addUrl.text = ""; addOverlay.visible = false } }
                    BtnFlat {
                        primary: true; text: "Adicionar"
                        onClicked: {
                            if (typeof rss !== "undefined" && addUrl.text.trim().length > 0) rss.addFeed(addUrl.text)
                            addUrl.text = ""; addOverlay.visible = false
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // titlebar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: "RSS"; color: Theme.t2; font.pointSize: 12.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Text {
                visible: win.statusMsg.length > 0
                anchors.right: parent.right; anchors.rightMargin: 12; anchors.verticalCenter: parent.verticalCenter
                text: win.statusMsg
                color: win.statusMsg.indexOf("Erro") === 0 ? Theme.accentText : Theme.t4
                font.pointSize: 10.5; font.family: Theme.fontSans
                elide: Text.ElideRight; width: Math.min(implicitWidth, parent.width / 2)
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // .rhead
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 32 + 2 * Theme.sp4
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp3
                Text { text: "Feeds RSS"; color: Theme.t1; font.pointSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                TChip { text: win.feedList.length + (win.feedList.length === 1 ? " feed" : " feeds") }
                Item { Layout.fillWidth: true }
                BtnFlat { sm: true; text: "Atualizar"; onClicked: if (typeof rss !== "undefined") rss.checkAllFeeds() }
                BtnFlat { sm: true; primary: true; text: "＋ Adicionar feed"; onClicked: addOverlay.visible = true }
            }
        }

        // .rcols
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // .feeds (232)
            Rectangle {
                Layout.preferredWidth: 232
                Layout.fillHeight: true
                color: "transparent"
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }

                ListView {
                    anchors.fill: parent
                    anchors.topMargin: Theme.sp3
                    anchors.bottomMargin: Theme.sp3
                    anchors.leftMargin: Theme.sp2
                    anchors.rightMargin: Theme.sp2
                    clip: true
                    model: win.feedList
                    spacing: 1
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 38
                        radius: 7
                        color: win.selectedFeed === index ? Theme.sel : (feedMa.containsMouse ? Theme.hover : "transparent")

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 11
                            anchors.rightMargin: 11
                            spacing: 10
                            Rectangle {
                                Layout.preferredWidth: 7; Layout.preferredHeight: 7; radius: 3.5
                                color: modelData.enabled ? Theme.grn : Theme.t4
                            }
                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: win.selectedFeed === index ? Theme.t1 : (feedMa.containsMouse ? Theme.t1 : Theme.t2)
                                font.pointSize: 12.5
                                font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                            Text { text: modelData.count; color: Theme.t4; font.pointSize: 11; font.family: Theme.fontMono }
                        }
                        MouseArea {
                            id: feedMa; anchors.fill: parent; hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            cursorShape: Qt.PointingHandCursor
                            onClicked: function(mouse) {
                                win.selectedFeed = index
                                if (mouse.button === Qt.RightButton && typeof rss !== "undefined")
                                    feedMenu.popup()
                            }
                        }
                        Menu {
                            id: feedMenu
                            modal: true
                            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
                            MenuItem {
                                text: modelData.enabled ? "Desabilitar" : "Habilitar"
                                onTriggered: rss.setFeedEnabled(index, !modelData.enabled)
                            }
                            MenuItem { text: "Atualizar agora"; onTriggered: rss.checkFeed(index) }
                            MenuItem { text: "Editar…"; onTriggered: editOverlay.openFor(index) }
                            MenuItem { text: "Remover feed"; onTriggered: rss.removeFeed(index) }
                        }
                    }
                }

                // empty hint
                Text {
                    anchors.centerIn: parent
                    visible: win.feedList.length === 0
                    width: parent.width - 32
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    text: "Nenhum feed.\nClique em “＋ Adicionar feed”."
                    color: Theme.t4
                    font.pointSize: 11
                    font.family: Theme.fontSans
                }
            }

            // .items
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // .rule banner
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    color: Theme.panel
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp5
                        anchors.rightMargin: Theme.sp5
                        spacing: 9
                        IconImg { src: "qrc:/icons/rss.svg"; tint: Theme.accentText; s: 14 }
                        Text {
                            textFormat: Text.StyledText
                            text: (win.curFeed && win.curFeed.filterPattern && win.curFeed.filterPattern.length > 0)
                                ? "Auto-baixar itens que contenham <b><font color='" + Theme.t1 + "'>" + win.curFeed.filterPattern + "</font></b>"
                                : "Sem regra de auto-download neste feed"
                            color: Theme.t2
                            font.pointSize: 11.5
                            font.family: Theme.fontSans
                        }
                        Item { Layout.fillWidth: true }
                        TToggle {
                            on: win.curFeed ? win.curFeed.autoDownload === true : false
                            onToggled: function(value) {
                                if (typeof rss !== "undefined" && win.curFeed)
                                    rss.setAutoDownload(win.selectedFeed, value)
                            }
                        }
                    }
                }

                // .ilist
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: win.itemList
                    boundsBehavior: Flickable.StopAtBounds

                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 46
                        color: itemMa.containsMouse ? Theme.hover : "transparent"
                        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: Theme.sp5
                            anchors.rightMargin: Theme.sp5
                            spacing: Theme.sp3
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 1
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.title
                                    color: Theme.t1
                                    font.pointSize: 12.5
                                    font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                }
                                Text {
                                    visible: (modelData.date || "").length > 0
                                    text: modelData.date || ""
                                    color: Theme.t4
                                    font.pointSize: 10
                                    font.family: Theme.fontMono
                                }
                            }
                            // badge: already downloaded
                            Rectangle {
                                visible: modelData.downloaded
                                implicitWidth: badgeLbl.implicitWidth + 14
                                implicitHeight: 18
                                radius: 999
                                color: Qt.rgba(63/255, 185/255, 80/255, 0.12)
                                border.color: Qt.rgba(63/255, 185/255, 80/255, 0.3)
                                border.width: 1
                                Text { id: badgeLbl; anchors.centerIn: parent; text: "Baixado"; color: Theme.grn; font.pointSize: 9.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            }
                            Text { text: modelData.size; color: Theme.t4; font.pointSize: 11; font.family: Theme.fontMono }
                            // .dl
                            Rectangle {
                                Layout.preferredWidth: 28; Layout.preferredHeight: 28
                                radius: 7
                                color: "transparent"
                                border.color: dlMa.containsMouse ? Theme.accent : Theme.hair
                                border.width: 1
                                IconImg { anchors.centerIn: parent; src: "qrc:/icons/download.svg"; tint: dlMa.containsMouse ? Theme.accentText : Theme.t3; s: 14 }
                                MouseArea {
                                    id: dlMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: if (typeof rss !== "undefined") rss.downloadItem(win.selectedFeed, index)
                                }
                            }
                        }
                        MouseArea { id: itemMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }
                }
            }
        }
    }
}
