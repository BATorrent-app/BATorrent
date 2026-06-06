// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Embedded video player (4.0 step ④). Plays a torrent file streamed from the
// local StreamServer (download-while-watch), with resume per infohash+file and
// an external-player fallback if the codec isn't supported.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import "theme"
import "widgets"

Window {
    id: win
    width: 960; height: 600
    minimumWidth: 560; minimumHeight: 360
    color: "#000000"
    title: win.mediaTitle.length > 0 ? ("BATorrent — " + win.mediaTitle) : "BATorrent"

    property string streamUrl: ""
    property string mediaTitle: ""
    property string infoHash: ""
    property int fileIndex: 0
    readonly property string resumeKey: "resume_" + infoHash + "_" + fileIndex
    property bool resumed: false

    function fmt(ms) {
        if (ms <= 0) return "0:00"
        var s = Math.floor(ms / 1000), h = Math.floor(s / 3600), m = Math.floor((s % 3600) / 60), ss = s % 60
        var p = function(n){ return (n < 10 ? "0" : "") + n }
        return (h > 0 ? h + ":" + p(m) : m) + ":" + p(ss)
    }
    // entry point used by Main.qml when (re)opening the player with new media
    function openMedia(url, title, hash, fileIdx) {
        win.saveResume()
        win.resumed = false
        win.streamUrl = url
        win.mediaTitle = title
        win.infoHash = hash
        win.fileIndex = fileIdx
        player.play()
    }
    function saveResume() {
        if (typeof settings === "undefined" || player.duration <= 0) return
        // near the end → clear (watched); otherwise remember the position
        if (player.position > player.duration - 15000) settings.set(resumeKey, 0)
        else if (player.position > 5000) settings.set(resumeKey, Math.floor(player.position))
    }

    MediaPlayer {
        id: player
        source: win.streamUrl
        videoOutput: videoOut
        audioOutput: AudioOutput { id: audio; volume: volSlider.value }
        onDurationChanged: {
            if (!win.resumed && duration > 0 && typeof settings !== "undefined") {
                win.resumed = true
                var saved = Number(settings.get(win.resumeKey) || 0)
                if (saved > 5000 && saved < duration - 15000) position = saved
            }
        }
    }

    // periodic + lifecycle resume saves
    Timer { interval: 5000; running: player.playbackState === MediaPlayer.PlayingState; repeat: true; onTriggered: win.saveResume() }
    onClosing: win.saveResume()

    Rectangle { anchors.fill: parent; color: "#000000" }

    VideoOutput {
        id: videoOut
        anchors.fill: parent
        anchors.bottomMargin: bar.visible ? bar.height : 0
        fillMode: VideoOutput.PreserveAspectFit
    }

    // click video to toggle play/pause
    MouseArea {
        anchors.fill: videoOut
        onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
        onDoubleClicked: win.visibility = (win.visibility === Window.FullScreen) ? Window.Windowed : Window.FullScreen
    }

    // buffering / error overlay
    ColumnLayout {
        anchors.centerIn: parent
        visible: player.mediaStatus === MediaPlayer.LoadingMedia
                 || player.mediaStatus === MediaPlayer.StalledMedia
                 || player.error !== MediaPlayer.NoError
        spacing: 12
        BusyIndicator { Layout.alignment: Qt.AlignHCenter; running: player.error === MediaPlayer.NoError }
        Text {
            Layout.alignment: Qt.AlignHCenter
            color: "#e8e8ea"; font.pixelSize: 14; font.family: Theme.fontSans
            text: player.error !== MediaPlayer.NoError
                  ? (i18n.language, i18n.t("player_error"))
                  : (i18n.language, i18n.t("player_buffering"))
        }
        BtnFlat {
            Layout.alignment: Qt.AlignHCenter
            visible: player.error !== MediaPlayer.NoError
            primary: true
            text: (i18n.language, i18n.t("player_open_external"))
            onClicked: { win.saveResume(); if (typeof session !== "undefined") session.streamSelected(); win.close() }
        }
    }

    // ---- controls bar ----
    Rectangle {
        id: bar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 56
        color: "#cc101012"
        visible: win.visibility !== Window.FullScreen || barHover.containsMouse

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14; anchors.rightMargin: 14
            spacing: 12

            IconImg {
                Layout.alignment: Qt.AlignVCenter
                src: player.playbackState === MediaPlayer.PlayingState ? "qrc:/icons/pause.svg" : "qrc:/icons/play.svg"
                tint: Theme.t1; s: 20
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                    onClicked: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play() }
            }
            Text { text: win.fmt(player.position); color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono }
            Slider {
                id: seek
                Layout.fillWidth: true
                from: 0; to: Math.max(1, player.duration)
                value: player.position
                enabled: player.seekable
                onMoved: player.position = value
            }
            Text { text: win.fmt(player.duration); color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono }
            IconImg { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/play.svg"; tint: Theme.t3; s: 16; visible: false }
            Slider { id: volSlider; Layout.preferredWidth: 90; from: 0; to: 1; value: 0.9 }
        }
        MouseArea { id: barHover; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
    }

    Component.onCompleted: if (win.streamUrl.length > 0) player.play()
    Keys.onSpacePressed: player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
    Keys.onEscapePressed: win.visibility === Window.FullScreen ? (win.visibility = Window.Windowed) : win.close()
}
