// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

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
    property alias playerWinLoader: playerWinLoader

    function showWin(loader) {
        loader.active = true
        if (loader.item) { loader.item.show(); loader.item.raise(); loader.item.requestActivate() }
    }

    function showWrapped() {
        wrappedWinLoader.active = true
        if (wrappedWinLoader.item) wrappedWinLoader.item.openFor(new Date().getFullYear())
    }

    function openPlayer(url, title, hash, fileIndex) {
        playerWinLoader.active = true
        var w = playerWinLoader.item
        if (w) { w.show(); w.raise(); w.requestActivate(); w.openMedia(url, title, hash, fileIndex) }
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
    Loader {
        id: playerWinLoader
        active: false
        sourceComponent: PlayerWindow {
            onClosed: Qt.callLater(function() {
                playerWinLoader.active = false
                if (root.hubPage) root.hubPage.refresh()
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
        }
    }
}
