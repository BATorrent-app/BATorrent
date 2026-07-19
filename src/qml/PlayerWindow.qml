// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Embedded video player (4.0 step ④). Plays a torrent file streamed from the
// local StreamServer (download-while-watch), with resume per infohash+file and
// an external-player fallback if the codec isn't supported.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
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
    property string localFile: ""   // on-disk path of the playing file (seek previews decode this, not the HTTP stream)
    readonly property bool ambientGlow: (typeof settings === "undefined") || settings.get("ambientGlow") !== false
    property string mediaTitle: ""
    property string mediaFileName: ""
    // resolved display title (metadata) + "S4 · E10"/year; raw name lives in
    // mediaFileName (info tooltip). Falls back to mediaTitle if unresolved.
    property string resolvedTitle: ""
    property string resolvedSubtitle: ""
    readonly property string headerTitle: resolvedTitle.length > 0 ? resolvedTitle : mediaTitle
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

    // human label for an embedded audio/subtitle track: prefer the stream's
    // language (localized, e.g. "English"), then its title, else "Subtitles N".
    // QtMultimedia exposes these only as metadata on the track list.
    function trackName(meta, fallback) {
        if (!meta) return fallback
        var lang = meta.stringValue(MediaMetaData.Language)
        var title = meta.stringValue(MediaMetaData.Title)
        if (lang.length > 0 && title.length > 0) return lang + " · " + title
        if (lang.length > 0) return lang
        if (title.length > 0) return title
        return fallback
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
        readonly property bool active: sl.pressed || sl.hovered
        implicitHeight: 18
        background: Rectangle {
            x: sl.leftPadding; y: sl.topPadding + sl.availableHeight / 2 - height / 2
            width: sl.availableWidth
            height: sl.active ? 7 : 5; radius: height / 2
            color: "#26ffffff"
            Behavior on height { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            // downloaded layer (what's safe to seek to)
            Rectangle { width: Math.max(0, Math.min(1, sl.buffered)) * parent.width; height: parent.height; radius: parent.radius; color: "#4dffffff" }
            // playback layer (what you're watching) — accent with a soft glow
            Rectangle {
                width: sl.visualPosition * parent.width; height: parent.height; radius: parent.radius
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: Theme.accent }
                    GradientStop { position: 1.0; color: "#ff2e3d" }
                }
                layer.enabled: sl.active
                layer.effect: MultiEffect { blurEnabled: true; blur: 0.6; blurMax: 8 }
            }
        }
        handle: Item {
            implicitWidth: sl.active ? 15 : 0
            implicitHeight: implicitWidth
            x: sl.leftPadding + sl.visualPosition * (sl.availableWidth - width)
            y: sl.topPadding + sl.availableHeight / 2 - height / 2
            Behavior on implicitWidth { NumberAnimation { duration: 130; easing.type: Easing.OutBack } }
            MultiEffect {
                source: disc
                anchors.fill: disc
                shadowEnabled: true
                shadowBlur: 1.0
                blurMax: 10
                shadowColor: "#80000000"
                visible: parent.width > 0
            }
            Rectangle { id: disc; anchors.fill: parent; radius: width / 2; color: "#ffffff" }
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
    // Reading from a growing torrent file, FFmpeg's backend freezes on an
    // underrun without ever reporting MediaPlayer.StalledMedia — position just
    // stops advancing while playbackState stays Playing, with no visual sign.
    // Detect it ourselves from the same runway math the "ahead" pill already
    // uses, debounced so a normal neck-and-neck moment doesn't flicker the spinner.
    readonly property bool starvedNow: player.playbackState === MediaPlayer.PlayingState
                                       && win.stillDownloading && win.bufferedAheadMs < 400
    property bool starved: false
    onStarvedNowChanged: {
        if (starvedNow) starveDebounce.restart()
        else { starveDebounce.stop(); starved = false }   // recovery is instant
    }
    Timer { id: starveDebounce; interval: 350; onTriggered: if (win.starvedNow) win.starved = true }
    function fmtAhead(ms) {
        var s = Math.floor(ms / 1000)
        if (s >= 3600) return Math.floor(s / 3600) + "h " + Math.floor((s % 3600) / 60) + "m buffered ahead"
        if (s >= 60)   return Math.floor(s / 60) + " min buffered ahead"
        return s + "s buffered ahead"
    }
    function fileUrl(p) {
        if (!p || p.length === 0) return ""
        if (/^(file|qrc|https?|image):/.test(p)) return p
        return (Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(p)
    }
    // compact runway for the control-bar pill ("+38 min", "+45s")
    function fmtRunway(ms) {
        var s = Math.floor(ms / 1000)
        if (s >= 3600) return Math.floor(s / 3600) + "h " + Math.floor((s % 3600) / 60) + "m"
        if (s >= 60)   return Math.floor(s / 60) + " min"
        return s + "s"
    }
    // entry point used by Main.qml when (re)opening the player with new media
    property int nextIdx: -1     // file index of the next episode, or -1
    property bool autoplayNext: (typeof settings === "undefined") || settings.get("autoplayNext") !== false
    // MKV chapters → skip intro/credits chip
    property var chapters: []
    readonly property var activeSkip: {
        for (var i = 0; i < chapters.length; ++i) {
            var c = chapters[i]
            if (c.kind && c.endMs > 0 && player.position >= c.startMs && player.position < c.endMs - 1000)
                return c
        }
        return null
    }
    // next-episode end card
    property string nextPoster: ""
    property string nextTitle: ""
    property string nextSubtitle: ""
    property bool endCardDismissed: false
    property int countdownSec: 0            // >0 while auto-advancing
    readonly property real endCardLeadMs: 28000   // card appears this close to the end
    readonly property bool inLeadWindow: player.duration > 0 && player.position > 0
        && (player.duration - player.position) <= endCardLeadMs
    readonly property bool showEndCard: win.nextIdx >= 0 && !win.endCardDismissed
        && win.autoplayNext && (win.inLeadWindow || win.countdownSec > 0)
    function playNextNow() {
        countdown.stop(); win.countdownSec = 0
        if (typeof session !== "undefined" && win.nextIdx >= 0)
            session.playFile(win.infoHash, win.nextIdx)
    }
    function maybePlayNext() {
        if (typeof session === "undefined" || win.nextIdx < 0) return
        if (!win.autoplayNext || win.endCardDismissed) return
        // hand off to the visible countdown instead of a silent cut
        win.countdownSec = 8
        countdown.restart()
    }
    Timer {
        id: countdown; interval: 1000; repeat: true
        onTriggered: { win.countdownSec -= 1; if (win.countdownSec <= 0) win.playNextNow() }
    }
    function openMedia(url, title, hash, fileIdx) {
        win.saveResume()
        win.resumed = false
        win.streamUrl = url
        win.mediaTitle = title
        win.infoHash = hash
        win.fileIndex = fileIdx
        win.mediaFileName = (typeof session !== "undefined") ? session.streamFileName(hash, fileIdx) : ""
        win.localFile = (typeof session !== "undefined") ? session.streamLocalPath(hash, fileIdx) : ""
        var pt = (typeof session !== "undefined") ? session.playerTitle(hash, fileIdx) : ({})
        win.resolvedTitle = pt.title || ""
        win.resolvedSubtitle = pt.subtitle || ""
        win.pendingResumeMs = (typeof settings !== "undefined") ? Number(settings.get(win.resumeKey) || 0) : 0
        win.resumeAtMs = 0
        win.resumeTries = 0
        resumeRetry.stop()
        win.aheadShown = false
        win.chapters = (typeof session !== "undefined") ? session.mkvChapters(hash, fileIdx) : []
        win.nextIdx = (typeof session !== "undefined") ? session.nextEpisode(hash, fileIdx) : -1
        // pre-resolve the next episode's card data (same show → same poster)
        win.endCardDismissed = false
        win.countdownSec = 0
        countdown.stop()
        if (win.nextIdx >= 0 && typeof session !== "undefined") {
            var np = session.playerTitle(hash, win.nextIdx)
            win.nextTitle = np.title || ""
            win.nextSubtitle = np.subtitle || ""
            win.nextPoster = session.posterForHash(hash)
        } else { win.nextTitle = ""; win.nextSubtitle = ""; win.nextPoster = "" }
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
        onPositionChanged: win.updateCue(player.position)
        onDurationChanged: win.tryApplyResume()
        onSeekableChanged: win.tryApplyResume()
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.PlayingState) win.tryApplyResume()
            else win.showControls()   // pausing always brings the chrome back
        }
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

    // ambient glow — a blurred, upscaled copy of the frame bleeding into the
    // letterbox bars (Apple TV). One decoder: MultiEffect samples the same
    // VideoOutput, so there's no second stream. Gated behind a setting for
    // weaker GPUs (default on).
    MultiEffect {
        anchors.fill: parent
        source: videoOut
        z: -1
        visible: win.ambientGlow && player.hasVideo
        blurEnabled: true
        blur: 1.0
        blurMax: 64
        scale: 1.18
        opacity: 0.45
        brightness: -0.05
        saturation: 0.35
    }

    VideoOutput {
        id: videoOut
        anchors.fill: parent
        // video fills the window; the chrome floats over it (Stremio-style) so
        // the gradient scrims read as depth, not a flat deck on a black frame
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
                 || win.starved
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
        anchors.bottomMargin: (bar.visible ? bar.height : 0) + 26
        Behavior on anchors.bottomMargin { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
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
    function showControls() { win.controlsShown = true; idle.restart() }

    // subtitle sync nudges (mpv-style)
    Shortcut { sequence: "["; enabled: win.extSubsActive; onActivated: win.bumpSubOffset(-500) }
    Shortcut { sequence: "]"; enabled: win.extSubsActive; onActivated: win.bumpSubOffset(500) }

    // auto-hide the chrome after inactivity (both windowed and fullscreen, like
    // Stremio) — but never while paused, hovering the bar, or with a panel open.
    Timer {
        id: idle; interval: 3000
        onTriggered: {
            if (subPanel.open || optsPanel.open || barHover.containsMouse) return
            if (player.playbackState !== MediaPlayer.PlayingState) return
            win.controlsShown = false
        }
    }
    onFullscreenChanged: { win.controlsShown = true; idle.restart() }

    // audio · subtitles · speed live in one options panel (see below); the
    // "…" menu keeps only what doesn't fit a picker
    BatMenu {
        id: moreMenu
        implicitWidth: 210
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

    // ---- online-subtitles panel (slide-in drawer + scrim) ----
    // above the title/control bars so its close button and header aren't trapped
    // underneath the top chrome (they overlap the drawer's top edge).
    PlayerSubPanel {
        id: subPanel
        anchors.fill: parent
        z: 60
        pw: win
        mediaPlayer: player
    }

    // unified audio/subtitles/speed popover (replaces the old context menus)
    PlayerOptionsPanel {
        id: optsPanel
        anchors.fill: parent
        z: 55
        pw: win
        mediaPlayer: player
        onSearchOnline: subPanel.openPanel()
        onLoadFile: subFileDlg.open()
    }

    // ---- skip intro / credits chip (bottom-right, above the control bar) ----
    // Netflix-style: appears while the playhead sits inside a chapter the MKV
    // labelled intro/opening/credits, seeks to its end. Visible even when the
    // chrome is hidden, but yields to the end card.
    Rectangle {
        id: skipChip
        z: 57
        readonly property var sk: win.activeSkip
        visible: opacity > 0
        opacity: (sk && !win.showEndCard) ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 24
        anchors.bottomMargin: win.controlsShown ? 134 : 40
        Behavior on anchors.bottomMargin { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        width: skipRow.width + 30; height: 40
        radius: 8
        color: skMa.containsMouse ? "#ffffff" : "#e6101014"
        border.color: skMa.containsMouse ? "#ffffff" : Theme.hair; border.width: 1
        Behavior on color { ColorAnimation { duration: 120 } }
        Row {
            id: skipRow; anchors.centerIn: parent; spacing: 8
            IconImg {
                anchors.verticalCenter: parent.verticalCenter
                src: "qrc:/icons/skip-forward.svg"; s: 15
                tint: skMa.containsMouse ? "#0a0a0c" : Theme.t1
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: skipChip.sk ? (skipChip.sk.kind === "credits"
                        ? (i18n.language, i18n.t("player_skip_credits"))
                        : (i18n.language, i18n.t("player_skip_intro"))) : ""
                color: skMa.containsMouse ? "#0a0a0c" : Theme.t1
                font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
        }
        MouseArea {
            id: skMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: if (skipChip.sk) { player.position = skipChip.sk.endMs; win.showControls() }
        }
    }

    // ---- next-episode end card (bottom-right, above the control bar) ----
    Rectangle {
        id: endCard
        z: 58
        visible: opacity > 0
        opacity: win.showEndCard ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 24; anchors.bottomMargin: 134
        width: 348; height: 116
        radius: 12
        color: "#f2101014"
        border.color: Theme.hair; border.width: 1
        transform: Translate { y: win.showEndCard ? 0 : 10; Behavior on y { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } } }

        Row {
            anchors.fill: parent; anchors.margins: 12; spacing: 12

            // poster with a countdown ring overlay
            Item {
                width: 62; height: 92; anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    anchors.fill: parent; radius: 7; clip: true
                    color: "#1c1c20"; border.color: Theme.hairSoft; border.width: 1
                    Image {
                        anchors.fill: parent
                        source: win.nextPoster.length > 0 ? win.fileUrl(win.nextPoster) : ""
                        fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true
                    }
                }
                // ring that fills as the countdown runs down
                Canvas {
                    id: ring
                    anchors.centerIn: parent; width: 40; height: 40
                    visible: win.countdownSec > 0
                    property real frac: win.countdownSec / 8
                    onFracChanged: requestPaint()
                    onPaint: {
                        var ctx = getContext("2d"); ctx.reset()
                        var cx = width/2, cy = height/2, r = 17
                        ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI*2)
                        ctx.lineWidth = 3; ctx.strokeStyle = "#40000000"; ctx.stroke()
                        ctx.beginPath(); ctx.arc(cx, cy, r, -Math.PI/2, -Math.PI/2 + Math.PI*2*frac)
                        ctx.lineWidth = 3; ctx.lineCap = "round"; ctx.strokeStyle = "#e5332b"; ctx.stroke()
                    }
                    Text {
                        anchors.centerIn: parent; text: win.countdownSec
                        color: "#fff"; font.pixelSize: 15; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                }
            }

            Column {
                width: parent.width - 62 - 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 3
                Text {
                    text: (win.countdownSec > 0)
                          ? ((i18n.language, i18n.t("player_next_ep")) + " · " + win.countdownSec + "s")
                          : (i18n.language, i18n.t("player_up_next"))
                    color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
                    font.letterSpacing: 0.6; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                }
                Text {
                    width: parent.width; text: win.nextTitle
                    color: "#f3f3f4"; font.pixelSize: 14; font.weight: Font.DemiBold
                    font.family: Theme.fontSans; elide: Text.ElideRight
                }
                Text {
                    width: parent.width; visible: win.nextSubtitle.length > 0
                    text: win.nextSubtitle
                    color: "#8a8b90"; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight
                }
                Row {
                    spacing: 8; topPadding: 4
                    Rectangle {
                        width: playNowRow.width + 22; height: 28; radius: 7; color: pnMa.containsMouse ? "#ff2e37" : Theme.accent
                        Behavior on color { ColorAnimation { duration: 100 } }
                        Row {
                            id: playNowRow; anchors.centerIn: parent; spacing: 6
                            IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/play.svg"; tint: "#fff"; s: 13 }
                            Text { anchors.verticalCenter: parent.verticalCenter; text: (i18n.language, i18n.t("player_watch_now")); color: "#fff"; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        }
                        MouseArea { id: pnMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.playNextNow() }
                    }
                    Rectangle {
                        width: cancelTxt.width + 22; height: 28; radius: 7
                        color: cnMa.containsMouse ? "#1affffff" : "transparent"
                        border.color: Theme.hair; border.width: 1
                        Text { id: cancelTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t("btn_cancel")); color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans }
                        MouseArea { id: cnMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: { countdown.stop(); win.countdownSec = 0; win.endCardDismissed = true } }
                    }
                }
            }
        }
    }

    // ---- title bar: gradient scrim · centered title + chips · info ----
    Item {
        id: topBar
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        height: 52
        opacity: win.controlsShown ? 1 : 0
        visible: opacity > 0 && win.mediaTitle.length > 0
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        // scrim fading downward — mirrors the bottom bar so the chrome frames the
        // video without hard edges.
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#e6070708" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }

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
                text: win.headerTitle.replace(/\s*\(\d{4}\)\s*$/, "")
                color: "#f3f3f4"; font.pixelSize: 15; font.weight: Font.DemiBold
                font.letterSpacing: -0.2; font.family: Theme.fontSans
            }
            // resolved subtitle ("S4 · E10" / year) — a soft separator + muted text,
            // so the header reads like a show, not a filename
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: win.resolvedSubtitle.length > 0
                text: win.resolvedSubtitle
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
    Item {
        id: bar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 124
        opacity: win.controlsShown ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        // gradient scrim: the video bleeds into the controls (Stremio/YouTube),
        // instead of a hard slab with a hairline. In windowed mode it sits on the
        // black frame below the video, so it just reads as a soft dark deck.
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0;  color: "#00000000" }
                GradientStop { position: 0.42; color: "#c4090909" }
                GradientStop { position: 1.0;  color: "#f6070708" }
            }
        }

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
                        readonly property real targetMs: (seek.pressed ? seek.position : frac) * player.duration
                        // frame previews only where there's data to decode
                        readonly property bool canThumb: player.duration > 0
                            && (!win.stillDownloading || targetMs <= win.downloadedToMs)

                        // silent second decoder — paused at the hovered instant, its
                        // output IS the preview. Reads the on-disk file directly (a
                        // second client on the local HTTP stream made ffmpeg spew
                        // resync noise). Warmed up whenever the controls are visible:
                        // loading only on bar-hover cost ~half a second per entry.
                        MediaPlayer {
                            id: thumbPlayer
                            source: win.controlsShown && win.localFile.length > 0
                                    ? ((Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(win.localFile))
                                    : ""
                            videoOutput: thumbOut
                            onMediaStatusChanged: if (mediaStatus === MediaPlayer.LoadedMedia) {
                                play(); pause()   // stopped players don't render frames
                                thumbSeek.restart()
                            }
                        }
                        Timer {
                            id: thumbSeek
                            interval: 60   // debounce: seek once the cursor settles
                            onTriggered: if (thumbPlayer.seekable && (seekHover.containsMouse || seek.pressed))
                                thumbPlayer.position = seekHover.targetMs
                        }
                        onMouseXChanged: if (containsMouse || seek.pressed) thumbSeek.restart()
                        // the file can be renamed when it finishes (.!bt stripped) —
                        // re-resolve the on-disk path at each hover start
                        onContainsMouseChanged: if (containsMouse && typeof session !== "undefined")
                            win.localFile = session.streamLocalPath(win.infoHash, win.fileIndex)

                        Rectangle {
                            readonly property bool showThumb: seekHover.canThumb
                            visible: (seekHover.containsMouse || seek.pressed) && player.duration > 0
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
                                text: win.fmt(seekHover.targetMs)
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
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                // LEFT zone: "+X min" ready-to-watch runway (only while streaming)
                RowLayout {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        readonly property real total: (win.streamStats && win.streamStats.totalBytes) || 0
                        readonly property bool low: win.bufferedAheadMs < 30000
                        visible: total > 0 && win.stillDownloading
                        implicitWidth: dlRow.implicitWidth + 22; implicitHeight: 30
                        radius: 8
                        color: dlMa.containsMouse ? "#1affffff" : "transparent"
                        border.color: low ? Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.5) : Theme.hair
                        border.width: 1
                        Behavior on border.color { ColorAnimation { duration: 200 } }
                        Row {
                            id: dlRow; anchors.centerIn: parent; spacing: 6
                            IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/clock.svg"; tint: parent.parent.low ? Theme.amber : Theme.t3; s: 13 }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: "+" + win.fmtRunway(win.bufferedAheadMs)
                                color: parent.parent.low ? Theme.amber : Theme.t2
                                font.pixelSize: 12; font.family: Theme.fontMono
                            }
                        }
                        MouseArea { id: dlMa; anchors.fill: parent; hoverEnabled: true }
                        ToolTip.visible: dlMa.containsMouse
                        ToolTip.delay: 250
                        ToolTip.text: win.fmtAhead(win.bufferedAheadMs) + "  ·  "
                                      + win.fmtBytes((win.streamStats && win.streamStats.downloadedBytes) || 0)
                                      + " / " + win.fmtBytes((win.streamStats && win.streamStats.totalBytes) || 0)
                    }
                }

                // CENTER zone: transport — anchored to the bar's true center
                RowLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    // rewind 10 — icon-only, circular hover, matching the right cluster
                    Item {
                        Layout.alignment: Qt.AlignVCenter
                        implicitWidth: 34; implicitHeight: 34
                        scale: rwMa.pressed ? 0.9 : 1.0
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                        Rectangle {
                            anchors.fill: parent; radius: width / 2
                            color: rwMa.containsMouse ? "#1effffff" : "transparent"
                            Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
                        }
                        IconImg {
                            anchors.centerIn: parent; src: "qrc:/icons/replay.svg"; s: 22
                            tint: rwMa.containsMouse ? Theme.t1 : Theme.t2
                            Behavior on tint { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                        }
                        Text {
                            anchors.centerIn: parent; anchors.verticalCenterOffset: 1; text: "10"
                            color: rwMa.containsMouse ? Theme.t1 : Theme.t2
                            font.pixelSize: 7; font.weight: Font.Bold; font.family: Theme.fontSans
                            Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                        }
                        MouseArea { id: rwMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.seekBy(-10000) }
                    }
                    // play / pause — the hero: same minimalist language, a touch
                    // larger, red as the hover detail (signal, not a white disc)
                    Item {
                        Layout.alignment: Qt.AlignVCenter
                        implicitWidth: 44; implicitHeight: 44
                        scale: playMa.pressed ? 0.94 : (playMa.containsMouse ? 1.04 : 1.0)
                        Behavior on scale { NumberAnimation { duration: 130; easing.type: Easing.OutCubic } }
                        Rectangle {
                            anchors.fill: parent; radius: width / 2
                            color: playMa.containsMouse ? "#1effffff" : "transparent"
                            border.color: playMa.containsMouse ? Theme.accent : "transparent"
                            border.width: 1
                            Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
                            Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
                        }
                        IconImg {
                            anchors.centerIn: parent
                            anchors.horizontalCenterOffset: player.playbackState === MediaPlayer.PlayingState ? 0 : 2
                            src: player.playbackState === MediaPlayer.PlayingState ? "qrc:/icons/pause.svg" : "qrc:/icons/play.svg"
                            tint: playMa.containsMouse ? Theme.accent : Theme.t1; s: 26
                            Behavior on tint { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                        }
                        MouseArea { id: playMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.togglePlay() }
                    }
                    // forward 10
                    Item {
                        Layout.alignment: Qt.AlignVCenter
                        implicitWidth: 34; implicitHeight: 34
                        scale: fwMa.pressed ? 0.9 : 1.0
                        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                        Rectangle {
                            anchors.fill: parent; radius: width / 2
                            color: fwMa.containsMouse ? "#1effffff" : "transparent"
                            Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
                        }
                        IconImg {
                            anchors.centerIn: parent; src: "qrc:/icons/forward.svg"; s: 22
                            tint: fwMa.containsMouse ? Theme.t1 : Theme.t2
                            Behavior on tint { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                        }
                        Text {
                            anchors.centerIn: parent; anchors.verticalCenterOffset: 1; text: "10"
                            color: fwMa.containsMouse ? Theme.t1 : Theme.t2
                            font.pixelSize: 7; font.weight: Font.Bold; font.family: Theme.fontSans
                            Behavior on color { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                        }
                        MouseArea { id: fwMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.seekBy(10000) }
                    }
                    PIconBtn {
                        Layout.alignment: Qt.AlignVCenter
                        visible: win.nextIdx >= 0
                        src: "qrc:/icons/skip-forward.svg"
                        tip: (i18n.language, i18n.t("player_next"))
                        onClicked: if (typeof session !== "undefined") session.playFile(win.infoHash, win.nextIdx)
                    }
                }

                // RIGHT zone: options — speed · subtitles · volume · more · pip · fullscreen
                RowLayout {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    // speed
                    PChip { Layout.alignment: Qt.AlignVCenter; active: player.playbackRate !== 1.0; label: player.playbackRate + "×"; onClicked: optsPanel.open ? optsPanel.closePanel() : optsPanel.openPanel() }
                    // subtitles / audio / speed — one icon, one panel
                    PIconBtn {
                        Layout.alignment: Qt.AlignVCenter
                        src: "qrc:/icons/subtitles.svg"; s: 19
                        active: player.activeSubtitleTrack >= 0 || win.extSubsActive
                        tip: (i18n.language, i18n.t("player_subs"))
                        onClicked: optsPanel.open ? optsPanel.closePanel() : optsPanel.openPanel()
                    }
                    // volume: icon + an inline horizontal slider that expands on
                    // hover (YouTube-style). One container, one HoverHandler —
                    // handlers receive hover cooperatively, so there's no popup
                    // layer, no icon→slider crossing, no dead zone. Three floating-
                    // popup attempts all ghosted; inline is structurally immune.
                    Item {
                        id: volCtl
                        Layout.alignment: Qt.AlignVCenter
                        readonly property bool expanded: volHov.hovered || hsl.pressed
                        implicitHeight: 32
                        implicitWidth: 32 + (expanded ? 78 : 0)
                        Behavior on implicitWidth { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
                        clip: true
                        HoverHandler { id: volHov }
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 2
                            Item {
                                width: 32; height: 32
                                Rectangle {
                                    anchors.fill: parent; radius: width / 2
                                    color: volMa.containsMouse ? "#1effffff" : "transparent"
                                    Behavior on color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
                                }
                                IconImg {
                                    anchors.centerIn: parent
                                    src: (win.muted || win.volume <= 0) ? "qrc:/icons/volume-mute.svg" : "qrc:/icons/volume.svg"
                                    tint: volCtl.expanded ? Theme.t1 : Theme.t2
                                    s: 18
                                }
                                MouseArea {
                                    id: volMa; anchors.fill: parent
                                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: win.muted = !win.muted
                                }
                            }
                            Slider {
                                id: hsl
                                width: 72; height: 32
                                anchors.verticalCenter: parent.verticalCenter
                                opacity: volCtl.expanded ? 1 : 0
                                Behavior on opacity { NumberAnimation { duration: 120 } }
                                from: 0; to: 1
                                value: win.muted ? 0 : win.volume
                                onMoved: { win.muted = false; win.volume = value }
                                background: Rectangle {
                                    x: hsl.leftPadding; y: hsl.topPadding + hsl.availableHeight / 2 - height / 2
                                    width: hsl.availableWidth; height: 4; radius: 2; color: "#3a3a42"
                                    Rectangle { width: hsl.visualPosition * parent.width; height: parent.height; radius: 2; color: "#ffffff" }
                                }
                                handle: Rectangle {
                                    x: hsl.leftPadding + hsl.visualPosition * (hsl.availableWidth - width)
                                    y: hsl.topPadding + hsl.availableHeight / 2 - height / 2
                                    implicitWidth: 12; implicitHeight: 12; radius: 6; color: "#fff"
                                }
                            }
                        }
                    }
                    PIconBtn { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/ellipsis.svg"; tip: (i18n.language, i18n.t("ctx_grp_more")); onClicked: moreMenu.popup() }
                    PIconBtn { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/pip.svg"; active: win.pipMode; tip: (i18n.language, i18n.t("player_pip")); onClicked: win.togglePip() }
                    PIconBtn { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/maximize.svg"; tip: (i18n.language, i18n.t("player_fullscreen")); onClicked: win.toggleFullscreen() }
                }
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
    Shortcut { sequence: "Escape"; onActivated: optsPanel.open ? optsPanel.closePanel()
                                              : subPanel.open ? subPanel.closePanel()
                                              : (win.pipMode ? win.togglePip()
                                              : (win.visibility === Window.FullScreen ? (win.visibility = Window.Windowed) : win.close())) }
}
