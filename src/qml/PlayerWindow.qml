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
    property int resumeAtMs: 0
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
            win.flags = Qt.Window
            win.width = win.savedGeom.width > 0 ? win.savedGeom.width : 960
            win.height = win.savedGeom.height > 0 ? win.savedGeom.height : 600
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
            color: chip.active ? "#ffffff" : Theme.t1
            font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
        }
        MouseArea { id: cma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: chip.clicked() }
    }

    // slim, themed slider (no default "ball" handle)
    component PSlider: Slider {
        id: sl
        property real buffered: 0   // 0..1 downloaded-from-start, drawn dim behind the fill
        implicitHeight: 16
        background: Rectangle {
            x: sl.leftPadding; y: sl.topPadding + sl.availableHeight / 2 - height / 2
            width: sl.availableWidth; height: 4; radius: 2
            color: "#3a3a42"
            // downloaded layer (what's safe to seek to)
            Rectangle { width: Math.max(0, Math.min(1, sl.buffered)) * parent.width; height: parent.height; radius: 2; color: "#59ffffff" }
            // playback layer (what you're watching)
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
    function fmtBytes(b) {
        if (!b || b <= 0) return "0 MB"
        var u = ["KB", "MB", "GB", "TB"], i = -1
        do { b /= 1024; i++ } while (b >= 1024 && i < u.length - 1)
        return b.toFixed(b < 10 ? 1 : 0) + " " + u[i]
    }

    // download stats for the buffer bar + badge, polled while the player is open
    property var streamStats: ({})
    Timer {
        interval: 1000; repeat: true
        running: win.visible && typeof session !== "undefined"
        triggeredOnStart: true
        onTriggered: win.streamStats = session.streamFileStats(win.infoHash, win.fileIndex)
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
        onDurationChanged: {
            if (!win.resumed && duration > 0 && typeof settings !== "undefined") {
                win.resumed = true
                var saved = Number(settings.get(win.resumeKey) || 0)
                if (saved > 5000 && saved < duration - 15000) {
                    position = saved
                    win.resumeAtMs = saved
                    resumeCue.show()
                }
            }
        }
        onMediaStatusChanged: if (mediaStatus === MediaPlayer.EndOfMedia) { win.saveResume(); win.maybePlayNext() }
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

    // resume cue — a brief "↩ 12:34" pill so the viewer notices the movie picked
    // up where they left off (the jump is otherwise silent and easy to distrust)
    Rectangle {
        id: resumeCue
        function show() { opacity = 1; hideTimer.restart() }
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 28
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
    Rectangle {
        id: subPanel
        visible: false
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: bar.top
        width: Math.min(370, parent.width * 0.46)
        color: "#f2101013"
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: "#22ffffff" }

        function openPanel() {
            visible = true
            if (typeof subsearch !== "undefined")
                subsearch.searchFor(win.infoHash, win.fileIndex, win.mediaTitle)
        }

        property string panelError: ""
        property int downloadingIdx: -1
        Connections {
            target: typeof subsearch !== "undefined" ? subsearch : null
            ignoreUnknownSignals: true
            function onSubtitleReady(path) {
                subPanel.downloadingIdx = -1
                win.loadExternalSubs(path)
            }
            function onSearchError(message) {
                subPanel.downloadingIdx = -1
                subPanel.panelError = message === "no_sources"
                    ? (i18n.language, i18n.t("subsearch_no_sources"))
                    : ((i18n.language, i18n.t("subsearch_dl_failed")) + (message.length > 0 ? " — " + message : ""))
            }
            function onSearchingChanged() { if (subsearch.searching) subPanel.panelError = "" }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, i18n.t("subsearch_title"))
                    color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
                PChip { label: "✕"; onClicked: subPanel.visible = false }
            }

            // sync controls (visible once a subtitle is active)
            RowLayout {
                Layout.fillWidth: true
                visible: win.extSubsActive
                spacing: 8
                Text {
                    text: (i18n.language, i18n.t("subsearch_sync"))
                    color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
                }
                Item { Layout.fillWidth: true }
                PChip { label: "−0.5s"; onClicked: win.bumpSubOffset(-500) }
                Text {
                    text: (win.extSubOffset >= 0 ? "+" : "") + (win.extSubOffset / 1000).toFixed(1) + "s"
                    color: win.extSubOffset === 0 ? Theme.t4 : Theme.t1
                    font.pixelSize: 12; font.family: Theme.fontMono
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { win.extSubOffset = 0; win.updateCue(player.position) } }
                }
                PChip { label: "+0.5s"; onClicked: win.bumpSubOffset(500) }
            }
            Rectangle { Layout.fillWidth: true; visible: win.extSubsActive; implicitHeight: 1; color: "#22ffffff" }

            Spinner {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: 8
                visible: typeof subsearch !== "undefined" && subsearch.searching
                s: 24
            }
            Text {
                Layout.fillWidth: true
                visible: subPanel.panelError.length > 0
                text: subPanel.panelError
                color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontSans
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: typeof subsearch !== "undefined" && !subsearch.searching
                         && subsearch.results.length === 0 && subPanel.panelError.length === 0
                text: (i18n.language, i18n.t("subsearch_empty"))
                color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans
                wrapMode: Text.WordWrap
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 4
                model: typeof subsearch !== "undefined" ? subsearch.results : []
                boundsBehavior: Flickable.StopAtBounds
                delegate: Rectangle {
                    required property var modelData
                    required property int index
                    width: ListView.view.width
                    height: 46
                    radius: 8
                    color: rowMa.containsMouse ? "#26ffffff" : "#14ffffff"
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10; anchors.rightMargin: 10
                        spacing: 8
                        Rectangle {
                            implicitWidth: langText.width + 12; implicitHeight: 18; radius: 5
                            color: Theme.accentTint
                            Text { id: langText; anchors.centerIn: parent; text: modelData.language; color: Theme.accentText; font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1
                            Text {
                                Layout.fillWidth: true
                                text: modelData.name.length > 0 ? modelData.name : modelData.provider
                                color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
                                elide: Text.ElideMiddle
                            }
                            Text { text: modelData.provider; color: Theme.t4; font.pixelSize: 10; font.family: Theme.fontSans }
                        }
                        Spinner { visible: subPanel.downloadingIdx === index; s: 14 }
                    }
                    MouseArea {
                        id: rowMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            subPanel.downloadingIdx = index
                            subPanel.panelError = ""
                            subsearch.download(index)
                        }
                    }
                }
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

        // behind the controls so it only catches movement over EMPTY bar space —
        // the chips/icons on top keep their own hover (tooltips, volume reveal)
        MouseArea {
            id: barHover; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton
            onContainsMouseChanged: if (containsMouse) win.showControls()
            onPositionChanged: win.showControls()
        }

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
            PChip {
                Layout.alignment: Qt.AlignVCenter
                visible: win.nextIdx >= 0
                label: "⏭"
                onClicked: if (typeof session !== "undefined") session.playFile(win.infoHash, win.nextIdx)
            }

            Text { text: win.fmt(player.position); color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontMono }
            PSlider {
                id: seek
                Layout.fillWidth: true; Layout.alignment: Qt.AlignVCenter
                from: 0; to: Math.max(1, player.duration)
                value: player.position
                buffered: (win.streamStats && win.streamStats.buffered) || 0
                enabled: player.seekable
                onMoved: player.position = value
                // hover or drag → preview the target time above the cursor
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
            }
            Text { text: win.fmt(player.duration); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono }

            // download progress — the % informs at a glance; hover shows exact
            // size. Hidden once the file is fully downloaded (nothing to report).
            // download status as a distinct, dim metadata chip — set apart from
            // the playback time (which is plain text) so they don't read as one.
            Rectangle {
                id: dlChip
                Layout.alignment: Qt.AlignVCenter
                readonly property real prog: (win.streamStats && win.streamStats.progress) || 0
                readonly property real total: (win.streamStats && win.streamStats.totalBytes) || 0
                visible: total > 0
                implicitWidth: dlt.implicitWidth + 14
                implicitHeight: 20
                radius: 6
                color: dlMa.containsMouse ? "#26ffffff" : "#14ffffff"
                Text {
                    id: dlt
                    anchors.centerIn: parent
                    // downloading → "↓ 73%"; complete → the file size
                    text: dlChip.prog < 0.999 ? ("↓ " + Math.round(dlChip.prog * 100) + "%")
                                              : win.fmtBytes(dlChip.total)
                    color: dlMa.containsMouse ? Theme.t1 : Theme.t4
                    font.pixelSize: 10; font.weight: Font.Medium; font.family: Theme.fontMono
                }
                MouseArea { id: dlMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor }
                ToolTip.visible: dlMa.containsMouse
                ToolTip.delay: 250
                ToolTip.text: win.fmtBytes((win.streamStats && win.streamStats.downloadedBytes) || 0)
                              + " / " + win.fmtBytes(dlChip.total)
            }

            PChip {
                // always visible: the menu carries search/load even when the
                // file has no embedded tracks
                Layout.alignment: Qt.AlignVCenter
                active: player.activeSubtitleTrack >= 0 || win.extSubsActive
                label: (i18n.language, i18n.t("player_subs"))
                onClicked: subMenu.popup()
            }

            // volume: click the icon to mute/unmute; hover reveals a vertical slider
            Item {
                Layout.alignment: Qt.AlignVCenter
                implicitWidth: 22; implicitHeight: 22
                IconImg {
                    anchors.centerIn: parent
                    src: (win.muted || win.volume <= 0) ? "qrc:/icons/volume-mute.svg" : "qrc:/icons/volume.svg"
                    tint: volMa.containsMouse ? Theme.t1 : Theme.t2
                    s: 17
                }
                MouseArea {
                    id: volMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: win.muted = !win.muted
                }
                Rectangle {
                    visible: volMa.containsMouse || volPopMa.containsMouse || vsl.pressed
                    width: 32; height: 116; radius: 9
                    color: "#f50a0a0c"; border.color: Theme.hair; border.width: 1
                    anchors.bottom: parent.top; anchors.bottomMargin: 4
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
                            Rectangle {
                                anchors.bottom: parent.bottom
                                width: parent.width; height: vsl.position * parent.height
                                radius: 2; color: Theme.accent
                            }
                        }
                        handle: Rectangle {
                            x: vsl.leftPadding + vsl.availableWidth / 2 - width / 2
                            y: vsl.topPadding + (1 - vsl.position) * (vsl.availableHeight - height)
                            implicitWidth: 12; implicitHeight: 12; radius: 6; color: "#fff"
                        }
                    }
                }
            }

            // everything used occasionally lives behind one "more" menu — the
            // bar keeps only the controls touched every session
            PChip {
                Layout.alignment: Qt.AlignVCenter
                active: win.muted || player.playbackRate !== 1.0
                label: "⋯"
                onClicked: moreMenu.popup()
            }
            PChip { Layout.alignment: Qt.AlignVCenter; active: win.pipMode; label: "⧉"; onClicked: win.togglePip() }
            PChip { Layout.alignment: Qt.AlignVCenter; label: "⛶"; onClicked: win.toggleFullscreen() }
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
