// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Resume policy + cue pill. Host passes mediaPlayer + identity; cue anchors
// under the title bar via topInset. Windows FFmpeg often drops the first seek
// during buffering: retry re-issues until it lands (bounded).
import QtQuick
import QtMultimedia
import "../theme"

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
            // Back in the middle of it, so no longer watched: left set, Find
            // said "Watched" while the HUB offered to resume the same episode.
            settings.set(resumeKey + "_watched", false)
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

    // Said in words, in the skip chip's style and place: where it picked up,
    // and the way back to the start.
    property bool controlsShown: true
    Rectangle {
        id: cue
        function show() { opacity = 1; hideTimer.restart() }
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: 24
        anchors.bottomMargin: root.controlsShown ? 184 : 40
        Behavior on anchors.bottomMargin { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        opacity: 0
        visible: opacity > 0
        radius: 8
        color: Qt.rgba(20 / 255, 20 / 255, 22 / 255, 0.85)
        border.color: Qt.rgba(1, 1, 1, 0.18); border.width: 1
        implicitWidth: cueRow.implicitWidth + 32; implicitHeight: 40
        Behavior on opacity { NumberAnimation { duration: Theme.durSlower; easing.type: Theme.easeOut } }
        Row {
            id: cueRow
            anchors.centerIn: parent
            spacing: 14
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t("player_resumed_at")).arg(formatHelper ? formatHelper.fmt(root.resumeAtMs) : "")
                color: "#ffffff"; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t("player_from_start"))
                color: startMa.containsMouse ? "#ffffff" : Theme.stageMuted
                font.pixelSize: 13; font.family: Theme.fontSans
                font.underline: startMa.containsMouse
                MouseArea {
                    id: startMa
                    anchors.fill: parent; anchors.margins: -6
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { resumeRetry.stop(); root.resumeAtMs = 0; if (mediaPlayer) mediaPlayer.position = 0; cue.opacity = 0 }
                }
            }
        }
        HoverHandler { id: cueHover; onHoveredChanged: if (hovered) hideTimer.stop(); else hideTimer.restart() }
        Timer { id: hideTimer; interval: 6000; onTriggered: cue.opacity = 0 }
    }
}
