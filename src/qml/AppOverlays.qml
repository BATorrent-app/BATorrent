// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import "theme"
import "widgets"
import "dialogs"

// In-app overlay dialogs + torrent open queue. Host is the Main Window; dialog
// ids are aliased back onto Main for menus/chrome.
Item {
    id: root
    anchors.fill: parent

    required property var host

    property alias openFileDlg: openFileDlg
    property alias magnetDlg: magnetDlg
    property alias addTorrentDlg: addTorrentDlg
    property alias removeDlg: removeDlg
    property alias makeRoomPanel: makeRoomPanel
    property alias inputPrompt: inputPrompt
    property alias updateDlg: updateDlg
    property alias diagnoseDlg: diagnoseDlg
    property alias createDlg: createDlg
    property alias addAddonDlg: addAddonDlg
    property alias releaseNotesDlg: releaseNotesDlg
    property alias welcomeDlg: welcomeDlg
    property alias aboutDlg: aboutDlg
    property alias inspectorDlg: inspectorDlg
    property alias pairingDlg: pairingDlg
    property alias inspectFileDlg: inspectFileDlg
    property alias exportTorrentDlg: exportTorrentDlg
    property alias importQbtDlg: importQbtDlg
    property alias shutdownDlg: shutdownDlg
    property alias refreshFlash: refreshFlash

    property var torrentQueue: []

    function enqueueTorrentUrls(urls) {
        for (var i = 0; i < urls.length; ++i) {
            var u = urls[i]
            if (u.toLowerCase().endsWith(".torrent")) root.torrentQueue.push(u)
        }
        queueTimer.restart()
    }
    function processTorrentQueue() {
        if (typeof session === "undefined") { root.torrentQueue = []; return }
        if (addTorrentDlg.opened) return
        var useDefault = settings.get("useDefaultPath") === true
        while (root.torrentQueue.length > 0) {
            var u = root.torrentQueue.shift()
            var p = session.previewTorrent(u)
            var known = p && p.ok && (p.totalSizeBytes || 0) > 0
            var fits = !known || session.freeSaveBytes() < 0 || p.totalSizeBytes <= session.freeSaveBytes()
            if (useDefault && fits) { session.addTorrentFile(u); continue }
            if (p && p.ok) {
                addTorrentDlg.savePath = session.defaultSavePath()
                addTorrentDlg.loadPreview(p, u)
                addTorrentDlg.open()
                return
            }
            session.addTorrentFile(u)
        }
    }

    // ================== NATIVE FILE PICKER (Abrir) ==================
    FileDialog {
        id: openFileDlg
        title: (i18n.language, i18n.t("dlg_open_torrent"))
        nameFilters: [(i18n.language, i18n.t("filter_torrent_files")), (i18n.language, i18n.t("filter_all_files"))]
        onAccepted: {
            if (typeof session === "undefined") return
            var path = selectedFile.toString()
            var p = session.previewTorrent(path)
            if (!p.ok) { session.addTorrentFile(path); return }
            addTorrentDlg.savePath = session.defaultSavePath()
            addTorrentDlg.loadPreview(p, path)
            addTorrentDlg.open()
        }
    }

    // ================== OVERLAY DIALOGS (in-app, backdrop covers all) ==================
    MagnetDialog {
        id: magnetDlg
        onAccepted: if (magnetText.length > 0 && typeof session !== "undefined") session.addMagnetUri(magnetText, savePath)
    }
    Timer { id: queueTimer; interval: 130; onTriggered: root.processTorrentQueue() }
    Connections {
        target: typeof session !== "undefined" ? session : null
        function onOpenTorrentRequested(path) { root.enqueueTorrentUrls([path]) }
    }
    AddTorrentDialog {
        id: addTorrentDlg
        onAccepted: {
            if (typeof session !== "undefined") session.addTorrentWithPrefs(torrentPath, savePath, priorities())
            queueTimer.restart()                // open the next once this one has closed
        }
        onRejected: queueTimer.restart()
        onFreeSpaceRequested: function (bytes) { makeRoomPanel.targetBytes = bytes; makeRoomPanel.open = true }
    }
    RemoveDialog {
        id: removeDlg
        onAccepted: if (typeof session !== "undefined") {
            if (deleteFiles) {
                if (deletePermanently) session.removeSelectedWithFilesPermanent()
                else session.removeSelectedWithFiles()
            } else session.removeSelected()
        }
    }
    RefreshFlash { id: refreshFlash }

    MakeRoomPanel {
        id: makeRoomPanel
        onDeleteRequested: function (infoHash) {
            if (typeof session === "undefined") return
            if (session.selectByInfoHash(infoHash)) removeDlg.open()
        }
        // the row list is a snapshot (Q_INVOKABLE, not a bound property) — refresh
        // it after a delete goes through so the panel doesn't show a stale entry
        Connections {
            target: typeof session !== "undefined" ? session : null
            ignoreUnknownSignals: true
            function onStatsChanged() { if (makeRoomPanel.open) makeRoomPanel.reload() }
        }
    }
    InputPromptDialog   { id: inputPrompt }
    UpdateDialog        { id: updateDlg }

    // "why is this slow" diagnostic report
    BatDialog {
        id: diagnoseDlg
        property string body: ""
        title: (i18n.language, i18n.t("ctx_why_slow"))
        cardW: 460; cardH: 320
        showCancel: false
        Text {
            Layout.fillWidth: true
            text: diagnoseDlg.body
            color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono
            wrapMode: Text.WordWrap; lineHeight: 1.4
        }
    }

    Timer {
        id: shutdownTimer; interval: 1000; repeat: true
        onTriggered: {
            host.shutdownLeft--
            if (host.shutdownLeft <= 0) { stop(); shutdownDlg.close(); if (typeof session !== "undefined") session.performPostDownloadAction() }
        }
    }
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onAllDownloadsComplete() {
            host.shutdownActionLabel = host.postDownloadActionLabel(
                typeof settings !== "undefined" ? settings.get("postDownloadAction") : 6)
            host.shutdownLeft = 60; shutdownTimer.restart(); shutdownDlg.open()
        }
    }
    BatDialog {
        id: shutdownDlg
        title: (i18n.language, i18n.t("shutdown_title"))
        cardW: 420; cardH: 190
        showOk: false
        cancelText: (i18n.language, i18n.t("btn_cancel"))
        onRejected: shutdownTimer.stop()
        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t("shutdown_msg2")).arg(host.shutdownActionLabel).arg(host.shutdownLeft)
            color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }
    }
    Connections {
        target: typeof updater !== "undefined" ? updater : null
        ignoreUnknownSignals: true
        function onUpdateFound(version, url, assetName) { updateDlg.showAvailable(version, url, assetName) }
        function onNoUpdate(silent) { if (!silent) updateDlg.showNone() }
    }
    CreateTorrentDialog { id: createDlg }
    AddAddonDialog      { id: addAddonDlg }
    ReleaseNotesDialog  { id: releaseNotesDlg }
    WelcomeDialog {
        id: welcomeDlg
        onAccepted: host.maybeStartTour()
        onRejected: host.maybeStartTour()
        onOpenReleaseNotes: releaseNotesDlg.open()
    }
    AboutDialog         { id: aboutDlg }


    InspectorDialog      { id: inspectorDlg }
    PairingDialog        { id: pairingDlg }

    // Inspect a .torrent file before adding (File menu)
    FileDialog {
        id: inspectFileDlg
        title: (i18n.language, i18n.t("inspector_title"))
        nameFilters: [(i18n.language, i18n.t("filter_torrent_files"))]
        onAccepted: inspectorDlg.load(session.urlToLocalPath(inspectFileDlg.selectedFile.toString()))
    }

    // Export the selected torrent's .torrent metadata to disk (ctx menu)
    FileDialog {
        id: exportTorrentDlg
        title: (i18n.language, i18n.t("ctx_export_torrent"))
        fileMode: FileDialog.SaveFile
        defaultSuffix: "torrent"
        nameFilters: [(i18n.language, i18n.t("filter_torrent_files"))]
        currentFile: (typeof session !== "undefined" && session.selectedName.length > 0)
                     ? ("file:" + session.selectedName + ".torrent") : "file:export.torrent"
        onAccepted: {
            if (typeof session === "undefined") return
            var ok = session.exportSelectedTorrent(exportTorrentDlg.selectedFile.toString())
            host.notifyUser("BATorrent", i18n.t(ok ? "export_torrent_ok" : "export_torrent_failed"), ok ? 0 : 2)
        }
    }

    // Import torrents from an existing qBittorrent install (choose default save path)
    FolderDialog {
        id: importQbtDlg
        title: (i18n.language, i18n.t("import_savepath_title"))
        onAccepted: if (typeof session !== "undefined") session.importQbittorrent(session.urlToLocalPath(importQbtDlg.selectedFolder.toString()))
    }

}
