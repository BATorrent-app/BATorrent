// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent Home.html + batorrent-home.css (+ batorrent-home.js model)
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
    visible: true
    width: 1360
    height: 884
    // floor wide enough that the full toolbar (labels + speed module) always
    // fits — below this the RowLayout would have to clip, which is what made
    // the old icon-only "compact" hack feel broken.
    // classic keeps the old +188 rail budget; the top bar frees that width
    // Capped to the screen: on DPI-scaled laptops (150% on 1920 = 1280 logical)
    // an uncapped floor forces the window wider than the desktop and the right
    // column (grid detail sidebar) renders past the screen edge.
    minimumWidth: Math.min(layoutClassic ? 1288 : 1200,
                           Screen.desktopAvailableWidth > 0 ? Screen.desktopAvailableWidth : 1288)
    minimumHeight: 640
    color: Theme.bg
    // classic rail merges the macOS titlebar into its brand zone; the top-bar
    // layout keeps the native titlebar so the traffic lights live outside the bar
    flags: (Theme.unifiedChrome && layoutClassic) ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    title: (Theme.unifiedChrome && layoutClassic) ? "" : "BATorrent"

    // Close button hides to the tray instead of quitting (quitOnLastWindowClosed
    // is false). If no tray is available, really quit so the app can't get stuck
    // running with no window. Real quit otherwise goes through the tray/app menu.
    onClosing: function(close) {
        var toTray = (typeof settings === "undefined") || settings.get("closeToTray") !== false
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

    // current page of the content stack (0 Downloads · 1 Discover · 2 Search ·
    // 3 HUB · 4 Settings). Owned by the window so navigation works whichever
    // nav component (rail or top bar) is loaded.
    property int currentPage: 0
    onCurrentPageChanged: clearFilterFocus()
    // classic layout = the pre-4.5 left nav rail; default is the top bar.
    // Only one nav component is ever loaded — a hidden rail would keep its
    // carousel timer and ~30 session bindings alive for nothing.
    property bool layoutClassic: false
    // grid-mode detail panel: side inspector (default) or the bottom deck. Both
    // components already exist; this just picks which one the grid uses — a
    // side panel suits wide screens, a bottom deck suits narrow ones.
    property bool detailBottom: false
    // the contextual continue/download chip in the top bar (some users find it
    // redundant on the Downloads page)
    property bool showDownloadChip: true
    readonly property Item navHost: layoutClassic ? navRailLoader.item : navBarLoader.item

    // Selection/filter state lives on library. Leaf chrome takes `controller`
    // explicitly — no win.* aliases for selection anymore.
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
    function promptRenameFile(idx, current) {
        inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), current, "",
            function(t){ if (t.length > 0) session.renameSelectedFile(idx, t) })
    }
    // Shared by the File menu, the toolbar "Link" button and the empty state.
    function promptHttpDownload() {
        inputPrompt.openWith(i18n.t("menu_add_http"), i18n.t("prompt_http_url"), "", "https://…/file.zip",
            function(t){ if (t.length > 0) session.addHttpUrl(t) })
    }
    Connections {
        target: typeof settings !== "undefined" ? settings : null
        function onChanged() {
            var v = settings.get("layoutClassic")
            win.layoutClassic = (v === true || v === 1 || v === "1" || v === "true")
            win.detailBottom = settings.get("detailBottom") === true
            win.showDownloadChip = settings.get("showDownloadChip") !== false
        }
    }

    // startup splash — ceremony only when something happened: first run or the
    // first launch after an update. A routine (often magnet-click) launch goes
    // straight to the UI. The Settings toggle still kills it entirely.
    property bool showSplash: false
    Component.onCompleted: {
        // restore the last window size (only if it's still sane vs the minimums)
        if (typeof settings !== "undefined") {
            var sw = Number(settings.get("winWidth") || 0)
            var sh = Number(settings.get("winHeight") || 0)
            if (sw >= win.minimumWidth && sw <= Screen.desktopAvailableWidth) win.width = sw
            if (sh >= win.minimumHeight && sh <= Screen.desktopAvailableHeight) win.height = sh
        }
        if (typeof settings !== "undefined") library.classicMode = settings.get("classicMode") === true
        if (library.classicMode) library.gridView = false   // classic is a list layout
        if (typeof settings !== "undefined") {
            var lc = settings.get("layoutClassic")
            win.layoutClassic = (lc === true || lc === 1 || lc === "1" || lc === "true")
            win.detailBottom = settings.get("detailBottom") === true
            win.showDownloadChip = settings.get("showDownloadChip") !== false
        }
        if (typeof settings === "undefined") {
            showSplash = true
        } else {
            // read welcomeShown/lastSeenVersion BEFORE maybeShowWelcome mutates them
            var curVer = (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""
            var isFirstRun = settings.get("welcomeShown") !== true
            var isUpdate = curVer.length > 0 && settings.get("lastSeenVersion") !== curVer
            showSplash = settings.get("showSplash") !== false && (isFirstRun || isUpdate)
        }
        // Start hidden only when the login item launched us. Applying this to a
        // deliberate launch made the app look like it had failed to open: you
        // double-click the icon and nothing appears (tester report, beta7 #12).
        if (typeof launchedBySystem !== "undefined" && launchedBySystem
                && typeof settings !== "undefined" && settings.get("startTray") === true
                && trayIcon.available)
            win.visible = false
        if (!showSplash) win.maybeShowWelcome()
    }
    AppWindowLifecycle { host: win }

    function checkClipboardMagnet() {
        if (typeof session === "undefined") return
        if (magnetDlg.opened || addTorrentDlg.opened) return
        var m = session.clipboardMagnetIfNew()
        if (m.length > 0) magnetDlg.openWithMagnet(m)
    }
    // first launch → the interactive tour (opens with a welcome step); an update
    // (version changed) → the what's-new screen. Once each, never both, never on
    // a plain relaunch.
    function maybeShowWelcome() {
        if (typeof settings === "undefined") return
        var cur = (typeof themeBridge !== "undefined" && themeBridge.appVersion) ? themeBridge.appVersion : ""
        var firstRun = settings.get("welcomeShown") !== true
        if (firstRun) {
            settings.set("welcomeShown", true)
            welcomeDlg.mode = "welcome"
            welcomeDlg.open()
        } else if (cur.length > 0 && settings.get("lastSeenVersion") !== cur) {
            welcomeDlg.mode = "update"
            welcomeDlg.open()
        }
        if (cur.length > 0) settings.set("lastSeenVersion", cur)
    }

    // The tour runs once ever, right after the first welcome/update screen the
    // user closes (fresh install OR the big update). Later updates: dialog only.
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
    readonly property var presetCats: ["Apps", "Games", "Movies", "Series"]
    property int detailTab: 0   // 0 Geral · 1 Peers · 2 Arquivos · 3 Trackers · 4 Pedaços
    property bool detailsCollapsed: typeof settings !== "undefined" && settings.get("detailsCollapsed") === true
    function toggleDetailsCollapsed() {
        detailsCollapsed = !detailsCollapsed
        if (typeof settings !== "undefined") settings.set("detailsCollapsed", detailsCollapsed)
    }
    // Missing-files recovery: pick where the files actually live (or a fresh folder
    // to re-download into) — libtorrent moves storage there, then a recheck picks
    // up whatever's present. Shared by the context menu and the recovery banner.
    function promptSetLocation() { setLocationDlg.open() }
    property alias setLocationDlg: libraryShortcuts.setLocationDlg
    // Full-screen visual acknowledgement for a manual Refresh.
    function flashRefresh() { refreshFlash.flash() }

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

    // lock pins the panel to its current open/closed state, overriding auto-collapse
    property bool detailsLocked: typeof settings !== "undefined" && settings.get("detailsLocked") === true
    function toggleDetailsLocked() {
        detailsLocked = !detailsLocked
        if (typeof settings !== "undefined") settings.set("detailsLocked", detailsLocked)
    }
    // The Peers tab pulls every peer from libtorrent — only keep it live while open.
    readonly property bool peersTabOpen: win.hasSel && win.detailTab === 1
    onPeersTabOpenChanged: if (typeof session !== "undefined") session.setDetailPeersActive(peersTabOpen)

    // live model from C++ (QmlTorrentFilterProxy → QmlPosterModel). Roles:
    // torrentName, metaTitle, stateKey, progress(0..1), posterPath, stateString,
    // downSpeed, upSpeed, category, numPeers, downRate, upRate, size, infoHash.
    readonly property var model: typeof torrentModel !== "undefined" ? torrentModel : null
    readonly property bool hasSel: typeof session !== "undefined" && session.hasSelection
    // auto-collapse when there's nothing to show, unless the user locked the panel's state
    readonly property bool detailsShownCollapsed: win.detailsLocked ? win.detailsCollapsed : (win.detailsCollapsed || !win.hasSel)

    // ----- state→color helpers (keyed by real stateKey) -----
    function fillFor(k) {
        // match the dot/text semantics: done = green, seeding = amber — a red
        // 100% pill reads as an error at a glance
        if (k === "finished" || k === "completed") return Theme.grn
        if (k === "seeding") return Theme.amber
        if (k === "paused" || k === "queued") return Theme.pausedFill
        return Theme.accent
    }
    function textFor(k) {
        if (k === "finished" || k === "completed") return Theme.grn   // done = green
        if (k === "seeding") return Theme.up                          // seeding = amber
        if (k === "paused" || k === "queued") return Theme.t3
        return Theme.accentText
    }
    function dotFor(k) {
        if (k === "finished" || k === "completed") return Theme.grn   // done = green
        if (k === "seeding") return Theme.amber                       // seeding = amber
        if (k === "paused" || k === "queued") return Theme.t4         // paused = gray
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
    // something else claims it. Every gesture that means "I'm done typing" —
    // picking a torrent, clicking blank space, leaving the page — routes here.
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
        var builtins = ["Apps", "Games", "Movies", "Series"]
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
        default:       return value   // custom category — show as the user typed it
        }
    }
    function openContext(proxyRow) {
        // right-clicking inside an existing multi-selection must not collapse
        // it to just this row — that silently turned "remove 3 selected" into
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

    // Must stay a direct child of Window (not nested under Layout) — macOS menus
    // break otherwise.
    AppMenuBar {
        host: win
        openFileDialog: openFileDlg
        magnetDialog: magnetDlg
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

    // ----- system tray -----
    AppTray {
        id: trayIcon
        onRestoreRequested: { win.show(); win.raise(); win.requestActivate() }
        onContextRequested: function(geo) { trayPopup.popUpAt(geo) }
    }

    // Background events → in-app toast (when the window is up) AND the native
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

    AppCommandPalette {
        id: appCmdPalette
        host: win
        settingsPage: settingsPage
        library: library
        magnetDlg: overlays.magnetDlg
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
        AppWinMenuBar {
            id: winMenuBar
            visible: Qt.platform.os !== "osx"
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? implicitHeight : 0
            host: win
            openFileDialog: openFileDlg
            magnetDialog: magnetDlg
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
            makeRoomPanel: win.makeRoomPanel
            settingsPage: settingsPage
        }

        // ===== top nav bar (default layout) =====
        Loader {
            id: navBarLoader
            Layout.fillWidth: true
            active: !win.layoutClassic
            visible: active
            sourceComponent: NavBar {
                currentIndex: win.currentPage
                showDownloadChip: win.showDownloadChip
                onPageRequested: function(page) { win.currentPage = page }
                onSettingsClicked: win.currentPage = 3
                onVpnClicked: { settingsPage.sec = 3; win.currentPage = 3 }
                onSelectTorrent: function(infoHash) { win.selectTorrentByHash(infoHash) }
                onMakeRoomRequested: { makeRoomPanel.targetBytes = 0; makeRoomPanel.open = true }
                onAboutRequested: aboutDlg.open()
            }
        }

        // ===== nav rail (classic layout) + content stack (4.0 hub shell) =====
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Loader {
                id: navRailLoader
                Layout.fillHeight: true
                active: win.layoutClassic
                visible: active
                sourceComponent: NavRail {
                    currentIndex: win.currentPage
                    showDownloadChip: win.showDownloadChip
                    onPageRequested: function(page) { win.currentPage = page }
                    onSettingsClicked: win.currentPage = 3
                    onVpnClicked: { settingsPage.sec = 3; win.currentPage = 3 }
                    onSelectTorrent: function(infoHash) { win.selectTorrentByHash(infoHash) }
                    onMakeRoomRequested: { makeRoomPanel.targetBytes = 0; makeRoomPanel.open = true }
                    onAboutRequested: aboutDlg.open()
                }
            }

            StackLayout {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: win.currentPage

                // Directional page switch. It used to rise 12px from below no
                // matter which tab you came from, which says nothing — the tabs
                // sit in a row, so moving right should enter from the right and
                // moving left from the left. Now the motion matches the gesture,
                // and going back reverses it instead of repeating it.
                transform: Translate { id: pageShift }
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
                        NumberAnimation { target: contentStack; property: "opacity"; from: 0.0; to: 1.0; duration: 170; easing.type: Easing.OutCubic }
                        NumberAnimation {
                            target: pageShift; property: "x"
                            from: contentStack.enterFrom; to: 0
                            duration: 260; easing.type: Easing.OutCubic
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
            onOpenFile: openFileDlg.open()
            onAddMagnet: magnetDlg.open()
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
            onAddMagnetRequested: magnetDlg.open()
            onAddLinkRequested: promptHttpDownload()
            onRenameFileRequested: function(idx, current) { win.promptRenameFile(idx, current) }
        }
                }
                // ----- page 1: Encontrar (Find) — browse + search -----
                SearchView {
                    id: searchPage
                    Layout.fillWidth: true; Layout.fillHeight: true
                    onFreeSpaceRequested: function (bytes) { makeRoomPanel.targetBytes = bytes; makeRoomPanel.open = true }
                }
                // ----- page 2: HUB -----
                HubView {
                    id: hubPage; Layout.fillWidth: true; Layout.fillHeight: true
                    onOpenSearch: function(q) { win.currentPage = 1; searchPage.runQuery(q) }
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
    property alias magnetDlg: overlays.magnetDlg
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
    // Built on first open via Loader, not at startup — instantiating all of
    // them eagerly stalled the UI thread for seconds on launch.
    MainWindowLoaders {
        id: windowLoaders
        hubPage: hubPage
    }
    property alias rssWinLoader: windowLoaders.rssWinLoader
    property alias shortcutsWinLoader: windowLoaders.shortcutsWinLoader
    property alias statsWinLoader: windowLoaders.statsWinLoader
    property alias wrappedWinLoader: windowLoaders.wrappedWinLoader
    property alias removedWinLoader: windowLoaders.removedWinLoader
    property alias logWinLoader: windowLoaders.logWinLoader
    property alias diagWinLoader: windowLoaders.diagWinLoader
    property alias playerWinLoader: windowLoaders.playerWinLoader

    function showWin(loader) { windowLoaders.showWin(loader) }
    function showWrapped() { windowLoaders.showWrapped() }
    // Build a valid file: URL for a local path. On Windows a path is "C:/…",
    // so plain "file://"+path yields "file://C:/…" where QUrl reads "C:" as a
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






