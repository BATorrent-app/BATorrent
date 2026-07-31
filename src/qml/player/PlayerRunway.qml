// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Streaming runway: poll stream stats, compute buffered-ahead, detect starve
// (FFmpeg freezes without StalledMedia), and show the one-shot ahead pill.
import QtQuick
import QtMultimedia
import "../theme"

Item {
    id: root
    anchors.fill: parent
    z: 49

    property var mediaPlayer
    property string infoHash: ""
    property int fileIndex: 0
    property bool windowVisible: false
    property var formatHelper
    property real topInset: 0

    property var streamStats: ({})
    property bool aheadShown: false
    property bool starved: false

    readonly property bool stillDownloading: ((streamStats && streamStats.progress) || 0) < 0.999
    readonly property real downloadedToMs: (streamStats && streamStats.buffered > 0
                                            && mediaPlayer && mediaPlayer.duration > 0)
                                           ? streamStats.buffered * mediaPlayer.duration : 0
    readonly property real bufferedAheadMs: mediaPlayer
        ? Math.max(0, downloadedToMs - mediaPlayer.position) : 0
    readonly property bool starvedNow: mediaPlayer
        && mediaPlayer.playbackState === MediaPlayer.PlayingState
        && stillDownloading && bufferedAheadMs < 400

    onStarvedNowChanged: {
        if (starvedNow) starveDebounce.restart()
        else { starveDebounce.stop(); starved = false }
    }

    function reset() { aheadShown = false }

    Timer {
        interval: 1000; repeat: true
        running: root.windowVisible && typeof session !== "undefined"
        triggeredOnStart: true
        onTriggered: {
            root.streamStats = session.streamFileStats(root.infoHash, root.fileIndex)
            if (!root.aheadShown && root.stillDownloading && root.bufferedAheadMs > 1500) {
                root.aheadShown = true
                aheadPill.show()
            }
        }
    }

    Timer {
        id: starveDebounce; interval: 350
        onTriggered: if (root.starvedNow) root.starved = true
    }

    Rectangle {
        id: aheadPill
        anchors.top: parent.top; anchors.left: parent.left
        anchors.topMargin: root.topInset + 14; anchors.leftMargin: 18
        function show() { opacity = 1; apHide.restart() }
        opacity: 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 600; easing.type: Easing.InOutQuad } }
        Timer { id: apHide; interval: 6000; onTriggered: aheadPill.opacity = 0 }
        radius: 8
        color: "#cc0a0a0c"; border.width: 1
        border.color: Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.35)
        implicitWidth: apRow.implicitWidth + 22; implicitHeight: 30
        Row {
            id: apRow; anchors.centerIn: parent; spacing: 7
            Rectangle {
                width: 7; height: 7; radius: 4; color: Theme.grn
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: formatHelper ? formatHelper.fmtAhead(root.bufferedAheadMs) : ""
                color: Theme.grn; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
        }
    }
}
