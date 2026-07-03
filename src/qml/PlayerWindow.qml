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
import QtQuick.Dialogs
import "theme"
import "widgets"

Window {
    id: win
    width: 1120; height: 720
    minimumWidth: 560; minimumHeight: 360
    color: "#000000"
    title: Theme.unifiedChrome ? "" : (win.mediaTitle.length > 0 ? ("BATorrent — " + win.mediaTitle) : "BATorrent")
    // standalone, non-transient window — otherwise macOS treats it as an
    // auxiliary window and won't enter the native fullscreen space (menu bar +
    // Dock stay on top). Expanded client area (macOS) lets the title band with
    // the quality/audio badges share the strip with the traffic lights.
    readonly property int baseFlags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    flags: baseFlags
    transientParent: null

    property string streamUrl: ""
    property string mediaTitle: ""
    property string mediaFileName: ""
    readonly property string mediaQuality: {
        var n = mediaFileName.toLowerCase()
        if (/2160p|\buhd\b|\b4k\b/.test(n)) return "4K"
        if (/1080p/.test(n)) return "1080p"
        if (/720p/.test(n))  return "720p"
        if (/480p/.test(n))  return "480p"
        return ""
    }
    readonly property string mediaAudio: {
        var n = mediaFileName.toLowerCase()
        if (/7\.1/.test(n)) return "7.1"
        if (/5\.1/.test(n)) return "5.1"
        if (/atmos/.test(n)) return "Atmos"
        if (/\b2\.0\b|stereo|aac2/.test(n)) return "2.0"
        return ""
    }
    property string infoHash: ""
    property int fileIndex: 0
    readonly property string resumeKey: "resume_" + infoHash + "_" + fileIndex
    property bool resumed: false
    property int resumeAtMs: 0
    property int pendingResumeMs: -1
    property int resumeTries: 0
    function tryApplyResume() {
        if (win.pendingResumeMs > 5000 && player.duration > 0 && player.seekable
            && win.pendingResumeMs < player.duration - 15000) {
            win.resumeAtMs = win.pendingResumeMs
            win.pendingResumeMs = -1
            win.resumeTries = 0
            player.position = win.resumeAtMs
            resumeCue.show()
            resumeRetry.restart()
        }
    }
    // Windows' FFmpeg backend often discards a seek issued during initial buffering,
    // snapping the position back to 0 once playback actually starts. Re-issue the
    // seek until it lands (bounded), so resume isn't silently lost on Windows.
    Timer {
        id: resumeRetry
        interval: 300; repeat: true
        onTriggered: {
            if (win.resumeAtMs <= 5000) { stop(); return }
            if (Math.abs(player.position - win.resumeAtMs) <= 4000) { stop(); return }   // landed
            if (win.resumeTries >= 8) { stop(); return }                                  // give up after ~2.4s
            win.resumeTries++
            if (player.seekable) player.position = win.resumeAtMs
        }
    }
    property bool muted: false
    property real volume: 0.9
    property bool controlsShown: true

    // external subtitles (sidecar .srt/.vtt) — rendered as a synced overlay,
    // independent of QtMultimedia's embedded-track support
    property var extCues: []
    property int extCueIdx: -1
    property string extCueText: ""
    property string extSubName: ""
    property int extSubOffset: 0          // ms; positive shows subtitles later
    readonly property bool extSubsActive: extCues.length > 0

    // subtitle styling (read from settings on each open)
    property real subScale: 1.0
    property string subColor: "#ffffff"
    property real subBgOpacity: 0.0
    function loadSubStyle() {
        if (typeof settings === "undefined") return
        subScale = Math.max(0.5, Number(settings.get("subFontScale") || 100) / 100)
        var colors = ["#ffffff", "#ffe24a", "#7fdfff", "#9cff9c"]   // White / Yellow / Cyan / Green
        var ci = Number(settings.get("subColor") || 0)
        subColor = colors[(ci >= 0 && ci < colors.length) ? ci : 0]
        subBgOpacity = Math.max(0, Math.min(1, Number(settings.get("subBgOpacity") || 0) / 100))
    }

    // remember the chosen audio/subtitle track per torrent
    property bool tracksRestored: false
    function restoreTracks() {
        if (win.tracksRestored || typeof settings === "undefined") return
        if (player.audioTracks.length === 0 && player.subtitleTracks.length === 0) return
        win.tracksRestored = true
        var at = settings.get("audioTrack_" + win.infoHash)
        if (at !== undefined && at !== "" && Number(at) >= 0 && Number(at) < player.audioTracks.length)
            player.activeAudioTrack = Number(at)
        var st = settings.get("subTrack_" + win.infoHash)
        if (st !== undefined && st !== "" && Number(st) < player.subtitleTracks.length)
            player.activeSubtitleTrack = Number(st)
    }

    // picture-in-picture: shrink to a small always-on-top corner window
    property bool pipMode: false
    property rect savedGeom: Qt.rect(0, 0, 0, 0)
    function togglePip() {
        if (!win.pipMode) {
            win.savedGeom = Qt.rect(win.x, win.y, win.width, win.height)
            if (win.visibility === Window.FullScreen) win.visibility = Window.Windowed
            win.flags = Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
            var scr = win.screen
            win.width = 400; win.height = 225
            win.x = scr.virtualX + scr.width - 416
            win.y = scr.virtualY + scr.height - 241
            win.pipMode = true
        } else {
            win.flags = win.baseFlags
            win.width = win.savedGeom.width > 0 ? win.savedGeom.width : 1120
            win.height = win.savedGeom.height > 0 ? win.savedGeom.height : 720
            win.x = win.savedGeom.x; win.y = win.savedGeom.y
            win.pipMode = false
        }
    }

    function loadExternalSubs(path) {
        var cues = (typeof session !== "undefined") ? session.loadSubtitleFile(path) : []
        if (!cues || cues.length === 0) return false
        extCues = cues
        extCueIdx = -1
        extCueText = ""
        extSubName = String(path).split("/").pop()
        player.activeSubtitleTrack = -1
        return true
    }
    function clearExternalSubs() { extCues = []; extCueIdx = -1; extCueText = ""; extSubName = ""; extSubOffset = 0 }
    function bumpSubOffset(ms) {
        extSubOffset += ms
        updateCue(player.position)
    }
    function updateCue(rawPos) {
        if (extCues.length === 0) { extCueText = ""; return }
        var pos = rawPos - extSubOffset
        var i = extCueIdx
        if (i >= 0 && i < extCues.length) {
            var c = extCues[i]
            if (pos >= c.start && pos <= c.end) { extCueText = c.text; return }
            if (pos > c.end && i + 1 < extCues.length && pos < extCues[i + 1].start) { extCueText = ""; return }
        }
        // seek or first sample: binary-search the last cue starting at/before pos
        var lo = 0, hi = extCues.length - 1, found = -1
        while (lo <= hi) {
            var mid = (lo + hi) >> 1
            if (extCues[mid].start <= pos) { found = mid; lo = mid + 1 } else hi = mid - 1
        }
        extCueIdx = found
        extCueText = (found >= 0 && pos <= extCues[found].end) ? extCues[found].text : ""
    }
    readonly property bool fullscreen: win.visibility === Window.FullScreen

    // slim, themed slider (no default "ball" handle)
    component PSlider: Slider {
        id: sl
        property real buffered: 0   // 0..1 downloaded-from-start, drawn dim behind the fill
        implicitHeight: 16
        background: Rectangle {
            x: sl.leftPadding; y: sl.topPadding + sl.availableHeight / 2 - height / 2
            width: sl.availableWidth; height: 6; radius: 3
            color: "#1fffffff"
            // downloaded layer (what's safe to seek to)
            Rectangle { width: Math.max(0, Math.min(1, sl.buffered)) * parent.width; height: parent.height; radius: 3; color: "#80ffffff" }
            // playback layer (what you're watching)
            Rectangle { width: sl.visualPosition * parent.width; height: parent.height; radius: 3; color: Theme.accent }
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
    function fmtBytes(b) {
        if (!b || b <= 0) return "0 MB"
        var u = ["KB", "MB", "GB", "TB"], i = -1
        do { b /= 1024; i++ } while (b >= 1024 && i < u.length - 1)
        return b.toFixed(b < 10 ? 1 : 0) + " " + u[i]
    }

    // download stats for the buffer bar + badge, polled while the player is open
    property var streamStats: ({})
    property bool aheadShown: false
    Timer {
        interval: 1000; repeat: true
        running: win.visible && typeof session !== "undefined"
        triggeredOnStart: true
        onTriggered: {
            win.streamStats = session.streamFileStats(win.infoHash, win.fileIndex)
            if (!win.aheadShown && win.stillDownloading && win.bufferedAheadMs > 1500) {
                win.aheadShown = true
                aheadPill.show()
            }
        }
    }
    // streaming runway: how far the contiguous download reaches, and how much
    // playback time is buffered past the playhead.
    readonly property bool stillDownloading: ((win.streamStats && win.streamStats.progress) || 0) < 0.999
    readonly property real downloadedToMs: (win.streamStats && win.streamStats.buffered > 0 && player.duration > 0)
                                           ? win.streamStats.buffered * player.duration : 0
    readonly property real bufferedAheadMs: Math.max(0, downloadedToMs - player.position)
    function fmtAhead(ms) {
        var s = Math.floor(ms / 1000)
        if (s >= 3600) return Math.floor(s / 3600) + "h " + Math.floor((s % 3600) / 60) + "m buffered ahead"
        if (s >= 60)   return Math.floor(s / 60) + " min buffered ahead"
        return s + "s buffered ahead"
    }
    // entry point used by Main.qml when (re)opening the player with new media
    property int nextIdx: -1     // file index of the next episode, or -1
    function maybePlayNext() {
        if (typeof session === "undefined" || win.nextIdx < 0) return
        if (typeof settings !== "undefined" && settings.get("autoplayNext") === false) return  // default on
        session.playFile(win.infoHash, win.nextIdx)
    }
    function openMedia(url, title, hash, fileIdx) {
        win.saveResume()
        win.resumed = false
        win.streamUrl = url
        win.mediaTitle = title
        win.infoHash = hash
        win.fileIndex = fileIdx
        win.mediaFileName = (typeof session !== "undefined") ? session.streamFileName(hash, fileIdx) : ""
        win.pendingResumeMs = (typeof settings !== "undefined") ? Number(settings.get(win.resumeKey) || 0) : 0
        win.resumeAtMs = 0
        win.resumeTries = 0
        resumeRetry.stop()
        win.aheadShown = false
        win.nextIdx = (typeof session !== "undefined") ? session.nextEpisode(hash, fileIdx) : -1
        win.tracksRestored = false
        win.loadSubStyle()
        win.clearExternalSubs()
        if (typeof session !== "undefined") {
            var sc = session.findSidecarSubtitle(hash, fileIdx)
            if (sc.length > 0) win.loadExternalSubs(sc)
        }
        player.play()
    }
    function saveResume() {
        if (typeof settings === "undefined" || player.duration <= 0) return
        // remember the runtime so the HUB can draw a "watched %" bar on the poster
        settings.set(resumeKey + "_dur", Math.floor(player.duration))
        settings.set(resumeKey + "_at", Date.now())   // recency for HUB "Continue watching"
        // near the end → clear (watched); otherwise remember the position
        if (player.position > player.duration - 15000) {
            settings.set(resumeKey, 0)
            settings.set(resumeKey + "_watched", true)
        } else if (player.position > 5000) settings.set(resumeKey, Math.floor(player.position))
    }

    MediaPlayer {
        id: player
        source: win.streamUrl
        videoOutput: videoOut
        audioOutput: AudioOutput { id: audio; volume: win.volume; muted: win.muted }
        onPositionChanged: win.updateCue(position)
        onDurationChanged: win.tryApplyResume()
        onSeekableChanged: win.tryApplyResume()
        onPlaybackStateChanged: if (playbackState === MediaPlayer.PlayingState) win.tryApplyResume()
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.LoadedMedia || mediaStatus === MediaPlayer.BufferedMedia) win.tryApplyResume()
            else if (mediaStatus === MediaPlayer.EndOfMedia) { win.saveResume(); win.maybePlayNext() }
        }
        onTracksChanged: win.restoreTracks()
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
        // solid title bar above + control bar below frame the video when windowed; both overlay in fullscreen
        anchors.topMargin: win.fullscreen ? 0 : (topBar.visible ? topBar.height : 0)
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

    // resume cue — a brief "↩ 12:34" pill so the viewer notices the movie picked
    // up where they left off (the jump is otherwise silent and easy to distrust)
    Rectangle {
        id: resumeCue
        function show() { opacity = 1; hideTimer.restart() }
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: (topBar.visible ? topBar.height : 0) + 16
        z: 50
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
            text: "↩  " + win.fmt(win.resumeAtMs)
            color: "white"; font.pixelSize: 13; font.family: Theme.fontMono
        }
        Timer { id: hideTimer; interval: 3500; onTriggered: resumeCue.opacity = 0 }
    }

    // streaming runway pill — how much playback is buffered past the playhead
    Rectangle {
        id: aheadPill
        anchors.top: parent.top; anchors.left: parent.left
        anchors.topMargin: (topBar.visible ? topBar.height : 0) + 14; anchors.leftMargin: 18
        z: 49
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
            Rectangle { width: 7; height: 7; radius: 4; color: Theme.grn; anchors.verticalCenter: parent.verticalCenter }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: win.fmtAhead(win.bufferedAheadMs)
                color: Theme.grn; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
        }
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

    // external-subtitle overlay (over the video, above the controls bar) — styled
    // by the user's subtitle settings (size / color / background)
    Item {
        visible: win.extCueText.length > 0
        width: parent.width
        height: subText.implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: videoOut.bottom
        anchors.bottomMargin: (win.fullscreen && bar.visible ? bar.height : 0) + 26
        Rectangle {
            visible: win.subBgOpacity > 0
            anchors.centerIn: subText
            width: subText.paintedWidth + 22
            height: subText.paintedHeight + 8
            radius: 6
            color: Qt.rgba(0, 0, 0, win.subBgOpacity)
        }
        Text {
            id: subText
            anchors.centerIn: parent
            text: win.extCueText
            textFormat: Text.StyledText
            color: win.subColor
            style: win.subBgOpacity > 0 ? Text.Normal : Text.Outline
            styleColor: "#000000"
            font.pixelSize: Math.max(12, Math.round(win.height * 0.034 * win.subScale))
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: Math.min(win.width * 0.86, 980)
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
    function bumpVolume(d) { win.muted = false; win.volume = Math.max(0, Math.min(1, win.volume + d)) }
    function showControls() { win.controlsShown = true; if (win.fullscreen) idle.restart() }

    // subtitle sync nudges (mpv-style)
    Shortcut { sequence: "["; enabled: win.extSubsActive; onActivated: win.bumpSubOffset(-500) }
    Shortcut { sequence: "]"; enabled: win.extSubsActive; onActivated: win.bumpSubOffset(500) }

    // auto-hide the bar after inactivity while in fullscreen
    Timer { id: idle; interval: 3000; onTriggered: if (win.fullscreen) win.controlsShown = false }
    onFullscreenChanged: { win.controlsShown = true; if (win.fullscreen) idle.restart() }

    // track pickers (embedded audio / subtitle streams)
    BatMenu {
        id: audioMenu
        implicitWidth: 180
        Repeater {
            model: player.audioTracks.length
            BatMenuItem {
                required property int index
                text: (i18n.language, i18n.t("player_audio")) + " " + (index + 1)
                checkable: true; checked: player.activeAudioTrack === index
                onTriggered: {
                    player.activeAudioTrack = index
                    if (typeof settings !== "undefined") settings.set("audioTrack_" + win.infoHash, index)
                }
            }
        }
    }
    BatMenu {
        id: speedMenu
        implicitWidth: 140
        Repeater {
            model: [0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0]
            BatMenuItem {
                required property var modelData
                text: modelData + "×"
                checkable: true; checked: player.playbackRate === modelData
                onTriggered: player.playbackRate = modelData
            }
        }
    }
    BatMenu {
        id: subMenu
        implicitWidth: 220
        BatMenuItem {
            text: (i18n.language, i18n.t("player_subs_off"))
            checkable: true; checked: player.activeSubtitleTrack < 0 && !win.extSubsActive
            onTriggered: { win.clearExternalSubs(); player.activeSubtitleTrack = -1 }
        }
        Repeater {
            model: player.subtitleTracks.length
            BatMenuItem {
                required property int index
                text: (i18n.language, i18n.t("player_subs")) + " " + (index + 1)
                checkable: true; checked: player.activeSubtitleTrack === index
                onTriggered: {
                    win.clearExternalSubs(); player.activeSubtitleTrack = index
                    if (typeof settings !== "undefined") settings.set("subTrack_" + win.infoHash, index)
                }
            }
        }
        BatMenuItem {
            visible: win.extSubsActive
            text: (i18n.language, i18n.t("player_subs_external")) + ": " + win.extSubName
            elideMode: Text.ElideMiddle
            checkable: true; checked: win.extSubsActive
            onTriggered: win.clearExternalSubs()
        }
        BatMenuSep {}
        BatMenuItem {
            text: (i18n.language, i18n.t("subsearch_menu"))
            onTriggered: subPanel.openPanel()
        }
        BatMenuItem {
            text: (i18n.language, i18n.t("player_load_subtitle"))
            onTriggered: subFileDlg.open()
        }
    }

    BatMenu {
        id: moreMenu
        implicitWidth: 210
        BatMenuItem {
            text: (i18n.language, i18n.t("player_speed")) + ": " + player.playbackRate + "×"
            onTriggered: speedMenu.popup()
        }
        BatMenuItem {
            visible: player.audioTracks.length > 1
            text: (i18n.language, i18n.t("player_audio")) + "…"
            onTriggered: audioMenu.popup()
        }
        BatMenuSep {}
        BatMenuItem {
            text: (i18n.language, i18n.t("player_external"))
            onTriggered: { win.saveResume(); win.openExternal() }
        }
    }

    FileDialog {
        id: subFileDlg
        title: (i18n.language, i18n.t("player_load_subtitle"))
        nameFilters: [(i18n.language, i18n.t("filter_subtitle_files"))]
        onAccepted: win.loadExternalSubs(selectedFile.toString())
    }

    // ---- online-subtitles panel (results list + sync controls) ----
    PlayerSubPanel {
        id: subPanel
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: bar.top
        width: Math.min(370, parent.width * 0.46)
        pw: win
        mediaPlayer: player
    }

    // ---- title bar: solid panel · traffic-light space · centered title + chips · info ----
    Rectangle {
        id: topBar
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        height: 46
        color: "#161618"
        opacity: (!win.fullscreen || win.controlsShown) ? 1 : 0
        visible: opacity > 0 && win.mediaTitle.length > 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: "#0fffffff" }

        // drag the window by the title bar (windowed only)
        MouseArea {
            anchors.fill: parent
            enabled: !win.fullscreen
            onPressed: win.startSystemMove()
            onDoubleClicked: win.toggleFullscreen()
        }

        Row {
            anchors.centerIn: parent
            spacing: 9
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: win.mediaTitle.replace(/\s*\(\d{4}\)\s*$/, "")
                color: "#f3f3f4"; font.pixelSize: 15; font.weight: Font.DemiBold
                font.letterSpacing: -0.2; font.family: Theme.fontSans
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: /\(\d{4}\)\s*$/.test(win.mediaTitle)
                text: { var m = win.mediaTitle.match(/\((\d{4})\)\s*$/); return m ? ("(" + m[1] + ")") : "" }
                color: "#6f7077"; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.fontSans
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: win.mediaQuality.length > 0
                radius: 5; color: "transparent"; border.color: "#24ffffff"; border.width: 1
                implicitWidth: qB.implicitWidth + 12; implicitHeight: 18
                Text { id: qB; anchors.centerIn: parent; text: win.mediaQuality; color: "#b4b5ba"; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: win.mediaAudio.length > 0
                radius: 5; color: "transparent"; border.color: "#24ffffff"; border.width: 1
                implicitWidth: aB.implicitWidth + 12; implicitHeight: 18
                Text { id: aB; anchors.centerIn: parent; text: win.mediaAudio; color: "#b4b5ba"; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 15; height: 15; radius: 7.5; color: "transparent"
                border.color: infoMa.containsMouse ? "#7affffff" : "#38ffffff"; border.width: 1
                Text { anchors.centerIn: parent; text: "i"; color: infoMa.containsMouse ? "#b4b5ba" : "#818288"; font.pixelSize: 9; font.weight: Font.Bold; font.family: Theme.fontSans }
                MouseArea { id: infoMa; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true }
                ToolTip.visible: infoMa.containsMouse && win.mediaFileName.length > 0
                ToolTip.text: win.mediaFileName
                ToolTip.delay: 250
            }
        }
    }

    // ---- controls bar (scrubber + legend + control row) ----
    Rectangle {
        id: bar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 112
        color: "#0c0c0e"
        opacity: (!win.fullscreen || win.controlsShown) ? 1 : 0
        visible: opacity > 0
        Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#0fffffff" }
        Behavior on opacity { NumberAnimation { duration: 200 } }

        MouseArea {
            id: barHover; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton
            onContainsMouseChanged: if (containsMouse) win.showControls()
            onPositionChanged: win.showControls()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 20; anchors.rightMargin: 20
            anchors.topMargin: 6; anchors.bottomMargin: 14
            spacing: 4

            // --- scrubber row: time · bar · time ---
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                Text {
                    text: win.fmt(player.position); color: Theme.t1
                    font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontMono
                    Layout.minimumWidth: 52; horizontalAlignment: Text.AlignRight
                }
                PSlider {
                    id: seek
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                    from: 0; to: Math.max(1, player.duration)
                    value: player.position
                    buffered: (win.streamStats && win.streamStats.buffered) || 0
                    enabled: player.seekable
                    onMoved: player.position = value
                    MouseArea {
                        id: seekHover
                        anchors.fill: parent
                        acceptedButtons: Qt.NoButton
                        hoverEnabled: true
                        readonly property real frac: Math.max(0, Math.min(1, mouseX / Math.max(1, seek.availableWidth)))
                        Rectangle {
                            visible: (seekHover.containsMouse || seek.pressed) && player.duration > 0
                            height: 22; radius: 5; width: ttl.implicitWidth + 14
                            color: "#e60a0a0c"; border.color: Theme.hair; border.width: 1
                            y: -30
                            x: Math.max(0, Math.min(seek.width - width,
                                (seek.pressed ? seek.position * seek.width : seekHover.mouseX) - width / 2))
                            Text {
                                id: ttl; anchors.centerIn: parent
                                text: win.fmt((seek.pressed ? seek.position : seekHover.frac) * player.duration)
                                color: "#fff"; font.pixelSize: 11; font.family: Theme.fontMono
                            }
                        }
                    }
                    // marker where the contiguous download reaches (safe seek limit)
                    Rectangle {
                        visible: win.stillDownloading && seek.buffered > 0.005 && seek.buffered < 0.995
                        width: 2; height: 12; radius: 1
                        color: Theme.accent
                        anchors.verticalCenter: parent.verticalCenter
                        x: seek.leftPadding + seek.buffered * seek.availableWidth - width / 2
                    }
                }
                Text { text: win.fmt(player.duration); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontMono; Layout.minimumWidth: 52 }
            }

            // --- legend: watched · downloaded to X (only while streaming) ---
            RowLayout {
                Layout.fillWidth: true
                visible: win.stillDownloading && win.downloadedToMs > 0
                spacing: 12
                Item { Layout.minimumWidth: 52 }
                Row { spacing: 6
                    Rectangle { width: 9; height: 3; radius: 2; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "watched"; color: "#9a9aa0"; font.pixelSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                }
                Row { spacing: 6
                    Rectangle { width: 9; height: 3; radius: 2; color: "#80ffffff"; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "downloaded to " + win.fmt(win.downloadedToMs); color: "#9a9aa0"; font.pixelSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                }
                Item { Layout.fillWidth: true }
            }

            Item { Layout.fillHeight: true }

            // --- control row: rewind · play · forward | … | right cluster ---
            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                // rewind 10
                Item {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 30; implicitHeight: 30
                    IconImg { anchors.centerIn: parent; src: "qrc:/icons/replay.svg"; tint: rwMa.containsMouse ? Theme.t1 : Theme.t2; s: 26 }
                    Text { anchors.centerIn: parent; anchors.verticalCenterOffset: 1; text: "10"; color: rwMa.containsMouse ? Theme.t1 : Theme.t2; font.pixelSize: 8; font.weight: Font.Bold; font.family: Theme.fontSans }
                    MouseArea { id: rwMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.seekBy(-10000) }
                }
                // big play / pause
                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 44; implicitHeight: 44; radius: 22
                    color: playMa.containsMouse ? "#ffffff" : "#ededf0"
                    Behavior on color { ColorAnimation { duration: 120 } }
                    IconImg {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: player.playbackState === MediaPlayer.PlayingState ? 0 : 2
                        src: player.playbackState === MediaPlayer.PlayingState ? "qrc:/icons/pause.svg" : "qrc:/icons/play.svg"
                        tint: "#0a0a0c"; s: 20
                    }
                    MouseArea { id: playMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.togglePlay() }
                }
                // forward 10
                Item {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 30; implicitHeight: 30
                    IconImg { anchors.centerIn: parent; src: "qrc:/icons/forward.svg"; tint: fwMa.containsMouse ? Theme.t1 : Theme.t2; s: 26 }
                    Text { anchors.centerIn: parent; anchors.verticalCenterOffset: 1; text: "10"; color: fwMa.containsMouse ? Theme.t1 : Theme.t2; font.pixelSize: 8; font.weight: Font.Bold; font.family: Theme.fontSans }
                    MouseArea { id: fwMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.seekBy(10000) }
                }
                PChip {
                    Layout.alignment: Qt.AlignVCenter
                    visible: win.nextIdx >= 0
                    label: "⏭"
                    onClicked: if (typeof session !== "undefined") session.playFile(win.infoHash, win.nextIdx)
                }

                Item { Layout.fillWidth: true }

                // "↓ to 1:19:49" — bordered pill (downloaded-to time); hover = size
                Rectangle {
                    Layout.alignment: Qt.AlignVCenter
                    readonly property real prog: (win.streamStats && win.streamStats.progress) || 0
                    readonly property real total: (win.streamStats && win.streamStats.totalBytes) || 0
                    visible: total > 0
                    implicitWidth: dlRow.implicitWidth + 22; implicitHeight: 30
                    radius: 8
                    color: dlMa.containsMouse ? "#1affffff" : "transparent"
                    border.color: Theme.hair; border.width: 1
                    Row {
                        id: dlRow; anchors.centerIn: parent; spacing: 6
                        IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/download.svg"; tint: Theme.t3; s: 13 }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: parent.parent.prog < 0.999 ? ("to " + win.fmt(win.downloadedToMs)) : win.fmtBytes(parent.parent.total)
                            color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono
                        }
                    }
                    MouseArea { id: dlMa; anchors.fill: parent; hoverEnabled: true }
                    ToolTip.visible: dlMa.containsMouse
                    ToolTip.delay: 250
                    ToolTip.text: win.fmtBytes((win.streamStats && win.streamStats.downloadedBytes) || 0)
                                  + " / " + win.fmtBytes((win.streamStats && win.streamStats.totalBytes) || 0)
                }
                // speed
                PChip { Layout.alignment: Qt.AlignVCenter; active: player.playbackRate !== 1.0; label: player.playbackRate + "×"; onClicked: speedMenu.popup() }
                // subtitles
                Item {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: subRow.implicitWidth; implicitHeight: 24
                    Row {
                        id: subRow; anchors.centerIn: parent; spacing: 6
                        IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/subtitles.svg"; tint: (player.activeSubtitleTrack >= 0 || win.extSubsActive) ? Theme.accent : (subMa.containsMouse ? Theme.t1 : Theme.t2); s: 16 }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: (i18n.language, i18n.t("player_subs")); color: subMa.containsMouse ? Theme.t1 : Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
                    }
                    MouseArea { id: subMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: subMenu.popup() }
                }
                // volume: click = mute; hover = vertical slider
                Item {
                    Layout.alignment: Qt.AlignVCenter
                    implicitWidth: 24; implicitHeight: 24
                    IconImg {
                        anchors.centerIn: parent
                        src: (win.muted || win.volume <= 0) ? "qrc:/icons/volume-mute.svg" : "qrc:/icons/volume.svg"
                        tint: volMa.containsMouse ? Theme.t1 : Theme.t2
                        s: 18
                    }
                    MouseArea { id: volMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.muted = !win.muted }
                    Rectangle {
                        visible: volMa.containsMouse || volPopMa.containsMouse || vsl.pressed
                        width: 32; height: 116; radius: 9
                        color: "#f50a0a0c"; border.color: Theme.hair; border.width: 1
                        anchors.bottom: parent.top; anchors.bottomMargin: 6
                        anchors.horizontalCenter: parent.horizontalCenter
                        MouseArea { id: volPopMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                        Slider {
                            id: vsl
                            orientation: Qt.Vertical
                            anchors.centerIn: parent
                            height: 96; from: 0; to: 1
                            value: win.muted ? 0 : win.volume
                            onMoved: { win.muted = false; win.volume = value }
                            background: Rectangle {
                                x: vsl.leftPadding + vsl.availableWidth / 2 - width / 2
                                y: vsl.topPadding
                                width: 4; height: vsl.availableHeight; radius: 2; color: "#3a3a42"
                                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: vsl.position * parent.height; radius: 2; color: Theme.accent }
                            }
                            handle: Rectangle {
                                x: vsl.leftPadding + vsl.availableWidth / 2 - width / 2
                                y: vsl.topPadding + (1 - vsl.position) * (vsl.availableHeight - height)
                                implicitWidth: 12; implicitHeight: 12; radius: 6; color: "#fff"
                            }
                        }
                    }
                }
                PChip { Layout.alignment: Qt.AlignVCenter; label: "⋯"; onClicked: moreMenu.popup() }
                PChip { Layout.alignment: Qt.AlignVCenter; active: win.pipMode; label: "⧉"; onClicked: win.togglePip() }
                PChip { Layout.alignment: Qt.AlignVCenter; label: "⛶"; onClicked: win.toggleFullscreen() }
            }
        }
    }

    Component.onCompleted: if (win.streamUrl.length > 0) player.play()

    Shortcut { sequence: "Space";  onActivated: win.togglePlay() }
    Shortcut { sequence: "P";      onActivated: win.togglePip() }
    Shortcut { sequence: "Right";  onActivated: win.seekBy(10000) }
    Shortcut { sequence: "Left";   onActivated: win.seekBy(-10000) }
    Shortcut { sequence: "Up";     onActivated: win.bumpVolume(0.05) }
    Shortcut { sequence: "Down";   onActivated: win.bumpVolume(-0.05) }
    Shortcut { sequence: "F";      onActivated: win.toggleFullscreen() }
    Shortcut { sequence: "M";      onActivated: win.muted = !win.muted }
    Shortcut { sequence: "Escape"; onActivated: win.pipMode ? win.togglePip()
                                              : (win.visibility === Window.FullScreen ? (win.visibility = Window.Windowed) : win.close()) }
}
