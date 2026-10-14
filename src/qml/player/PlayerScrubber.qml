// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Scrubber row: position · seek bar with hover thumb preview · time left.
// Thumb decoder reads the on-disk file (not the HTTP stream) to avoid ffmpeg
// resync noise from a second stream client.
import QtQuick
import QtQuick.Layouts
import QtMultimedia
import "../theme"

RowLayout {
    id: root
    spacing: 14

    property var mediaPlayer
    property var formatHelper
    property bool controlsShown: true
    property string localFile: ""
    property bool stillDownloading: false
    property real downloadedToMs: 0
    property real buffered: 0
    property real bufferedFrom: 0
    property var chapters: []
    function chapterAt(ms) {
        for (var i = 0; i < chapters.length; i++) {
            var c = chapters[i]
            var end = c.endMs > 0 ? c.endMs : (i + 1 < chapters.length ? chapters[i + 1].startMs : Infinity)
            if (ms >= c.startMs && ms < end) return c.name || ""
        }
        return ""
    }

    signal localPathRefreshRequested()

    function fmt(ms) { return formatHelper ? formatHelper.fmt(ms) : "" }

    Text {
        text: root.fmt(mediaPlayer ? mediaPlayer.position : 0); color: "#ffffff"
        font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
    }
    PlayerSlider {
        id: seek
        Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
        from: 0; to: Math.max(1, mediaPlayer ? mediaPlayer.duration : 1)
        value: mediaPlayer ? mediaPlayer.position : 0
        buffered: root.buffered
        bufferedFrom: root.bufferedFrom
        // plain chapters are notches; the opening and the ending are brackets
        marks: {
            var d = mediaPlayer ? mediaPlayer.duration : 0
            if (d <= 0) return []
            var out = []
            for (var i = 0; i < root.chapters.length; i++) {
                var c = root.chapters[i]
                if (!c.kind && c.startMs > 0 && c.startMs < d) out.push(c.startMs / d)
            }
            return out
        }
        spans: {
            var d = mediaPlayer ? mediaPlayer.duration : 0
            if (d <= 0) return []
            var out = []
            for (var i = 0; i < root.chapters.length; i++) {
                var c = root.chapters[i]
                if (!c.kind || c.startMs >= d) continue
                var end = c.endMs > 0 ? Math.min(c.endMs, d) : d
                out.push({ from: c.startMs / d, to: end / d, open: c.kind !== "credits" })
            }
            return out
        }
        enabled: !!(mediaPlayer && mediaPlayer.seekable)
        onMoved: if (mediaPlayer) mediaPlayer.position = value
        MouseArea {
            id: seekHover
            anchors.fill: parent
            acceptedButtons: Qt.NoButton
            hoverEnabled: true
            readonly property real frac: Math.max(0, Math.min(1, mouseX / Math.max(1, seek.availableWidth)))
            readonly property real targetMs: (seek.pressed ? seek.position : frac) * (mediaPlayer ? mediaPlayer.duration : 0)
            readonly property bool canThumb: !!(mediaPlayer && mediaPlayer.duration > 0
                && (!root.stillDownloading || targetMs <= root.downloadedToMs))

            PlayerBackend {
                id: thumbBackend
                source: root.controlsShown && root.localFile.length > 0
                        ? ((Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(root.localFile))
                        : ""
                videoOutput: thumbOut
                audible: false
            }
            readonly property var thumbPlayer: thumbBackend.player
            Connections {
                target: seekHover.thumbPlayer
                function onMediaStatusChanged() {
                    if (seekHover.thumbPlayer.mediaStatus !== MediaPlayer.LoadedMedia) return
                    seekHover.thumbPlayer.play()
                    seekHover.thumbPlayer.pause()
                    thumbSeek.restart()
                }
            }
            Timer {
                id: thumbSeek
                interval: 60
                onTriggered: if (seekHover.thumbPlayer.seekable && (seekHover.containsMouse || seek.pressed))
                    seekHover.thumbPlayer.position = seekHover.targetMs
            }
            onMouseXChanged: if (containsMouse || seek.pressed) thumbSeek.restart()
            onContainsMouseChanged: if (containsMouse) root.localPathRefreshRequested()

            Rectangle {
                readonly property bool showThumb: seekHover.canThumb
                visible: (seekHover.containsMouse || seek.pressed) && mediaPlayer && mediaPlayer.duration > 0
                width: showThumb ? 172 : ttl.implicitWidth + 14
                height: showThumb ? 122 : 22
                radius: showThumb ? 9 : 5
                color: "#e60a0a0c"; border.color: Theme.hair; border.width: 1
                y: -(height + 8)
                x: Math.max(0, Math.min(seek.width - width,
                    (seek.pressed ? seek.position * seek.width : seekHover.mouseX) - width / 2))
                VideoOutput {
                    id: thumbOut
                    visible: parent.showThumb
                    anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
                    anchors.margins: 3
                    height: 94
                    fillMode: VideoOutput.PreserveAspectCrop
                }
                Text {
                    id: ttl
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.showThumb ? parent.bottom : undefined
                    anchors.bottomMargin: 5
                    anchors.verticalCenter: parent.showThumb ? undefined : parent.verticalCenter
                    readonly property string chapter: root.chapterAt(seekHover.targetMs)
                    text: root.fmt(seekHover.targetMs) + (chapter.length > 0 ? "  ·  " + chapter : "")
                    color: "#fff"; font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
        }
        Rectangle {
            visible: root.stillDownloading && seek.buffered > 0.005 && seek.buffered < 0.995
            width: 2; height: 12; radius: 1
            color: Theme.accent
            anchors.verticalCenter: parent.verticalCenter
            x: seek.leftPadding + seek.buffered * seek.availableWidth - width / 2
        }
    }
    Text {
        text: "−" + root.fmt(mediaPlayer ? Math.max(0, mediaPlayer.duration - mediaPlayer.position) : 0)
        color: Theme.stageT2; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
    }
}
