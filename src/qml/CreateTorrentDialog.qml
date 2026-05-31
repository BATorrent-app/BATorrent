// Source: BATorrent Create Torrent.html — wired to session.createTorrent().
import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Criar torrent"
    cardW: 560
    cardH: 620
    showOk: false
    cancelText: "Fechar"
    footHint: "Salva um arquivo .torrent"

    readonly property var sess: typeof session !== "undefined" ? session : null
    property bool busy: false

    // piece-size combo index → bytes (0 = auto)
    readonly property var pieceBytes: [0, 262144, 524288, 1048576, 2097152, 4194304, 8388608]

    FolderDialog {
        id: srcFolderDlg
        title: "Escolher pasta de origem"
        onAccepted: {
            sourceFld.text = srcFolderDlg.selectedFolder.toString().replace(/^file:\/\//, "")
            if (dlg.sess && outputFld.text.length === 0)
                outputFld.text = dlg.sess.suggestTorrentOutput(sourceFld.text)
        }
    }
    FileDialog {
        id: srcFileDlg
        title: "Escolher arquivo de origem"
        onAccepted: {
            sourceFld.text = srcFileDlg.selectedFile.toString().replace(/^file:\/\//, "")
            if (dlg.sess && outputFld.text.length === 0)
                outputFld.text = dlg.sess.suggestTorrentOutput(sourceFld.text)
        }
    }
    FileDialog {
        id: outDlg
        title: "Salvar torrent como"
        fileMode: FileDialog.SaveFile
        nameFilters: ["Torrent (*.torrent)"]
        onAccepted: outputFld.text = outDlg.selectedFile.toString().replace(/^file:\/\//, "")
    }

    // 1. eyebrow + title
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: "NOVO"; red: true }
        Text {
            text: "Criar torrent"
            color: Theme.t1
            font.pointSize: 19; font.weight: Font.DemiBold; font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }
    }

    // 2. source (file or folder)
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        RowLayout {
            Layout.fillWidth: true
            Text { text: "Origem (arquivo ou pasta)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Item { Layout.fillWidth: true }
            BtnFlat { sm: true; text: "Arquivo…"; onClicked: srcFileDlg.open() }
            BtnFlat { sm: true; text: "Pasta…"; onClicked: srcFolderDlg.open() }
        }
        PathFld { id: sourceFld; Layout.fillWidth: true; placeholder: "/caminho/para/origem"; onBrowseClicked: srcFolderDlg.open() }
    }

    // 3. trackers
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text { text: "Trackers (um por linha)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        TArea {
            id: trackersArea
            Layout.fillWidth: true
            Layout.preferredHeight: 80
            placeholder: "udp://tracker.opentrackr.org:1337/announce"
        }
    }

    // 4. piece size + comment
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.sp4
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 7
            Text { text: "Tamanho de peça"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            TSelect {
                id: pieceSel
                Layout.fillWidth: true
                model: ["Automático", "256 KB", "512 KB", "1 MB", "2 MB", "4 MB", "8 MB"]
                currentIndex: 0
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 7
            Text { text: "Comentário (opcional)"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            TFld { id: commentFld; Layout.fillWidth: true; placeholder: "ex: build noturna" }
        }
    }

    // 5. output path
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text { text: "Salvar como"; color: Theme.t3; font.pointSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        PathFld { id: outputFld; Layout.fillWidth: true; placeholder: "/caminho/saida.torrent"; onBrowseClicked: outDlg.open() }
    }

    // 6. options card
    Rectangle {
        Layout.fillWidth: true
        radius: 11
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        implicitHeight: cardCol.implicitHeight

        ColumnLayout {
            id: cardCol
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
                    Text { text: "Torrent privado"; color: Theme.t1; font.pointSize: 12.5; font.family: Theme.fontSans }
                    Text { text: "Desativa DHT, PEX e LSD — para trackers privados"; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                }
                TToggle { id: privToggle; on: false }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hairSoft }
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 12; Layout.bottomMargin: 12
                spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2
                    Text { text: "Iniciar seeding após criar"; color: Theme.t1; font.pointSize: 12.5; font.family: Theme.fontSans }
                    Text { text: "Adiciona o torrent e começa a semear"; color: Theme.t4; font.pointSize: 10.5; font.family: Theme.fontSans }
                }
                TToggle { id: seedToggle; on: true }
            }
        }
    }

    // 7. status + create button
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.sp3
        Text {
            id: statusTxt
            Layout.fillWidth: true
            wrapMode: Text.WordWrap
            text: dlg.busy ? "Criando torrent…" : ""
            color: text.indexOf("Erro") === 0 ? Theme.accentText : Theme.t3
            font.pointSize: 11
            font.family: Theme.fontSans
        }
        BtnFlat {
            primary: true
            text: dlg.busy ? "Criando…" : "Criar"
            enabled: !dlg.busy && sourceFld.text.length > 0 && outputFld.text.length > 0
            onClicked: dlg.doCreate()
        }
    }

    function doCreate() {
        if (!dlg.sess) return
        dlg.busy = true
        statusTxt.text = "Criando torrent…"
        // synchronous in the bridge; defer one tick so the UI shows the busy state
        Qt.callLater(function() {
            var err = dlg.sess.createTorrent({
                source: sourceFld.text,
                output: outputFld.text,
                trackers: trackersArea.text,
                pieceSize: dlg.pieceBytes[pieceSel.currentIndex],
                comment: commentFld.text,
                priv: privToggle.on,
                startSeeding: seedToggle.on
            })
            dlg.busy = false
            if (err && err.length > 0) {
                statusTxt.text = "Erro: " + err
            } else {
                statusTxt.text = ""
                dlg.close()
            }
        })
    }
}
