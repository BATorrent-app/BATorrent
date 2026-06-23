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
    minimumWidth: 1288   // +188 for the nav rail (content area stays >=1100 so the toolbar never clips)
    minimumHeight: 640
    color: Theme.bg
    flags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    title: Theme.unifiedChrome ? "" : "BATorrent"

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

    property int selected: -1          // focus row (drives the detail panel)
    property var selectedRows: []      // multi-selection (proxy rows)
    property int anchorRow: -1         // shift-range anchor
    property bool gridView: true
    property string activeFilter: "all"
    property string catFilter: ""
    // startup splash — shown on every launch unless disabled in Settings
    property bool showSplash: false
    Component.onCompleted: {
        // restore the last window size (only if it's still sane vs the minimums)
        if (typeof settings !== "undefined") {
            var sw = Number(settings.get("winWidth") || 0)
            var sh = Number(settings.get("winHeight") || 0)
            if (sw >= win.minimumWidth && sw <= Screen.desktopAvailableWidth) win.width = sw
            if (sh >= win.minimumHeight && sh <= Screen.desktopAvailableHeight) win.height = sh
        }
        showSplash = (typeof settings === "undefined") || settings.get("showSplash") !== false
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
        win.selectRow(proxyRow)
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
    function gridCols() { return Math.max(1, Math.floor(grid.width / grid.cellWidth)) }
    function moveSel(step) {
        var view = win.gridView ? grid : list
        var n = view.count
        if (n <= 0) return
        var cur = win.selected
        var next = cur < 0 ? (step > 0 ? 0 : n - 1)
                           : Math.max(0, Math.min(n - 1, cur + step))
        win.selectRow(next)
        view.positionViewAtIndex(next, win.gridView ? GridView.Contain : ListView.Contain)
    }

    // styled menu item reused by the category menus
    component CatItem: MenuItem {
        id: cmi
        implicitHeight: 30
        padding: 0
        contentItem: Text {
            leftPadding: 14; rightPadding: 14
            text: cmi.text
            color: cmi.highlighted ? Theme.t1 : Theme.t2
            font.pixelSize: 12; font.family: Theme.fontSans
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle { color: cmi.highlighted ? Theme.hover : "transparent"; radius: 5 }
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
        CtxItem {
            visible: session.selectedHasVideo
            height: visible ? implicitHeight : 0
            text: (i18n.language, i18n.t("ctx_play"))
            onTriggered: session.playSelected()
        }
        CtxItem { text: (i18n.language, i18n.t("ctx_rename")); onTriggered: inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), session.selectedName, "", function(t){ session.renameSelected(t) }) }
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
            CtxItem { text: session.selectedCompleted ? (i18n.language, i18n.t("ctx_unmark_completed_plain")) : (i18n.language, i18n.t("ctx_mark_completed_plain")); onTriggered: session.selectedCompleted ? session.unmarkSelectedCompleted() : session.markSelectedCompleted() }
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
        sequence: StandardKey.Paste
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
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_preferences")); shortcut: StandardKey.Preferences; onTriggered: win.showWin(settingsWinLoader) }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_addons")); onTriggered: addAddonDlg.open() }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_rss")); onTriggered: win.showWin(rssWinLoader) }
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_pair")); onTriggered: { pairingDlg.reload(); pairingDlg.open() } }
            Platform.MenuSeparator {}
            Platform.MenuItem { text: (i18n.language, i18n.t("menu_search_torrents")); onTriggered: navRail.currentIndex = 2 }
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
        // Windows: a native context menu. The frameless painted popup never
        // reliably activated from the Win tray (it could open off-screen or
        // fail to dismiss — issue #20), so Windows gets the OS-native menu and
        // macOS/Linux keep the rich popup via the Context reason below.
        menu: Qt.platform.os === "windows" ? trayMenu : null
        // Left click / double click restores the window (Windows convention).
        onActivated: function(reason) {
            if (reason === Platform.SystemTrayIcon.Trigger
                || reason === Platform.SystemTrayIcon.DoubleClick) {
                win.show(); win.raise(); win.requestActivate()
            } else if (reason === Platform.SystemTrayIcon.Context
                       && Qt.platform.os !== "windows") {
                trayPopup.popUpAt(trayIcon.geometry)
            }
        }
    }

    // Native tray menu (Windows only — attached above). Routes through the same
    // trayPopup signals the painted popup uses, so behaviour stays identical.
    Platform.Menu {
        id: trayMenu
        Platform.MenuItem { text: i18n.t("tray_show");          onTriggered: trayPopup.showApp() }
        Platform.MenuItem { text: i18n.t("tray_open_torrent");  onTriggered: trayPopup.openTorrent() }
        Platform.MenuItem { text: i18n.t("tray_open_magnet");   onTriggered: trayPopup.openMagnet() }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: i18n.t("tray_pause_all");     onTriggered: trayPopup.pauseAll() }
        Platform.MenuItem { text: i18n.t("tray_resume_all");    onTriggered: trayPopup.resumeAll() }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: i18n.t("tray_preferences");   onTriggered: trayPopup.openSettings() }
        Platform.MenuItem { text: i18n.t("tray_quit");          onTriggered: trayPopup.quitApp() }
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
    }

    // Ctrl/⌘+K command palette — actions + torrent jump
    CommandPalette {
        id: cmdPalette
        actions: {
            var l = i18n.language   // re-evaluate labels on language switch
            return [
                { label: i18n.t("magnet_title"), run: function() { magnetDlg.open() } },
                { label: i18n.t("menu_open_torrent"), run: function() { openFileDlg.open() } },
                { label: i18n.t("menu_create_torrent"), run: function() { createDlg.open() } },
                { label: i18n.t("menu_pause_all"), run: function() { if (typeof session !== "undefined") session.pauseAll() } },
                { label: i18n.t("menu_resume_all"), run: function() { if (typeof session !== "undefined") session.resumeAll() } },
                { label: i18n.t("tb_alt_speed"), hint: i18n.t("palette_hint_toggle"), run: function() { if (typeof session !== "undefined") session.setAltSpeedsActive(!session.altSpeedsActive) } },
                { label: i18n.t("nav_downloads"), hint: i18n.t("palette_hint_page"), run: function() { navRail.currentIndex = 0 } },
                { label: i18n.t("nav_discover"), hint: i18n.t("palette_hint_page"), run: function() { navRail.currentIndex = 1 } },
                { label: i18n.t("nav_search"), hint: i18n.t("palette_hint_page"), run: function() { navRail.currentIndex = 2 } },
                { label: i18n.t("nav_hub"), hint: i18n.t("palette_hint_page"), run: function() { navRail.currentIndex = 3 } },
                { label: i18n.t("tb_settings"), run: function() { win.showWin(settingsWinLoader) } },
                { label: i18n.t("menu_rss"), run: function() { win.showWin(rssWinLoader) } },
                { label: i18n.t("menu_statistics"), run: function() { win.showWin(statsWinLoader) } },
                { label: i18n.t("menu_logs"), run: function() { win.showWin(logWinLoader) } },
                { label: i18n.t("menu_diagnostics"), run: function() { win.showWin(diagWinLoader) } },
                { label: i18n.t("shortcuts_title2"), run: function() { win.showWin(shortcutsWinLoader) } }
            ]
        }
        onTorrentPicked: function(src) {
            navRail.currentIndex = 0
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
            if (phase === "searching") { if (typeof search !== "undefined") search.cancelGetAndWatch() }
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
        onOpenSettings: { win.showWin(settingsWinLoader) }
        onQuitApp:      Qt.quit()
    }

    // (IconImg vem de widgets/)

    // ----- reusable toolbar button (.tbtn) -----
    component TBtn: Rectangle {
        id: tb
        property string label
        property string icon
        property bool disabled: false
        property bool active: false       // toggled-on state (e.g. alt-speed turtle)
        signal clicked()
        Layout.preferredWidth: 52
        Layout.minimumWidth: 52          // never let the RowLayout squeeze/clip the button
        Layout.preferredHeight: 54
        color: !disabled && tbMa.containsMouse ? Theme.hover : "transparent"
        radius: 8
        opacity: disabled ? 0.35 : 1.0

        activeFocusOnTab: !disabled
        Keys.onReturnPressed: if (!disabled) tb.clicked()
        Keys.onSpacePressed: if (!disabled) tb.clicked()
        scale: tbMa.pressed && !tb.disabled ? Theme.pressScale : 1
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }
        Rectangle {
            visible: tb.activeFocus
            anchors.fill: parent
            anchors.margins: -2
            radius: 10
            color: "transparent"
            border.color: Theme.focusRing
            border.width: Theme.focusRingWidth
        }

        Column {
            anchors.centerIn: parent
            spacing: 3
            IconImg {
                anchors.horizontalCenter: parent.horizontalCenter
                src: tb.icon
                tint: tb.active ? Theme.accent : (!tb.disabled && tbMa.containsMouse ? Theme.t1 : Theme.t3)
                s: 18
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: tb.label
                color: tb.active ? Theme.accent : (!tb.disabled && tbMa.containsMouse ? Theme.t1 : Theme.t3)
                font.pixelSize: 11
                font.family: Theme.fontSans
                font.weight: Font.Medium
            }
        }
        MouseArea {
            id: tbMa
            anchors.fill: parent
            hoverEnabled: !tb.disabled
            cursorShape: tb.disabled ? Qt.ArrowCursor : Qt.PointingHandCursor
            onClicked: if (!tb.disabled) tb.clicked()
        }
    }

    // ----- vertical divider between toolbar groups (.tgrp + .tgrp) -----
    component TGrpDiv: Rectangle {
        Layout.preferredWidth: 1
        Layout.preferredHeight: 26
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.alignment: Qt.AlignVCenter
        color: Theme.hair
    }

    // ----- pill (filter) -----
    component Pill: Rectangle {
        id: pi
        property string label
        property string count
        property string state: "all"
        property bool on: win.activeFilter === state
        signal clicked()
        radius: 8
        height: 30
        implicitWidth: pillRow.implicitWidth + 26
        color: on ? Theme.accentTint : (piMa.containsMouse ? Theme.hover : "transparent")

        activeFocusOnTab: true
        Keys.onReturnPressed: pi.clicked()
        Keys.onSpacePressed: pi.clicked()
        scale: piMa.pressed ? Theme.pressScale : 1
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }
        Rectangle {
            visible: pi.activeFocus
            anchors.fill: parent
            anchors.margins: -2
            radius: 10
            color: "transparent"
            border.color: Theme.focusRing
            border.width: Theme.focusRingWidth
        }

        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: 7
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: pi.label
                color: pi.on ? Theme.accentText : (piMa.containsMouse ? Theme.t2 : Theme.t3)
                font.pixelSize: 12
                font.family: Theme.fontSans
                font.weight: Font.Medium
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: pi.count
                color: pi.on ? Theme.accentText : Theme.t4
                font.pixelSize: 11
                font.family: Theme.fontMono
            }
        }
        MouseArea {
            id: piMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: pi.clicked()
        }
    }

    // ----- clickable list header column (sort) -----
    component HCol: Item {
        id: hc
        property string label
        property string col
        property bool fill: false
        property int w: 78
        property bool alignRight: false
        Layout.fillWidth: fill
        Layout.preferredWidth: fill ? -1 : w
        Layout.fillHeight: true
        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: hc.alignRight ? undefined : parent.left
            anchors.right: hc.alignRight ? parent.right : undefined
            spacing: 4
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: hc.label
                // anime art sits behind the right columns — lift the weak grey + a contrasting
                // outline so headers stay legible over both dark and bright parts of the art
                color: win.sortColumn === hc.col ? Theme.t2 : (hcMa.containsMouse ? Theme.t3 : (Theme.hasAnime ? Theme.t2 : Theme.t4))
                style: Theme.hasAnime ? Text.Outline : Text.Normal
                styleColor: Theme.isLight ? "#ffffff" : "#000000"
                font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: win.sortColumn === hc.col
                text: win.sortAsc ? "▲" : "▼"
                color: Theme.accent
                font.pixelSize: 7
            }
        }
        MouseArea { id: hcMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.toggleSort(hc.col) }
    }

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
                color: Theme.elev
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
                BarItem { text: (i18n.language, i18n.t("menu_preferences")); onTriggered: win.showWin(settingsWinLoader) }
                BarItem { text: (i18n.language, i18n.t("menu_addons")); onTriggered: addAddonDlg.open() }
                BarItem { text: (i18n.language, i18n.t("menu_rss")); onTriggered: win.showWin(rssWinLoader) }
                BarItem { text: (i18n.language, i18n.t("menu_pair")); onTriggered: { pairingDlg.reload(); pairingDlg.open() } }
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_search_torrents")); onTriggered: navRail.currentIndex = 2 }
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
                BarSep {}
                BarItem { text: (i18n.language, i18n.t("menu_donate")); onTriggered: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19") }
                BarItem { text: (i18n.language, i18n.t("menu_about")); onTriggered: aboutDlg.open() }
            }
        }

        // ===== nav rail + content stack (4.0 hub shell) =====
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            NavRail {
                id: navRail
                Layout.fillHeight: true
                onSettingsClicked: win.showWin(settingsWinLoader)
            }

            StackLayout {
                id: contentStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: navRail.currentIndex

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
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            color: Theme.elev
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp2

                // brand moved to the nav rail; toolbar starts at the actions
                // G1: Abrir, Magnet
                TBtn { id: tbOpen; label: (i18n.language, i18n.t("tb_open"));   icon: "qrc:/icons/open.svg";  onClicked: openFileDlg.open() }
                TBtn { label: (i18n.language, i18n.t("tb_magnet"));  icon: "qrc:/icons/magnet.svg"; onClicked: magnetDlg.open() }
                TGrpDiv {}
                // G2: Pausar, Retomar, Parar
                TBtn { label: (i18n.language, i18n.t("tb_pause"));  icon: "qrc:/icons/pause.svg"; disabled: !win.hasSel; onClicked: session.pauseSelected() }
                TBtn { label: (i18n.language, i18n.t("tb_resume")); icon: "qrc:/icons/play.svg";  disabled: !win.hasSel; onClicked: session.resumeSelected() }
                TBtn { label: (i18n.language, i18n.t("tb_stop"));   icon: "qrc:/icons/stop.svg";  disabled: !win.hasSel; onClicked: session.pauseSelected() }
                TGrpDiv {}
                // G3: Remover
                TBtn { label: (i18n.language, i18n.t("tb_remove")); icon: "qrc:/icons/trash.svg"; disabled: !win.hasSel; onClicked: removeDlg.open() }
                TGrpDiv {}
                // G4: Buscar, RSS
                TBtn { label: (i18n.language, i18n.t("tb_search"));  icon: "qrc:/icons/search.svg"; onClicked: navRail.currentIndex = 2 }
                TBtn { label: (i18n.language, i18n.t("tb_rss"));     icon: "qrc:/icons/rss.svg";    onClicked: win.showWin(rssWinLoader) }
                TGrpDiv {}
                // G5: Config.
                TBtn { label: (i18n.language, i18n.t("tb_settings")); icon: "qrc:/icons/settings.svg"; onClicked: win.showWin(settingsWinLoader) }


                // .tb-spacer
                Item { Layout.fillWidth: true }

                // .spd-mod (border hair, radius 9, 2 cols)
                Rectangle {
                    Layout.preferredHeight: 44
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: spdRow.width + 0
                    color: "transparent"
                    border.color: Theme.hair
                    border.width: 1
                    radius: 9

                    Row {
                        id: spdRow
                        anchors.centerIn: parent
                        spacing: 0

                        // .c Download
                        Item {
                            width: dlCol.implicitWidth + 28
                            height: 44
                            Column {
                                id: dlCol
                                anchors.centerIn: parent
                                spacing: 3
                                Text {
                                    text: (i18n.language, i18n.t("graph_download"))
                                    color: Theme.t4
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                    font.letterSpacing: 1
                                    font.family: Theme.fontSans
                                }
                                Row {
                                    spacing: 5
                                    IconImg {
                                        anchors.verticalCenter: parent.verticalCenter
                                        src: "qrc:/icons/download.svg"
                                        tint: Theme.accentText
                                        s: 12
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s"
                                        color: Theme.accentText
                                        font.pixelSize: 13
                                        font.weight: Font.Bold
                                        font.family: Theme.fontMono
                                    }
                                }
                            }
                        }
                        // divider .c + .c
                        Rectangle { width: 1; height: 44; color: Theme.hair }
                        // .c Upload
                        Item {
                            width: upCol.implicitWidth + 28
                            height: 44
                            Column {
                                id: upCol
                                anchors.centerIn: parent
                                spacing: 3
                                Text {
                                    text: (i18n.language, i18n.t("graph_upload"))
                                    color: Theme.t4
                                    font.pixelSize: 9
                                    font.weight: Font.Bold
                                    font.letterSpacing: 1
                                    font.family: Theme.fontSans
                                }
                                Row {
                                    spacing: 5
                                    IconImg {
                                        anchors.verticalCenter: parent.verticalCenter
                                        src: "qrc:/icons/upload.svg"
                                        tint: Theme.up
                                        s: 12
                                    }
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s"
                                        color: Theme.up
                                        font.pixelSize: 13
                                        font.family: Theme.fontMono
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        // ================== SUBBAR ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp3

                // .search (240×34, padding 0 11, gap 8, bg panel) — the one
                // element that absorbs the shrink: it gives back width down to
                // 150 so the pills/category never have to clip.
                Rectangle {
                    Layout.preferredWidth: 240
                    Layout.minimumWidth: 150
                    Layout.preferredHeight: 34
                    Layout.alignment: Qt.AlignVCenter
                    color: Theme.panel
                    border.color: searchInput.activeFocus ? Theme.accent : Theme.hair
                    border.width: 1
                    radius: 8

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 11
                        anchors.rightMargin: 11
                        spacing: 8
                        IconImg {
                            Layout.alignment: Qt.AlignVCenter
                            src: "qrc:/icons/search.svg"
                            tint: Theme.t4
                            s: 14
                        }
                        TextInput {
                            id: searchInput
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            verticalAlignment: TextInput.AlignVCenter
                            color: Theme.t1
                            font.pixelSize: 13
                            font.family: Theme.fontSans
                            clip: true
                            // debounce: re-filtering the whole list on every
                            // keystroke stutters with a large library
                            onTextChanged: searchDebounce.restart()
                            Timer {
                                id: searchDebounce
                                interval: 150
                                onTriggered: if (typeof torrentFilter !== "undefined") torrentFilter.setSearchText(searchInput.text)
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: (i18n.language, i18n.t("search_heading"))
                                color: Theme.t4
                                font: searchInput.font
                                visible: searchInput.text.length === 0 && !searchInput.activeFocus
                            }
                        }
                    }
                }

                // .seg (toggle Grade/Lista) — padding 2, bg panel
                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: 32
                    Layout.minimumWidth: implicitWidth      // never clip the toggle
                    implicitWidth: segRow.implicitWidth + 4
                    color: Theme.panel
                    border.color: Theme.hair
                    border.width: 1
                    radius: 8

                    Row {
                        id: segRow
                        anchors.centerIn: parent
                        spacing: 2

                        Rectangle {
                            implicitWidth: segGr.implicitWidth + 22
                            height: 28
                            radius: 6
                            color: win.gridView ? Qt.rgba(1,1,1,0.08) : "transparent"
                            Row {
                                id: segGr
                                anchors.centerIn: parent
                                spacing: 6
                                IconImg {
                                    anchors.verticalCenter: parent.verticalCenter
                                    src: "qrc:/icons/grid.svg"
                                    tint: win.gridView ? Theme.t1 : Theme.t3
                                    s: 14
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: (i18n.language, i18n.t("view_grid"))
                                    color: win.gridView ? Theme.t1 : Theme.t3
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    font.family: Theme.fontSans
                                }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: win.gridView = true }
                        }
                        Rectangle {
                            implicitWidth: segLi.implicitWidth + 22
                            height: 28
                            radius: 6
                            color: !win.gridView ? Qt.rgba(1,1,1,0.08) : "transparent"
                            Row {
                                id: segLi
                                anchors.centerIn: parent
                                spacing: 6
                                IconImg {
                                    anchors.verticalCenter: parent.verticalCenter
                                    src: "qrc:/icons/list.svg"
                                    tint: !win.gridView ? Theme.t1 : Theme.t3
                                    s: 14
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: (i18n.language, i18n.t("view_list"))
                                    color: !win.gridView ? Theme.t1 : Theme.t3
                                    font.pixelSize: 12
                                    font.weight: Font.Medium
                                    font.family: Theme.fontSans
                                }
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: win.gridView = false }
                        }
                    }
                }

                // .pills (gap 4) — 6 pills, counts from session, click sets filter
                Row {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.minimumWidth: implicitWidth      // pills never clip
                    spacing: Theme.sp1
                    Pill { label: (i18n.language, i18n.t("filter_all"));     state: "all";         count: typeof session !== "undefined" ? session.torrentCount : 0;     onClicked: win.setFilter("all") }
                    Pill { label: (i18n.language, i18n.t("filter_all_active"));    state: "active";      count: typeof session !== "undefined" ? session.activeCount : 0;      onClicked: win.setFilter("active") }
                    Pill { label: (i18n.language, i18n.t("filter_downloading"));  state: "downloading"; count: typeof session !== "undefined" ? session.downloadingCount : 0; onClicked: win.setFilter("downloading") }
                    Pill { label: (i18n.language, i18n.t("filter_seeding"));  state: "seeding";     count: typeof session !== "undefined" ? session.seedingCount : 0;     onClicked: win.setFilter("seeding") }
                    Pill { label: (i18n.language, i18n.t("filter_paused"));   state: "paused";      count: typeof session !== "undefined" ? session.pausedCount : 0;      onClicked: win.setFilter("paused") }
                    Pill { label: (i18n.language, i18n.t("filter_completed")); state: "completed";   count: typeof session !== "undefined" ? session.completedCount : 0;   onClicked: win.setFilter("completed") }
                }

                Item { Layout.fillWidth: true }

                // .cat (Todas as categorias + chevron)
                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.preferredHeight: 34
                    Layout.minimumWidth: implicitWidth      // never clip the category dropdown
                    implicitWidth: catRow.implicitWidth + 24
                    color: "transparent"
                    border.color: Theme.hair
                    border.width: 1
                    radius: 8

                    Row {
                        id: catRow
                        anchors.centerIn: parent
                        spacing: 8
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: win.catFilter.length > 0 ? win.catLabel(win.catFilter) : (i18n.language, i18n.t("filter_all_categories"))
                            color: win.catFilter.length > 0 ? Theme.t1 : Theme.t2
                            font.pixelSize: 12
                            font.family: Theme.fontSans
                        }
                        IconImg {
                            anchors.verticalCenter: parent.verticalCenter
                            src: "qrc:/icons/chevron.svg"
                            tint: Theme.t4
                            s: 13
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: catFilterMenu.open()
                    }
                    Menu {
                        id: catFilterMenu
                        y: parent.height + 4
                        implicitWidth: 200
                        delegate: CatItem {}
                        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
                        CatItem { text: (i18n.language, i18n.t("filter_all_categories")); onTriggered: win.applyCatFilter("") }
                        MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
                        CatItem { text: win.catLabel("Apps");   onTriggered: win.applyCatFilter("Apps") }
                        CatItem { text: win.catLabel("Games");  onTriggered: win.applyCatFilter("Games") }
                        CatItem { text: win.catLabel("Movies"); onTriggered: win.applyCatFilter("Movies") }
                        CatItem { text: win.catLabel("Series"); onTriggered: win.applyCatFilter("Series") }
                    }
                }

                // donate moved to the nav rail (bottom) — it was cramped here
                // and got clipped when the filter row filled up.
                // (the port indicator now lives in the status bar: it's status,
                // not a filter, and it was the first thing to clip here)
            }
        }

        // ================== CONTENT (grid OR list) ==================
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            readonly property bool empty: typeof session !== "undefined" && session.torrentCount === 0

            // full custom background image (z:-1, behind anime art and grid/list).
            // A theme-bg scrim at user-controlled opacity sits on top for legibility.
            Item {
                id: bgImageWrap
                anchors.fill: parent
                visible: Theme.hasBgImage && !parent.empty
                z: -1
                Image {
                    anchors.fill: parent
                    source: Theme.bgImageSource
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    sourceSize.width: parent.width
                    sourceSize.height: parent.height
                    cache: false
                }
                Rectangle {
                    anchors.fill: parent
                    color: Theme.bg
                    opacity: Theme.bgImageOpacity
                }
            }

            // empty state (no torrents)
            EmptyState {
                anchors.centerIn: parent
                visible: parent.empty
                onOpenClicked: openFileDlg.open()
                onMagnetClicked: magnetDlg.open()
            }


            // anime art (eyes top-right / spider bottom-right). Ported 1:1 from .eyes-accent:
            // the CSS fades the edges via two intersected linear masks; since only Theme.bg sits
            // behind the art, we reproduce it with two bg-colored gradient scrims (left + bottom/top).
            Item {
                id: animeArtWrap
                visible: Theme.hasAnime && !parent.empty
                width: Math.min(Theme.animeBottom ? 560 : 460, parent.width * 0.46)
                height: animeArt.implicitWidth > 0 ? animeArt.implicitHeight * (width / animeArt.implicitWidth) : 0
                anchors.right: parent.right
                anchors.top: Theme.animeBottom ? undefined : parent.top
                anchors.bottom: Theme.animeBottom ? parent.bottom : undefined
                anchors.bottomMargin: Theme.animeBottom ? -80 : 0   // spider sits lower (peeks from bottom)
                z: 0

                Image {
                    id: animeArt
                    anchors.fill: parent
                    source: Theme.hasAnime ? Theme.animeSource : ""
                    fillMode: Image.PreserveAspectFit
                    // list rows put state/peer columns right on top of the art —
                    // drop it to a watermark there so data wins the contrast fight
                    opacity: win.gridView ? 0.9 : 0.25
                    Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }
                }
                // fade left edge (mask: linear-gradient(90deg, transparent, #000 55%))
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0.0; color: Theme.bg }
                        GradientStop { position: 0.55; color: "transparent" }
                    }
                }
                // fade bottom (eyes) / top (spider) — mask: linear-gradient(180deg, #000 60%, transparent)
                Rectangle {
                    anchors.fill: parent
                    gradient: Gradient {
                        orientation: Gradient.Vertical
                        GradientStop { position: 0.0; color: Theme.animeBottom ? Theme.bg : "transparent" }
                        GradientStop { position: Theme.animeBottom ? 0.40 : 0.60; color: "transparent" }
                        GradientStop { position: 1.0; color: Theme.animeBottom ? "transparent" : Theme.bg }
                    }
                }
            }

            // ----- GRID -----
            GridView {
                id: grid
                opacity: (win.gridView && !parent.empty) ? 1 : 0
                visible: opacity > 0.01
                Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                anchors.fill: parent
                topMargin: Theme.sp5
                bottomMargin: Theme.sp5
                leftMargin: Theme.sp4
                rightMargin: Theme.sp4
                cellWidth: 178 + Theme.sp4
                cellHeight: 286
                // No `populate` transition: it re-runs (fading every tile from 0)
                // when the filter proxy reports a reorder as a re-layout, which read
                // as the whole grid flashing. The container's opacity Behavior already
                // covers the initial fade-in. (List view has no populate, never flashed.)
                add: Transition {
                    NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 180; easing.type: Easing.OutCubic }
                    NumberAnimation { properties: "scale"; from: 0.9; to: 1; duration: 180; easing.type: Easing.OutCubic }
                }
                remove: Transition {
                    NumberAnimation { properties: "opacity"; to: 0; duration: 160; easing.type: Easing.OutCubic }
                    NumberAnimation { properties: "scale"; to: 0.85; duration: 160; easing.type: Easing.OutCubic }
                }
                displaced: Transition {
                    NumberAnimation { properties: "x,y"; duration: 280; easing.type: Easing.OutBack; easing.overshoot: 0.9 }
                    NumberAnimation { properties: "scale"; to: 1; duration: 280; easing.type: Easing.OutCubic }
                }
                move: Transition {
                    NumberAnimation { properties: "x,y"; duration: 300; easing.type: Easing.OutBack; easing.overshoot: 1.1 }
                }
                moveDisplaced: Transition {
                    NumberAnimation { properties: "x,y"; duration: 280; easing.type: Easing.OutBack; easing.overshoot: 0.9 }
                }
                clip: true
                model: win.model
                interactive: true
                z: 1

                delegate: Item {
                    id: tile
                    width: 178
                    height: 286

                    required property int index
                    required property string torrentName
                    required property string metaTitle
                    required property string stateKey
                    required property real progress
                    required property string posterPath
                    required property string stateString
                    required property string category
                    required property string size

                    readonly property string posterUrl: win.fileUrl(posterPath)

                    // soft drop shadow under the cover, fading in on hover
                    Rectangle {
                        z: -1
                        width: 178 * 0.84
                        x: (178 - width) / 2
                        y: 237 - 10
                        height: 22
                        radius: 11
                        color: "#000000"
                        opacity: tileMa.containsMouse ? 0.5 : 0
                        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                        layer.enabled: true
                        layer.effect: MultiEffect { blurEnabled: true; blur: 1.0; blurMax: 28 }
                    }

                    // .poster wrapper (aspect 3:4 ≈ 178:237)
                    Item {
                        id: posterWrap
                        width: 178
                        height: 237

                        // fallback (no poster): tinted bg + watermark + category + title
                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: "#161618"
                            visible: tile.posterUrl === ""
                            // watermark: BATorrent logo (not the title's first letter)
                            Image {
                                anchors.centerIn: parent
                                width: parent.width * 0.5
                                height: width
                                source: "qrc:/images/logo.svg"
                                sourceSize: Qt.size(width * 2, width * 2)
                                fillMode: Image.PreserveAspectFit
                                opacity: 0.06
                                layer.enabled: Theme.isLight
                                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
                            }
                            Text {
                                anchors.left: parent.left; anchors.top: parent.top
                                anchors.leftMargin: 13; anchors.topMargin: 12
                                text: tile.category
                                color: Qt.rgba(1, 1, 1, 0.42)
                                font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.3
                                font.capitalization: Font.AllUppercase
                                font.family: Theme.fontSans
                            }
                            Text {
                                anchors.left: parent.left; anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                anchors.leftMargin: 13; anchors.rightMargin: 13; anchors.bottomMargin: 15
                                text: tile.metaTitle || tile.torrentName
                                color: "#f5f5f6"
                                font.pixelSize: 18; font.weight: Font.Bold; font.letterSpacing: -0.3
                                font.family: Theme.fontSans
                                wrapMode: Text.WordWrap
                                maximumLineCount: 3
                                elide: Text.ElideRight
                            }
                        }

                        // poster image (masked rounded) — only when present
                        Rectangle {
                            id: posterBg
                            anchors.fill: parent
                            color: "#161618"
                            visible: false
                            layer.enabled: true
                            Image {
                                anchors.fill: parent
                                source: tile.posterUrl
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                // decode at ~2× display size, not the poster's full
                                // resolution — cuts memory and decode time per cover.
                                sourceSize: Qt.size(356, 474)
                                cache: true
                            }
                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: parent.height * 0.6
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "transparent" }
                                    GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.45) }
                                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.92) }
                                }
                            }
                        }
                        Rectangle {
                            id: posterMask
                            anchors.fill: parent
                            radius: 10
                            color: "white"
                            visible: false
                            layer.enabled: true
                        }
                        MultiEffect {
                            source: posterBg
                            anchors.fill: parent
                            maskEnabled: true
                            maskSource: posterMask
                            visible: tile.posterUrl !== ""
                        }
                        // title over the fade (only when poster present)
                        Text {
                            visible: tile.posterUrl !== ""
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            anchors.bottomMargin: 12
                            text: tile.metaTitle || tile.torrentName
                            color: "#f5f5f6"
                            font.pixelSize: 15
                            font.weight: Font.Bold
                            font.letterSpacing: -0.2
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                            maximumLineCount: 2
                            wrapMode: Text.WordWrap
                        }
                        // .pbar progress (bottom, over everything)
                        Rectangle {
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 3
                            color: Qt.rgba(0, 0, 0, 0.5)
                            Rectangle {
                                height: parent.height
                                width: parent.width * tile.progress
                                color: win.fillFor(tile.stateKey)
                                Behavior on width { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
                            }
                        }
                        // border overlay (radius 10, hair / accent when sel)
                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: "transparent"
                            border.color: win.isRowSelected(tile.index) ? Theme.accent : (tileMa.containsMouse ? Qt.rgba(1,1,1,0.2) : Theme.hair)
                            border.width: win.isRowSelected(tile.index) ? 2 : 1
                            Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
                        }
                        MouseArea {
                            id: tileMa
                            anchors.fill: parent
                            hoverEnabled: true
                            acceptedButtons: Qt.LeftButton | Qt.RightButton
                            cursorShape: Qt.PointingHandCursor
                            onClicked: function(mouse) {
                                if (mouse.button === Qt.RightButton) {
                                    if (!win.isRowSelected(tile.index)) win.selectRow(tile.index, 0)
                                    win.openContext(tile.index)
                                } else {
                                    win.selectRow(tile.index, mouse.modifiers)
                                }
                            }
                            onDoubleClicked: function(mouse) {
                                if (mouse.button !== Qt.RightButton) {
                                    win.selectRow(tile.index, 0); session.openSelectedFile()
                                }
                            }
                        }
                    }

                    // .tmeta (padding-top 12: .st dot+label · .sz)
                    RowLayout {
                        anchors.top: posterWrap.bottom
                        anchors.topMargin: 12
                        anchors.left: posterWrap.left
                        anchors.right: posterWrap.right
                        spacing: 0

                        Row {
                            spacing: 6
                            Rectangle {
                                width: 6
                                height: 6
                                radius: 3
                                anchors.verticalCenter: parent.verticalCenter
                                color: win.dotFor(tile.stateKey)
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: tile.stateString
                                color: win.textFor(tile.stateKey)
                                font.pixelSize: 12
                                font.family: Theme.fontSans
                            }
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: tile.size
                            color: Theme.t4
                            font.pixelSize: 12
                            font.family: Theme.fontMono
                        }
                    }
                }
            }

            // Empty-grid click → clear selection. Sits OVER the grid (z:2) and
            // propagates clicks that land on a tile so tileMa still handles them;
            // only clicks on blank space (grid.indexAt == -1) deselect. A plain
            // MouseArea inside the Flickable never received these.
            MouseArea {
                anchors.fill: grid
                visible: win.gridView && !parent.empty
                enabled: visible
                z: 2
                acceptedButtons: Qt.LeftButton
                propagateComposedEvents: true
                onClicked: function(mouse) {
                    var idx = grid.indexAt(mouse.x + grid.contentX, mouse.y + grid.contentY)
                    if (idx < 0) {
                        if (win.selectedRows.length > 0) {
                            win.selectedRows = []; win.selected = -1; win._commitSel()
                        }
                    } else {
                        mouse.accepted = false   // let the tile's MouseArea handle it
                    }
                }
                onPressed: function(mouse) {
                    // don't swallow the press over a tile, or scrolling/clicks break
                    if (grid.indexAt(mouse.x + grid.contentX, mouse.y + grid.contentY) >= 0)
                        mouse.accepted = false
                }
            }

            // ----- LIST -----
            ListView {
                id: list
                opacity: (!win.gridView && !parent.empty) ? 1 : 0
                visible: opacity > 0.01
                Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                anchors.fill: parent
                clip: true
                model: win.model
                interactive: true
                z: 1
                add: Transition { NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 160; easing.type: Easing.OutCubic } }
                remove: Transition { NumberAnimation { properties: "opacity"; to: 0; duration: 120; easing.type: Easing.OutCubic } }
                displaced: Transition { NumberAnimation { properties: "x,y"; duration: 180; easing.type: Easing.OutCubic } }

                header: Rectangle {
                    width: ListView.view.width
                    height: 36
                    color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp4
                        anchors.rightMargin: Theme.sp4
                        spacing: Theme.sp4

                        HCol { label: (i18n.language, i18n.t("col_name")); col: "name"; fill: true }
                        HCol { label: (i18n.language, i18n.t("col_size")); col: "size"; w: 78; alignRight: true }
                        HCol { label: (i18n.language, i18n.t("col_progress")); col: "progress"; w: 104 }
                        HCol { label: (i18n.language, i18n.t("col_down")); col: "down"; w: 78; alignRight: true }
                        HCol { label: (i18n.language, i18n.t("col_up")); col: "up"; w: 78; alignRight: true }
                        HCol { label: (i18n.language, i18n.t("col_state")); col: "state"; w: 110 }
                        HCol { label: (i18n.language, i18n.t("col_category")); col: "category"; w: 90 }
                        HCol { label: (i18n.language, i18n.t("col_peers")); col: "peers"; w: 56; alignRight: true }
                    }
                }

                delegate: Rectangle {
                    id: lrow
                    width: ListView.view.width
                    height: 56

                    required property int index
                    required property string torrentName
                    required property string metaTitle
                    required property string stateKey
                    required property real progress
                    required property string stateString
                    required property string stateDetail
                    required property string size
                    required property string downSpeed
                    required property string upSpeed
                    required property int downRate
                    required property int upRate
                    required property string category
                    required property int numPeers
                    required property string posterPath

                    readonly property string posterUrl: win.fileUrl(posterPath)
                    // the "stalled why" tooltip is driven by listArea (its z:2
                    // hover-exclusive MouseArea starves any in-delegate handler)
                    readonly property Item stateCell: lrowStateText

                    color: win.isRowSelected(index) ? Theme.sel : (listArea.hoveredRow === index ? Theme.hover : "transparent")

                    // .sel inset 2px barra esquerda
                    Rectangle {
                        visible: win.isRowSelected(lrow.index)
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: 2
                        color: Theme.accent
                    }
                    // border-bottom hairSoft
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp4
                        anchors.rightMargin: Theme.sp4
                        spacing: Theme.sp4

                        // .name: poster thumb + nome
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: Theme.sp3
                            PosterThumb {
                                Layout.alignment: Qt.AlignVCenter
                                posterUrl: lrow.posterUrl
                                label: lrow.metaTitle || lrow.torrentName || ""
                            }
                            Text {
                                Layout.fillWidth: true
                                Layout.alignment: Qt.AlignVCenter
                                text: lrow.metaTitle || lrow.torrentName
                                color: Theme.t1
                                font.pixelSize: 13
                                font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                        }
                        Text {
                            text: lrow.size
                            Layout.preferredWidth: 78
                            horizontalAlignment: Text.AlignRight
                            color: Theme.t2
                            font.pixelSize: 12
                            font.family: Theme.fontMono
                        }
                        // progress: surfaceAlt track, state-colored fill, centered % (white over fill, t1 over track)
                        Item {
                            Layout.preferredWidth: 104
                            Layout.preferredHeight: 18
                            Rectangle {
                                id: pbarTrack
                                anchors.fill: parent
                                radius: 4
                                color: Theme.field
                                clip: true
                                Rectangle {
                                    anchors.top: parent.top
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left
                                    width: Math.max(lrow.progress > 0.001 ? 2 : 0, parent.width * lrow.progress)
                                    radius: 4
                                    color: win.fillFor(lrow.stateKey)
                                }
                                Text {
                                    id: pbarPct
                                    anchors.centerIn: parent
                                    text: (lrow.progress * 100).toFixed(1) + "%"
                                    color: (parent.width / 2) < (parent.width * lrow.progress - 4) ? "#ffffff" : Theme.t1
                                    font.pixelSize: 9
                                    font.weight: Font.DemiBold
                                    font.family: Theme.fontSans
                                }
                            }
                        }
                        Text {
                            text: lrow.downRate > 0 ? lrow.downSpeed : "—"
                            Layout.preferredWidth: 78
                            horizontalAlignment: Text.AlignRight
                            color: lrow.downRate > 0 ? Theme.accentText : Theme.t4
                            font.pixelSize: 12
                            font.family: Theme.fontMono
                        }
                        Text {
                            text: lrow.upRate > 0 ? lrow.upSpeed : "—"
                            Layout.preferredWidth: 78
                            horizontalAlignment: Text.AlignRight
                            color: lrow.upRate > 0 ? Theme.up : Theme.t4
                            font.pixelSize: 12
                            font.family: Theme.fontMono
                        }
                        Text {
                            id: lrowStateText
                            text: lrow.stateString
                            Layout.preferredWidth: 110
                            color: win.textFor(lrow.stateKey)
                            font.pixelSize: 12
                            font.weight: Theme.hasAnime ? Font.DemiBold : Font.Medium
                            font.family: Theme.fontSans
                        }
                        Text {
                            text: win.catLabel(lrow.category)
                            Layout.preferredWidth: 90
                            color: Theme.hasAnime ? Theme.t1 : Theme.t3
                            style: Theme.hasAnime ? Text.Outline : Text.Normal
                            styleColor: Theme.isLight ? "#ffffff" : "#000000"
                            font.pixelSize: 12
                            font.weight: Theme.hasAnime ? Font.Medium : Font.Normal
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                        Text {
                            text: lrow.numPeers
                            Layout.preferredWidth: 56
                            horizontalAlignment: Text.AlignRight
                            color: lrow.numPeers === 0 ? (Theme.hasAnime ? Theme.t2 : Theme.t4) : (Theme.hasAnime ? Theme.t1 : Theme.t2)
                            style: Theme.hasAnime ? Text.Outline : Text.Normal
                            styleColor: Theme.isLight ? "#ffffff" : "#000000"
                            font.pixelSize: 12
                            font.weight: Theme.hasAnime ? Font.Medium : Font.Normal
                            font.family: Theme.fontMono
                        }
                    }
                }
            }

            // ----- marquee + click/hover overlay for the list -----
            MouseArea {
                id: listArea
                anchors.fill: list
                visible: !win.gridView && !parent.empty
                enabled: visible
                hoverEnabled: true
                preventStealing: true   // don't let the ListView steal the gesture
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                z: 2

                property int hoveredRow: -1
                readonly property int rowH: 56
                property bool dragging: false
                property real startX: 0
                property real startY: 0
                property int pressRow: -1

                // "stalled why" tooltip over the State cell — owned here because
                // this hover-exclusive MouseArea starves in-delegate handlers
                property string tipText: ""
                property point tipPos: Qt.point(0, 0)
                Timer {
                    id: stateTipDelay
                    interval: 350
                    onTriggered: if (listArea.tipText.length > 0) stateTip.visible = true
                }
                function updateStateTip(mx, my) {
                    var row = rowAt(my, mx)
                    var d = row >= 0 ? list.itemAtIndex(row) : null
                    if (d && d.stateDetail !== undefined && d.stateDetail.length > 0 && !dragging) {
                        var p = d.stateCell.mapToItem(listArea, 0, 0)
                        if (mx >= p.x - 6 && mx <= p.x + d.stateCell.width + 6) {
                            tipText = d.stateDetail
                            tipPos = Qt.point(p.x, p.y + 26)
                            if (!stateTip.visible) stateTipDelay.restart()
                            return
                        }
                    }
                    tipText = ""
                    stateTipDelay.stop()
                    stateTip.visible = false
                }
                Rectangle {
                    id: stateTip
                    visible: false
                    x: Math.max(8, Math.min(listArea.tipPos.x, listArea.width - width - 8))
                    y: listArea.tipPos.y
                    z: 10
                    radius: 7
                    color: Theme.panel
                    border.color: Theme.hair
                    border.width: 1
                    width: stateTipText.implicitWidth + 20
                    height: stateTipText.implicitHeight + 12
                    Text {
                        id: stateTipText
                        anchors.centerIn: parent
                        text: listArea.tipText
                        color: Theme.t1
                        font.pixelSize: 11
                        font.family: Theme.fontSans
                    }
                }

                function rowAt(my, mx) {
                    if (!win.model || list.count === 0) return -1
                    // use the ListView's own layout so detection lines up exactly
                    // with where each delegate is drawn (header returns -1).
                    return list.indexAt((mx === undefined ? width / 2 : mx) + list.contentX,
                                        my + list.contentY)
                }

                onPositionChanged: function(mouse) {
                    hoveredRow = rowAt(mouse.y, mouse.x)
                    updateStateTip(mouse.x, mouse.y)
                    if (pressed && !dragging &&
                        (Math.abs(mouse.x - startX) > 8 || Math.abs(mouse.y - startY) > 8))
                        dragging = true
                    if (dragging) {
                        marquee.x = Math.min(startX, mouse.x)
                        marquee.y = Math.min(startY, mouse.y)
                        marquee.width = Math.abs(mouse.x - startX)
                        marquee.height = Math.abs(mouse.y - startY)
                    }
                }
                onExited: { hoveredRow = -1; tipText = ""; stateTipDelay.stop(); stateTip.visible = false }
                onPressed: function(mouse) {
                    startX = mouse.x; startY = mouse.y
                    pressRow = rowAt(mouse.y, mouse.x)
                    dragging = false
                }
                onReleased: function(mouse) {
                    // a real marquee = dragged past threshold AND box big enough
                    if (dragging && (marquee.width > 6 || marquee.height > 6)) {
                        var top = marquee.y, bot = marquee.y + marquee.height
                        var rows = []
                        for (var i = 0; i < list.count; ++i) {
                            var ry = list.headerItem.height + i * rowH - list.contentY
                            if (ry + rowH > top && ry < bot) rows.push(i)
                        }
                        win.selectedRows = rows
                        win.selected = rows.length > 0 ? rows[rows.length - 1] : -1
                        win.anchorRow = rows.length > 0 ? rows[0] : -1
                        win._commitSel()
                        dragging = false
                        return
                    }
                    dragging = false
                    // otherwise treat as a click (use release position, robust to jitter)
                    var clickRow = rowAt(mouse.y, mouse.x)
                    if (clickRow < 0) {
                        if (mouse.button === Qt.LeftButton) {
                            win.selectedRows = []; win.selected = -1; win._commitSel()
                        }
                        return
                    }
                    if (mouse.button === Qt.RightButton) {
                        if (!win.isRowSelected(clickRow)) win.selectRow(clickRow, 0)
                        win.openContext(clickRow)
                    } else {
                        win.selectRow(clickRow, mouse.modifiers)
                    }
                }
                onDoubleClicked: function(mouse) {
                    var r = rowAt(mouse.y, mouse.x)
                    // select the clicked row first so we reveal *that* torrent,
                    // not whatever was selected before, then open its folder
                    // with the file highlighted.
                    if (r >= 0) { win.selectRow(r, 0); session.openSelectedFile() }
                }

                Rectangle {
                    id: marquee
                    visible: listArea.dragging
                    color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12)
                    border.color: Theme.accent
                    border.width: 1
                    radius: 2
                }
            }
        }

        // ================== GRAPH ==================
        Rectangle {
            id: graphPanel
            Layout.fillWidth: true
            Layout.preferredHeight: 64
            color: Theme.bg
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

            readonly property var dl: typeof session !== "undefined" ? session.downloadHistory : []
            readonly property var ul: typeof session !== "undefined" ? session.uploadHistory : []
            // auto-scale to the real peak of the visible window (like speedgraph.cpp:
            // no fixed floor → the curve always uses ~87% of the height, whether
            // traffic is a few B/s or several MB/s). A floor squished low traffic
            // into a flat band at the bottom.
            readonly property int scaledMax: {
                var m = 1
                for (var i = 0; i < dl.length; ++i) if (dl[i] > m) m = dl[i]
                for (var j = 0; j < ul.length; ++j) if (ul[j] > m) m = ul[j]
                return Math.round(m * 1.15)
            }
            readonly property int slots: 60

            function scaleText(b) {
                if (b >= 1024 * 1024) return (b / (1024 * 1024)).toFixed(1) + " MB/s"
                return Math.round(b / 1024) + " KB/s"
            }
            // build a smooth filled area path from a byte-history array
            function areaPath(arr, h) {
                if (!arr || arr.length === 0) return ""
                var n = arr.length
                var step = graphShape.width / (slots - 1)
                var off = (slots - n) * step
                function yAt(v) { return h - (v / scaledMax) * (h - 2) }
                var x0 = off
                var s = "M " + x0.toFixed(1) + "," + h.toFixed(1)
                s += " L " + x0.toFixed(1) + "," + yAt(arr[0]).toFixed(1)
                for (var i = 1; i < n; ++i) {
                    var px = off + (i - 1) * step, py = yAt(arr[i - 1])
                    var x = off + i * step, y = yAt(arr[i])
                    var cx = (px + x) / 2
                    s += " C " + cx.toFixed(1) + "," + py.toFixed(1) + " " + cx.toFixed(1) + "," + y.toFixed(1) + " " + x.toFixed(1) + "," + y.toFixed(1)
                }
                s += " L " + (off + (n - 1) * step).toFixed(1) + "," + h.toFixed(1) + " Z"
                return s
            }
            // open smooth curve (top line only — no bottom edge / no fill close)
            function linePath(arr, h) {
                if (!arr || arr.length === 0) return ""
                var n = arr.length
                var step = graphShape.width / (slots - 1)
                var off = (slots - n) * step
                function yAt(v) { return h - (v / scaledMax) * (h - 2) }
                var s = "M " + off.toFixed(1) + "," + yAt(arr[0]).toFixed(1)
                for (var i = 1; i < n; ++i) {
                    var px = off + (i - 1) * step, py = yAt(arr[i - 1])
                    var x = off + i * step, y = yAt(arr[i])
                    var cx = (px + x) / 2
                    s += " C " + cx.toFixed(1) + "," + py.toFixed(1) + " " + cx.toFixed(1) + "," + y.toFixed(1) + " " + x.toFixed(1) + "," + y.toFixed(1)
                }
                return s
            }

            Text {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.topMargin: 6
                anchors.leftMargin: Theme.sp4
                text: graphPanel.scaleText(graphPanel.scaledMax)
                color: Theme.t4
                font.pixelSize: 10
                font.family: Theme.fontSans
            }
            Row {
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.topMargin: 6
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp4
                Row {
                    spacing: 6
                    Rectangle { implicitWidth: 6; implicitHeight: 6; radius: 3; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s"; color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                }
                Row {
                    spacing: 6
                    Rectangle { implicitWidth: 6; implicitHeight: 6; radius: 3; color: Theme.amber; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s"; color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                }
            }

            // real curves from session history (download accent + upload amber)
            Shape {
                id: graphShape
                anchors.fill: parent
                anchors.topMargin: 24
                anchors.bottomMargin: 4
                preferredRendererType: Shape.CurveRenderer   // smooth lines on Windows (no MSAA)
                antialiasing: true

                // order matches speedgraph.cpp: upload under, download on top.
                // fills carry no stroke (the bottom edge was the phantom floor line).
                ShapePath {
                    strokeColor: "transparent"
                    strokeWidth: 0
                    fillGradient: LinearGradient {
                        x1: 0; y1: 0; x2: 0; y2: graphShape.height
                        GradientStop { position: 0.0; color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.09) }
                        GradientStop { position: 1.0; color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.0) }
                    }
                    PathSvg { path: graphPanel.areaPath(graphPanel.ul, graphShape.height) }
                }
                ShapePath {
                    strokeColor: "transparent"
                    strokeWidth: 0
                    fillGradient: LinearGradient {
                        x1: 0; y1: 0; x2: 0; y2: graphShape.height
                        GradientStop { position: 0.0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.09) }
                        GradientStop { position: 1.0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.0) }
                    }
                    PathSvg { path: graphPanel.areaPath(graphPanel.dl, graphShape.height) }
                }
                ShapePath {
                    strokeColor: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.5)
                    strokeWidth: 1.25
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    PathSvg { path: graphPanel.linePath(graphPanel.ul, graphShape.height) }
                }
                ShapePath {
                    strokeColor: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.5)
                    strokeWidth: 1.25
                    fillColor: "transparent"
                    capStyle: ShapePath.RoundCap
                    joinStyle: ShapePath.RoundJoin
                    PathSvg { path: graphPanel.linePath(graphPanel.dl, graphShape.height) }
                }
            }
        }

        // ================== DETAIL ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 270
            color: Theme.panel
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // .dtabs (42px, 5 tabs)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 42
                    color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

                    Row {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.sp5
                        spacing: Theme.sp5

                        Repeater {
                            model: [
                                { label: (i18n.language, i18n.t("detail_general")),    ct: "" },
                                { label: (i18n.language, i18n.t("detail_peers")),    ct: win.hasSel ? String(session.selectedPeers) : "" },
                                { label: (i18n.language, i18n.t("detail_files")), ct: win.hasSel ? String(session.selectedFiles.length) : "" },
                                { label: (i18n.language, i18n.t("detail_trackers")), ct: win.hasSel ? String(session.selectedTrackers.length) : "" },
                                { label: (i18n.language, i18n.t("detail_pieces")),  ct: "" }
                            ]
                            delegate: Item {
                                height: 42
                                width: dtabRow.implicitWidth
                                readonly property bool on: win.detailTab === index

                                Row {
                                    id: dtabRow
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 7
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.label
                                        color: parent.parent.on ? Theme.t1 : (dtabMa.containsMouse ? Theme.t2 : Theme.t3)
                                        font.pixelSize: 13
                                        font.weight: parent.parent.on ? Font.DemiBold : Font.Medium
                                        font.family: Theme.fontSans
                                    }
                                    Text {
                                        visible: modelData.ct !== ""
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.ct
                                        color: Theme.t4
                                        font.pixelSize: 11
                                        font.family: Theme.fontMono
                                    }
                                }
                                Rectangle {
                                    visible: parent.on
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.bottom: parent.bottom
                                    height: 2
                                    color: Theme.accent
                                }
                                MouseArea {
                                    id: dtabMa
                                    anchors.fill: parent
                                    hoverEnabled: true
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: win.detailTab = index
                                }
                            }
                        }
                    }
                }

                // .dbody — stacked panes (Geral / Peers / Arquivos / Trackers / Pedaços)
                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: win.detailTab

                  // --- 0: Geral ---
                  Item {
                  RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.sp5
                    spacing: Theme.sp6

                    // .dcover (radius 8 — mask via MultiEffect)
                    Item {
                        Layout.preferredWidth: 104
                        Layout.preferredHeight: 146
                        Layout.alignment: Qt.AlignTop

                        Rectangle {
                            id: dcoverContent
                            anchors.fill: parent
                            color: "#161618"
                            visible: false
                            layer.enabled: true
                            Image {
                                anchors.fill: parent
                                source: win.fileUrl(win.hasSel ? session.selectedPoster : "")
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: true
                                sourceSize: Qt.size(208, 292)
                            }
                        }
                        Rectangle {
                            id: dcoverMask
                            anchors.fill: parent
                            radius: 8
                            color: "white"
                            visible: false
                            layer.enabled: true
                        }
                        MultiEffect {
                            source: dcoverContent
                            anchors.fill: parent
                            maskEnabled: true
                            maskSource: dcoverMask
                            visible: win.hasSel && session.selectedPoster.length > 0
                        }
                        // placeholder logo (no poster / no selection)
                        Image {
                            anchors.centerIn: parent
                            visible: !(win.hasSel && session.selectedPoster.length > 0)
                            width: 52; height: 52
                            source: "qrc:/images/logo.svg"
                            sourceSize: Qt.size(104, 104)
                            fillMode: Image.PreserveAspectFit
                            opacity: 0.4
                            layer.enabled: Theme.isLight
                            layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
                        }
                        Rectangle {
                            anchors.fill: parent
                            radius: 8
                            color: "transparent"
                            border.color: Theme.hair
                            border.width: 1
                        }
                    }

                    // .dmain max-width 460
                    ColumnLayout {
                        Layout.preferredWidth: 460
                        Layout.maximumWidth: 460
                        Layout.alignment: Qt.AlignTop
                        spacing: 6

                        Text {
                            text: win.hasSel ? (session.selectedMetaTitle.length > 0 ? session.selectedMetaTitle : session.selectedName) : (i18n.language, i18n.t("empty_no_selection"))
                            color: Theme.t1
                            font.pixelSize: 17
                            font.weight: Font.DemiBold
                            font.letterSpacing: -0.2
                            font.family: Theme.fontSans
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            visible: win.hasSel && session.selectedMetaInfo.length > 0
                            text: win.hasSel ? session.selectedMetaInfo : ""
                            color: Theme.t3
                            font.pixelSize: 12
                            font.family: Theme.fontSans
                        }
                        Text {
                            visible: win.hasSel && session.selectedDescription.length > 0
                            Layout.topMargin: 6
                            Layout.fillWidth: true
                            wrapMode: Text.WordWrap
                            maximumLineCount: 4
                            elide: Text.ElideRight
                            color: Theme.t2
                            font.pixelSize: 12
                            font.family: Theme.fontSans
                            lineHeight: 1.55
                            text: win.hasSel ? session.selectedDescription : ""
                        }
                    }

                    Item { Layout.fillWidth: true }

                    // .dcols (3 colunas KV)
                    RowLayout {
                        Layout.alignment: Qt.AlignTop
                        spacing: Theme.sp6

                        // INFO
                        ColumnLayout {
                            Layout.preferredWidth: 168
                            Layout.alignment: Qt.AlignTop
                            spacing: 0

                            Text {
                                text: (i18n.language, i18n.t("detail_section_info"))
                                color: Theme.t4
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                Layout.bottomMargin: Theme.sp3
                            }
                            Repeater {
                                model: [
                                    { kk: "detail_kv_name",  v: win.hasSel ? session.selectedName : "—" },
                                    { kk: "detail_kv_size",  v: win.hasSel ? session.selectedSize : "—" },
                                    { kk: "detail_kv_hash",  v: win.hasSel ? session.selectedHash : "—" },
                                    { kk: "detail_kv_state", v: win.hasSel ? session.selectedState : "—" }
                                ]
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: (i18n.language, i18n.t(modelData.kk)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                                    Item { Layout.fillWidth: true }
                                    Text { text: modelData.v; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideMiddle; Layout.maximumWidth: 110; horizontalAlignment: Text.AlignRight }
                                }
                            }
                        }

                        // TRANSFERÊNCIA
                        ColumnLayout {
                            Layout.preferredWidth: 168
                            Layout.alignment: Qt.AlignTop
                            spacing: 0

                            Text {
                                text: (i18n.language, i18n.t("detail_section_transfer"))
                                color: Theme.t4
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                Layout.bottomMargin: Theme.sp3
                            }
                            Repeater {
                                model: [
                                    { kk: "detail_kv_downloaded", v: win.hasSel ? session.selectedDownloaded : "—" },
                                    { kk: "col_down",             v: win.hasSel ? session.selectedDownSpeed : "—" },
                                    { kk: "col_up",               v: win.hasSel ? session.selectedUpSpeed : "—" },
                                    { kk: "detail_kv_eta",        v: win.hasSel ? session.selectedEta : "—" }
                                ]
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: (i18n.language, i18n.t(modelData.kk)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                                    Item { Layout.fillWidth: true }
                                    Text { text: modelData.v; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans }
                                }
                            }
                        }

                        // PEERS
                        ColumnLayout {
                            Layout.preferredWidth: 168
                            Layout.alignment: Qt.AlignTop
                            spacing: 0

                            Text {
                                text: (i18n.language, i18n.t("detail_section_peers"))
                                color: Theme.t4
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                Layout.bottomMargin: Theme.sp3
                            }
                            Repeater {
                                model: [
                                    { kk: "detail_kv_seeds", v: win.hasSel ? String(session.selectedSeeds) : "—" },
                                    { kk: "detail_kv_peers", v: win.hasSel ? String(session.selectedPeers) : "—" },
                                    { kk: "detail_kv_ratio", v: win.hasSel ? session.selectedRatio : "—" }
                                ]
                                delegate: RowLayout {
                                    Layout.fillWidth: true
                                    Text { text: (i18n.language, i18n.t(modelData.kk)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                                    Item { Layout.fillWidth: true }
                                    Text { text: modelData.v; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans }
                                }
                            }
                        }
                    }
                  }
                  }
                  // --- 1: Peers · 2: Arquivos · 3: Trackers · 4: Pedaços ---
                  // Only build the heavy list for the tab that's actually open.
                  // StackLayout instantiates every child and keeps its bindings
                  // live, so without the tab guard, clicking a torrent rebuilt
                  // ALL of these at once — and selectedPieces alone is one entry
                  // per piece (hundreds of thousands on a big torrent), which
                  // froze the GUI thread on every single selection.
                  DetailPeers    { peers:    (win.hasSel && win.detailTab === 1) ? session.selectedPeerList : []
                                   loading:  win.peersTabOpen && session.peersLoading }
                  DetailFiles {
                      files: (win.hasSel && win.detailTab === 2) ? session.selectedFiles : []
                      onRenameFile: function(idx, current) {
                          inputPrompt.openWith(i18n.t("ctx_rename"), i18n.t("ctx_rename_prompt"), current, "",
                              function(t){ if (t.length > 0) session.renameSelectedFile(idx, t) })
                      }
                  }
                  DetailTrackers { trackers: (win.hasSel && win.detailTab === 3) ? session.selectedTrackers : [] }
                  DetailPieces   { pieces:   (win.hasSel && win.detailTab === 4) ? session.selectedPieces   : [] }
                }
            }
        }

        // ================== STATUSBAR ==================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp2

                Text {
                    text: typeof session !== "undefined"
                          ? (i18n.language, i18n.t("status_torrents_active")).arg(session.torrentCount).arg(session.activeCount)
                          : "0 torrents"
                    color: Theme.t4
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                }
                // alt-speed (turtle): a mode toggle lives with the other status
                // signals, not among the toolbar actions
                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: Theme.sp2
                    width: 26; height: 22; radius: 6
                    readonly property bool on: typeof session !== "undefined" && session.altSpeedsActive
                    color: on ? Theme.accentTint : (turtleMa.containsMouse ? Theme.hover : "transparent")
                    IconImg {
                        anchors.centerIn: parent
                        src: "qrc:/icons/turtle.svg"
                        tint: parent.on ? Theme.accentText : Theme.t4
                        s: 15
                    }
                    MouseArea {
                        id: turtleMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (typeof session !== "undefined") session.setAltSpeedsActive(!session.altSpeedsActive)
                    }
                    ToolTip.text: (i18n.language, i18n.t("tb_alt_speed"))
                    ToolTip.visible: turtleMa.containsMouse
                    ToolTip.delay: 400
                }

                // port reachability (UPnP/NAT-PMP + listen heuristic)
                Row {
                    id: portInd
                    Layout.alignment: Qt.AlignVCenter
                    Layout.leftMargin: Theme.sp2
                    spacing: 6
                    property int ps: typeof session !== "undefined" ? session.portStatus : 0
                    Rectangle {
                        anchors.verticalCenter: parent.verticalCenter
                        width: 8; height: 8; radius: 4
                        color: portInd.ps === 1 ? Theme.grn
                             : portInd.ps === 2 ? Theme.amber
                             : portInd.ps === 3 ? Theme.accent : Theme.t4
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (i18n.language, i18n.t("tb_port") + ": " + (portInd.ps === 1 ? i18n.t("port_open")
                             : portInd.ps === 2 ? i18n.t("port_firewalled")
                             : portInd.ps === 3 ? i18n.t("port_closed") : i18n.t("port_checking")))
                        color: Theme.t4
                        font.pixelSize: 12
                        font.family: Theme.fontSans
                    }
                }
                Item { Layout.fillWidth: true }
                Rectangle { Layout.alignment: Qt.AlignVCenter; implicitWidth: 6; implicitHeight: 6; radius: 3; color: Theme.accent }
                Text { text: typeof session !== "undefined" ? session.totalDownSpeed : "0 KB/s"; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono }
                Rectangle { Layout.alignment: Qt.AlignVCenter; implicitWidth: 6; implicitHeight: 6; radius: 3; color: Theme.amber }
                Text { text: typeof session !== "undefined" ? session.totalUpSpeed : "0 KB/s"; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono }
                Text {
                    text: typeof session !== "undefined"
                          ? "·  Total " + session.totalDownloaded + " ↓ · " + session.totalUploaded + " ↑ · Ratio " + session.globalRatio
                          : ""
                    color: Theme.t4
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                }
                Text {
                    visible: typeof session !== "undefined" && session.freeDiskSpace.length > 0
                    text: typeof session !== "undefined"
                          ? "·  " + (i18n.language, i18n.t("status_free_space")).arg(session.freeDiskSpace)
                          : ""
                    color: Theme.t4
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                }
            }
        }
                }
                // ----- page 1: Discover -----
                DiscoverView {
                    Layout.fillWidth: true; Layout.fillHeight: true
                    onOpenSearch: function(q) {
                        navRail.currentIndex = 2
                        searchPage.runQuery(q)   // fills the bar + runs (no ghost search)
                    }
                }
                // ----- page 2: Search -----
                SearchView { id: searchPage; Layout.fillWidth: true; Layout.fillHeight: true }
                // ----- page 3: HUB -----
                HubView {
                    id: hubPage; Layout.fillWidth: true; Layout.fillHeight: true
                    onOpenSearch: function(q) { navRail.currentIndex = 2; searchPage.runQuery(q) }
                }
            }
        }
    }

    // ================== DRAG & DROP (.torrent / magnet) ==================
    DropArea {
        id: dropZone
        anchors.fill: parent
        z: 150
        function accepts(drag) {
            if (drag.hasUrls) {
                for (var i = 0; i < drag.urls.length; ++i)
                    if (drag.urls[i].toString().toLowerCase().endsWith(".torrent")) return true
            }
            if (drag.hasText && drag.text.indexOf("magnet:") === 0) return true
            return false
        }
        onEntered: function(drag) { drag.accepted = accepts(drag) }
        onDropped: function(drop) {
            if (typeof session === "undefined") return
            if (drop.hasUrls) {
                // Queue every dropped .torrent so each gets the preview/choose-folder
                // dialog in turn, instead of only the first.
                var urls = []
                for (var i = 0; i < drop.urls.length; ++i)
                    urls.push(drop.urls[i].toString())
                win.enqueueTorrentUrls(urls)
                drop.accept()
            } else if (drop.hasText && drop.text.indexOf("magnet:") === 0) {
                session.addMagnetUri(drop.text); drop.accept()
            }
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
            if (useDefault) { session.addTorrentFile(u); continue }   // skip the dialog
            var p = session.previewTorrent(u)
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
    }
    RemoveDialog {
        id: removeDlg
        onAccepted: if (typeof session !== "undefined") {
            if (deleteFiles) session.removeSelectedWithFiles(); else session.removeSelected()
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

    // auto-shutdown: cancelable countdown after all downloads finish
    property int shutdownLeft: 0
    Timer {
        id: shutdownTimer; interval: 1000; repeat: true
        onTriggered: {
            win.shutdownLeft--
            if (win.shutdownLeft <= 0) { stop(); shutdownDlg.close(); if (typeof session !== "undefined") session.performShutdown() }
        }
    }
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onAllDownloadsComplete() { win.shutdownLeft = 60; shutdownTimer.restart(); shutdownDlg.open() }
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
            text: i18n.t("shutdown_msg").arg(win.shutdownLeft)
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
        onPageRequested: function(page) { navRail.currentIndex = page }
        // step 0 is the welcome (centered, no spotlight); the rest spotlight the UI
        // bat/pose vary per step so all 3 candidate SVGs (noto/twemoji/openmoji)
        // and poses (perch/hang, left/right) show in one run — for picking one.
        steps: (i18n.language, [
            { page: 0, title: i18n.t("tour_s1_t"), text: i18n.t("tour_s1_d"),
              rectFn: function() { return navRail.itemRect("rail", tourOverlay) } },
            { page: 0, title: i18n.t("tour_s2_t"), text: i18n.t("tour_s2_d"),
              rectFn: function() { return win.rectIn(tbOpen, tourOverlay) } },
            { page: 1, title: i18n.t("tour_s3_t"), text: i18n.t("tour_s3_d"),
              rectFn: function() { return navRail.itemRect(1, tourOverlay) } },
            { page: 2, title: i18n.t("tour_s4_t"), text: i18n.t("tour_s4_d"),
              rectFn: function() { return navRail.itemRect(2, tourOverlay) } },
            { page: 3, title: i18n.t("tour_s5_t"), text: i18n.t("tour_s5_d"),
              rectFn: function() { return navRail.itemRect(3, tourOverlay) } },
            { page: 0, title: i18n.t("tour_s6_t"), text: i18n.t("tour_s6_d"),
              rectFn: function() { return navRail.itemRect("settings", tourOverlay) } }
        ])
    }

    // ================== TOP-LEVEL WINDOWS (lazy) ==================
    // Built on first open via Loader, not at startup — instantiating all of
    // them eagerly (SettingsWindow alone is huge) stalled the UI thread for
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
    Loader { id: settingsWinLoader;  active: false; sourceComponent: SettingsWindow {} }
    Loader { id: shortcutsWinLoader; active: false; sourceComponent: ShortcutsWindow {} }
    Loader { id: statsWinLoader;     active: false; sourceComponent: StatisticsWindow {} }
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

    // Adding a torrent from Search jumps to Downloads and selects it once it lands
    // (the add is async, so retry briefly until the torrent shows up in the model).
    Connections {
        target: typeof search !== "undefined" ? search : null
        ignoreUnknownSignals: true
        function onAddedTorrent(infoHash) {
            navRail.currentIndex = 0
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
    Shortcut { sequence: "Ctrl+F"; onActivated: searchInput.forceActiveFocus() }
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
