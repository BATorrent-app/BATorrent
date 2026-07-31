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
import "../theme"
import "../widgets"

Window {
    id: win
    PlayerFormat { id: playerFmt }
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
    readonly property string mediaQuality: playerFmt.qualityFromName(mediaFileName)
    readonly property string mediaAudio: playerFmt.audioFromName(mediaFileName)
    property string infoHash: ""
    property int fileIndex: 0
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
        if (st !== undefined && st !== "") {
            var sn = Number(st)
            // -1 = user chose "Subtitles off"; don't treat it as "unset"
            if (sn < 0)
                player.activeSubtitleTrack = -1
            else if (sn < player.subtitleTracks.length)
                player.activeSubtitleTrack = sn
        }
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
        var r = playerFmt.cueAt(extCues, extCueIdx, rawPos, extSubOffset)
        extCueIdx = r.idx
        extCueText = r.text
    }
    readonly property bool fullscreen: win.visibility === Window.FullScreen

    function fmt(ms) { return playerFmt.fmt(ms) }
    function fmtBytes(b) { return playerFmt.fmtBytes(b) }
    function fileUrl(p) { return playerFmt.fileUrl(p) }

    property int nextIdx: -1
    property bool autoplayNext: (typeof settings === "undefined") || settings.get("autoplayNext") !== false
    property var chapters: []
    property string nextPoster: ""
    property string nextTitle: ""
    property string nextSubtitle: ""

    readonly property bool stillDownloading: runway.stillDownloading
    readonly property real downloadedToMs: runway.downloadedToMs
    readonly property real bufferedAheadMs: runway.bufferedAheadMs
    readonly property var streamStats: runway.streamStats
    readonly property bool starved: runway.starved
    readonly property bool showEndCard: endCard.showEndCard

    function openMedia(url, title, hash, fileIdx) {
        resume.save()
        win.streamUrl = url
        win.mediaTitle = title
        win.infoHash = hash
        win.fileIndex = fileIdx
        win.mediaFileName = (typeof session !== "undefined") ? session.streamFileName(hash, fileIdx) : ""
        win.localFile = (typeof session !== "undefined") ? session.streamLocalPath(hash, fileIdx) : ""
        var pt = (typeof session !== "undefined") ? session.playerTitle(hash, fileIdx) : ({})
        win.resolvedTitle = pt.title || ""
        win.resolvedSubtitle = pt.subtitle || ""
        resume.prepareOpen()
        runway.reset()
        win.chapters = (typeof session !== "undefined") ? session.mkvChapters(hash, fileIdx) : []
        win.nextIdx = (typeof session !== "undefined") ? session.nextEpisode(hash, fileIdx) : -1
        endCard.reset()
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

    MediaPlayer {
        id: player
        source: win.streamUrl
        videoOutput: videoOut
        audioOutput: AudioOutput { id: audio; volume: win.volume; muted: win.muted }
        onPositionChanged: win.updateCue(player.position)
        onDurationChanged: resume.tryApply()
        onSeekableChanged: resume.tryApply()
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.PlayingState) resume.tryApply()
            else win.showControls()
        }
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.LoadedMedia || mediaStatus === MediaPlayer.BufferedMedia) resume.tryApply()
            else if (mediaStatus === MediaPlayer.EndOfMedia) { resume.save(); endCard.maybePlayNext() }
        }
        onTracksChanged: win.restoreTracks()
    }

    signal closed()
    onClosing: { resume.save(); player.stop(); win.closed() }

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

    PlayerResume {
        id: resume
        mediaPlayer: player
        infoHash: win.infoHash
        fileIndex: win.fileIndex
        formatHelper: playerFmt
        topInset: chrome.topVisible ? chrome.topHeight : 0
    }

    PlayerRunway {
        id: runway
        mediaPlayer: player
        infoHash: win.infoHash
        fileIndex: win.fileIndex
        windowVisible: win.visible
        formatHelper: playerFmt
        topInset: chrome.topVisible ? chrome.topHeight : 0
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
            onClicked: { resume.save(); win.openExternal(); win.close() }
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
        anchors.bottomMargin: (chrome.barVisible ? chrome.barHeight : 0) + 26
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

    // auto-hide the chrome after inactivity (both windowed and fullscreen, like
    // Stremio) — but never while paused, hovering the bar, or with a panel open.
    Timer {
        id: idle; interval: 3000
        onTriggered: {
            if (subPanel.open || optsPanel.open || chrome.barHovered) return
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
            onTriggered: { resume.save(); win.openExternal() }
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

    PlayerSkipChip {
        mediaPlayer: player
        chapters: win.chapters
        endCardVisible: win.showEndCard
        controlsShown: win.controlsShown
        onSkipped: win.showControls()
    }

    PlayerEndCard {
        id: endCard
        mediaPlayer: player
        infoHash: win.infoHash
        nextIdx: win.nextIdx
        autoplayNext: win.autoplayNext
        nextPoster: win.nextPoster
        nextTitle: win.nextTitle
        nextSubtitle: win.nextSubtitle
        formatHelper: playerFmt
    }

    PlayerChrome {
        id: chrome
        pw: win
        mediaPlayer: player
        formatHelper: playerFmt
        optionsPanel: optsPanel
        overflowMenu: moreMenu
    }

    PlayerShortcuts {
        pw: win
        optionsPanel: optsPanel
        subsPanel: subPanel
        extSubsActive: win.extSubsActive
    }

    Component.onCompleted: if (win.streamUrl.length > 0) player.play()
}
