// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

// Player / debrid / search → overlay + window wiring. Keeps Main free of the
// media-flow Connections clutter; host owns page jumps and notifyUser.
Item {
    required property var host
    // Names intentionally differ from Main ids/aliases — `gwOverlay: gwOverlay`
    // self-shadows to undefined and aborts openPlayer before the window opens.
    required property var watchOverlay
    required property var loaders

    Connections {
        target: session
        function onOpenPlayer(url, title, hash, fileIndex) {
            if (watchOverlay) watchOverlay.hide()
            loaders.openPlayer(url, title, hash, fileIndex)
        }
        function onWatchProgress(hash, percent) {
            if (watchOverlay && watchOverlay.phase === "buffering" && hash === watchOverlay.hash)
                watchOverlay.percent = percent
        }
        function onWatchFailed(title) {
            if (watchOverlay) watchOverlay.fail(i18n.t("gw_failed").arg(title))
        }
    }

    // Debrid: magnet → provider cache → direct link → built-in player.
    Connections {
        target: typeof debrid !== "undefined" ? debrid : null
        ignoreUnknownSignals: true
        function onStreamReady(url, name) {
            if (watchOverlay) watchOverlay.hide()
            loaders.openPlayer(url, name, "debrid", 0)
        }
        function onErrorOccurred(msg) {
            if (watchOverlay) watchOverlay.hide()
            host.notifyUser(debrid.providerName, msg, 2)
        }
        function onBusyChanged() {
            if (!watchOverlay) return
            if (debrid.busy) watchOverlay.show("buffering", debrid.providerName)
            else if (watchOverlay.phase === "buffering") watchOverlay.hide()
        }
        function onStatusChanged() {
            if (!watchOverlay || !debrid.busy) return
            if (debrid.status !== "") watchOverlay.title = debrid.status
            watchOverlay.percent = debrid.progress / 100
        }
    }

    // Adding a torrent from Search jumps to Downloads and selects it once it lands
    // (the add is async, so retry briefly until the torrent shows up in the model).
    Connections {
        target: typeof search !== "undefined" ? search : null
        ignoreUnknownSignals: true
        function onAddedTorrent(infoHash) {
            host.currentPage = 0
            selectAddedTimer.hash = infoHash || ""
            selectAddedTimer.tries = 0
            selectAddedTimer.restart()
        }
        // Get & Watch / Get & Install → preparing overlay
        function onWatchSearching(title) {
            if (!watchOverlay) return
            watchOverlay.forGame = (typeof search !== "undefined" && search.getFlowType === "game")
            watchOverlay.show("searching", title)
        }
        function onWatchNoRelease(title) {
            if (!watchOverlay) return
            var key = watchOverlay.forGame ? "gi_no_release" : "gw_no_release"
            watchOverlay.fail(i18n.t(key).arg(title))
        }
        function onPrepareAndWatch(infoHash, title) {
            if (!watchOverlay) return
            watchOverlay.forGame = false
            watchOverlay.hash = infoHash
            watchOverlay.phase = "buffering"
            if (typeof session !== "undefined") session.watchWhenReady(infoHash, title)
        }
        function onPrepareAndInstall(infoHash, title) {
            if (!watchOverlay) return
            watchOverlay.forGame = true
            watchOverlay.hash = infoHash
            watchOverlay.title = title
            watchOverlay.phase = "downloading"
            watchOverlay.percent = 0
            if (typeof session !== "undefined") session.installWhenReady(infoHash, title)
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
}
