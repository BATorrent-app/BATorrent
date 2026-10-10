// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A file still downloading that fails to open is usually early, not broken:
// FFmpeg gave up on a piece that had not arrived. Reopen it a few times before
// telling anyone, and only call a complete file's failure a format problem.
import QtQuick
import QtMultimedia

Item {
    id: retry
    property var mediaPlayer
    property bool stillDownloading: false
    property int maxTries: 8
    property int tries: 0
    signal reloadRequested()

    readonly property bool retrying: pending.running
    readonly property bool failed: mediaPlayer && mediaPlayer.error !== MediaPlayer.NoError && !retrying
    readonly property bool formatProblem: failed && !stillDownloading
                                          && mediaPlayer.error === MediaPlayer.FormatError

    function reset() { tries = 0; pending.stop() }
    function retryNow() { tries = 0; pending.stop(); reloadRequested() }

    Connections {
        target: retry.mediaPlayer
        function onErrorOccurred() {
            if (retry.stillDownloading && retry.tries < retry.maxTries) {
                retry.tries++
                pending.restart()
            }
        }
        function onMediaStatusChanged() {
            if (retry.mediaPlayer.mediaStatus === MediaPlayer.LoadedMedia
                || retry.mediaPlayer.mediaStatus === MediaPlayer.BufferedMedia)
                retry.tries = 0
        }
    }
    Timer {
        id: pending
        interval: 2500
        onTriggered: retry.reloadRequested()
    }
}
