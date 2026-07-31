// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Scrubber row: position · seek bar with hover thumb preview · duration.
// Thumb decoder reads the on-disk file (not the HTTP stream) to avoid ffmpeg
// resync noise from a second stream client.
import QtQuick
import QtQuick.Layouts
import QtMultimedia
import "theme"

RowLayout {
    id: root
    spacing: 12

    property var mediaPlayer
    property var formatHelper
    property bool controlsShown: true
    property string localFile: ""
    property bool stillDownloading: false
    property real downloadedToMs: 0
    property real buffered: 0

    signal localPathRefreshRequested()

    function fmt(ms) { return formatHelper ? formatHelper.fmt(ms) : "" }

    Text {
        text: root.fmt(mediaPlayer ? mediaPlayer.position : 0); color: Theme.t1
        font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontMono
        Layout.minimumWidth: 52; horizontalAlignment: Text.AlignRight
    }
    PlayerSlider {
        id: seek
        Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
        from: 0; to: Math.max(1, mediaPlayer ? mediaPlayer.duration : 1)
        value: mediaPlayer ? mediaPlayer.position : 0
        buffered: root.buffered
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

            MediaPlayer {
                id: thumbPlayer
                source: root.controlsShown && root.localFile.length > 0
                        ? ((Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(root.localFile))
                        : ""
                videoOutput: thumbOut
                onMediaStatusChanged: if (mediaStatus === MediaPlayer.LoadedMedia) {
                    play(); pause()
                    thumbSeek.restart()
                }
            }
            Timer {
                id: thumbSeek
                interval: 60
                onTriggered: if (thumbPlayer.seekable && (seekHover.containsMouse || seek.pressed))
                    thumbPlayer.position = seekHover.targetMs
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
                    text: root.fmt(seekHover.targetMs)
                    color: "#fff"; font.pixelSize: 11; font.family: Theme.fontMono
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
        text: root.fmt(mediaPlayer ? mediaPlayer.duration : 0)
        color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontMono; Layout.minimumWidth: 52
    }
}
