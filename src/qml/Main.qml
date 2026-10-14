// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Shapes
import QtQuick.Dialogs
import Qt.labs.platform as Platform
import "theme"
import "widgets"
import "overlays"
import "views"

Window {
    id: win
    // Shown from Component.onCompleted once the title bar is set up, so a
    // frameless window never flashes its system frame first.
    visible: false
    width: 1360
    height: 884
    // floor wide enough that the full toolbar (labels + speed module) always
    // fits: below this the RowLayout would have to clip, which is what made
    // the old icon-only "compact" hack feel broken.
    // Capped to the screen: on DPI-scaled laptops (150% on 1920 = 1280 logical)
    // an uncapped floor forces the window wider than the desktop and the right
    // column (grid detail sidebar) renders past the screen edge.
    minimumWidth: Math.min(1200, Screen.desktopAvailableWidth > 0 ? Screen.desktopAvailableWidth : 1200)
    minimumHeight: 640
    color: Theme.bg
    // On Windows the navigation bar is the title bar (AppFramelessAgent, when
    // the build has QWindowKit). macOS and Linux keep the native one above it.
    flags: Qt.Window
    readonly property bool frameless: typeof framelessWindow !== "undefined" && framelessWindow === true
    title: "BATorrent"

    // Close button hides to the tray instead of quitting (quitOnLastWindowClosed
    // is false). If no tray is available, really quit so the app can't get stuck
    // running with no window. Real quit otherwise goes through the tray/app menu.
    onClosing: function(close) {
        var toTray = (typeof settings === "undefined") || settings.getBool("closeToTray", true)
        if (trayIcon.available && toTray) {
            close.accepted = false
            win.hide()
            // First time only: tell the user where the window went so it doesn't
            // look like the app vanished (the tray icon hides in the Win11 overflow).
            if (typeof settings !== "undefined" && settings.get("trayHintShown") !== true) {
                settings.set("trayHintShown", true)
                trayIcon.showMessage("BATorrent",
                    i18n.t("tray_still_running"),
                    Platform.SystemTrayIcon.Information, 4000)
            }
        } else {
            Qt.quit()
        }
    }

    // current page of the content stack (0 Downloads Â· 1 Find Â· 2 HUB Â·
    // 3 Settings).
    property int currentPage: 0
    onCurrentPageChanged: {
        clearFilterFocus()
        if (currentPage !== 1) searchPage.returnPage = -1
        if (currentPage !== 2) hubPage.gameReturnPage = -1
    }
    // the contextual continue/download chip in the top bar (some users find it
    // redundant on the Downloads page)
    property bool showDownloadChip: true
    readonly property Item navHost: navBar

    // Selection/filter state lives on library. Leaf chrome takes `controller`
    // explicitly; there are no win.* aliases for selection.
    LibraryController {
        id: library
        onClearFilterFocusRequested: win.clearFilterFocus()
        onReleaseSearchFocusRequested: {
            var fb = libraryChrome.filterBar
            if (fb && fb.searchInput && fb.searchInput.activeFocus)
                fb.searchInput.focus = false
        }
        onScrollToRowRequested: function(row) {
            var v = library.gridView ? libraryChrome.libraryView.grid : libraryChrome.libraryView.list
            if (v) v.positionViewAtIndex(row, ListView.Contain)
        }
    }

    function selectTorrentByHash(infoHash) { library.selectTorrentByHash(infoHash) }
    function promptRenameSelected() {
        if (typeof session === "undefined" || !win.hasSel) return
        inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), session.selectedName, "",
            function(t){ if (t.length > 0) session.renameSelected(t) })
    }
    function promptRenameFile(idx, current) {
        inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), current, "",
            function(t){ if (t.length > 0) session.renameSelectedFile(idx, t) })
    }
    // Shared by the File menu, the toolbar "Link" button and the empty state.
    function promptHttpDownload() {
        inputPrompt.openWith(i18n.t("menu_add_http"), i18n.t("prompt_http_url"), "", "https://â€¦/file.zip",
            function(t){ if (t.length > 0) session.addHttpUrl(t) })
    }
    // What Find and HUB show is per language: a change has to fetch it again,
    // not leave the old language on screen until the cache runs out.
    function contentLanguageFollowsApp() {
        var cv = typeof settings !== "undefined" ? settings.get("contentLanguage") : undefined
        return cv === undefined || cv === null || cv === "" || Number(cv) < 0
    }
    Connections {
        target: typeof i18n !== "undefined" ? i18n : null
        function onLanguageChanged() {
            if (win.contentLanguageFollowsApp() && typeof discovery !== "undefined") discovery.reloadForLanguage()
        }
    }
    Connections {
        target: typeof settings !== "undefined" ? settings : null
        function onContentLanguageChanged() { if (typeof discovery !== "undefined") discovery.reloadForLanguage() }
    }
    Connections {
        target: typeof settings !== "undefined" ? settings : null
        function onChanged() {
            win.showDownloadChip = settings.getBool("showDownloadChip", true)
            // Read here too, not only at startup: the setup wizard writes this
            // one like the other three and promises the app follows along.
            var cm = settings.getBool("classicMode")
            if (cm !== library.classicMode) {
                library.classicMode = cm
                if (cm) library.gridView = false
                else library.gridView = true
            }
        }
    }

    // startup splash: ceremony only when something happened: first run or the
    // first launch after an update. A routine (often magnet-click) launch goes
    // straight to the UI. The Settings toggle still kills it entirely.
    property bool showSplash: false
    // The grid's first-run cascade waits on this.
    readonly property bool libraryObscured:
        showSplash
        || (welcomeDlg && welcomeDlg.opened)
        || (tourOverlay && tourOverlay.running)
    Component.onCompleted: {
        if (typeof settings !== "undefined") library.classicMode = settings.getBool("classicMode")
        if (library.classicMode) library.gridView = false   // classic is a list layout
        if (typeof settings !== "undefined")
            win.showDownloadChip = settings.getBool("showDownloadChip", true)
        if (typeof settings === "undefined") {
            showSplash = true
        } else {
            // read welcomeShown/lastSeenVersion BEFORE maybeShowWelcome mutates them
            var curVer = (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""
            var isFirstRun = settings.get("welcomeShown") !== true
            var isUpdate = curVer.length > 0 && settings.get("lastSeenVersion") !== curVer
            showSplash = settings.getBool("showSplash", true) && (isFirstRun || isUpdate)
        }
        // Start hidden only when the login item launched us. Applying this to a
        // deliberate launch made the app look like it had failed to open: you
        // double-click the icon and nothing appears (tester report, beta7 #12).
        var startHidden = typeof launchedBySystem !== "undefined" && launchedBySystem
                && typeof settings !== "undefined" && settings.getBool("startTray")
                && trayIcon.available
        // The frameless setup runs on the hidden window, before any show:
        // also when starting in the tray, since a later show (tray, second
        // launch) can come from C++ without passing through QML.
        // The saved size goes on after that: set before, the frame came off
        // around it and its caption height joined the window, which then
        // grew by that much on every launch.
        Qt.callLater(function() {
            if (framelessLoader.item) framelessLoader.item.attach()
            win.restoreSize()
            if (!startHidden) win.visible = true
            // and once more after the first show: the frame only really comes
            // off when the native window exists, and takes its caption with it
            Qt.callLater(win.restoreSize)
        })
        if (!showSplash) win.maybeShowWelcome()
    }
    // the last window size, if it is still sane for the minimums and the screen
    function restoreSize() {
        if (typeof settings === "undefined") return
        var sw = Number(settings.get("winWidth") || 0)
        var sh = Number(settings.get("winHeight") || 0)
        if (sw >= win.minimumWidth && sw <= Screen.desktopAvailableWidth) win.width = sw
        if (sh >= win.minimumHeight && sh <= Screen.desktopAvailableHeight) win.height = sh
    }
    Loader {
        id: framelessLoader
        active: win.frameless
        source: win.frameless ? "AppFramelessAgent.qml" : ""
        onLoaded: { item.window = win; item.navBar = navBar }
    }
    AppWindowLifecycle { host: win }

    function checkClipboardMagnet() {
        if (typeof session === "undefined") return
        if (addDlg.opened || addTorrentDlg.opened) return
        var m = session.clipboardMagnetIfNew()
        if (m.length > 0) addDlg.openWithText(m)
    }
    // first launch â†’ the interactive tour (opens with a welcome step); an update
    // (version changed) â†’ the what's-new screen. Once each, never both, never on
    // a plain relaunch.
    // "4.7.1" â†’ 40701, so versions compare as numbers instead of strings, where
    // "4.10" would sort below "4.9". Missing or unparseable reads as 0, which is
    // the right answer: an install too old to have recorded a version is exactly
    // the one that needs the most help.
    function versionRank(v) {
        var p = String(v || "").split(".")
        var n = 0
        for (var i = 0; i < 3; ++i) n = n * 100 + (parseInt(p[i]) || 0)
        return n
    }
    // Installs older than 4.8 get the wizard too, not just new ones. 4.8 moved
    // the layout around again and old settings describe an app that isn't there
    // anymore. Only bump this for a release that reshuffles the UI.
    readonly property int kWizardIntroVersion: 40800
    function maybeShowWelcome() {
        if (typeof settings === "undefined") return
        var cur = (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""
        var firstRun = settings.get("welcomeShown") !== true
        var last = settings.get("lastSeenVersion")
        if (firstRun) {
            welcomeDlg.mode = "welcome"
            welcomeDlg.open()
        } else if (cur.length > 0 && last !== cur) {
            // No silent undo on dismiss. Existing users see this so they discover settings they never found: content
            // language, which side the navigation sits on. A choice made here is
            // a choice, and quietly reverting it on close would undo the reason
            // for showing the wizard at all. Everything stays reachable in
            // Settings, which they now know.
            var intro = versionRank(cur) >= win.kWizardIntroVersion
                        && versionRank(last) < win.kWizardIntroVersion
            welcomeDlg.mode = intro ? "welcome" : "update"
            welcomeDlg.open()
        }
        if (cur.length > 0) settings.set("lastSeenVersion", cur)
    }

    // Dismissed, not completed: remember it was offered, but do not start the
    // tour â€” that is a reward for finishing, not a consolation for leaving.
    function dismissWelcome() {
        if (typeof settings === "undefined") return
        settings.set("welcomeShown", true)
    }

    function completeWelcome() {
        if (typeof settings === "undefined") return
        settings.set("welcomeShown", true)
        maybeStartTour()
    }

    // The tour runs once ever, after setup is completed or the first update
    // screen is accepted. Dismissing setup leaves it available next launch.
    function maybeStartTour() {
        if (typeof settings === "undefined") return
        if (settings.get("tourSeen") !== true) {
            settings.set("tourSeen", true)
            tourOverlay.start()
        }
    }

    function rectIn(item, ref) {
        if (!item || !ref) return Qt.rect(0, 0, 0, 0)
        var p = item.mapToItem(ref, 0, 0)
        return Qt.rect(p.x, p.y, item.width, item.height)
    }
    readonly property var presetCats: ["Apps", "Games", "Movies", "Series", "Compressed"]
    property int detailTab: 0   // 0 Geral Â· 1 Peers Â· 2 Arquivos Â· 3 Trackers Â· 4 PedaÃ§os
    property bool detailsCollapsed: typeof settings !== "undefined" && settings.getBool("detailsCollapsed")
    function toggleDetailsCollapsed() {
        detailsCollapsed = !detailsCollapsed
        if (typeof settings !== "undefined") settings.set("detailsCollapsed", detailsCollapsed)
    }
    // Missing-files recovery: pick where the files actually live (or a fresh folder
    // to re-download into): libtorrent moves storage there, then a recheck picks
    // up whatever's present. Shared by the context menu and the recovery banner.
    function promptSetLocation() { setLocationDlg.open() }
    function openSearch(query) { win.currentPage = 1; searchPage.runQuery(query) }
    // A torrent's own page, from Downloads. False when it has none.
    function openTorrentPage(hash) {
        if (typeof session === "undefined" || !hash) return false
        var page = session.torrentPage(hash)
        if (page.kind === "game") {
            win.currentPage = 2
            hubPage.openGame(hash, 0)
            return true
        }
        if (page.kind === "movie" || page.kind === "series") {
            win.currentPage = 1
            searchPage.openTitle(page, 0)
            return true
        }
        return false
    }
    property alias setLocationDlg: libraryShortcuts.setLocationDlg
    // Full-screen visual acknowledgement for a manual Refresh.
    function flashRefresh() { refreshFlash.flash() }
    // F5 and the menu's Refresh: re-read every torrent, with the flash as proof
    function refreshView() {
        flashRefresh()
        if (typeof session !== "undefined") session.refreshAll()
    }

    // post-download action: cancelable countdown after all downloads finish
    property int shutdownLeft: 0
    property string shutdownActionLabel: ""
    readonly property var postDownloadActionKeys: ["post_action_none", "post_action_close",
        "post_action_lock", "post_action_sleep", "post_action_hibernate",
        "post_action_signout", "post_action_shutdown", "post_action_restart"]
    function postDownloadActionLabel(idx) {
        var key = win.postDownloadActionKeys[idx] || "post_action_shutdown"
        return i18n.t(key)
    }

    // The Peers tab pulls every peer from libtorrent: only keep it live while open.
    readonly property bool peersTabOpen: win.hasSel && win.detailTab === 1
    onPeersTabOpenChanged: if (typeof session !== "undefined") session.setDetailPeersActive(peersTabOpen)

    // live model from C++ (QmlTorrentFilterProxy â†’ QmlPosterModel). Roles:
    // torrentName, metaTitle, stateKey, progress(0..1), posterPath, stateString,
    // downSpeed, upSpeed, category, numPeers, downRate, upRate, size, infoHash.
    readonly property var model: typeof torrentModel !== "undefined" ? torrentModel : null
    readonly property bool hasSel: typeof session !== "undefined" && session.hasSelection
    // Only the user's own choice. Folding on an empty selection meant the next
    // click unfolded it again, and the grid reflowed on both.
    readonly property bool detailsShownCollapsed: win.detailsCollapsed

    // ----- stateâ†’color helpers (keyed by real stateKey) -----
    // The mapping lives in Theme so the progress bar can reach it without a
    // window; this stays as the call site every view already uses.
    function fillFor(k) { return Theme.fillFor(k) }
    function textFor(k) {
        if (k === "finished" || k === "completed") return Theme.grn   // done = green
        if (k === "seeding") return Theme.up                          // seeding = amber
        if (k === "paused" || k === "queued") return Theme.t3
        return Theme.accentText
    }
    function dotFor(k) {
        if (k === "finished" || k === "completed") return Theme.grn   // done = green
        if (k === "seeding") return Theme.amber                       // seeding = amber
        // t3, not t4: t4 is the last step before invisible, and a paused dot
        // that cannot be seen says nothing at all.
        if (k === "paused" || k === "queued") return Theme.t3
        return Theme.accent                                           // downloading = red
    }
    function fmtEta(sec) {
        if (sec < 0) return ""
        var u = sec >= 86400 ? Math.floor(sec / 86400) + "d"
              : sec >= 3600  ? Math.floor(sec / 3600) + "h"
              : sec >= 60    ? Math.floor(sec / 60) + "m"
              : sec + "s"
        return i18n.t("eta_left").arg(u)
    }
    function _commitSel() { library.commitSel() }
    // The downloads filter keeps activeFocus (and its accent ring) until
    // something else claims it. Every gesture that means "I'm done typing":
    // picking a torrent, clicking blank space, leaving the page: routes here.
    function clearFilterFocus() { if (libraryChrome.filterBar) libraryChrome.filterBar.clearSearchFocus() }

    function selectRow(proxyRow, mods) { library.selectRow(proxyRow, mods) }
    function isRowSelected(proxyRow) { return library.isRowSelected(proxyRow) }
    function selectAll() { library.selectAll() }
    function toggleSort(col) { library.toggleSort(col) }
    function setFilter(f) { library.setFilter(f) }
    function applyCatFilter(c) { library.applyCatFilter(c) }
    // Categories are stored/filtered by a stable language-independent value
    // (presetCats); only the *display* is translated. Switching language must
    // not desync a torrent's category from the filter/menu, so never store the
    // translated label.
    // categories the user created, i.e. everything the engine knows minus the
    // four built-ins the menu already lists statically
    function customCategories() {
        if (typeof session === "undefined") return []
        var builtins = ["Apps", "Games", "Movies", "Series", "Compressed"]
        return session.categories().filter(function (c) {
            return c.length > 0 && builtins.indexOf(c) < 0
        })
    }

    function catLabel(value) {
        switch (value) {
        case "Apps":   return i18n.language, i18n.t("cat_apps")
        case "Games":  return i18n.language, i18n.t("cat_games")
        case "Movies": return i18n.language, i18n.t("cat_movies")
        case "Series": return i18n.language, i18n.t("cat_series")
        // "Apps" stays the stored value on purpose: it is what every existing
        // torrent already carries. Only its label became Software, per Sherwan.
        case "Compressed": return i18n.language, i18n.t("cat_compressed")
        default:       return value   // custom category: show as the user typed it
        }
    }
    function openContext(proxyRow) {
        // right-clicking inside an existing multi-selection must not collapse
        // it to just this row: that silently turned "remove 3 selected" into
        // "remove 1" (reported by a user)
        if (!win.isRowSelected(proxyRow)) win.selectRow(proxyRow)
        ctxMenu.popup()
    }
    function remapRow(r, from, to) { return library.remapRow(r, from, to) }
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onQueueMoved(from, to) { library.onQueueMoved(from, to) }
    }
    function gridCols() { return Math.max(1, Math.floor(libraryChrome.libraryView.grid.width / libraryChrome.libraryView.grid.cellWidth)) }
    function moveSel(step) {
        var view = library.gridView ? libraryChrome.libraryView.grid : libraryChrome.libraryView.list
        var n = view.count
        if (n <= 0) return
        var cur = library.selected
        var next = cur < 0 ? (step > 0 ? 0 : n - 1)
                           : Math.max(0, Math.min(n - 1, cur + step))
        win.selectRow(next)
        view.positionViewAtIndex(next, library.gridView ? GridView.Contain : ListView.Contain)
    }

    // ----- shared context menu (right-click on grid tile / list row) -----
    LibraryContextMenu {
        id: ctxMenu
        host: win
        controller: library
        inputPrompt: win.inputPrompt
        removeDialog: removeDlg
        setLocationDialog: setLocationDlg
        exportDialog: exportTorrentDlg
        diagnoseDialog: diagnoseDlg
    }

    // Must stay a direct child of Window (not nested under Layout): macOS menus
    // break otherwise.
    AppMenuBar {
        host: win
        openFileDialog: openFileDlg
        addDialog: addDlg
        inputPrompt: win.inputPrompt
        createDialog: createDlg
        inspectFileDialog: inspectFileDlg
        importQbtDialog: importQbtDlg
        removeDialog: removeDlg
        addAddonDialog: addAddonDlg
        pairingDialog: pairingDlg
        tourOverlay: appTour.tourOverlay
        welcomeDialog: welcomeDlg
        releaseNotesDialog: releaseNotesDlg
        aboutDialog: aboutDlg
    }

    // The app menu behind the logo. macOS has the system menu bar instead.
    AppLogoMenu {
        id: logoMenu
        host: win
        openFileDialog: openFileDlg
        addDialog: addDlg
        createDialog: createDlg
        inspectFileDialog: inspectFileDlg
        importQbtDialog: importQbtDlg
        removeDialog: removeDlg
        addAddonDialog: addAddonDlg
        pairingDialog: pairingDlg
        tourOverlay: appTour.tourOverlay
        welcomeDialog: welcomeDlg
        releaseNotesDialog: releaseNotesDlg
        aboutDialog: aboutDlg
        settingsPage: settingsPage
    }

    // ----- system tray -----
    AppTray {
        id: trayIcon
        onRestoreRequested: { win.show(); win.raise(); win.requestActivate() }
        onContextRequested: function(geo) { trayPopup.popUpAt(geo) }
    }

    // Background events â†’ in-app toast (when the window is up) AND the native
    // OS notification (so it's seen when minimized/in the tray). Both, like the
    // legacy app.
    // Unified notification: when the window is up, show the custom toast at the
    // screen corner; when minimized / hidden in the tray, fall back to the native
    // OS notification so it's seen with the app closed.
    function notifyUser(title, body, level) {
        var shown = win.visible
                    && win.visibility !== Window.Minimized
                    && win.visibility !== Window.Hidden
        if (shown) {
            toastHost.show(title, body, level,
                           level === 2 ? "logs" : "", level === 2 ? i18n.t("toast_view_logs") : "")
        } else if (trayIcon.available) {
            // `supportsMessages` reads false on Windows even when showMessage
            // works, so gate on `available`.
            trayIcon.showMessage(title, body,
                level === 2 ? Platform.SystemTrayIcon.Critical
                : level === 1 ? Platform.SystemTrayIcon.Warning
                : Platform.SystemTrayIcon.Information, 5000)
        } else {
            toastHost.show(title, body, level)
        }
    }

    // For what happens while another app is in front (a game running): the
    // in-app toast would sit behind it, so the OS notification goes first.
    function notifyNative(title, body) {
        if (trayIcon.available) trayIcon.showMessage(title, body, Platform.SystemTrayIcon.Information, 5000)
        else toastHost.show(title, body, 0)
    }

    AppCommandPalette {
        id: appCmdPalette
        host: win
        settingsPage: settingsPage
        library: library
        addDlg: overlays.addDlg
        openFileDlg: overlays.openFileDlg
        createDlg: overlays.createDlg
    }
    property alias cmdPalette: appCmdPalette.cmdPalette

    AppNotifications {
        id: appNotifications
        host: win
    }
    property alias gwOverlay: appNotifications.gwOverlay
    property alias toastHost: appNotifications.toastHost

    AppTrayPopup {
        id: trayPopup
        host: win
    }

    // (IconImg vem de widgets/)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        // ===== top navigation bar (the title bar too, on Windows) =====
        NavBar {
            id: navBar
            Layout.fillWidth: true
            appWindow: win
            ownsTitleBar: win.frameless
            currentIndex: win.currentPage
            showDownloadChip: win.showDownloadChip && windowLoaders.playersOpen === 0
            menuOpen: logoMenu.opened
            onPageRequested: function(page) { win.currentPage = page }
            onSettingsClicked: win.currentPage = 3
            onVpnClicked: { settingsPage.sec = 3; win.currentPage = 3 }
            onSelectTorrent: function(infoHash) { win.selectTorrentByHash(infoHash) }
            onMakeRoomRequested: { makeRoomPanel.targetBytes = 0; makeRoomPanel.open = true }
            onMenuRequested: function(anchor) {
                if (logoMenu.opened) { logoMenu.close(); return }
                var p = anchor.mapToItem(win.contentItem, 0, anchor.height + 4)
                logoMenu.popup(win.contentItem, p.x, p.y)
            }
        }

        // ===== content stack (4.0 hub shell) =====
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            StackLayout {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: win.currentPage

                // Directional page switch. The tabs sit in a row, so moving
                // right enters from the right and moving left from the left:
                // the motion matches the gesture, and going back reverses it
                // instead of repeating it.
                transform: Translate { id: pageShift }
                // Faded as one picture while it runs. Opacity on a parent applies
                // to each child on its own, so a page over another (a game's page
                // over the hub) went see-through and showed what lay beneath.
                layer.enabled: pageSwitchAnim.running
                property int prevPage: 0
                property real enterFrom: 34
                onCurrentIndexChanged: {
                    enterFrom = Theme.reduceMotion ? 0 : (currentIndex > prevPage ? 34 : -34)
                    prevPage = currentIndex
                    pageSwitchAnim.restart()
                }
                SequentialAnimation {
                    id: pageSwitchAnim
                    ParallelAnimation {
                        NumberAnimation { target: contentStack; property: "opacity"; from: 0.0; to: 1.0; duration: 170; easing.type: Theme.easeOut }
                        NumberAnimation {
                            target: pageShift; property: "x"
                            from: contentStack.enterFrom; to: 0
                            duration: Theme.durSlower; easing.type: Theme.easeOut
                        }
                    }
                }

                // ----- page 0: Downloads (original main column) -----
                ColumnLayout {
                    spacing: 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true

        // ================== TOOLBAR ==================
        Toolbar {
            id: toolbar
            win: win
            controller: library
            onOpenFile: openFileDlg.open()
            onAddMagnet: addDlg.open()
            onAddLink: promptHttpDownload()
            onRemoveSelected: removeDlg.open()
            onOpenRss: win.showWin(rssWinLoader)
            onMakeRoomRequested: { makeRoomPanel.targetBytes = 0; makeRoomPanel.open = true }
        }

        // ================== SUBBAR + LIBRARY ==================
        LibraryChrome {
            id: libraryChrome
            Layout.fillWidth: true
            Layout.fillHeight: true
            host: win
            controller: library
            onAddMagnetRequested: addDlg.open()
            onAddLinkRequested: promptHttpDownload()
            onRenameFileRequested: function(idx, current) { win.promptRenameFile(idx, current) }
        }
                }
                // ----- page 1: Encontrar (Find); browse + search -----
                SearchView {
                    id: searchPage
                    Layout.fillWidth: true; Layout.fillHeight: true
                    onFreeSpaceRequested: function (bytes) { makeRoomPanel.targetBytes = bytes; makeRoomPanel.open = true }
                    onBackTo: function (page) { win.currentPage = page }
                }
                // ----- page 2: HUB -----
                HubView {
                    id: hubPage; Layout.fillWidth: true; Layout.fillHeight: true
                    onOpenSearch: function(q) { win.openSearch(q) }
                    onOpenTitle: function (work) { win.currentPage = 1; searchPage.openTitle(work, 2) }
                    onReturnTo: function (page) { win.currentPage = page }
                }
                // ----- page 3: Settings (fullscreen tab, was a top-level window) -----
                SettingsView {
                    id: settingsPage; Layout.fillWidth: true; Layout.fillHeight: true
                    isCurrent: win.currentPage === 3
                    onClosed: win.currentPage = 0
                }
            }
        }
    }

    AppDropOverlay {
        onTorrentUrlsDropped: function(urls) { win.enqueueTorrentUrls(urls) }
    }

    AppOverlays {
        id: overlays
        host: win
    }
    property alias openFileDlg: overlays.openFileDlg
    property alias addDlg: overlays.addDlg
    property alias addTorrentDlg: overlays.addTorrentDlg
    property alias removeDlg: overlays.removeDlg
    property alias makeRoomPanel: overlays.makeRoomPanel
    property alias inputPrompt: overlays.inputPrompt
    property alias updateDlg: overlays.updateDlg
    property alias diagnoseDlg: overlays.diagnoseDlg
    property alias createDlg: overlays.createDlg
    property alias addAddonDlg: overlays.addAddonDlg
    property alias releaseNotesDlg: overlays.releaseNotesDlg
    property alias welcomeDlg: overlays.welcomeDlg
    property alias aboutDlg: overlays.aboutDlg
    property alias inspectorDlg: overlays.inspectorDlg
    property alias pairingDlg: overlays.pairingDlg
    property alias inspectFileDlg: overlays.inspectFileDlg
    property alias exportTorrentDlg: overlays.exportTorrentDlg
    property alias importQbtDlg: overlays.importQbtDlg
    property alias refreshFlash: overlays.refreshFlash
    property alias torrentQueue: overlays.torrentQueue
    function enqueueTorrentUrls(urls) { overlays.enqueueTorrentUrls(urls) }
    function processTorrentQueue() { overlays.processTorrentQueue() }

    AppTour {
        id: appTour
        host: win
        toolBar: toolbar
    }
    property alias tourOverlay: appTour.tourOverlay

    // ================== TOP-LEVEL WINDOWS (lazy) ==================
    // Built on first open via Loader, not at startup: instantiating all of
    // them eagerly stalled the UI thread for seconds on launch.
    MainWindowLoaders {
        id: windowLoaders
        hubPage: hubPage
        searchPage: searchPage
    }
    property alias rssWinLoader: windowLoaders.rssWinLoader
    property alias shortcutsWinLoader: windowLoaders.shortcutsWinLoader
    property alias statsWinLoader: windowLoaders.statsWinLoader
    property alias wrappedWinLoader: windowLoaders.wrappedWinLoader
    property alias removedWinLoader: windowLoaders.removedWinLoader
    property alias logWinLoader: windowLoaders.logWinLoader
    property alias diagWinLoader: windowLoaders.diagWinLoader

    function showWin(loader) { windowLoaders.showWin(loader) }
    function showWrapped() { windowLoaders.showWrapped() }
    // Build a valid file: URL for a local path. On Windows a path is "C:/â€¦",
    // so plain "file://"+path yields "file://C:/â€¦" where QUrl reads "C:" as a
    // host and the image fails to load; Windows needs the triple-slash form.
    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        if (p.indexOf("http") === 0 || p.indexOf("qrc:") === 0 || p.indexOf("file:") === 0)
            return p
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }
    AppMediaConnections {
        host: win
        watchOverlay: appNotifications.gwOverlay
        loaders: windowLoaders
    }

    LibraryShortcuts {
        id: libraryShortcuts
        host: win
        library: library
        filterBar: libraryChrome.filterBar
        cmdPalette: appCmdPalette.cmdPalette
    }

    AppSplash { host: win }

}






