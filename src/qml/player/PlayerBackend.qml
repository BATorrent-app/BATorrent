// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtMultimedia
import BATorrent.Media

// The decoder behind a player: Qt's in-process MediaPlayer, or the isolated
// one whose ffmpeg runs in a child process. Both answer to `player` with the
// same API. Read once per window: switching mid-playback would drop the stream.
Item {
    id: root
    property bool isolated: (typeof settings !== "undefined") && settings.getBool("isolatedPlayer", false)
    property url source
    property var videoOutput: null
    property real volume: 1.0
    property bool muted: false
    property bool audible: true
    readonly property var player: isolated ? isolatedPlayer : stockPlayer

    MediaPlayer {
        id: stockPlayer
        source: root.isolated ? "" : root.source
        videoOutput: root.isolated ? null : root.videoOutput
        audioOutput: root.audible ? stockAudio : null
    }
    AudioOutput { id: stockAudio; volume: root.volume; muted: root.muted }

    IsolatedMediaPlayer {
        id: isolatedPlayer
        source: root.isolated ? root.source : ""
        videoOutput: root.isolated ? root.videoOutput : null
        volume: root.volume
        muted: root.muted || !root.audible
    }
}
