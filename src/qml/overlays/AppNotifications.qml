// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Dialogs
import "../windows"

// Get&Watch overlay + toast host + one-shot star/crash asks.
Item {
    id: root
    anchors.fill: parent
    // Declared before the page stack in Main, so without its own z the
    // pages would draw on top of it.
    z: 350
    required property var host

    property alias gwOverlay: gwOverlay
    property alias toastHost: toastHost

    // background events (finished, error, kill switch, RSS)
    Connections {
        target: typeof notifications !== "undefined" ? notifications : null
        ignoreUnknownSignals: true
        function onNotify(title, body, level) { host.notifyUser(title, body, level) }
        // a film or a game says it its own way: give them a moment to
        function onFinished(name, infoHash) { root.pendingDone[infoHash] = name; doneTimer.restart() }
        function onVpnDropped() { if (!root.shown()) host.notifyNative(i18n.t("vpn_dropped_title"), i18n.t("vpn_dropped_tray")) }
    }
    property var pendingDone: ({})
    Timer {
        id: doneTimer
        interval: 1500
        onTriggered: {
            for (var h in root.pendingDone) root.ready(h, root.pendingDone[h], "")
            root.pendingDone = ({})
        }
    }
    function shown() { return host.visible && host.visibility !== Window.Minimized && host.visibility !== Window.Hidden }
    // One way out for every notice: off in Settings, nothing; a game or video
    // filling the screen, held for the summary; the window up, the stack;
    // otherwise the system's notification, whose click does the first action.
    function deliver(n) {
        if (!toastHost.wanted(n.category || "")) return
        if (toastHost.hold(n)) return
        if (shown()) { toastHost.push(n); return }
        var a = n.actions || []
        host.notifyNative(n.title, n.body || "", a.length > 0 ? a[0] : null)
    }
    // the episode a finished series download is at, for "Watch S01E01"
    function episodeCode(hash) {
        if (typeof session === "undefined") return ""
        var lib = session.movieLibrary()
        for (var i = 0; i < lib.length; i++) {
            var m = lib[i]
            if (m.infoHash !== hash || !m.isSeries) continue
            var v = m.videos || []
            for (var j = 0; j < v.length; j++) if (v[j].idx === m.fileIndex && v[j].code) return v[j].code
        }
        return ""
    }
    // "Hollow Knight is ready", with what can be done with it
    function ready(hash, name, kind) {
        delete pendingDone[hash]
        var poster = typeof session !== "undefined" ? session.posterForHash(hash) : ""
        var ep = kind === "video" ? episodeCode(hash) : ""
        var open = { id: "open", hash: hash, label: i18n.t("ctx_open_folder") }
        var actions = kind === "video" ? [{ id: "play", hash: hash, label: ep ? i18n.t("notif_watch_ep").arg(ep) : i18n.t("notif_watch") }, open]
                    : kind === "game" ? [{ id: "install", hash: hash, label: i18n.t("hub_gs_install") }, open]
                    : [open]
        deliver({ tone: "ready", category: "ready", title: i18n.t("notif_ready").arg(name),
                  poster: poster ? host.fileUrl(poster) : "", actions: actions })
    }
    // RSS adds come in a burst, once per check: one notice for the lot
    property var rssBatch: []
    Timer {
        id: rssTimer
        interval: 4000
        onTriggered: {
            var b = root.rssBatch
            root.rssBatch = []
            if (b.length === 0) return
            var title = b.length === 1 ? b[0].item : i18n.t("notif_rss_n").arg(b.length).arg(b[0].feed)
            root.deliver({ tone: "ready", category: "rss", title: title, body: i18n.t("notif_rss_body"),
                           actions: [{ id: "rss", label: i18n.t("notif_show") }] })
        }
    }
    Connections {
        target: typeof rss !== "undefined" ? rss : null
        ignoreUnknownSignals: true
        function onAutoDownloaded(feedName, itemTitle) {
            var b = root.rssBatch
            b.push({ feed: feedName, item: itemTitle })
            root.rssBatch = b
            rssTimer.restart()
        }
    }

    // session-originated toasts (stream feedback, etc.)
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onToast(title, body) { host.notifyUser(title, body, 0) }
        // The game is in front: our own card over it, or, when asked for (or
        // a game in exclusive fullscreen hides it), the OS notification.
        function onAchievementUnlocked(game, name, icon, percentLabel, rare, description, unlocked, total) {
            if (typeof settings !== "undefined" && !settings.getBool("notifyAchievements", true)) return
            if ((typeof settings !== "undefined" && settings.getBool("notifyAchievementsSystem"))
                    || (typeof themeBridge !== "undefined" && themeBridge.exclusiveFullscreenInFront()))
                host.notifyNative(i18n.t("toast_achievement").arg(name), description || game)
            else
                achievementToast.push({ name: name, icon: icon, percentLabel: percentLabel, rare: rare,
                                        description: description, unlocked: unlocked, total: total })
        }
        // what only the person can fix, with the fix as the button
        function onNeedsYou(n) {
            var a, title, body
            if (n.reason === "nospace") {
                title = i18n.t("notif_nospace").arg(n.title)
                body = n.disk ? i18n.t("notif_nospace_body").arg(n.need).arg(n.disk).arg(n.diskFree)
                              : i18n.t("notif_nospace_body_none").arg(n.need)
                a = n.disk ? [{ id: "move", hash: n.infoHash, path: n.diskPath, label: i18n.t("notif_move_to").arg(n.disk) },
                              { id: "files", hash: n.infoHash, label: i18n.t("notif_choose_files") }]
                           : [{ id: "freeup", label: i18n.t("search_wontfit_freeup") },
                              { id: "files", hash: n.infoHash, label: i18n.t("notif_choose_files") }]
            } else if (n.reason === "fetching") {
                title = i18n.t("notif_fetching").arg(n.title).arg(n.minutes)
                body = i18n.t("notif_fetching_body")
                a = [{ id: "keep", hash: n.infoHash, label: i18n.t("notif_keep_trying") },
                     { id: "remove", hash: n.infoHash, label: i18n.t("notif_remove") }]
            } else {
                title = i18n.t("notif_missing").arg(n.title)
                body = i18n.t("notif_missing_body")
                a = [{ id: "locate", hash: n.infoHash, label: i18n.t("ov_act_locate") }]
            }
            root.deliver({ tone: "needs", category: "needs", title: title, body: body, poster: n.poster || "", actions: a })
        }
        // a movie/series finished downloading → actionable "Play now" toast
        function onMovieReady(infoHash, name) { root.ready(infoHash, name, "video") }
        function onGameReady(infoHash, name) { root.ready(infoHash, name, "game") }
        function onInstallProgress(hash, percent) {
            if (hash !== gwOverlay.hash) return
            gwOverlay.percent = percent
            if (percent >= 1 && (gwOverlay.phase === "downloading" || gwOverlay.phase === "searching"))
                gwOverlay.phase = "installing"
            else if (gwOverlay.phase === "searching" || gwOverlay.phase === "")
                gwOverlay.phase = "downloading"
        }
        function onInstallFailed(title, reasonKey) {
            var key = reasonKey || "gi_failed"
            gwOverlay.forGame = true
            gwOverlay.fail(i18n.t(key).arg(title))
        }
        function onInstallFolderNeeded(infoHash) {
            gamesFolderDlg.pendingHash = infoHash
            gamesFolderDlg.open()
        }
        function onInstallFinished(infoHash, title) {
            if (gwOverlay.hash === infoHash || gwOverlay.forGame)
                gwOverlay.hide()
        }
    }

    // Asked once, the first time an installer that runs on its own (GOG) has
    // nowhere chosen to go; kept as the "Game install folder" setting.
    FolderDialog {
        id: gamesFolderDlg
        property string pendingHash: ""
        title: (i18n.language, i18n.t("hub_pick_games_folder"))
        onAccepted: {
            if (typeof session === "undefined" || typeof settings === "undefined") return
            settings.set("gameInstallDir", session.urlToLocalPath(selectedFolder.toString()))
            session.installGame(pendingHash)
        }
    }

    // "Preparing to watch / install" overlay for one-click Get & Watch / Get & Install
    GetWatchOverlay {
        id: gwOverlay
        onCanceled: {
            if (typeof debrid !== "undefined" && debrid.busy) debrid.cancelStream()
            else if (phase === "searching") { if (typeof search !== "undefined") search.cancelGetAndWatch() }
            else if (hash !== "" && typeof session !== "undefined") {
                if (forGame) session.cancelInstall(hash)
                else session.cancelWatch(hash)
            }
        }
        onAnotherRequested: if (hash !== "" && typeof session !== "undefined") session.abandonWatch(hash)
    }

    // notices inside the window, above the status bar
    NotificationStack {
        id: toastHost
        onActionTriggered: function (a) { root.act(a) }
        // what waited out a full screen: one summary, wherever the person is
        onSummaryReady: function (title, body) {
            var a = [{ id: "overview", label: i18n.t("notif_show") }]
            if (root.shown()) toastHost.push({ tone: "", title: title, body: body, actions: a })
            else host.notifyNative(title, body, a[0])
        }
    }
    // What a notice's button (or a click on the system's notice) does.
    function act(a) {
        var s = typeof session !== "undefined" ? session : null
        switch (a.id) {
        case "overview": host.showOverview(); break
        case "rss": host.showWin(host.rssWinLoader); break
        case "freeup": host.makeRoomPanel.open(); break
        case "update": host.updateDlg.open(); break
        case "logs": host.showWin(host.logWinLoader); break
        case "crashreport": if (typeof logs !== "undefined") Qt.openUrlExternally(logs.crashReportUrl()); break
        case "oldtunnel": if (typeof vpn !== "undefined" && vpn) vpn.removeLegacyTunnels(); break
        case "star": Qt.openUrlExternally("https://github.com/BATorrent-app/BATorrent"); break
        case "open": if (s && s.selectByInfoHash(a.hash)) s.openSaveFolder(); break
        case "move": if (s && s.selectByInfoHash(a.hash)) { s.moveSelectedStorage(a.path); s.resumeSelected() } break
        case "files": host.showOverview(); host.selectTorrentByHash(a.hash); host.detailTab = 2; break
        case "remove": if (s && s.selectByInfoHash(a.hash)) host.removeDlg.open(); break
        case "locate": host.selectTorrentByHash(a.hash); host.promptSetLocation(); break
        case "keep": if (s) s.keepTrying(a.hash); break
        case "play": if (s) s.playByHash(a.hash); break
        case "install": if (s) { s.installGame(a.hash); host.currentPage = 2 } break
        }
    }

    // one-time star ask: only after the app earned it (14+ days and 10+
    // launches), dismissible, never repeats: converts long-time users, the
    // segment download counts prove we lose
    Timer {
        interval: 8000
        running: true
        repeat: false
        onTriggered: {
            // not over the setup wizard, What's new or the tour: ask again later
            if (host.libraryObscured) { interval = 2000; restart(); return }
            if (typeof settings === "undefined") return
            if (settings.getBool("starAskShown")) return
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
            // it sat on the wizard's Continue button: wait for the screens a
            // first start or an update opens to close
            if (host.libraryObscured) { interval = 2000; restart(); return }
            if (typeof logs !== "undefined" && logs.previousSessionCrashed())
                toastHost.show(i18n.t("crash_toast_title"), i18n.t("crash_toast_body"), 1,
                               "crashreport", i18n.t("crash_toast_action"))
        }
    }

    // A 4.8 beta's in-app VPN left a WireGuard service installed: it starts
    // with Windows and can route the whole machine through a tunnel nothing
    // manages. Once the start-up screens are gone, offer to remove it.
    Timer {
        interval: 6000
        running: Qt.platform.os === "windows"
        repeat: false
        onTriggered: {
            if (host.libraryObscured) { interval = 2000; restart(); return }
            if (typeof vpn === "undefined" || !vpn || vpn.legacyTunnels().length === 0) return
            toastHost.show(i18n.t("vpn_legacy_title"), i18n.t("vpn_legacy_body"), 1,
                           "oldtunnel", i18n.t("vpn_legacy_action"))
        }
    }
    Connections {
        target: typeof vpn !== "undefined" ? vpn : null
        ignoreUnknownSignals: true
        function onLegacyTunnelsRemoved(ok) {
            if (ok) toastHost.show(i18n.t("vpn_legacy_done"), "", 3)
            else toastHost.show(i18n.t("vpn_legacy_failed"), i18n.t("vpn_legacy_failed_body"), 2)
        }
    }

    AchievementToast { id: achievementToast }
}
