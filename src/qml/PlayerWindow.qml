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
    // standalone, non-transient window — otherwise macOS treats it as an
    // auxiliary window and won't enter the native fullscreen space (menu bar +
    // Dock stay on top).
    flags: Qt.Window
    transientParent: null

    property string streamUrl: ""
    property string mediaTitle: ""
    property string infoHash: ""
    property int fileIndex: 0
    readonly property string resumeKey: "resume_" + infoHash + "_" + fileIndex
    property bool resumed: false
    property bool muted: false
    property bool controlsShown: true
    readonly property bool fullscreen: win.visibility === Window.FullScreen

    // small pill button for the controls bar
    component PChip: Rectangle {
        id: chip
        property string label: ""
        property bool active: false
        signal clicked()
        implicitHeight: 26
        implicitWidth: ct.width + 18
        radius: 7
        color: chip.active ? Theme.accent : (cma.containsMouse ? "#33ffffff" : "#1d1d20")
        Text {
            id: ct; anchors.centerIn: parent; text: chip.label
            color: chip.active ? Theme.accentText : Theme.t1
            font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
        }
        MouseArea { id: cma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: chip.clicked() }
    }

    // slim, themed slider (no default "ball" handle)
    component PSlider: Slider {
        id: sl
        implicitHeight: 16
        background: Rectangle {
            x: sl.leftPadding; y: sl.topPadding + sl.availableHeight / 2 - height / 2
            width: sl.availableWidth; height: 4; radius: 2
            color: "#3a3a42"
            Rectangle { width: sl.visualPosition * parent.width; height: parent.height; radius: 2; color: Theme.accent }
        }
        handle: Rectangle {
            x: sl.leftPadding + sl.visualPosition * (sl.availableWidth - width)
            y: sl.topPadding + sl.availableHeight / 2 - height / 2
            implicitWidth: (sl.pressed || sl.hovered) ? 14 : 11
            implicitHeight: implicitWidth
            radius: implicitWidth / 2
            color: "#ffffff"
            Behavior on implicitWidth { NumberAnimation { duration: 110; easing.type: Easing.OutCubic } }
        }
    }

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
        // remember the runtime so the HUB can draw a "watched %" bar on the poster
        settings.set(resumeKey + "_dur", Math.floor(player.duration))
        // near the end → clear (watched); otherwise remember the position
        if (player.position > player.duration - 15000) settings.set(resumeKey, 0)
        else if (player.position > 5000) settings.set(resumeKey, Math.floor(player.position))
    }

    MediaPlayer {
        id: player
        source: win.streamUrl
        videoOutput: videoOut
        audioOutput: AudioOutput { id: audio; volume: volSlider.value; muted: win.muted }
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
    // X must actually stop playback (the window only hides while the app keeps
    // running in tray) and tear down the player so reopening starts fresh.
    signal closed()
    onClosing: { win.saveResume(); player.stop(); win.closed() }

    Rectangle { anchors.fill: parent; color: "#000000" }

    VideoOutput {
        id: videoOut
        anchors.fill: parent
        // overlay the bar in fullscreen; keep it below the video when windowed
        anchors.bottomMargin: win.fullscreen ? 0 : (bar.visible ? bar.height : 0)
        fillMode: VideoOutput.PreserveAspectFit
    }

    // click toggles play/pause, double-click toggles fullscreen, movement
    // reveals the controls (and hides the cursor once they auto-hide)
    MouseArea {
        anchors.fill: videoOut
        hoverEnabled: true
        cursorShape: (win.fullscreen && !win.controlsShown) ? Qt.BlankCursor : Qt.ArrowCursor
        onPositionChanged: win.showControls()
        onClicked: win.togglePlay()
        onDoubleClicked: win.toggleFullscreen()
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
            onClicked: { win.saveResume(); win.openExternal(); win.close() }
        }
    }

    function openExternal() {
        if (typeof session !== "undefined")
            session.openExternalForHash(win.infoHash, win.fileIndex)
    }
    function togglePlay() {
        player.playbackState === MediaPlayer.PlayingState ? player.pause() : player.play()
    }
    function toggleFullscreen() {
        win.visibility = (win.visibility === Window.FullScreen) ? Window.Windowed : Window.FullScreen
    }
    function seekBy(ms) {
        if (player.seekable) player.position = Math.max(0, Math.min(player.duration, player.position + ms))
    }
    function bumpVolume(d) { volSlider.value = Math.max(0, Math.min(1, volSlider.value + d)) }
    function showControls() { win.controlsShown = true; if (win.fullscreen) idle.restart() }

    // auto-hide the bar after inactivity while in fullscreen
    Timer { id: idle; interval: 3000; onTriggered: if (win.fullscreen) win.controlsShown = false }
    onFullscreenChanged: { win.controlsShown = true; if (win.fullscreen) idle.restart() }

    // track pickers (embedded audio / subtitle streams)
    Menu {
        id: audioMenu
        Repeater {
            model: player.audioTracks.length
            MenuItem {
                required property int index
                text: (i18n.language, i18n.t("player_audio")) + " " + (index + 1)
                checkable: true; checked: player.activeAudioTrack === index
                onTriggered: player.activeAudioTrack = index
            }
        }
    }
    Menu {
        id: subMenu
        MenuItem {
            text: (i18n.language, i18n.t("player_subs_off"))
            checkable: true; checked: player.activeSubtitleTrack < 0
            onTriggered: player.activeSubtitleTrack = -1
        }
        Repeater {
            model: player.subtitleTracks.length
            MenuItem {
                required property int index
                text: (i18n.language, i18n.t("player_subs")) + " " + (index + 1)
                checkable: true; checked: player.activeSubtitleTrack === index
                onTriggered: player.activeSubtitleTrack = index
            }
        }
    }

    // ---- controls bar ----
    Rectangle {
        id: bar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 56
        // top→bottom scrim so controls read over bright video
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#00000000" }
            GradientStop { position: 0.25; color: "#aa0a0a0c" }
            GradientStop { position: 1.0; color: "#f00a0a0c" }
        }
        opacity: (!win.fullscreen || win.controlsShown) ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 200 } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 14; anchors.rightMargin: 14
            spacing: 9

            PChip { Layout.alignment: Qt.AlignVCenter; label: "−" + "10"; onClicked: win.seekBy(-10000) }
            IconImg {
                Layout.alignment: Qt.AlignVCenter
                src: player.playbackState === MediaPlayer.PlayingState ? "qrc:/icons/pause.svg" : "qrc:/icons/play.svg"
                tint: Theme.t1; s: 22
                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: win.togglePlay() }
            }
            PChip { Layout.alignment: Qt.AlignVCenter; label: "+10"; onClicked: win.seekBy(10000) }

            Text { text: win.fmt(player.position); color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontMono }
            PSlider {
                id: seek
                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                from: 0; to: Math.max(1, player.duration)
                value: player.position
                enabled: player.seekable
                onMoved: player.position = value
            }
            Text { text: win.fmt(player.duration); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono }

            PChip {
                Layout.alignment: Qt.AlignVCenter
                visible: player.audioTracks.length > 1
                label: (i18n.language, i18n.t("player_audio"))
                onClicked: audioMenu.popup()
            }
            PChip {
                Layout.alignment: Qt.AlignVCenter
                visible: player.subtitleTracks.length > 0
                active: player.activeSubtitleTrack >= 0
                label: (i18n.language, i18n.t("player_subs"))
                onClicked: subMenu.popup()
            }
            PChip {
                Layout.alignment: Qt.AlignVCenter
                active: win.muted
                label: (i18n.language, i18n.t("player_mute"))
                onClicked: win.muted = !win.muted
            }
            PSlider { id: volSlider; Layout.preferredWidth: 76; Layout.alignment: Qt.AlignVCenter; from: 0; to: 1; value: 0.9 }

            PChip { Layout.alignment: Qt.AlignVCenter; label: (i18n.language, i18n.t("player_external")); onClicked: { win.saveResume(); win.openExternal() } }
            PChip { Layout.alignment: Qt.AlignVCenter; label: "⛶"; onClicked: win.toggleFullscreen() }
        }
        MouseArea {
            id: barHover; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton
            onContainsMouseChanged: if (containsMouse) win.showControls()
            onPositionChanged: win.showControls()
        }
    }

    Component.onCompleted: if (win.streamUrl.length > 0) player.play()

    Shortcut { sequence: "Space";  onActivated: win.togglePlay() }
    Shortcut { sequence: "Right";  onActivated: win.seekBy(10000) }
    Shortcut { sequence: "Left";   onActivated: win.seekBy(-10000) }
    Shortcut { sequence: "Up";     onActivated: win.bumpVolume(0.05) }
    Shortcut { sequence: "Down";   onActivated: win.bumpVolume(-0.05) }
    Shortcut { sequence: "F";      onActivated: win.toggleFullscreen() }
    Shortcut { sequence: "M";      onActivated: win.muted = !win.muted }
    Shortcut { sequence: "Escape"; onActivated: win.visibility === Window.FullScreen ? (win.visibility = Window.Windowed) : win.close() }
}
