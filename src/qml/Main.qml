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

Window {
    id: win
    visible: true
    width: 1360
    height: 884
    // floor wide enough that the full toolbar (labels + speed module) always
    // fits — below this the RowLayout would have to clip, which is what made
    // the old icon-only "compact" hack feel broken.
    // classic keeps the old +188 rail budget; the top bar frees that width
    minimumWidth: layoutClassic ? 1288 : 1200
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
    // classic layout = the pre-4.5 left nav rail; default is the top bar.
    // Only one nav component is ever loaded — a hidden rail would keep its
    // carousel timer and ~30 session bindings alive for nothing.
    property bool layoutClassic: false
    readonly property Item navHost: layoutClassic ? navRailLoader.item : navBarLoader.item
    function selectTorrentByHash(infoHash) {
        if (typeof session === "undefined") return
        if (!session.selectByInfoHash(infoHash)) { win.setFilter("all"); session.selectByInfoHash(infoHash) }
    }
    function promptRenameFile(idx, current) {
        inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), current, "",
            function(t){ if (t.length > 0) session.renameSelectedFile(idx, t) })
    }
    Connections {
        target: typeof settings !== "undefined" ? settings : null
        function onChanged() {
            var v = settings.get("layoutClassic")
            win.layoutClassic = (v === true || v === 1 || v === "1" || v === "true")
        }
    }

    property int selected: -1          // focus row (drives the detail panel)
    property var selectedRows: []      // multi-selection (proxy rows)
    property int anchorRow: -1         // shift-range anchor
    property bool gridView: true
    // classic mode: a clean, cover-less, raw-name list for users the media-hub
    // styling puts off (qBittorrent-like). Persisted; implies the list layout.
    property bool classicMode: false
    onClassicModeChanged: if (typeof settings !== "undefined") settings.set("classicMode", classicMode)
    property string activeFilter: "all"
    property string catFilter: ""
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
        if (typeof settings !== "undefined") win.classicMode = settings.get("classicMode") === true
        if (win.classicMode) win.gridView = false   // classic is a list layout
        if (typeof settings !== "undefined") {
            var lc = settings.get("layoutClassic")
            win.layoutClassic = (lc === true || lc === 1 || lc === "1" || lc === "true")
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
        // start hidden in the tray if the user asked for it (and a tray exists)
        if (typeof settings !== "undefined" && settings.get("startTray") === true && trayIcon.available)
            win.visible = false
        if (!showSplash) win.maybeShowWelcome()
    }
    // persist window size (debounced; only while in normal windowed state so a
    // maximize/fullscreen doesn't get saved as the restore size)
    Timer {
        id: geomSave
        interval: 600; repeat: false
        onTriggered: {
            if (typeof settings === "undefined") return
            if (win.visibility !== Window.Windowed) return
            settings.set("winWidth", win.width)
            settings.set("winHeight", win.height)
        }
    }
    onWidthChanged: if (win.visibility === Window.Windowed) geomSave.restart()
    onHeightChanged: if (win.visibility === Window.Windowed) geomSave.restart()
    // once the app has survived a few seconds, clear the boot-crash sentinel
    // (used by main.cpp's safe-mode recovery)
    Timer {
        interval: 3500; running: true; repeat: false
        onTriggered: if (typeof themeBridge !== "undefined") themeBridge.markBootHealthy()
    }
    // Regaining focus with a fresh magnet link on the clipboard pops the Add
    // dialog pre-filled — copy a link in the browser, alt-tab back, confirm.
    onActiveChanged: if (active) win.checkClipboardMagnet()
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
    // lock pins the panel to its current open/closed state, overriding auto-collapse
    property bool detailsLocked: typeof settings !== "undefined" && settings.get("detailsLocked") === true
    function toggleDetailsLocked() {
        detailsLocked = !detailsLocked
        if (typeof settings !== "undefined") settings.set("detailsLocked", detailsLocked)
    }
    // The Peers tab pulls every peer from libtorrent — only keep it live while open.
    readonly property bool peersTabOpen: win.hasSel && win.detailTab === 1
    onPeersTabOpenChanged: if (typeof session !== "undefined") session.setDetailPeersActive(peersTabOpen)
    property string sortColumn: ""
    property bool sortAsc: true

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
    function _commitSel() {
        if (typeof session === "undefined" || typeof torrentFilter === "undefined") return
        var src = []
        for (var i = 0; i < win.selectedRows.length; ++i) {
            var s = torrentFilter.mapToSource(win.selectedRows[i])
            if (s >= 0) src.push(s)
        }
        session.setSelectedRows(src)
    }
    function selectRow(proxyRow, mods) {
        mods = mods || 0
        var ctrl = (mods & Qt.ControlModifier) || (mods & Qt.MetaModifier)
        var shift = (mods & Qt.ShiftModifier)
        var rows = win.selectedRows.slice()
        if (shift && win.anchorRow >= 0) {
            rows = []
            var a = Math.min(win.anchorRow, proxyRow)
            var b = Math.max(win.anchorRow, proxyRow)
            for (var i = a; i <= b; ++i) rows.push(i)
        } else if (ctrl) {
            var idx = rows.indexOf(proxyRow)
            if (idx >= 0) rows.splice(idx, 1); else rows.push(proxyRow)
            win.anchorRow = proxyRow
        } else {
            rows = [proxyRow]
            win.anchorRow = proxyRow
        }
        win.selectedRows = rows
        win.selected = proxyRow
        _commitSel()
        if (filterBar.searchInput.activeFocus) filterBar.searchInput.focus = false   // release the search field so its accent border doesn't stick
    }
    function isRowSelected(proxyRow) { return win.selectedRows.indexOf(proxyRow) >= 0 }
    function selectAll() {
        if (typeof session === "undefined" || typeof torrentFilter === "undefined") return
        var rows = []
        for (var s = 0; s < session.torrentCount; ++s) {
            var p = torrentFilter.mapFromSource(s)
            if (p >= 0) rows.push(p)
        }
        rows.sort(function(x, y) { return x - y })
        win.selectedRows = rows
        win.anchorRow = rows.length > 0 ? rows[0] : -1
        win.selected = rows.length > 0 ? rows[rows.length - 1] : -1
        _commitSel()
    }
    function toggleSort(col) {
        if (win.sortColumn === col) win.sortAsc = !win.sortAsc
        else { win.sortColumn = col; win.sortAsc = true }
        if (typeof torrentFilter !== "undefined") torrentFilter.setSortColumn(col, win.sortAsc)
    }
    function setFilter(f) {
        win.activeFilter = f
        if (typeof torrentFilter !== "undefined") torrentFilter.setFilterState(f)
    }
    function applyCatFilter(c) {
        win.catFilter = c
        if (typeof torrentFilter !== "undefined") torrentFilter.setCategoryFilter(c)
    }
    // Categories are stored/filtered by a stable language-independent value
    // (presetCats); only the *display* is translated. Switching language must
    // not desync a torrent's category from the filter/menu, so never store the
    // translated label.
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
    // keep the visual selection glued to the item when the queue reorders
    function remapRow(r, from, to) {
        if (r === from) return to
        if (from < to) return (r > from && r <= to) ? r - 1 : r
        else           return (r >= to && r < from) ? r + 1 : r
    }
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onQueueMoved(from, to) {
            var rows = []
            for (var i = 0; i < win.selectedRows.length; ++i)
                rows.push(win.remapRow(win.selectedRows[i], from, to))
            win.selectedRows = rows
            if (win.selected >= 0) win.selected = win.remapRow(win.selected, from, to)
            if (win.anchorRow >= 0) win.anchorRow = win.remapRow(win.anchorRow, from, to)
        }
    }
    function gridCols() { return Math.max(1, Math.floor(libraryView.grid.width / libraryView.grid.cellWidth)) }
    function moveSel(step) {
        var view = win.gridView ? libraryView.grid : libraryView.list
        var n = view.count
        if (n <= 0) return
        var cur = win.selected
        var next = cur < 0 ? (step > 0 ? 0 : n - 1)
                           : Math.max(0, Math.min(n - 1, cur + step))
        win.selectRow(next)
        view.positionViewAtIndex(next, win.gridView ? GridView.Contain : ListView.Contain)
    }

    // ----- shared context menu (right-click on grid tile / list row) -----
    Menu {
        id: ctxMenu
        modal: true
        implicitWidth: 220
        background: Rectangle {
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            radius: 8
        }
        component CtxItem: MenuItem {
            id: ci
            implicitHeight: enabled ? 30 : 1
            visible: enabled
            padding: 0
            contentItem: Text {
                leftPadding: 14
                rightPadding: 14
                text: ci.text
                color: ci.highlighted ? Theme.t1 : Theme.t2
                font.pixelSize: 12
                font.family: Theme.fontSans
                verticalAlignment: Text.AlignVCenter
            }
            background: Rectangle {
                color: ci.highlighted ? Theme.hover : "transparent"
                radius: 5
            }
            arrow: Text {
                visible: ci.subMenu
                text: "›"
                color: ci.highlighted ? Theme.t1 : Theme.t4
                font.pixelSize: 16
                font.family: Theme.fontSans
                x: ci.width - width - 12
                y: (ci.height - height) / 2
            }
        }
        component Sep: MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
        delegate: CtxItem {}

        // Games lead the menu with an accent button (state-driven, Steam model):
        // Play when ready, else Install. A torrent is a game XOR a video, so only
        // one of gameCtx/playCtx is ever visible — both sit at the very top.
        MenuItem {
            id: gameCtx
            // depend on selectedHash (NOTIFY selectionChanged), not win.selected:
            // the QML row changes BEFORE the C++ selection commits, so binding to
            // it showed the PREVIOUS torrent's game state (movies got "Install")
            readonly property bool gReady: (session.selectedHash, session.selectedGameState() === 4)
            visible: (session.selectedHash, session.selectedIsGame() && session.selectedGameState() !== 5)
            height: visible ? 36 : 0
            implicitHeight: height
            padding: 0
            onTriggered: gameCtx.gReady ? session.playSelectedGame() : session.installSelectedGame()
            contentItem: Row {
                leftPadding: 12
                spacing: 9
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    src: gameCtx.gReady ? "qrc:/icons/play.svg" : "qrc:/icons/download.svg"; tint: Theme.accent; s: 13
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, gameCtx.gReady ? i18n.t("hub_gs_play") : i18n.t("hub_gs_install"))
                    color: Theme.accent
                    font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
            }
            background: Rectangle {
                radius: 6
                color: gameCtx.highlighted
                       ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                       : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.07)
            }
        }
        // Play leads the menu as a minimalist accent button (not a plain row) so
        // the primary action for a video torrent stands out at a glance.
        MenuItem {
            id: playCtx
            visible: session.selectedHasVideo
            height: visible ? 36 : 0
            implicitHeight: height
            padding: 0
            onTriggered: session.playSelected()
            contentItem: Row {
                leftPadding: 12
                spacing: 9
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    src: "qrc:/icons/play.svg"; tint: Theme.accent; s: 13
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: (i18n.language, i18n.t("ctx_play"))
                    color: Theme.accent
                    font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
            }
            background: Rectangle {
                radius: 6
                color: playCtx.highlighted
                       ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.18)
                       : Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.07)
            }
        }

        // Common actions stay one click; the rest is grouped into submenus so
        // the menu doesn't run the whole height of the screen.
        CtxItem { text: (i18n.language, i18n.t("tb_pause")); enabled: !session.selectedPaused; onTriggered: session.pauseSelected() }
        CtxItem { text: (i18n.language, i18n.t("tb_resume")); enabled: session.selectedPaused; onTriggered: session.resumeSelected() }
        CtxItem { text: (i18n.language, i18n.t("ctx_open_folder")); onTriggered: session.openSaveFolder() }
        CtxItem { text: (i18n.language, i18n.t("ctx_copy_path")); onTriggered: session.copySelectedContentPath() }
        CtxItem {
            visible: Qt.platform.os === "windows"
            height: visible ? implicitHeight : 0
            text: (i18n.language, i18n.t("ctx_defender_exclude"))
            onTriggered: session.excludeTorrentFromDefender(torrentFilter.mapToSource(win.selected))
        }
        CtxItem {
            visible: session.selectedHasArchives
            height: visible ? implicitHeight : 0
            text: (i18n.language, i18n.t("ctx_extract"))
            onTriggered: inputPrompt.openWith(i18n.t("ctx_extract"), i18n.t("extract_password_label"),
                                              "", i18n.t("extract_password_ph"),
                                              function(pw){ session.extractSelected(pw) })
        }
        CtxItem { text: (i18n.language, i18n.t("ctx_rename")); onTriggered: inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), session.selectedName, "", function(t){ session.renameSelected(t) }) }
        CtxItem {
            // only once the data is done — marking mid-download freezes the torrent
            visible: session.selectedDataDone || session.selectedCompleted
            height: visible ? implicitHeight : 0
            text: session.selectedCompleted ? (i18n.language, i18n.t("ctx_unmark_completed_plain")) : (i18n.language, i18n.t("ctx_mark_completed_plain"))
            onTriggered: session.selectedCompleted ? session.unmarkSelectedCompleted() : session.markSelectedCompleted()
        }
        Sep {}

        Menu {
            title: (i18n.language, i18n.t("ctx_grp_queue"))
            implicitWidth: 200
            delegate: CtxItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CtxItem { text: (i18n.language, i18n.t("ctx_queue_top")); onTriggered: session.queueTopSelected() }
            CtxItem { text: (i18n.language, i18n.t("ctx_queue_up")); onTriggered: session.queueUpSelected() }
            CtxItem { text: (i18n.language, i18n.t("ctx_queue_down")); onTriggered: session.queueDownSelected() }
            CtxItem { text: (i18n.language, i18n.t("ctx_queue_bottom")); onTriggered: session.queueBottomSelected() }
        }
        Menu {
            title: (i18n.language, i18n.t("ctx_grp_download"))
            implicitWidth: 230
            delegate: CtxItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CtxItem { text: (i18n.language, i18n.t("ctx_speed_down")); onTriggered: inputPrompt.openWith(i18n.t("ctx_speed_down"), i18n.t("prompt_speed_kbs"), String(session.selectedDownloadLimit()), "0", function(t){ session.setSelectedDownloadLimit(parseInt(t) || 0) }) }
            CtxItem { text: (i18n.language, i18n.t("ctx_speed_up")); onTriggered: inputPrompt.openWith(i18n.t("ctx_speed_up"), i18n.t("prompt_speed_kbs"), String(session.selectedUploadLimit()), "0", function(t){ session.setSelectedUploadLimit(parseInt(t) || 0) }) }
            CtxItem { text: (session.selectedSequential() ? "✓ " : "") + (i18n.language, i18n.t("ctx_sequential")); onTriggered: session.setSelectedSequential(!session.selectedSequential()) }
            CtxItem { text: (session.selectedForceStart ? "✓ " : "") + (i18n.language, i18n.t("ctx_force_start_plain")); onTriggered: session.setSelectedForceStart(!session.selectedForceStart) }
            CtxItem { text: (session.selectedSuperSeeding ? "✓ " : "") + (i18n.language, i18n.t("ctx_super_seeding")); onTriggered: session.setSelectedSuperSeeding(!session.selectedSuperSeeding) }
        }
        Menu {
            title: (i18n.language, i18n.t("ctx_grp_organize"))
            implicitWidth: 200
            delegate: CatItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CatItem { text: (session.selectedCategory() === "Apps"   ? "✓ " : "") + win.catLabel("Apps");   onTriggered: session.setSelectedCategory("Apps") }
            CatItem { text: (session.selectedCategory() === "Games"  ? "✓ " : "") + win.catLabel("Games");  onTriggered: session.setSelectedCategory("Games") }
            CatItem { text: (session.selectedCategory() === "Movies" ? "✓ " : "") + win.catLabel("Movies"); onTriggered: session.setSelectedCategory("Movies") }
            CatItem { text: (session.selectedCategory() === "Series" ? "✓ " : "") + win.catLabel("Series"); onTriggered: session.setSelectedCategory("Series") }
            MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
            CatItem { text: (session.selectedCategory() === "" ? "✓ " : "") + (i18n.language, i18n.t("category_none")); onTriggered: session.setSelectedCategory("") }
            CatItem { text: (i18n.language, i18n.t("ctx_category_other")); onTriggered: inputPrompt.openWith(i18n.t("ctx_category"), i18n.t("prompt_category_name"), session.selectedCategory(), i18n.t("prompt_category_eg"), function(t){ session.setSelectedCategory(t) }) }
            MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
            CatItem { text: (i18n.language, i18n.t("ctx_add_tag")); onTriggered: inputPrompt.openWith(i18n.t("prompt_add_tag_title"), i18n.t("prompt_new_tag"), "", i18n.t("prompt_tag_eg"), function(t){ if (t.length === 0) return; var tags = session.selectedTagList(); if (tags.indexOf(t) < 0) { tags.push(t); session.setSelectedTags(tags) } }) }
            CatItem { text: (i18n.language, i18n.t("tracker_add")); onTriggered: inputPrompt.openWith(i18n.t("prompt_add_tracker_title"), i18n.t("prompt_tracker_url"), "", "udp://tracker:porta", function(t){ session.addTrackerToSelected(t) }) }
        }
        Menu {
            title: (i18n.language, i18n.t("ctx_grp_copy"))
            implicitWidth: 180
            delegate: CtxItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CtxItem { text: (i18n.language, i18n.t("ctx_copy_name")); onTriggered: session.copySelectedName() }
            CtxItem { text: (i18n.language, i18n.t("ctx_copy_magnet")); onTriggered: session.copyMagnetLink() }
            CtxItem { text: (i18n.language, i18n.t("ctx_copy_hash")); onTriggered: session.copyInfoHash() }
        }
        Menu {
            title: (i18n.language, i18n.t("ctx_fix_cover"))
            implicitWidth: 200
            delegate: CtxItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CtxItem { text: win.catLabel("Movies"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Euphoria", function(t){ session.relinkSelectedCover(t, "movie") }) }
            CtxItem { text: win.catLabel("Series"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Euphoria", function(t){ session.relinkSelectedCover(t, "series") }) }
            CtxItem { text: win.catLabel("Games"); onTriggered: inputPrompt.openWith(i18n.t("ctx_fix_cover"), i18n.t("ctx_fix_cover_hint"), "", "Cyberpunk 2077", function(t){ session.relinkSelectedCover(t, "game") }) }
            MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
            CtxItem { text: (i18n.language, i18n.t("ctx_no_cover")); onTriggered: session.clearSelectedCover() }
        }
        Menu {
            title: (i18n.language, i18n.t("ctx_grp_more"))
            implicitWidth: 230
            delegate: CtxItem {}
            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            CtxItem { text: (i18n.language, i18n.t("ctx_move_storage")); onTriggered: setLocationDlg.open() }
            CtxItem { text: (i18n.language, i18n.t("ctx_force_recheck")); onTriggered: session.forceRecheckSelected() }
            CtxItem { text: (i18n.language, i18n.t("ctx_force_reannounce")); onTriggered: session.forceReannounceSelected() }
            CtxItem { text: (i18n.language, i18n.t("ctx_export_torrent")); onTriggered: exportTorrentDlg.open() }
            CtxItem { text: (i18n.language, i18n.t("ctx_why_slow")); onTriggered: { diagnoseDlg.body = session.diagnoseSelectedSlow(); diagnoseDlg.open() } }
            CtxItem { text: (i18n.language, i18n.t("ctx_stop_seeding")); onTriggered: session.stopSeedingSelected() }
            Menu {
                title: (i18n.language, i18n.t("ctx_seed_rules"))
                implicitWidth: 220
                delegate: CtxItem {}
                background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
                CtxItem { text: (i18n.language, i18n.t("ctx_seed_use_default")); onTriggered: { session.setSelectedStopAfter(-1); session.setSelectedMaxSeedDays(-1) } }
                CtxItem { text: (session.selectedStopAfter() === 1 ? "✓ " : "") + (i18n.language, i18n.t("ctx_stop_after_download")); onTriggered: session.setSelectedStopAfter(session.selectedStopAfter() === 1 ? 0 : 1) }
                CtxItem { text: (i18n.language, i18n.t("ctx_max_seed_time")); onTriggered: inputPrompt.openWith(i18n.t("ctx_max_seed_time"), i18n.t("ctx_max_seed_prompt"), String(Math.max(0, session.selectedMaxSeedDays())), "0", function(t){ session.setSelectedMaxSeedDays(parseInt(t) || 0) }) }
            }
        }
        Sep {}
        CtxItem { text: (i18n.language, i18n.t("ctx_remove")); onTriggered: removeDlg.open() }
    }

    Shortcut { sequence: StandardKey.SelectAll; onActivated: win.selectAll() }
    // Smart Paste: Ctrl+V adds a magnet / 40-char hash / .torrent URL straight from
    // the clipboard — unless a text field has focus (then let it paste text).
    Shortcut {
        sequences: [ StandardKey.Paste ]
        onActivated: {
            var f = win.activeFocusItem
            if (f && f.cursorPosition !== undefined) return
            session.smartPaste()
        }
    }

    // "Definir local…" — move the selected torrent's storage to a new folder
    FolderDialog {
        id: setLocationDlg
        title: (i18n.language, i18n.t("ctx_move_storage_title"))
        onAccepted: session.moveSelectedStorage(session.urlToLocalPath(setLocationDlg.selectedFolder.toString()))
    }

    // ================== NATIVE MENU BAR (ported from mainwindow.cpp) ==================
    Platform.MenuBar {
        Platform.Menu {
            title: (i18n.language, i18n.t("menu_file_title"))
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_open_torrent")); shortcut: StandardKey.Open; onTriggered: openFileDlg.open() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_add_magnet")); shortcut: "Ctrl+M"; onTriggered: magnetDlg.open() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_add_url")); shortcut: "Ctrl+U"; onTriggered: inputPrompt.openWith(i18n.t("menu_add_url"), i18n.t("prompt_torrent_url"), "", "https://…/file.torrent", function(t){ if (t.length > 0) session.addTorrentUrl(t) }) }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_create_torrent")); onTriggered: createDlg.open() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_inspect_torrent")); onTriggered: inspectFileDlg.open() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_import_qbt")); onTriggered: importQbtDlg.open() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_recently_removed")); onTriggered: win.showWin(removedWinLoader) }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_quit")); shortcut: StandardKey.Quit; onTriggered: Qt.quit() }
        }
        Platform.Menu {
            title: (i18n.language, i18n.t("menu_torrent_title"))
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_pause")); enabled: win.hasSel; onTriggered: session.pauseSelected() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_resume")); enabled: win.hasSel; onTriggered: session.resumeSelected() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_pause_all")); onTriggered: if (typeof session !== "undefined") session.pauseAll() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_resume_all")); onTriggered: if (typeof session !== "undefined") session.resumeAll() }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_remove")); shortcut: StandardKey.Delete; enabled: win.hasSel; onTriggered: removeDlg.open() }
        }
        Platform.Menu {
            title: (i18n.language, i18n.t("menu_settings_title"))
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_preferences")); shortcut: StandardKey.Preferences; onTriggered: win.currentPage = 3 }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_addons")); onTriggered: addAddonDlg.open() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_rss")); onTriggered: win.showWin(rssWinLoader) }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_pair")); onTriggered: { pairingDlg.reload(); pairingDlg.open() } }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_search_torrents")); onTriggered: win.currentPage = 1 }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_statistics")); onTriggered: win.showWin(statsWinLoader) }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_speedtest")); onTriggered: Qt.openUrlExternally("https://fast.com") }
        }
        Platform.Menu {
            title: (i18n.language, i18n.t("menu_help_title"))
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_tour")); onTriggered: tourOverlay.start() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_whatsnew")); onTriggered: { welcomeDlg.mode = "update"; welcomeDlg.open() } }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_release_notes")); onTriggered: releaseNotesDlg.open() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_shortcuts")); onTriggered: win.showWin(shortcutsWinLoader) }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_logs")); shortcut: "Ctrl+Shift+L"; onTriggered: win.showWin(logWinLoader) }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_diagnostics")); onTriggered: win.showWin(diagWinLoader) }
            Platform.MenuItem {
                text: (i18n.language, i18n.t("menu_check_updates"))
                enabled: typeof updater !== "undefined" && updater !== null
                onTriggered: if (typeof updater !== "undefined" && updater) updater.check(false)
            }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_feedback")); onTriggered: Qt.openUrlExternally("https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform") }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_donate")); onTriggered: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19") }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_about")); role: Platform.MenuItem.AboutRole; onTriggered: aboutDlg.open() }
        }
    }

    // ----- system tray -----
    // Left click (Trigger) / double click restores the window — Windows
    // convention. Right click opens the context menu automatically. The rich
    // mini-stats popup is folded into the menu's first (disabled) line.
    Platform.SystemTrayIcon {
        id: trayIcon
        visible: true
        // Recolor the logo for the OS tray scheme so it isn't invisible on a
        // light Windows tray. The ?v= suffix busts the image-provider cache.
        icon.source: (typeof themeBridge !== "undefined" && themeBridge.osLight)
                     ? "image://applogo/dark?v=l" : "image://applogo/light?v=d"
        icon.mask: false
        tooltip: "BATorrent"
        // Right-click shows our custom painted popup (TrayPopupWindow) on EVERY
        // platform. The native Qt.labs `menu` is intentionally not used: it never
        // popped on the Windows tray here, and the painted popup is the design we
        // want anyway. Left/double click restores the window.
        onActivated: function(reason) {
            if (reason === Platform.SystemTrayIcon.Trigger
                || reason === Platform.SystemTrayIcon.DoubleClick) {
                win.show(); win.raise(); win.requestActivate()
            } else if (reason === Platform.SystemTrayIcon.Context) {
                trayPopup.popUpAt(trayIcon.geometry)
            }
        }
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

    // background events (finished, error, kill switch, RSS)
    Connections {
        target: typeof notifications !== "undefined" ? notifications : null
        ignoreUnknownSignals: true
        function onNotify(title, body, level) { win.notifyUser(title, body, level) }
    }

    // session-originated toasts (stream feedback, etc.)
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onToast(title, body) { win.notifyUser(title, body, 0) }
        // a movie/series finished downloading → actionable "Play now" toast
        function onMovieReady(infoHash, name) {
            if (win.visible && win.visibility !== Window.Minimized && win.visibility !== Window.Hidden)
                toastHost.show(i18n.t("toast_movie_ready"), name, 3, "play:" + infoHash, i18n.t("toast_play_now"))
            else
                win.notifyUser(i18n.t("toast_movie_ready"), name, 3)
        }
    }

    // Own schema instance so the palette can index individual options — it's
    // available before SettingsView is built, avoiding a binding-order race.
    SettingsSchema { id: paletteSchema }

    // Ctrl/⌘+K command palette — actions + torrent jump
    CommandPalette {
        id: cmdPalette
        actions: {
            var l = i18n.language   // re-evaluate labels on language switch
            var acts = [
                { label: i18n.t("magnet_title"), run: function() { magnetDlg.open() } },
                { label: i18n.t("menu_open_torrent"), run: function() { openFileDlg.open() } },
                { label: i18n.t("menu_create_torrent"), run: function() { createDlg.open() } },
                { label: i18n.t("menu_pause_all"), run: function() { if (typeof session !== "undefined") session.pauseAll() } },
                { label: i18n.t("menu_resume_all"), run: function() { if (typeof session !== "undefined") session.resumeAll() } },
                { label: i18n.t("tb_alt_speed"), hint: i18n.t("palette_hint_toggle"), run: function() { if (typeof session !== "undefined") session.setAltSpeedsActive(!session.altSpeedsActive) } },
                { label: i18n.t("nav_downloads"), hint: i18n.t("palette_hint_page"), run: function() { win.currentPage = 0 } },
                { label: i18n.t("nav_find"), hint: i18n.t("palette_hint_page"), run: function() { win.currentPage = 1 } },
                { label: i18n.t("nav_hub"), hint: i18n.t("palette_hint_page"), run: function() { win.currentPage = 2 } },
                { label: i18n.t("tb_settings"), run: function() { win.currentPage = 3 } },
                { label: i18n.t("menu_rss"), run: function() { win.showWin(rssWinLoader) } },
                { label: i18n.t("menu_statistics"), run: function() { win.showWin(statsWinLoader) } },
                { label: i18n.t("wrapped_title"), run: function() { win.showWrapped() } },
                { label: i18n.t("menu_logs"), run: function() { win.showWin(logWinLoader) } },
                { label: i18n.t("menu_diagnostics"), run: function() { win.showWin(diagWinLoader) } },
                { label: i18n.t("shortcuts_title2"), run: function() { win.showWin(shortcutsWinLoader) } }
            ]
            // settings sections + a few high-value deep links — so "torrent search",
            // "proxy", "network", etc. are reachable straight from the palette
            var setNav = ["detail_general", "detail_kv_speed", "settings_network", "set_nav_vpn",
                          "set_nav_proxy", "set_nav_webui", "set_nav_notif", "set_nav_addons", "settings_advanced"]
            for (var si = 0; si < setNav.length; ++si) {
                (function(idx) {
                    acts.push({ label: i18n.t("tb_settings") + " · " + i18n.t(setNav[idx]),
                                hint: i18n.t("palette_hint_page"),
                                run: function() { settingsPage.sec = idx; win.currentPage = 3 } })
                })(si)
            }
            acts.push({ label: i18n.t("set_grp_torrent_search"), hint: i18n.t("tb_settings"),
                        run: function() { settingsPage.sec = 7; win.currentPage = 3 } })
            // individual settings options — so Ctrl+K finds "Memory guard",
            // "Preallocate", etc., not just the section. Jumps to the option via
            // the Settings search box.
            var secs = paletteSchema.sections
            for (var ps = 0; ps < secs.length; ++ps) {
                for (var pf = 0; pf < secs[ps].length; ++pf) {
                    var fld = secs[ps][pf]
                    if (!fld.label || fld.type === "group" || fld.type === "warning") continue
                    (function(sectionIdx, field) {
                        acts.push({ label: i18n.t("tb_settings") + " · " + field.label,
                                    hint: i18n.t("palette_hint_page"),
                                    run: function() { win.currentPage = 3; settingsPage.sec = sectionIdx; settingsPage.searchFor(field.label) } })
                    })(ps, fld)
                }
            }
            return acts
        }
        onTorrentPicked: function(src) {
            win.currentPage = 0
            if (typeof torrentFilter === "undefined") return
            var p = torrentFilter.mapFromSource(src)
            if (p < 0) { win.setFilter("all"); p = torrentFilter.mapFromSource(src) }
            if (p >= 0) win.selectRow(p)
        }
    }
    Shortcut { sequence: "Ctrl+K"; onActivated: cmdPalette.toggle() }

    // "Preparing to watch" overlay for the one-click Get & Watch flow
    GetWatchOverlay {
        id: gwOverlay
        onCanceled: {
            if (typeof debrid !== "undefined" && debrid.busy) debrid.cancelStream()
            else if (phase === "searching") { if (typeof search !== "undefined") search.cancelGetAndWatch() }
            else if (hash !== "" && typeof session !== "undefined") session.cancelWatch(hash)
        }
    }

    // custom toast cards, pinned to the screen's bottom-right (native-like)
    ToastOverlay {
        id: toastHost
        onActionTriggered: function(actionId) {
            if (actionId === "logs") win.showWin(logWinLoader)
            else if (actionId === "crashreport" && typeof logs !== "undefined")
                Qt.openUrlExternally(logs.crashReportUrl())
            else if (actionId === "star")
                Qt.openUrlExternally("https://github.com/BATorrent-app/BATorrent")
            else if (actionId.indexOf("play:") === 0 && typeof session !== "undefined")
                session.playByHash(actionId.substring(5))
        }
    }

    // one-time star ask: only after the app earned it (14+ days and 10+
    // launches), dismissible, never repeats — converts long-time users, the
    // segment download counts prove we lose
    Timer {
        interval: 8000
        running: true
        repeat: false
        onTriggered: {
            if (typeof settings === "undefined") return
            if (settings.get("starAskShown") === true) return
            var first = Number(settings.get("firstLaunchAt") || 0)
            var count = Number(settings.get("launchCount") || 0) + 1
            if (first === 0) { settings.set("firstLaunchAt", Date.now()); settings.set("launchCount", 1); return }
            settings.set("launchCount", count)
            var days = (Date.now() - first) / 86400000
            if (days >= 14 && count >= 10) {
                settings.set("starAskShown", true)
                toastHost.show(i18n.t("star_ask_title"), i18n.t("star_ask_body"), 0,
                               "star", i18n.t("star_ask_action"))
            }
        }
    }

    // unclean shutdown last run → one quiet, actionable toast (never auto-sends
    // anything; the GitHub issue opens pre-filled in the browser for review)
    Timer {
        interval: 4000
        running: true
        repeat: false
        onTriggered: {
            if (typeof logs !== "undefined" && logs.previousSessionCrashed())
                toastHost.show(i18n.t("crash_toast_title"), i18n.t("crash_toast_body"), 1,
                               "crashreport", i18n.t("crash_toast_action"))
        }
    }

    TrayPopupWindow {
        id: trayPopup
        onShowApp:      { win.show(); win.raise(); win.requestActivate() }
        onOpenTorrent:  { win.show(); win.raise(); win.requestActivate(); openFileDlg.open() }
        onOpenMagnet:   { win.show(); win.raise(); win.requestActivate(); magnetDlg.open() }
        onPauseAll:     if (typeof session !== "undefined") session.pauseAll()
        onResumeAll:    if (typeof session !== "undefined") session.resumeAll()
        onOpenSettings: { win.show(); win.raise(); win.requestActivate(); win.currentPage = 3 }
        onQuitApp:      Qt.quit()
    }

    // (IconImg vem de widgets/)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ===== IN-WINDOW MENU BAR (Windows/Linux) =====
        // Qt.labs.platform.MenuBar (above) only renders as a *global* menu bar
        // (macOS, or a Linux global-menu shell) — on Windows it shows nothing,
        // so the whole File/Torrent/Settings/Help menu vanished there. Mirror
        // the same menus in a Qt Quick MenuBar that draws inside the window;
        // shown only where the native one doesn't apply.
        MenuBar {
            id: winMenuBar
            visible: Qt.platform.os !== "osx"
            Layout.fillWidth: true
            Layout.preferredHeight: visible ? implicitHeight : 0

            background: Rectangle {
                color: Theme.panel
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            }
            delegate: MenuBarItem {
                id: mbarItem
                padding: 6; leftPadding: 12; rightPadding: 12
                contentItem: Text {
                    text: mbarItem.text
                    color: Theme.t1
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: (mbarItem.highlighted || mbarItem.down) ? Theme.hover : "transparent"
                    radius: 5
                }
            }

            component BarItem: MenuItem {
                id: bi
                implicitHeight: 30; padding: 0
                contentItem: Text {
                    leftPadding: 14; rightPadding: 28
                    text: bi.text
                    color: !bi.enabled ? Theme.t4 : (bi.highlighted ? Theme.t1 : Theme.t2)
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: bi.highlighted ? Theme.hover : "transparent"; radius: 5 }
            }
            component BarSep: MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
            component BarMenu: Menu {
                implicitWidth: 240
                // Breathing room so the first/last item don't get clipped by the
                // rounded (radius 8) background corners — the "cut tail" on the
                // last entry.
                topPadding: 6; bottomPadding: 6
                delegate: BarItem {}
                background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
            }

            BarMenu {
                title: (i18n.language, i18n.t("menu_file_title"))
                BarItem { text: (i18n.language, i18n.t("menu_open_torrent")); onTriggered: openFileDlg.open() }
                BarItem { text: (i18n.language, i18n.t("menu_add_magnet")); onTriggered: magnetDlg.open() }
                BarItem { text: (i18n.language, i18n.t("menu_create_torrent")); onTriggered: createDlg.open() }
                BarItem { text: (i18n.language, i18n.t("menu_inspect_torrent")); onTriggered: inspectFileDlg.open() }
                BarItem { text: (i18n.language, i18n.t("menu_import_qbt")); onTriggered: importQbtDlg.open() }
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_recently_removed")); onTriggered: win.showWin(removedWinLoader) }
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_quit")); onTriggered: Qt.quit() }
            }
            BarMenu {
                title: (i18n.language, i18n.t("menu_torrent_title"))
                BarItem { text: (i18n.language, i18n.t("menu_pause")); enabled: win.hasSel; onTriggered: session.pauseSelected() }
                BarItem { text: (i18n.language, i18n.t("menu_resume")); enabled: win.hasSel; onTriggered: session.resumeSelected() }
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_pause_all")); onTriggered: if (typeof session !== "undefined") session.pauseAll() }
                BarItem { text: (i18n.language, i18n.t("menu_resume_all")); onTriggered: if (typeof session !== "undefined") session.resumeAll() }
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_remove")); enabled: win.hasSel; onTriggered: removeDlg.open() }
            }
            BarMenu {
                title: (i18n.language, i18n.t("menu_settings_title"))
                BarItem { text: (i18n.language, i18n.t("menu_preferences")); onTriggered: win.currentPage = 3 }
                BarItem { text: (i18n.language, i18n.t("menu_addons")); onTriggered: addAddonDlg.open() }
                BarItem { text: (i18n.language, i18n.t("menu_rss")); onTriggered: win.showWin(rssWinLoader) }
                BarItem { text: (i18n.language, i18n.t("menu_pair")); onTriggered: { pairingDlg.reload(); pairingDlg.open() } }
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_search_torrents")); onTriggered: win.currentPage = 1 }
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_statistics")); onTriggered: win.showWin(statsWinLoader) }
                BarItem { text: (i18n.language, i18n.t("menu_speedtest")); onTriggered: Qt.openUrlExternally("https://fast.com") }
            }
            BarMenu {
                title: (i18n.language, i18n.t("menu_help_title"))
                BarItem { text: (i18n.language, i18n.t("menu_tour")); onTriggered: tourOverlay.start() }
                BarItem { text: (i18n.language, i18n.t("menu_whatsnew")); onTriggered: { welcomeDlg.mode = "update"; welcomeDlg.open() } }
                BarItem { text: (i18n.language, i18n.t("menu_release_notes")); onTriggered: releaseNotesDlg.open() }
                BarItem { text: (i18n.language, i18n.t("menu_shortcuts")); onTriggered: win.showWin(shortcutsWinLoader) }
                BarItem { text: (i18n.language, i18n.t("menu_logs")); onTriggered: win.showWin(logWinLoader) }
                BarItem { text: (i18n.language, i18n.t("menu_diagnostics")); onTriggered: win.showWin(diagWinLoader) }
                BarItem {
                    text: (i18n.language, i18n.t("menu_check_updates"))
                    enabled: typeof updater !== "undefined" && updater !== null
                    onTriggered: if (typeof updater !== "undefined" && updater) updater.check(false)
                }
                BarItem { text: (i18n.language, i18n.t("menu_feedback")); onTriggered: Qt.openUrlExternally("https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform") }
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_donate")); onTriggered: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19") }
                BarItem { text: (i18n.language, i18n.t("menu_about")); onTriggered: aboutDlg.open() }
            }
        }

        // ===== top nav bar (default layout) =====
        Loader {
            id: navBarLoader
            Layout.fillWidth: true
            active: !win.layoutClassic
            visible: active
            sourceComponent: NavBar {
                currentIndex: win.currentPage
                onPageRequested: function(page) { win.currentPage = page }
                onSettingsClicked: win.currentPage = 3
                onSelectTorrent: function(infoHash) { win.selectTorrentByHash(infoHash) }
                onMakeRoomRequested: { makeRoomPanel.targetBytes = 0; makeRoomPanel.open = true }
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
                    onPageRequested: function(page) { win.currentPage = page }
                    onSettingsClicked: win.currentPage = 3
                    onSelectTorrent: function(infoHash) { win.selectTorrentByHash(infoHash) }
                    onMakeRoomRequested: { makeRoomPanel.targetBytes = 0; makeRoomPanel.open = true }
                }
            }

            StackLayout {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: win.currentPage

                // premium page switch: the new page fades + rises in
                transform: Translate { id: pageShift }
                onCurrentIndexChanged: pageSwitchAnim.restart()
                SequentialAnimation {
                    id: pageSwitchAnim
                    ParallelAnimation {
                        NumberAnimation { target: contentStack; property: "opacity"; from: 0.0; to: 1.0; duration: 190; easing.type: Easing.OutCubic }
                        NumberAnimation { target: pageShift; property: "y"; from: 12; to: 0; duration: 240; easing.type: Easing.OutCubic }
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
            onRemoveSelected: removeDlg.open()
            onOpenRss: win.showWin(rssWinLoader)
            onNavigate: function(index) { win.currentPage = index }
        }

        // ================== SUBBAR ==================
        FilterBar {
            id: filterBar
            win: win
        }

        // ================== CONTENT (grid OR list) + grid inspector ==================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            LibraryView {
                id: libraryView
                win: win
                onAddMagnetRequested: magnetDlg.open()
            }
            DetailSidebar {
                win: win
                onRenameFileRequested: function(idx, current) { win.promptRenameFile(idx, current) }
            }
        }

        // ================== DETAIL (list mode; the grid uses the inspector) ==================
        DetailPanel {
            win: win
            visible: !win.gridView
            onRenameFileRequested: function(idx, current) { win.promptRenameFile(idx, current) }
        }
        // ================== STATUS BAR ==================
        StatusBar {}
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

    // ================== DRAG & DROP (.torrent / magnet) ==================
    DropArea {
        id: dropZone
        anchors.fill: parent
        z: 150
        function isMagnetLike(s) {
            var u = s.toLowerCase()
            return u.indexOf("magnet:") === 0 || u.indexOf("bittorrent:") === 0
        }
        function accepts(drag) {
            if (drag.hasUrls) {
                for (var i = 0; i < drag.urls.length; ++i) {
                    var u = drag.urls[i].toString().toLowerCase()
                    if (u.endsWith(".torrent") || dropZone.isMagnetLike(u)) return true
                }
            }
            if (drag.hasText && dropZone.isMagnetLike(drag.text)) return true
            return false
        }
        onEntered: function(drag) { drag.accepted = accepts(drag) }
        onDropped: function(drop) {
            if (typeof session === "undefined") return
            // Browsers hand a dragged magnet link over as a url, not just text —
            // sort those out before treating the rest of drop.urls as .torrent files.
            var torrentUrls = []
            if (drop.hasUrls) {
                for (var i = 0; i < drop.urls.length; ++i) {
                    var u = drop.urls[i].toString()
                    if (dropZone.isMagnetLike(u)) session.addMagnetUri(u)
                    else torrentUrls.push(u)
                }
            }
            // Queue every dropped .torrent so each gets the preview/choose-folder
            // dialog in turn, instead of only the first.
            if (torrentUrls.length > 0) win.enqueueTorrentUrls(torrentUrls)
            if (drop.hasText && dropZone.isMagnetLike(drop.text)) session.addMagnetUri(drop.text)
            drop.accept()
        }
    }
    Rectangle {
        anchors.fill: parent
        z: 151
        color: Qt.rgba(0, 0, 0, 0.65)
        visible: opacity > 0.01
        opacity: dropZone.containsDrag ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 150; easing.type: Easing.OutCubic } }
        Rectangle {
            anchors.centerIn: parent
            width: 360; height: 200; radius: 16
            color: Theme.panel
            border.color: Theme.accent
            border.width: 2
            scale: dropZone.containsDrag ? 1.0 : 0.95
            Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12
                IconImg { Layout.alignment: Qt.AlignHCenter; src: "qrc:/icons/magnet.svg"; tint: Theme.accentText; s: 52 }
                Text { Layout.alignment: Qt.AlignHCenter; text: (i18n.language, i18n.t("dnd_drop_title")); color: Theme.t1; font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans }
                Text { Layout.alignment: Qt.AlignHCenter; text: (i18n.language, i18n.t("dnd_drop_sub")); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
            }
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
    // pending .torrent paths awaiting the add dialog (filled by drops / multi-open)
    property var torrentQueue: []
    // drive opens off a timer so the dialog never opens inside the drop event or
    // the dialog's own accept/close handler (both fight focus on macOS).
    Timer { id: queueTimer; interval: 130; onTriggered: win.processTorrentQueue() }
    // .torrent opened from outside (file association / CLI / second instance):
    // run it through the same queue+dialog as a drop, never a silent add.
    Connections {
        target: typeof session !== "undefined" ? session : null
        function onOpenTorrentRequested(path) { win.enqueueTorrentUrls([path]) }
    }
    function enqueueTorrentUrls(urls) {
        for (var i = 0; i < urls.length; ++i) {
            var u = urls[i]
            if (u.toLowerCase().endsWith(".torrent")) win.torrentQueue.push(u)
        }
        queueTimer.restart()
    }
    function processTorrentQueue() {
        if (typeof session === "undefined") { win.torrentQueue = []; return }
        if (addTorrentDlg.opened) return        // wait until the current one closes
        var useDefault = settings.get("useDefaultPath") === true
        while (win.torrentQueue.length > 0) {
            var u = win.torrentQueue.shift()
            var p = session.previewTorrent(u)
            // "skip the dialog" is a convenience for the common case — it stops
            // being convenient the moment the download can't actually fit, so a
            // known size that won't fit still surfaces the dialog (with its
            // disk-fit warning) instead of silently failing mid-download.
            var known = p && p.ok && (p.totalSizeBytes || 0) > 0
            var fits = !known || session.freeSaveBytes() < 0 || p.totalSizeBytes <= session.freeSaveBytes()
            if (useDefault && fits) { session.addTorrentFile(u); continue }
            if (p && p.ok) {
                addTorrentDlg.savePath = session.defaultSavePath()
                addTorrentDlg.loadPreview(p, u)
                addTorrentDlg.open()
                return                          // resume on accept/reject
            }
            session.addTorrentFile(u)           // unpreviewable → add directly, keep going
        }
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
    Timer {
        id: shutdownTimer; interval: 1000; repeat: true
        onTriggered: {
            win.shutdownLeft--
            if (win.shutdownLeft <= 0) { stop(); shutdownDlg.close(); if (typeof session !== "undefined") session.performPostDownloadAction() }
        }
    }
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onAllDownloadsComplete() {
            win.shutdownActionLabel = win.postDownloadActionLabel(
                typeof settings !== "undefined" ? settings.get("postDownloadAction") : 6)
            win.shutdownLeft = 60; shutdownTimer.restart(); shutdownDlg.open()
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
            text: (i18n.language, i18n.t("shutdown_msg2")).arg(win.shutdownActionLabel).arg(win.shutdownLeft)
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
        onAccepted: win.maybeStartTour()
        onRejected: win.maybeStartTour()        // closing via X/backdrop also leads into the tour
        onOpenReleaseNotes: releaseNotesDlg.open()
    }
    AboutDialog         { id: aboutDlg }

    TourOverlay {
        id: tourOverlay
        onPageRequested: function(page) { win.currentPage = page }
        // step 0 is the welcome (centered, no spotlight); the rest spotlight the UI
        // bat/pose vary per step so all 3 candidate SVGs (noto/twemoji/openmoji)
        // and poses (perch/hang, left/right) show in one run — for picking one.
        steps: (i18n.language, [
            { page: 0, title: i18n.t("tour_s1_t"), text: i18n.t(win.layoutClassic ? "tour_s1_d" : "tour_s1_d_top"),
              rectFn: function() { return win.navHost ? win.navHost.itemRect("rail", tourOverlay) : Qt.rect(0,0,0,0) } },
            { page: 0, title: i18n.t("tour_s2_t"), text: i18n.t("tour_s2_d"),
              rectFn: function() { return win.rectIn(toolbar.tbOpen, tourOverlay) } },
            { page: 1, title: i18n.t("tour_s3_t"), text: i18n.t("tour_s3_d"),
              rectFn: function() { return win.navHost ? win.navHost.itemRect(1, tourOverlay) : Qt.rect(0,0,0,0) } },
            { page: 2, title: i18n.t("tour_s5_t"), text: i18n.t("tour_s5_d"),
              rectFn: function() { return win.navHost ? win.navHost.itemRect(2, tourOverlay) : Qt.rect(0,0,0,0) } },
            { page: 0, title: i18n.t("tour_s6_t"), text: i18n.t("tour_s6_d"),
              rectFn: function() { return win.navHost ? win.navHost.itemRect("settings", tourOverlay) : Qt.rect(0,0,0,0) } }
        ])
    }

    // ================== TOP-LEVEL WINDOWS (lazy) ==================
    // Built on first open via Loader, not at startup — instantiating all of
    // them eagerly stalled the UI thread for
    // seconds on launch and inflated memory. showWin() creates then shows.
    function showWin(loader) {
        loader.active = true
        if (loader.item) { loader.item.show(); loader.item.raise(); loader.item.requestActivate() }
    }
    // Build a valid file: URL for a local path. On Windows a path is "C:/…",
    // so plain "file://"+path yields "file://C:/…" where QUrl reads "C:" as a
    // host and the image fails to load; Windows needs the triple-slash form.
    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }
    Loader { id: rssWinLoader;       active: false; sourceComponent: RssWindow {} }
    Loader { id: shortcutsWinLoader; active: false; sourceComponent: ShortcutsWindow {} }
    Loader { id: statsWinLoader;     active: false; sourceComponent: StatisticsWindow { onOpenWrapped: win.showWrapped() } }
    Loader { id: wrappedWinLoader;   active: false; sourceComponent: WrappedWindow {} }
    function showWrapped() {
        wrappedWinLoader.active = true
        if (wrappedWinLoader.item) wrappedWinLoader.item.openFor(new Date().getFullYear())
    }
    Loader { id: removedWinLoader;   active: false; sourceComponent: RemovedHistoryWindow {} }
    Loader { id: logWinLoader;       active: false; sourceComponent: LogViewerWindow {} }
    Loader { id: diagWinLoader;      active: false; sourceComponent: DiagnosticsWindow {} }
    Loader {
        id: playerWinLoader; active: false
        sourceComponent: PlayerWindow {
            // closing the window tears the player down so reopening starts fresh,
            // and refreshes the HUB so the watched-% bar reflects this session
            onClosed: Qt.callLater(function() { playerWinLoader.active = false; if (hubPage) hubPage.refresh() })
        }
    }
    Connections {
        target: session
        function onOpenPlayer(url, title, hash, fileIndex) {
            gwOverlay.hide()
            playerWinLoader.active = true
            var w = playerWinLoader.item
            if (w) { w.show(); w.raise(); w.requestActivate(); w.openMedia(url, title, hash, fileIndex) }
        }
        function onWatchProgress(hash, percent) {
            if (gwOverlay.phase === "buffering" && hash === gwOverlay.hash) gwOverlay.percent = percent
        }
        function onWatchFailed(title) { gwOverlay.fail(i18n.t("gw_failed").arg(title)) }
    }

    // Debrid: magnet → provider cache → direct link → built-in player.
    Connections {
        target: typeof debrid !== "undefined" ? debrid : null
        ignoreUnknownSignals: true
        function onStreamReady(url, name) {
            gwOverlay.hide()
            playerWinLoader.active = true
            var w = playerWinLoader.item
            if (w) { w.show(); w.raise(); w.requestActivate(); w.openMedia(url, name, "debrid", 0) }
        }
        function onErrorOccurred(msg) {
            gwOverlay.hide()
            win.notifyUser(debrid.providerName, msg, 2)
        }
        function onBusyChanged() {
            if (debrid.busy) gwOverlay.show("buffering", debrid.providerName)
            else if (gwOverlay.phase === "buffering") gwOverlay.hide()
        }
        function onStatusChanged() {
            if (!debrid.busy) return
            if (debrid.status !== "") gwOverlay.title = debrid.status
            gwOverlay.percent = debrid.progress / 100
        }
    }

    // Adding a torrent from Search jumps to Downloads and selects it once it lands
    // (the add is async, so retry briefly until the torrent shows up in the model).
    Connections {
        target: typeof search !== "undefined" ? search : null
        ignoreUnknownSignals: true
        function onAddedTorrent(infoHash) {
            win.currentPage = 0
            selectAddedTimer.hash = infoHash || ""
            selectAddedTimer.tries = 0
            selectAddedTimer.restart()
        }
        // Get & Watch flow → drives the preparing overlay
        function onWatchSearching(title) { gwOverlay.show("searching", title) }
        function onWatchNoRelease(title) { gwOverlay.fail(i18n.t("gw_no_release").arg(title)) }
        function onPrepareAndWatch(infoHash, title) {
            gwOverlay.hash = infoHash
            gwOverlay.phase = "buffering"
            if (typeof session !== "undefined") session.watchWhenReady(infoHash, title)
        }
    }
    Timer {
        id: selectAddedTimer
        property string hash: ""
        property int tries: 0
        interval: 200; repeat: true
        onTriggered: {
            tries++
            if (hash === "") { stop(); return }
            if (session.selectByInfoHash(hash)) { stop(); return }
            if (tries >= 15) stop()
        }
    }
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
            win.notifyUser("BATorrent", i18n.t(ok ? "export_torrent_ok" : "export_torrent_failed"), ok ? 0 : 2)
        }
    }

    // Import torrents from an existing qBittorrent install (choose default save path)
    FolderDialog {
        id: importQbtDlg
        title: (i18n.language, i18n.t("import_savepath_title"))
        onAccepted: if (typeof session !== "undefined") session.importQbittorrent(session.urlToLocalPath(importQbtDlg.selectedFolder.toString()))
    }

    Shortcut { sequences: [StandardKey.HelpContents]; onActivated: win.showWin(shortcutsWinLoader) }
    Shortcut { sequence: "Ctrl+F"; onActivated: filterBar.searchInput.forceActiveFocus() }
    Shortcut { sequence: "Ctrl+R"; onActivated: if (typeof session !== "undefined") session.forceRecheckSelected() }
    // reorder queue: vertical in list, horizontal in grid (tiles sit side by side)
    Shortcut { sequence: "Ctrl+Up";    enabled: !win.gridView; onActivated: if (typeof session !== "undefined") session.queueUpSelected() }
    Shortcut { sequence: "Ctrl+Down";  enabled: !win.gridView; onActivated: if (typeof session !== "undefined") session.queueDownSelected() }
    Shortcut { sequence: "Ctrl+Left";  enabled: win.gridView;  onActivated: if (typeof session !== "undefined") session.queueUpSelected() }
    Shortcut { sequence: "Ctrl+Right"; enabled: win.gridView;  onActivated: if (typeof session !== "undefined") session.queueDownSelected() }
    // navigate selection — suppressed while the command palette owns the keyboard
    // (otherwise the arrows scroll the list behind it instead of its results)
    Shortcut { sequence: "Up";    enabled: !cmdPalette.opened; onActivated: win.moveSel(win.gridView ? -win.gridCols() : -1) }
    Shortcut { sequence: "Down";  enabled: !cmdPalette.opened; onActivated: win.moveSel(win.gridView ?  win.gridCols() :  1) }
    Shortcut { sequence: "Left";  enabled: win.gridView && !cmdPalette.opened; onActivated: win.moveSel(-1) }
    Shortcut { sequence: "Right"; enabled: win.gridView && !cmdPalette.opened; onActivated: win.moveSel(1) }
    Shortcut { sequence: "Ctrl+1"; onActivated: win.setFilter("all") }
    Shortcut { sequence: "Ctrl+2"; onActivated: win.setFilter("downloading") }
    Shortcut { sequence: "Ctrl+3"; onActivated: win.setFilter("seeding") }
    Shortcut { sequence: "Ctrl+4"; onActivated: win.setFilter("completed") }
    Shortcut { sequence: "Ctrl+5"; onActivated: win.setFilter("active") }
    Shortcut { sequence: "Ctrl+6"; onActivated: win.setFilter("paused") }
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: Theme.cycle() }

    // startup splash overlay (above everything, incl. toasts at z:9000)
    Loader {
        active: win.showSplash
        anchors.fill: parent
        z: 10000
        sourceComponent: Splash {
            onFinished: { win.showSplash = false; win.maybeShowWelcome() }
        }
    }
}






