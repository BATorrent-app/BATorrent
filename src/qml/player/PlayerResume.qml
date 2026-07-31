// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Resume policy + cue pill. Host passes mediaPlayer + identity; cue anchors
// under the title bar via topInset. Windows FFmpeg often drops the first seek
// during buffering — retry re-issues until it lands (bounded).
import QtQuick
import QtMultimedia
import "theme"

Item {
    id: root
    anchors.fill: parent
    z: 50

    property var mediaPlayer
    property string infoHash: ""
    property int fileIndex: 0
    property var formatHelper
    property real topInset: 0

    readonly property string resumeKey: "resume_" + infoHash + "_" + fileIndex
    property int resumeAtMs: 0
    property int pendingResumeMs: -1
    property int resumeTries: 0

    function tryApply() {
        if (!mediaPlayer) return
        if (root.pendingResumeMs > 5000 && mediaPlayer.duration > 0 && mediaPlayer.seekable
            && root.pendingResumeMs < mediaPlayer.duration - 15000) {
            root.resumeAtMs = root.pendingResumeMs
            root.pendingResumeMs = -1
            root.resumeTries = 0
            mediaPlayer.position = root.resumeAtMs
            cue.show()
            resumeRetry.restart()
        }
    }

    function save() {
        if (typeof settings === "undefined" || !mediaPlayer || mediaPlayer.duration <= 0) return
        settings.set(resumeKey + "_dur", Math.floor(mediaPlayer.duration))
        settings.set(resumeKey + "_at", Date.now())
        if (mediaPlayer.position > mediaPlayer.duration - 15000) {
            settings.set(resumeKey, 0)
            settings.set(resumeKey + "_watched", true)
        } else if (mediaPlayer.position > 5000) {
            settings.set(resumeKey, Math.floor(mediaPlayer.position))
        }
    }

    function prepareOpen() {
        root.pendingResumeMs = (typeof settings !== "undefined")
            ? Number(settings.get(root.resumeKey) || 0) : 0
        root.resumeAtMs = 0
        root.resumeTries = 0
        resumeRetry.stop()
    }

    function stopRetry() { resumeRetry.stop() }

    Timer {
        id: resumeRetry
        interval: 300; repeat: true
        onTriggered: {
            if (!mediaPlayer) { stop(); return }
            if (root.resumeAtMs <= 5000) { stop(); return }
            if (Math.abs(mediaPlayer.position - root.resumeAtMs) <= 4000) { stop(); return }
            if (root.resumeTries >= 8) { stop(); return }
            root.resumeTries++
            if (mediaPlayer.seekable) mediaPlayer.position = root.resumeAtMs
        }
    }

    Timer {
        interval: 5000
        running: mediaPlayer && mediaPlayer.playbackState === MediaPlayer.PlayingState
        repeat: true
        onTriggered: root.save()
    }

    Rectangle {
        id: cue
        function show() { opacity = 1; hideTimer.restart() }
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: root.topInset + 16
        opacity: 0
        visible: opacity > 0
        radius: 999
        color: "#cc000000"
        border.color: Theme.accent; border.width: 1
        implicitWidth: rcLbl.implicitWidth + 26; implicitHeight: 30
        Behavior on opacity { NumberAnimation { duration: 280; easing.type: Easing.OutCubic } }
        Text {
            id: rcLbl
            anchors.centerIn: parent
            text: "↩  " + (formatHelper ? formatHelper.fmt(root.resumeAtMs) : "")
            color: "white"; font.pixelSize: 13; font.family: Theme.fontMono
        }
        Timer { id: hideTimer; interval: 3500; onTriggered: cue.opacity = 0 }
    }
}
