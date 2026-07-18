// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent Add Torrent.html + bat-dialog.css
// Data-driven: populated from session.previewTorrent(path). OK → addTorrentWithPrefs.
import QtQuick
import QtQuick.Dialogs
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("addt_title"))
    cardW: 580
    cardH: 632
    okText: (i18n.language, i18n.t("add_torrent_add_btn"))
    footHint: dlg.selectedCount + " " + (i18n.language, i18n.t("word_of")) + " " + dlg.fileCount + " " + i18n.t("word_files_lc") + " · " + dlg.totalSize

    property string torrentPath: ""
    property string torrentName: ""
    property string totalSize: ""
    property double totalSizeBytes: 0
    property int fileCount: 0
    property int selectedCount: 0
    property string infoHash: ""
    property string posterPath: ""
    property alias savePath: pathFld.text
    signal freeSpaceRequested(double targetBytes)

    readonly property string posterUrl: posterPath && posterPath.length > 0
        ? (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(posterPath) : ""

    // disk-fit check against the CURRENT destination — re-run when the path
    // changes and every 2s while open, so switching drives or freeing space
    // updates the warning live (it used to freeze on the first reading)
    property double freeBytes: -1
    readonly property bool wontFit: dlg.freeBytes >= 0 && dlg.totalSizeBytes > dlg.freeBytes
    function refreshFit() {
        if (typeof session === "undefined") return
        freeBytes = session.freeBytesAt(pathFld.text.length > 0 ? pathFld.text : "")
    }
    onSavePathChanged: refreshFit()
    onOpenedChanged: if (opened) { refreshFit(); favRow.reload() }
    Timer { interval: 2000; running: dlg.opened; repeat: true; onTriggered: dlg.refreshFit() }

    function loadPreview(p, path) {
        dlg.torrentPath = path
        dlg.torrentName = p.name || ""
        dlg.totalSize = p.totalSize || ""
        dlg.totalSizeBytes = p.totalSizeBytes || 0
        dlg.fileCount = p.fileCount || 0
        dlg.infoHash = p.infoHash || ""
        dlg.posterPath = p.posterPath || ""
        fileModel.clear()
        var fs = p.files || []
        for (var i = 0; i < fs.length; ++i) {
            fileModel.append({ path: fs[i].path, size: fs[i].size || "", dir: fs[i].dir === true, depth: fs[i].depth || 0, on: true })
        }
        recount()
        // if no cached poster, ask the resolver to fetch it
        if (dlg.posterPath === "" && dlg.infoHash !== "" && typeof session !== "undefined")
            session.resolvePreview(dlg.infoHash, dlg.torrentName)
    }

    // update the poster when the resolver finishes (matches our infoHash)
    Connections {
        target: typeof session !== "undefined" ? session : null
        function onPreviewPosterReady(hash, poster) {
            if (hash === dlg.infoHash) dlg.posterPath = poster
        }
    }
    function recount() {
        var n = 0
        for (var i = 0; i < fileModel.count; ++i)
            if (fileModel.get(i).on && !fileModel.get(i).dir) n++
        dlg.selectedCount = n
    }
    function priorities() {
        var prios = []
        for (var i = 0; i < fileModel.count; ++i) {
            var it = fileModel.get(i)
            if (!it.dir) prios.push(it.on ? 4 : 0)
        }
        return prios
    }

    ListModel { id: fileModel }

    FolderDialog {
        id: saveFolderDlg
        onAccepted: if (typeof session !== "undefined")
            pathFld.text = session.urlToLocalPath(saveFolderDlg.selectedFolder.toString())
    }

    // ----- header -----
    RowLayout {
        Layout.fillWidth: true
        spacing: Theme.sp4
        Item {
            Layout.preferredWidth: 44; Layout.preferredHeight: 44

            // placeholder (no poster): field bg + dimmed logo
            Rectangle {
                anchors.fill: parent
                radius: 10; color: Theme.field; border.color: Theme.hair; border.width: 1
                visible: dlg.posterUrl === ""
                Image {
                    anchors.centerIn: parent; width: 24; height: 24
                    source: "qrc:/images/logo.svg"; sourceSize: Qt.size(48, 48)
                    fillMode: Image.PreserveAspectFit; opacity: 0.5
                }
            }
            // poster (masked rounded)
            Rectangle {
                id: pContent
                anchors.fill: parent; color: "#161618"
                visible: false; layer.enabled: true
                Image { anchors.fill: parent; source: dlg.posterUrl; fillMode: Image.PreserveAspectCrop; asynchronous: true }
            }
            Rectangle { id: pMask; anchors.fill: parent; radius: 10; color: "white"; visible: false; layer.enabled: true }
            MultiEffect {
                source: pContent; anchors.fill: parent
                maskEnabled: true; maskSource: pMask
                visible: dlg.posterUrl !== ""
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2
            Eyebrow { text: (i18n.language, i18n.t("addt_confirm")); red: true }
            Text {
                Layout.fillWidth: true
                text: dlg.torrentName
                color: Theme.t1; font.pixelSize: 19; font.weight: Font.DemiBold
                font.letterSpacing: -0.3; font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Text {
                text: dlg.totalSize + " · " + dlg.fileCount + " itens"
                color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono
            }
        }
    }

    // ----- disk-fit warning -----
    Rectangle {
        Layout.fillWidth: true
        visible: dlg.wontFit
        implicitHeight: fitRow.implicitHeight + 16
        radius: 8
        color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.12)
        border.color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.32)
        border.width: 1
        RowLayout {
            id: fitRow
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10
            IconImg { src: "qrc:/icons/triangle-alert.svg"; tint: Theme.amber; s: 15; Layout.alignment: Qt.AlignVCenter }
            Text {
                Layout.fillWidth: true
                text: (i18n.language, i18n.t("addt_wontfit")).arg(dlg.totalSize)
                color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
                wrapMode: Text.WordWrap
            }
            BtnFlat {
                text: (i18n.language, i18n.t("search_wontfit_freeup"))
                onClicked: {
                    dlg.freeSpaceRequested(Math.max(0, dlg.totalSizeBytes - dlg.freeBytes))
                    dlg.close()
                }
            }
        }
    }

    // ----- file head -----
    RowLayout {
        Layout.fillWidth: true
        RowLayout {
            spacing: 10
            TChk { id: selAll; on: true; onToggled: function(v) { for (var i=0;i<fileModel.count;i++) fileModel.setProperty(i,"on",v); dlg.recount() } }
            Text { text: (i18n.language, i18n.t("addt_select_files")); color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
        }
        Item { Layout.fillWidth: true }
        Text {
            text: dlg.selectedCount + " de " + dlg.fileCount + " · " + dlg.totalSize
            color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono
        }
    }

    // ----- file tree -----
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 224
        radius: 11; color: Theme.panel; border.color: Theme.hair; border.width: 1
        clip: true
        ListView {
            anchors.fill: parent; anchors.margins: 1; clip: true; model: fileModel
            boundsBehavior: Flickable.StopAtBounds
            delegate: Rectangle {
                width: ListView.view.width; height: 40; color: "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 14 + model.depth * 18
                    anchors.rightMargin: 14
                    spacing: 10
                    TChk { Layout.alignment: Qt.AlignVCenter; on: model.on; onToggled: function(v) { fileModel.setProperty(index, "on", v); dlg.recount() } }
                    IconImg {
                        src: model.dir ? "qrc:/icons/open.svg" : "qrc:/icons/file.svg"
                        tint: model.dir ? Theme.amber : Theme.t3; s: 13
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Text {
                        Layout.fillWidth: true
                        text: model.path; color: Theme.t1
                        font.pixelSize: 12; font.weight: model.dir ? Font.DemiBold : Font.Normal
                        font.family: Theme.fontSans; elide: Text.ElideMiddle
                    }
                    Text { text: model.size; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono; visible: model.size !== "" }
                }
            }
        }
    }

    // ----- save path + favorite folders -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text { text: (i18n.language, i18n.t("detail_kv_save_to")); color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        PathFld { id: pathFld; Layout.fillWidth: true; onBrowseClicked: saveFolderDlg.open() }
        FavFolders {
            id: favRow
            Layout.fillWidth: true
            current: pathFld.text
            onPicked: function(p) { pathFld.text = p }
        }
    }

    // ----- start now -----
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Text { text: (i18n.language, i18n.t("addt_start_now")); color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
        Item { Layout.fillWidth: true }
        TToggle { on: true }
    }
}
