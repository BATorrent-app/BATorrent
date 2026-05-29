// Source: BATorrent Add Torrent.html + bat-dialog.css + <style> inline
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Adicionar torrent"
    cardW: 580
    cardH: 632
    okText: "Adicionar"
    footHint: selectedCount + " de " + files.count + " arquivos · " + totalSize

    property string totalSize: "92.1 GB"
    property int selectedCount: 5

    // ----- file tree model -----
    ListModel {
        id: files
        ListElement { name: "Forza Horizon 6"; size: ""; folder: true; depth: 0; on: true; dir: true }
        ListElement { name: "Forza.Horizon.6-CODEX.bin"; size: "78.0 GB"; folder: false; depth: 1; on: true; dir: false }
        ListElement { name: "data1.pak"; size: "8.2 GB"; folder: false; depth: 1; on: true; dir: false }
        ListElement { name: "data2.pak"; size: "5.9 GB"; folder: false; depth: 1; on: true; dir: false }
        ListElement { name: "setup.exe"; size: "18 MB"; folder: false; depth: 1; on: true; dir: false }
        ListElement { name: "bonus"; size: ""; folder: true; depth: 1; on: false; dir: true }
        ListElement { name: "soundtrack.flac"; size: "492 MB"; folder: false; depth: 2; on: false; dir: false }
        ListElement { name: "wallpapers.zip"; size: "120 MB"; folder: false; depth: 2; on: false; dir: false }
        ListElement { name: "readme.txt"; size: "4 KB"; folder: false; depth: 1; on: true; dir: false }
    }

    // ----- .hdr -----
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.sp4

        // .ic 44×44 cover
        Item {
            Layout.preferredWidth: 44
            Layout.preferredHeight: 44
            Rectangle {
                id: hdrIcoContent
                anchors.fill: parent
                color: "#161618"
                visible: false
                layer.enabled: true
                Image { anchors.fill: parent; source: "qrc:/images/forza.png"; fillMode: Image.PreserveAspectCrop; asynchronous: true }
            }
            Rectangle { id: hdrIcoMask; anchors.fill: parent; radius: 10; color: "white"; visible: false; layer.enabled: true }
            MultiEffect { source: hdrIcoContent; anchors.fill: parent; maskEnabled: true; maskSource: hdrIcoMask }
            Rectangle { anchors.fill: parent; radius: 10; color: "transparent"; border.color: Theme.hair; border.width: 1 }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Eyebrow { text: "CONFIRMAR DOWNLOAD"; red: true }
            Text {
                text: "Forza Horizon 6"
                color: Theme.t1
                font.pointSize: 19
                font.weight: Font.DemiBold
                font.letterSpacing: -0.3
                font.family: Theme.fontSans
            }
            Text {
                text: "92.4 GB · 6 itens"
                color: Theme.t3
                font.pointSize: 11.5
                font.family: Theme.fontMono
            }
        }
    }

    // ----- .fhead -----
    RowLayout {
        Layout.fillWidth: true
        RowLayout {
            spacing: 10
            TChk { id: selAll; on: true; onToggled: function(v) { for (var i=0;i<files.count;i++) files.setProperty(i,"on",v); dlg.recount() } }
            Text { text: "Selecionar arquivos"; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontSans }
        }
        Item { Layout.fillWidth: true }
        Text {
            text: dlg.selectedCount + " de 7 · " + dlg.totalSize
            color: Theme.t4
            font.pointSize: 11
            font.family: Theme.fontMono
        }
    }

    // ----- .card .ftree (max-height 224, scroll) -----
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 224
        radius: 11
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        clip: true

        ListView {
            anchors.fill: parent
            anchors.margins: 1
            clip: true
            model: files
            boundsBehavior: Flickable.StopAtBounds

            delegate: Rectangle {
                width: ListView.view.width
                height: 40
                color: "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14 + model.depth * 18
                    anchors.rightMargin: 14
                    spacing: 10

                    TChk {
                        Layout.alignment: Qt.AlignVCenter
                        on: model.on
                        onToggled: function(v) { files.setProperty(index, "on", v); dlg.recount() }
                    }
                    // folder/file icon
                    Text {
                        text: model.folder ? "📁" : "📄"
                        color: model.folder ? Theme.amber : Theme.t3
                        font.pointSize: 13
                        visible: model.folder
                    }
                    IconImg {
                        visible: !model.folder
                        Layout.alignment: Qt.AlignVCenter
                        src: "qrc:/icons/open.svg"
                        tint: Theme.t3
                        s: 15
                    }
                    Text {
                        Layout.fillWidth: true
                        text: model.name
                        color: Theme.t1
                        font.pointSize: 12
                        font.weight: model.dir ? Font.DemiBold : Font.Normal
                        font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                    Text {
                        text: model.size
                        color: Theme.t4
                        font.pointSize: 11
                        font.family: Theme.fontMono
                        visible: model.size !== ""
                    }
                }
            }
        }
    }

    // ----- Salvar em -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text { text: "Salvar em"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        PathFld { Layout.fillWidth: true; text: "/Users/voce/Downloads" }
    }

    // ----- togrow -----
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Text { text: "Iniciar imediatamente"; color: Theme.t2; font.pointSize: 12; font.family: Theme.fontSans }
        Item { Layout.fillWidth: true }
        TToggle { on: true }
    }

    function recount() {
        var n = 0
        for (var i = 0; i < files.count; i++)
            if (files.get(i).on && !files.get(i).folder) n++
        dlg.selectedCount = n
    }
}
