// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

// Get&Watch overlay + toast host + one-shot star/crash asks.
Item {
    id: root
    anchors.fill: parent
    required property var host

    property alias gwOverlay: gwOverlay
    property alias toastHost: toastHost

    // background events (finished, error, kill switch, RSS)
    Connections {
        target: typeof notifications !== "undefined" ? notifications : null
        ignoreUnknownSignals: true
        function onNotify(title, body, level) { host.notifyUser(title, body, level) }
    }

    // session-originated toasts (stream feedback, etc.)
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onToast(title, body) { host.notifyUser(title, body, 0) }
        // a movie/series finished downloading → actionable "Play now" toast
        function onMovieReady(infoHash, name) {
            if (host.visible && host.visibility !== Window.Minimized && host.visibility !== Window.Hidden)
                toastHost.show(i18n.t("toast_movie_ready"), name, 3, "play:" + infoHash, i18n.t("toast_play_now"))
            else
                host.notifyUser(i18n.t("toast_movie_ready"), name, 3)
        }
        function onGameReady(infoHash, name) {
            if (host.visible && host.visibility !== Window.Minimized && host.visibility !== Window.Hidden)
                toastHost.show(i18n.t("toast_game_ready"), name, 3, "install:" + infoHash, i18n.t("hub_gs_install"))
            else
                host.notifyUser(i18n.t("toast_game_ready"), name, 3)
        }
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
        function onInstallFinished(infoHash, title) {
            if (gwOverlay.hash === infoHash || gwOverlay.forGame)
                gwOverlay.hide()
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
    }

    // custom toast cards, pinned to the screen's bottom-right (native-like)
    ToastOverlay {
        id: toastHost
        onActionTriggered: function(actionId) {
            if (actionId === "logs") host.showWin(host.logWinLoader)
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

}
