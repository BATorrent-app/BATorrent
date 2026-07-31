// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

// Player / debrid / search → overlay + window wiring. Keeps Main free of the
// media-flow Connections clutter; host owns page jumps and notifyUser.
Item {
    required property var host
    required property var gwOverlay
    required property var windowLoaders

    Connections {
        target: session
        function onOpenPlayer(url, title, hash, fileIndex) {
            gwOverlay.hide()
            windowLoaders.openPlayer(url, title, hash, fileIndex)
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
            windowLoaders.openPlayer(url, name, "debrid", 0)
        }
        function onErrorOccurred(msg) {
            gwOverlay.hide()
            host.notifyUser(debrid.providerName, msg, 2)
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
            host.currentPage = 0
            selectAddedTimer.hash = infoHash || ""
            selectAddedTimer.tries = 0
            selectAddedTimer.restart()
        }
        // Get & Watch / Get & Install → preparing overlay
        function onWatchSearching(title) {
            gwOverlay.forGame = (typeof search !== "undefined" && search.getFlowType === "game")
            gwOverlay.show("searching", title)
        }
        function onWatchNoRelease(title) {
            var key = gwOverlay.forGame ? "gi_no_release" : "gw_no_release"
            gwOverlay.fail(i18n.t(key).arg(title))
        }
        function onPrepareAndWatch(infoHash, title) {
            gwOverlay.forGame = false
            gwOverlay.hash = infoHash
            gwOverlay.phase = "buffering"
            if (typeof session !== "undefined") session.watchWhenReady(infoHash, title)
        }
        function onPrepareAndInstall(infoHash, title) {
            gwOverlay.forGame = true
            gwOverlay.hash = infoHash
            gwOverlay.title = title
            gwOverlay.phase = "downloading"
            gwOverlay.percent = 0
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
