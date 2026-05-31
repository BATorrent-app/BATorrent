// Inspect a .torrent file before adding. session.previewTorrent() (extended).
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Inspecionar torrent"
    cardW: 600
    cardH: 560
    okText: "Fechar"
    showCancel: false

    readonly property var sess: typeof session !== "undefined" ? session : null
    property string filePath: ""
    property var info: ({})
    property int tab: 0
    property string copyLabel: "Copiar hash"

    function load(path) {
        filePath = path
        copyLabel = "Copiar hash"
        tab = 0
        info = sess ? sess.previewTorrent(path) : ({})
        open()
    }

    // ----- error state -----
    ColumnLayout {
        visible: dlg.info.ok === false
        Layout.fillWidth: true
        spacing: Theme.sp3
        Text {
            Layout.fillWidth: true
            text: "Falha ao ler o arquivo: " + (dlg.info.error || "")
            color: Theme.accentText
            font.pointSize: 12
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }
    }

    // ----- content state -----
    ColumnLayout {
        visible: dlg.info.ok === true
        Layout.fillWidth: true
        spacing: Theme.sp3

        Text {
            Layout.fillWidth: true
            text: dlg.info.name || ""
            color: Theme.t1
            font.pointSize: 16; font.weight: Font.Bold; font.letterSpacing: -0.2
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }

        // metadata KV grid
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Theme.sp4
            rowSpacing: 6

            component MK: Text { color: Theme.t3; font.pointSize: 11.5; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            component MV: Text { Layout.fillWidth: true; color: Theme.t1; font.pointSize: 12; font.family: Theme.fontSans; wrapMode: Text.WrapAnywhere }

            MK { text: "Tamanho" }   MV { text: dlg.info.totalSize || "—" }
            MK { text: "Arquivos" }  MV { text: dlg.info.fileCount !== undefined ? String(dlg.info.fileCount) : "—" }
            MK { text: "Pedaços" }   MV { text: dlg.info.pieces || "—" }
            MK { text: "Hash" }
            TextEdit {
                Layout.fillWidth: true
                text: dlg.info.infoHash || ""
                readOnly: true
                selectByMouse: true
                color: Theme.t1
                font.pointSize: 11
                font.family: Theme.fontMono
                wrapMode: TextEdit.WrapAnywhere
            }
            MK { visible: dlg.info.creator !== undefined; text: "Criador" }
            MV { visible: dlg.info.creator !== undefined; text: dlg.info.creator || "" }
            MK { visible: dlg.info.created !== undefined; text: "Criado em" }
            MV { visible: dlg.info.created !== undefined; text: dlg.info.created || "" }
            MK { visible: dlg.info.comment !== undefined; text: "Comentário" }
            MV { visible: dlg.info.comment !== undefined; text: dlg.info.comment || "" }
            MK { visible: dlg.info.isPrivate === true; text: "Privado" }
            MV { visible: dlg.info.isPrivate === true; text: "Sim" }
        }

        // tabs: Files / Trackers
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.sp2
            spacing: Theme.sp4
            component Tab: Text {
                property int idx: 0
                color: dlg.tab === idx ? Theme.t1 : Theme.t3
                font.pointSize: 12.5
                font.weight: dlg.tab === idx ? Font.DemiBold : Font.Normal
                font.family: Theme.fontSans
                MouseArea { anchors.fill: parent; anchors.margins: -6; cursorShape: Qt.PointingHandCursor; onClicked: dlg.tab = parent.idx }
            }
            Tab { idx: 0; text: "Arquivos" }
            Tab { idx: 1; text: "Trackers" }
            Item { Layout.fillWidth: true }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 180
            radius: 8
            color: Theme.field
            border.color: Theme.hair
            border.width: 1

            ListView {
                id: filesView
                visible: dlg.tab === 0
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                model: dlg.info.files || []
                boundsBehavior: Flickable.StopAtBounds
                delegate: RowLayout {
                    required property var modelData
                    width: ListView.view.width
                    height: 26
                    spacing: Theme.sp3
                    Text { Layout.fillWidth: true; leftPadding: 6; text: modelData.path; color: Theme.t2; font.pointSize: 11; font.family: Theme.fontMono; elide: Text.ElideMiddle }
                    Text { rightPadding: 6; text: modelData.size; color: Theme.t4; font.pointSize: 11; font.family: Theme.fontMono }
                }
            }
            ListView {
                id: trackersView
                visible: dlg.tab === 1
                anchors.fill: parent
                anchors.margins: 6
                clip: true
                model: dlg.info.trackers || []
                boundsBehavior: Flickable.StopAtBounds
                delegate: Text {
                    required property var modelData
                    width: ListView.view.width
                    height: 26
                    leftPadding: 6
                    verticalAlignment: Text.AlignVCenter
                    text: modelData
                    color: Theme.t2
                    font.pointSize: 11
                    font.family: Theme.fontMono
                    elide: Text.ElideRight
                }
            }
            Text {
                anchors.centerIn: parent
                visible: dlg.tab === 1 && (!dlg.info.trackers || dlg.info.trackers.length === 0)
                text: "Sem trackers"
                color: Theme.t4
                font.pointSize: 11.5
                font.family: Theme.fontSans
            }
        }

        // footer actions
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: Theme.sp2
            spacing: Theme.sp2
            BtnFlat {
                text: dlg.copyLabel
                onClicked: { if (dlg.sess) { dlg.sess.copyToClipboard(dlg.info.infoHash || ""); dlg.copyLabel = "Copiado!" } }
            }
            Item { Layout.fillWidth: true }
            BtnFlat {
                primary: true
                text: "Adicionar agora"
                onClicked: { if (dlg.sess) dlg.sess.addTorrentFile(dlg.filePath); dlg.close() }
            }
        }
    }
}
