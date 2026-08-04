// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "player"
import "windows"

// Lazy top-level utility windows. Instantiated inside Main; callers use
// showWin(loader) / showWrapped() or the aliased loader ids — no parent walks.
Item {
    id: root

    property var hubPage: null

    property alias rssWinLoader: rssWinLoader
    property alias shortcutsWinLoader: shortcutsWinLoader
    property alias statsWinLoader: statsWinLoader
    property alias wrappedWinLoader: wrappedWinLoader
    property alias removedWinLoader: removedWinLoader
    property alias logWinLoader: logWinLoader
    property alias diagWinLoader: diagWinLoader

    function showWin(loader) {
        loader.active = true
        if (loader.item) { loader.item.show(); loader.item.raise(); loader.item.requestActivate() }
    }

    function showWrapped() {
        wrappedWinLoader.active = true
        if (wrappedWinLoader.item) wrappedWinLoader.item.openFor(new Date().getFullYear())
    }

    // Players are created per video instead of sharing one Loader: a Loader has
    // exactly one item, so starting a second video replaced the first. Sherwan
    // asked to watch more than one at a time, and separate windows are what the
    // OS already knows how to arrange, move between screens and full-screen.
    property var openPlayers: []

    function openPlayer(url, title, hash, fileIndex) {
        // Same file twice: raise the window that already has it rather than
        // opening a duplicate that fights the first one for the same pieces.
        for (var i = 0; i < openPlayers.length; ++i) {
            var e = openPlayers[i]
            if (e && e.streamUrl === url) { e.show(); e.raise(); e.requestActivate(); return }
        }
        var w = playerWindowComp.createObject(null)
        if (!w) return
        openPlayers.push(w)
        w.show(); w.raise(); w.requestActivate()
        w.openMedia(url, title, hash, fileIndex)
    }

    function forgetPlayer(w) {
        var out = []
        for (var i = 0; i < openPlayers.length; ++i)
            if (openPlayers[i] !== w) out.push(openPlayers[i])
        openPlayers = out
        if (root.hubPage) root.hubPage.refresh()
    }

    Loader { id: rssWinLoader;       active: false; sourceComponent: RssWindow {} }
    Loader { id: shortcutsWinLoader; active: false; sourceComponent: ShortcutsWindow {} }
    Loader {
        id: statsWinLoader
        active: false
        sourceComponent: StatisticsWindow { onOpenWrapped: root.showWrapped() }
    }
    Loader { id: wrappedWinLoader;   active: false; sourceComponent: WrappedWindow {} }
    Loader { id: removedWinLoader;   active: false; sourceComponent: RemovedHistoryWindow {} }
    Loader { id: logWinLoader;       active: false; sourceComponent: LogViewerWindow {} }
    Loader { id: diagWinLoader;      active: false; sourceComponent: DiagnosticsWindow {} }
    Component {
        id: playerWindowComp
        PlayerWindow {
            id: playerWin
            // createObject(null) means nothing owns this window, so it must
            // destroy itself — and drop out of openPlayers first, or the array
            // keeps a dangling reference the duplicate check would trip on.
            // The id, not `this`: inside a Qt.callLater callback `this` is not
            // the window, so destroy() would quietly never run and every closed
            // player would leak.
            onClosed: Qt.callLater(function() {
                root.forgetPlayer(playerWin)
                playerWin.destroy()
            })
        }
    }

    // CI (BAT_SMOKE_LOADERS): instantiate deferred windows once so load errors
    // cannot hide behind active: false.
    Timer {
        running: typeof batSmokeLoaders !== "undefined" && batSmokeLoaders
        interval: 300
        onTriggered: {
            rssWinLoader.active = true
            shortcutsWinLoader.active = true
            statsWinLoader.active = true
            wrappedWinLoader.active = true
            removedWinLoader.active = true
            logWinLoader.active = true
            diagWinLoader.active = true
            playerWinLoader.active = true
        }
    }
}
