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
    // "Hollow Knight is ready", with what can be done with it
    function ready(hash, name, kind) {
        delete pendingDone[hash]
        var poster = typeof session !== "undefined" ? session.posterForHash(hash) : ""
        var title = i18n.t("notif_ready").arg(name)
        if (!shown()) { host.notifyUser(title, "", 3); return }
        var actions = kind === "video" ? [{ id: "play:" + hash, label: i18n.t("notif_watch") }, { id: "open:" + hash, label: i18n.t("ctx_open_folder") }]
                    : kind === "game" ? [{ id: "install:" + hash, label: i18n.t("hub_gs_install") }, { id: "open:" + hash, label: i18n.t("ctx_open_folder") }]
                    : [{ id: "open:" + hash, label: i18n.t("ctx_open_folder") }]
        toastHost.push({ tone: "ready", category: "ready", title: title, poster: poster ? host.fileUrl(poster) : "", actions: actions })
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
            if (!root.shown()) { host.notifyUser(title, i18n.t("notif_rss_body"), 0); return }
            toastHost.push({ tone: "ready", category: "rss", title: title, body: i18n.t("notif_rss_body"),
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
                a = n.disk ? [{ id: "move:" + n.infoHash + "|" + n.diskPath, label: i18n.t("notif_move_to").arg(n.disk) },
                              { id: "files:" + n.infoHash, label: i18n.t("notif_choose_files") }]
                           : [{ id: "freeup", label: i18n.t("search_wontfit_freeup") },
                              { id: "files:" + n.infoHash, label: i18n.t("notif_choose_files") }]
            } else if (n.reason === "fetching") {
                title = i18n.t("notif_fetching").arg(n.title).arg(n.minutes)
                body = i18n.t("notif_fetching_body")
                a = [{ id: "keep:" + n.infoHash, label: i18n.t("notif_keep_trying") }, { id: "remove:" + n.infoHash, label: i18n.t("notif_remove") }]
            } else {
                title = i18n.t("notif_missing").arg(n.title)
                body = i18n.t("notif_missing_body")
                a = [{ id: "locate:" + n.infoHash, label: i18n.t("ov_act_locate") }]
            }
            if (!root.shown()) { host.notifyUser(title, body, 1); return }
            toastHost.push({ tone: "needs", category: "needs", title: title, body: body, poster: n.poster || "", actions: a })
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
        onActionTriggered: function(actionId) {
            var hash = actionId.indexOf(":") > 0 ? actionId.substring(actionId.indexOf(":") + 1) : ""
            if (actionId === "overview") host.showOverview()
            else if (actionId === "rss") host.showWin(host.rssWinLoader)
            else if (actionId === "freeup") host.makeRoomPanel.open()
            else if (actionId === "update") host.updateDlg.open()
            else if (actionId.indexOf("open:") === 0 && typeof session !== "undefined") { if (session.selectByInfoHash(hash)) session.openSaveFolder() }
            else if (actionId.indexOf("move:") === 0 && typeof session !== "undefined") {
                var parts = hash.split("|")
                if (session.selectByInfoHash(parts[0])) { session.moveSelectedStorage(parts[1]); session.resumeSelected() }
            }
            else if (actionId.indexOf("files:") === 0) { host.showOverview(); host.selectTorrentByHash(hash); host.detailTab = 2 }
            else if (actionId.indexOf("remove:") === 0 && typeof session !== "undefined") { if (session.selectByInfoHash(hash)) host.removeDlg.open() }
            else if (actionId.indexOf("locate:") === 0) { host.selectTorrentByHash(hash); host.promptSetLocation() }
            else if (actionId.indexOf("keep:") === 0) {}
            else if (actionId === "logs") host.showWin(host.logWinLoader)
            else if (actionId === "crashreport" && typeof logs !== "undefined")
                Qt.openUrlExternally(logs.crashReportUrl())
            else if (actionId === "star")
                Qt.openUrlExternally("https://github.com/BATorrent-app/BATorrent")
            else if (actionId.indexOf("play:") === 0 && typeof session !== "undefined")
                session.playByHash(actionId.substring(5))
            else if (actionId.indexOf("install:") === 0 && typeof session !== "undefined") {
                session.installGame(actionId.substring(8))
                host.currentPage = 2
            }
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
            if (typeof logs !== "undefined" && logs.previousSessionCrashed())
                toastHost.show(i18n.t("crash_toast_title"), i18n.t("crash_toast_body"), 1,
                               "crashreport", i18n.t("crash_toast_action"))
        }
    }

    AchievementToast { id: achievementToast }
}
