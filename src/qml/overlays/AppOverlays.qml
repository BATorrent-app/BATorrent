// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Dialogs
import "../theme"
import "../widgets"
import "../dialogs"
import "../views"

// In-app overlay dialogs + torrent open queue. Host is the Main Window; dialog
// ids are aliased back onto Main for menus/chrome.
Item {
    id: root
    anchors.fill: parent

    required property var host

    property alias openFileDlg: openFileDlg
    property alias addDlg: addDlg
    property alias addTorrentDlg: addDlg
    property alias removeDlg: removeDlg
    property alias gameActionDlg: gameActionDlg
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
    // Into the Add download window, all of them at once; with "always use the
    // default folder" on, what fits there skips the window.
    function processTorrentQueue() {
        if (typeof session === "undefined") { root.torrentQueue = []; return }
        var useDefault = settings.getBool("useDefaultPath")
        var toWindow = []
        while (root.torrentQueue.length > 0) {
            var u = root.torrentQueue.shift()
            if (useDefault && !addDlg.opened) {
                var p = session.previewTorrent(u)
                var known = p && p.ok && (p.totalSizeBytes || 0) > 0
                var fits = !known || session.freeSaveBytes() < 0 || p.totalSizeBytes <= session.freeSaveBytes()
                if (p && p.ok && fits && !p.inLibrary) { session.addTorrentFile(u); continue }
            }
            toWindow.push(u)
        }
        if (toWindow.length > 0) addDlg.openWithFiles(toWindow)
    }

    // ================== NATIVE FILE PICKER (Abrir) ==================
    FileDialog {
        id: openFileDlg
        fileMode: FileDialog.OpenFiles
        title: (i18n.language, i18n.t("dlg_open_torrent"))
        nameFilters: [(i18n.language, i18n.t("filter_torrent_files")), (i18n.language, i18n.t("filter_all_files"))]
        onAccepted: {
            var l = []
            for (var i = 0; i < selectedFiles.length; ++i) l.push(selectedFiles[i].toString())
            addDlg.openWithFiles(l)
        }
    }

    // ================== OVERLAY DIALOGS (in-app, backdrop covers all) ==================
    AddDownloadDialog {
        id: addDlg
        onFreeSpaceRequested: function (bytes, name, path) {
            makeRoomPanel.targetBytes = bytes
            makeRoomPanel.targetName = name
            makeRoomPanel.targetPath = path
            makeRoomPanel.open()
        }
        onShowInLibraryRequested: function (hash) {
            if (typeof session === "undefined") return
            root.host.currentPage = 0
            session.selectByInfoHash(hash)
        }
        onChooseFilesRequested: function (hash) {
            if (typeof session === "undefined") return
            root.host.currentPage = 0
            selectTimer.hash = hash
            selectTimer.restart()
        }
    }
    // the new torrent reaches the list a moment after it is added
    Timer {
        id: selectTimer
        property string hash: ""
        interval: 400
        onTriggered: if (session.selectByInfoHash(hash)) root.host.detailTab = 2
    }
    Timer { id: queueTimer; interval: 130; onTriggered: root.processTorrentQueue() }
    Connections {
        target: typeof session !== "undefined" ? session : null
        function onOpenTorrentRequested(path) { root.enqueueTorrentUrls([path]) }
    }
    GameActionDialog { id: gameActionDlg }
    RemoveDialog {
        id: removeDlg
        onAccepted: function (choice) {
            if (typeof session === "undefined") return
            if (choice === 2) session.removeSelectedWithFilesPermanent()
            else if (choice === 1) session.removeSelectedWithFiles()
            else session.removeSelected()
        }
    }
    MediaGuardDialog {
        id: mediaGuardDlg
        onSearchRequested: function (query) { root.host.openSearch(query) }
        onTrashRequested: function (infoHash) {
            if (typeof session !== "undefined") session.removeTorrentByHash(infoHash, true, false)
        }
        Connections {
            target: typeof session !== "undefined" ? session : null
            ignoreUnknownSignals: true
            function onMediaQuarantined(infoHash, fileName, kindKey, searchQuery) {
                mediaGuardDlg.show("quarantine", infoHash, fileName, kindKey, searchQuery)
            }
            function onMediaLure(infoHash, fileName, searchQuery) {
                mediaGuardDlg.show("lure", infoHash, fileName, "", searchQuery)
            }
        }
    }
    RefreshFlash { id: refreshFlash }

    MakeRoomPanel { id: makeRoomPanel }
    InputPromptDialog   { id: inputPrompt }
    UpdateDialog {
        id: updateDlg
        onWhatsNewRequested: { welcomeDlg.mode = "update"; welcomeDlg.open() }
        onReleaseNotesRequested: releaseNotesDlg.open()
    }

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
            color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
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
        // asked for: the window; found at startup: a notice, once per release
        function onUpdateFound(version, url, assetName) {
            if (!updater.silentCheck()) { updateDlg.showAvailable(version, url, assetName); return }
            updateDlg.prepare(version, url, assetName)
            if (typeof settings !== "undefined") {
                if (settings.get("updateNoticeFor") === version) return
                settings.set("updateNoticeFor", version)
            }
            host.toastHost.push({ tone: "ready", category: "version", title: i18n.t("notif_version").arg(version),
                                  actions: [{ id: "update", label: i18n.t("notif_see_update") }] })
        }
        function onNoUpdate(silent) { if (!silent) updateDlg.showNone() }
    }
    CreateTorrentDialog { id: createDlg }
    AddAddonDialog      { id: addAddonDlg }
    ReleaseNotesDialog  { id: releaseNotesDlg }
    WelcomeDialog {
        id: welcomeDlg
        wizard: welcomeWizard
        onAccepted: host.maybeStartTour()
        onOpenReleaseNotes: function (v) {
            if (v === "previous") releaseNotesDlg.openAt(releaseNotesDlg.previousOf(""))
            else releaseNotesDlg.openAt(v)
        }
        onActionRequested: function (a) {
            if (a === "add") addDlg.open()
            else if (a === "downloads") host.currentPage = 0
        }
    }
    // The wizard's picture of the app needs a library, or grid and list look
    // the same; someone who just installed has none. While the wizard is open
    // and the library is empty, Downloads shows a sample made of what is
    // trending in Find (posters by URL from TMDB and IGDB, as Find shows
    // them), and goes back to the real, empty one when it closes.
    ListModel { id: wizardSample }
    function fillWizardSample() {
        wizardSample.clear()
        if (typeof discovery === "undefined" || !discovery) return
        var rows = discovery.rows || []
        var picked = [], seen = {}
        // one at a time from each row, so movies, series and games mix
        for (var round = 0; round < 8 && picked.length < 10; ++round) {
            for (var r = 0; r < rows.length && picked.length < 10; ++r) {
                var it = (rows[r].items || [])[round]
                if (it && it.poster && it.title && !seen[it.poster]) { seen[it.poster] = true; picked.push(it) }
            }
        }
        // what a lived-in library looks like: some seeding, some on the way
        var looks = [
            { k: "seeding",     p: 1.0,  s: "state_seeding",     up: 1260000, down: 0 },
            { k: "downloading", p: 0.62, s: "state_downloading", up: 0, down: 8400000 },
            { k: "completed",   p: 1.0,  s: "state_completed",   up: 0, down: 0 },
            { k: "seeding",     p: 1.0,  s: "state_seeding",     up: 495000, down: 0 },
            { k: "downloading", p: 0.31, s: "state_downloading", up: 0, down: 3100000 },
            { k: "paused",      p: 0.48, s: "state_paused",      up: 0, down: 0 },
            { k: "seeding",     p: 1.0,  s: "state_seeding",     up: 141000, down: 0 },
            { k: "completed",   p: 1.0,  s: "state_completed",   up: 0, down: 0 }
        ]
        var sizes = [2.4, 1.6, 41.2, 3.1, 8.7, 0.9, 54.6, 2.2, 1.4, 12.5]
        function rate(b) { return b > 0 ? (b >= 1000000 ? (b / 1000000).toFixed(1) + " MB/s" : Math.round(b / 1000) + " KB/s") : "" }
        for (var i = 0; i < picked.length; ++i) {
            var w = picked[i], l = looks[i % looks.length], gb = sizes[i % sizes.length]
            var game = w.type === "game"
            wizardSample.append({
                torrentName: w.title, metaTitle: w.title, stateKey: l.k, progress: l.p,
                posterPath: w.poster, stateString: i18n.t(l.s), stateDetail: "", stateDetailShort: "",
                fileKind: game ? "game" : "video", category: "", autoCategory: "",
                size: gb.toFixed(1) + " GB", sizeBytes: gb * 1073741824,
                downloaded: (gb * l.p).toFixed(1) + " GB",
                downSpeed: rate(l.down), upSpeed: rate(l.up), downRate: l.down, upRate: l.up,
                numPeers: l.down > 0 ? 24 : 0, numSeeds: l.down > 0 ? 61 : 0, ratio: l.k === "seeding" ? 1.4 : 0,
                availability: 1, eta: "", infoHash: "sample-" + i, playable: !game && l.p >= 1,
                year: Number(w.year) || 0, genres: "", queuePos: -1,
                season: 0, episode: 0, showTmdb: 0,
                isGroup: false, groupCount: 0, groupExpanded: false, groupChild: false
            })
        }
    }
    function useWizardSample(on) {
        var lv = host.libraryViewItem
        if (!lv) return
        if (on && typeof session !== "undefined" && session.torrentCount === 0) {
            if (wizardSample.count === 0) fillWizardSample()
            if (wizardSample.count === 0 && typeof discovery !== "undefined" && discovery) discovery.load()
            if (wizardSample.count > 0) lv.modelOverride = wizardSample
        } else if (lv.modelOverride === wizardSample) {
            lv.modelOverride = null
        }
    }
    Connections {
        target: welcomeWizard
        function onOpenedChanged() { root.useWizardSample(welcomeWizard.opened) }
    }
    Connections {
        target: typeof discovery !== "undefined" ? discovery : null
        ignoreUnknownSignals: true
        // the catalog arrived after the wizard opened
        function onRowsChanged() {
            if (welcomeWizard.opened && wizardSample.count === 0) root.useWizardSample(true)
        }
    }
    WelcomeWizard {
        id: welcomeWizard
        topInset: host.frameless ? 32 : 0
        appView: host.appView
        // Closing it counts as having seen it. Every question here has a
        // sensible default and the whole thing is in Help → Setup wizard, so
        // reopening it on each launch was the app arguing with a decision the
        // user already made.
        onRejected: { host.dismissWelcome(); host.wizardClosed() }
        // "Explore on my own": the tour stays in Help
        onAccepted: {
            host.dismissWelcome()
            if (typeof settings !== "undefined") settings.set("tourSeen", true)
            host.wizardClosed()
        }
        onTourRequested: {
            host.whatsNewAfterWizard = false   // the tour has the screen
            host.dismissWelcome()
            if (typeof settings !== "undefined") settings.set("tourSeen", false)
            host.maybeStartTour()
        }
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
