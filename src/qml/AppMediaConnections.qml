// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

// Player / debrid / search → overlay + window wiring. Keeps Main free of the
// media-flow Connections clutter; host owns page jumps and notifyUser.
Item {
    id: root
    required property var host
    // Names intentionally differ from Main ids/aliases: `gwOverlay: gwOverlay`
    // self-shadows to undefined and aborts openPlayer before the window opens.
    required property var watchOverlay
    required property var loaders

    Connections {
        target: session
        function onOpenPlayer(url, title, hash, fileIndex) {
            if (watchOverlay) watchOverlay.hide()
            loaders.openPlayer(url, title, hash, fileIndex)
        }
        function onMediaQuarantined(hash) {
            if (watchOverlay && watchOverlay.hash === hash) watchOverlay.hide()
            loaders.closePlayersFor(hash)
        }
        function onWatchProgress(hash, percent) {
            if (watchOverlay && watchOverlay.phase === "buffering" && hash === watchOverlay.hash)
                watchOverlay.percent = percent
        }
        function onWatchHealth(hash, downBps, peers, waited) {
            if (!watchOverlay || hash !== watchOverlay.hash) return
            watchOverlay.downBps = downBps
            watchOverlay.peers = peers
            watchOverlay.waited = waited
        }
        function onWatchFailed(title) {
            // a waiting player heard it by hash (waitState noSource) and says so itself
            if (loaders.anyWaiting()) return
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

    // Adding from Search stays in Search (the next episode or another release is
    // usually the next click); the torrent is selected for when Downloads opens.
    // The add is async, so retry briefly until it shows up in the model.
    Connections {
        target: typeof search !== "undefined" ? search : null
        ignoreUnknownSignals: true
        function onAddedTorrent(infoHash) {
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
        function onPickFileWhenReady(infoHash, target) {
            if (typeof session !== "undefined") session.pickFileWhenReady(infoHash, target)
        }
        // ▶ on a release: the player opens now and the wait happens inside it
        function onPrepareAndWatch(infoHash, title, target) {
            if (watchOverlay) watchOverlay.hide()
            loaders.openWaiting(root.waitFor(infoHash, title, target || ({})))
            if (typeof session !== "undefined") session.watchWhenReady(infoHash, title, target || ({}))
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
    // What the waiting player shows, from the release list it was picked in:
    // the work's title and art, the release, and the others it could switch to.
    function waitFor(infoHash, title, target) {
        var api = typeof search !== "undefined" ? search : null
        var rows = api ? api.results : []
        var h = String(infoHash || "").toLowerCase()
        var row = null
        for (var i = 0; i < rows.length; i++)
            if (String(rows[i].coverHash || "").toLowerCase() === h) { row = rows[i]; break }
        var season = target.season !== undefined ? target.season : (row ? row.season : -1)
        var episode = target.episode !== undefined ? target.episode : (row && row.episode > 0 ? row.episode : -1)
        function label(r) {
            var q = [r.quality || "", r.source || ""].filter(function (x) { return x.length > 0 }).join(" ")
            return q
        }
        // the episode's own still and name, from the season the release was picked in
        var ep = episode > 0 && loaders.searchPage ? loaders.searchPage.episodeOf(season, episode) : null
        var cands = []
        for (var j = 0; j < rows.length; j++) {
            var r = rows[j]
            if (String(r.coverHash || "").toLowerCase() === h || r.isTitle) continue
            if (episode > 0 && !(r.season === season && (r.episode === episode || !(r.episode > 0)))) continue
            cands.push({ idx: j, quality: r.quality || "", label: label(r), seeds: r.seedsN || 0 })
        }
        return {
            hash: infoHash,
            title: (api && api.workTitle) || title,
            subtitle: episode > 0 ? i18n.t("wait_ep_code").arg(season).arg(episode) + (ep && ep.name ? " · " + ep.name : "")
                                  : ((api && api.workYear) || ""),
            art: (ep && ep.still) || (api ? (api.workBackdrop || api.workPoster || "") : ""),
            release: row ? [label(row), row.sizeStr || ""].filter(function (x) { return x.length > 0 }).join(" · ") : "",
            quality: row ? (row.quality || "") : "",
            season: season, episode: episode,
            candidates: cands
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
