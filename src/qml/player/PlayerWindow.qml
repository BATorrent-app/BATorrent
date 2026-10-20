// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Embedded video player. Plays a torrent file streamed from the
// local StreamServer (download-while-watch), with resume per infohash+file and
// an external-player fallback if the codec isn't supported.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtQuick.Effects
import QtMultimedia
import QtQuick.Dialogs
import Qt.labs.platform as Platform
import "../theme"
import "../widgets"

Window {
    id: win
    PlayerFormat { id: playerFmt }
    // 16:9: the picture fills the window and the chrome floats on it, no bars
    width: 1280; height: 720
    minimumWidth: 560; minimumHeight: 360
    color: "#000000"
    title: Theme.unifiedChrome ? "" : (win.mediaTitle.length > 0 ? ("BATorrent — " + win.mediaTitle) : "BATorrent")
    // standalone, non-transient window: otherwise macOS treats it as an
    // auxiliary window and won't enter the native fullscreen space (menu bar +
    // Dock stay on top). Expanded client area (macOS) lets the title band with
    // the quality/audio badges share the strip with the traffic lights.
    readonly property int baseFlags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    flags: baseFlags
    transientParent: null

    // Windows builds with QWindowKit: no system title bar. The window buttons
    // sit at the top right over the picture and fade with the controls; the
    // strip along the top drags the window.
    readonly property bool frameless: typeof framelessWindow !== "undefined" && framelessWindow === true
    Loader {
        id: framelessLoader
        active: win.frameless
        source: win.frameless ? "../AppFramelessAgent.qml" : ""
        onLoaded: { item.window = win; item.navBar = chrome }
    }
    // Called before the first show (MainWindowLoaders), so the frame never flashes.
    function prepareFrame() { if (framelessLoader.item) framelessLoader.item.attach() }
    function toggleMaximized() {
        if (win.visibility === Window.FullScreen) return
        if (win.visibility === Window.Maximized) win.showNormal()
        else win.showMaximized()
    }

    property string streamUrl: ""
    // Before there is anything to play: the wait, shown inside this window
    // (PlayerWaiting). { hash, title, subtitle, art, release, quality,
    // season, episode, candidates }; null once the stream opens.
    property var waiting: null
    property var waitInfo: ({})
    signal waitCancelled(bool backToFind)
    function startWaiting(wv) {
        waiting = wv
        waitInfo = ({})
        infoHash = wv.hash
        streamUrl = ""
        mediaTitle = wv.title || ""
        resolvedTitle = wv.title || ""
        resolvedSubtitle = wv.subtitle || ""
    }
    // ← / Esc / ✕ while waiting: stop waiting, keep the download, go back
    function cancelWait(backToFind) {
        if (!waiting) return
        var h = waiting.hash
        waiting = null
        if (typeof session !== "undefined") session.cancelWatch(h)
        waitCancelled(!!backToFind)
        close()
    }
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onWaitState(hash, st) { if (win.waiting && hash === win.waiting.hash) win.waitInfo = st }
    }
    property string localFile: ""   // on-disk path of the playing file (seek previews decode this, not the HTTP stream)
    readonly property bool ambientGlow: (typeof settings === "undefined") || settings.getBool("ambientGlow", true)
    property string mediaTitle: ""
    property string mediaFileName: ""
    // resolved display title (metadata) + "S4 · E10"/year; raw name lives in
    // mediaFileName (info tooltip). Falls back to mediaTitle if unresolved.
    property string resolvedTitle: ""
    property string resolvedSubtitle: ""
    readonly property string headerTitle: resolvedTitle.length > 0 ? resolvedTitle : mediaTitle
    // "S01E01", for the subtitle search header
    readonly property string episodeCode: {
        var m = /S(\d+)E(\d+)/i.exec(resolvedSubtitle) || /S(\d+)\s*·\s*E(\d+)/i.exec(resolvedSubtitle)
        if (!m) return ""
        var se = Number(m[1]), ep = Number(m[2])
        return "S" + (se < 10 ? "0" : "") + se + "E" + (ep < 10 ? "0" : "") + ep
    }
    readonly property string mediaQuality: playerFmt.qualityFromName(mediaFileName)
    readonly property string mediaAudio: playerFmt.audioFromName(mediaFileName)
    property string infoHash: ""
    property int fileIndex: 0
    property bool muted: false
    property real volume: 0.9
    property bool controlsShown: true

    // external subtitles (sidecar .srt/.vtt): rendered as a synced overlay,
    // independent of QtMultimedia's embedded-track support
    property var extCues: []
    property int extCueIdx: -1
    property string extCueText: ""
    property string extSubName: ""
    property string extSubLang: ""        // the language it was fetched in, for the CC button
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

    // Start each file on the viewer's languages. A track picked by hand for
    // this torrent wins; with none saved, the first preferred language the
    // file has. Subtitles stay off when the audio already speaks the first
    // subtitle language (unless subAlways), and with no embedded subtitle in
    // that language the best one online is fetched (autoSubs).
    property bool tracksRestored: false
    property bool autoSubPending: false
    function prefLangs(key) {
        return String(settings.get(key) || "").split(",").filter(function (c) { return c.length > 0 })
    }
    function trackLang(meta) {
        if (!meta) return ""
        var code = (typeof session !== "undefined")
                   ? session.langCode(Number(meta.value(MediaMetaData.Language)) || 0) : ""
        if (code.length > 0) return code
        // untagged tracks often carry the language in their title
        var t = String(meta.stringValue(MediaMetaData.Title)).toLowerCase()
        var names = { pt: ["portug", "brazil", "pt-br"], en: ["english"], es: ["spanish", "español", "latino"],
                      fr: ["french", "français"], de: ["german", "deutsch"], it: ["italian"],
                      ja: ["japanese", "日本"], ko: ["korean"], zh: ["chinese", "中文"], ru: ["russian"],
                      uk: ["ukrain"], tr: ["turkish"], pl: ["polish"], nl: ["dutch"], sv: ["swedish"],
                      ar: ["arabic"], hi: ["hindi"], th: ["thai"] }
        for (var c in names)
            for (var i = 0; i < names[c].length; i++)
                if (t.indexOf(names[c][i]) >= 0) return c
        return ""
    }
    function firstTrackIn(tracks, langs) {
        for (var l = 0; l < langs.length; l++)
            for (var i = 0; i < tracks.length; i++)
                if (win.trackLang(tracks[i]) === langs[l]) return { idx: i, lang: langs[l] }
        return null
    }
    function restoreTracks() {
        if (win.tracksRestored || typeof settings === "undefined") return
        if (player.audioTracks.length === 0 && player.subtitleTracks.length === 0) return
        win.tracksRestored = true

        var audioIdx = -1
        var at = settings.get("audioTrack_" + win.infoHash)
        if (at !== undefined && at !== "" && Number(at) >= 0 && Number(at) < player.audioTracks.length) {
            audioIdx = Number(at)
        } else {
            var a = win.firstTrackIn(player.audioTracks, win.prefLangs("audioLangs"))
            if (a) audioIdx = a.idx
        }
        if (audioIdx >= 0) player.activeAudioTrack = audioIdx

        var st = settings.get("subTrack_" + win.infoHash)
        if (st !== undefined && st !== "") {
            var sn = Number(st)
            // -1 = user chose "Subtitles off"; don't treat it as "unset"
            if (sn < 0)
                player.activeSubtitleTrack = -1
            else if (sn < player.subtitleTracks.length)
                player.activeSubtitleTrack = sn
            return
        }
        var subs = win.prefLangs("subLangs")
        if (subs.length === 0) return
        var audioLang = player.audioTracks.length > 0
                        ? win.trackLang(player.audioTracks[Math.max(0, audioIdx)]) : ""
        if (!settings.getBool("subAlways", false) && audioLang === subs[0]) {
            player.activeSubtitleTrack = -1
            return
        }
        var s = win.firstTrackIn(player.subtitleTracks, subs)
        if (s) player.activeSubtitleTrack = s.idx
        if (s && s.lang === subs[0]) return
        // nothing embedded in the first language: fetch the best one online
        if (settings.getBool("autoSubs", true) && typeof subsearch !== "undefined" && !win.extSubsActive) {
            console.info("[player] no embedded " + subs[0] + " subtitle, searching online")
            win.autoSubPending = true
            win.extSubLang = subs[0]
            subsearch.searchFor(win.infoHash, win.fileIndex, win.mediaTitle, [subs[0]])
        }
    }
    Connections {
        target: typeof subsearch !== "undefined" ? subsearch : null
        ignoreUnknownSignals: true
        function onSearchingChanged() {
            if (!win.autoSubPending || subsearch.searching) return
            if (subsearch.results.length > 0) subsearch.download(0)
            else win.autoSubPending = false
        }
        function onSubtitleReady(path) { win.autoSubPending = false }
        function onSearchError(message) { win.autoSubPending = false }
    }

    // picture-in-picture: shrink to a small always-on-top corner window
    property bool pipMode: false
    property rect savedGeom: Qt.rect(0, 0, 0, 0)
    function togglePip() {
        if (!win.pipMode) {
            win.savedGeom = Qt.rect(win.x, win.y, win.width, win.height)
            if (win.visibility === Window.FullScreen) win.visibility = Window.Windowed
            // the frameless build is already without a frame: only on top
            win.flags = win.frameless ? (win.baseFlags | Qt.WindowStaysOnTopHint)
                                      : (Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint)
            var scr = win.screen
            win.width = 400; win.height = 225
            win.x = scr.virtualX + scr.width - 416
            win.y = scr.virtualY + scr.height - 241
            win.pipMode = true
        } else {
            win.flags = win.stayOnTop ? (win.baseFlags | Qt.WindowStaysOnTopHint) : win.baseFlags
            win.width = win.savedGeom.width > 0 ? win.savedGeom.width : 1280
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
    function clearExternalSubs() { extCues = []; extCueIdx = -1; extCueText = ""; extSubName = ""; extSubOffset = 0; extSubLang = "" }
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
    // The next episode when it is another torrent: { season, episode, release }.
    property var nextProvider: null
    property var externalNext: null
    readonly property bool hasNext: nextIdx >= 0 || externalNext !== null
    // A film the library does not have is offered, never opened on its own.
    readonly property bool nextAutoplays: !(externalNext && externalNext.film && !externalNext.owned)
    function nameNextFilm() {
        var f = win.externalNext.film
        win.nextTitle = f.title || ""
        win.nextSubtitle = ""
        win.nextPoster = f.poster || ""
        endCard.nextMeta = f.year ? String(f.year) : ""
    }
    property bool autoplayNext: (typeof settings === "undefined") || settings.getBool("autoplayNext", true)
    property var chapters: []
    property bool autoSkip: false
    Connections {
        target: typeof search !== "undefined" ? search : null
        ignoreUnknownSignals: true
        // the film after this one in its TMDB collection, which the library lacks
        function onNextFilmFound(hash, film) {
            if (hash !== win.infoHash || win.nextIdx >= 0 || win.externalNext) return
            win.externalNext = { film: film, owned: false }
            win.nameNextFilm()
        }
        function onNextEpisodeFound(hash, fileIdx, next) {
            if (hash !== win.infoHash || fileIdx !== win.fileIndex || win.nextIdx >= 0 || win.externalNext) return
            win.externalNext = next
            win.nextTitle = next.show || ""
            win.nextSubtitle = next.label || ""
            win.nextPoster = typeof session !== "undefined" ? session.posterForHash(hash) : ""
        }
    }
    Connections {
        target: typeof session !== "undefined" ? session : null
        ignoreUnknownSignals: true
        function onSkipTimesReady(hash, fileIdx, list) {
            if (hash !== win.infoHash || fileIdx !== win.fileIndex) return
            if (win.chapters.some(function (c) { return c.kind })) return
            win.chapters = win.chapters.concat(list)
        }
    }
    // The episode's real season and name from TMDB: "S16E11 · Name". A fansub's
    // absolute number ("Bleach - 353") walks the seasons until it lands.
    property var epq: ({ tmdb: 0 })
    function resolveEpisode(pt) {
        win.epq = { tmdb: 0 }
        if (!(pt.showTmdb > 0) || !(pt.episode > 0) || typeof discovery === "undefined") return
        win.epq = { tmdb: pt.showTmdb, absolute: !!pt.absolute, want: pt.episode,
                    season: pt.absolute ? 1 : pt.season, before: 0 }
        discovery.fetchEpisodes(pt.showTmdb, win.epq.season)
    }
    // the next file in this torrent, named the same way ("Next: S01E354 · …")
    function nameNext(season, n, eps) {
        if (win.nextIdx < 0) return
        for (var i = 0; i < eps.length; i++)
            if (eps[i].episode === n && eps[i].name) {
                win.nextSubtitle = win.epCode(season, n) + " · " + eps[i].name
                return
            }
    }
    function epCode(season, episode) {
        return "S" + (season < 10 ? "0" : "") + season + "E" + (episode < 10 ? "0" : "") + episode
    }
    Connections {
        target: typeof discovery !== "undefined" ? discovery : null
        ignoreUnknownSignals: true
        function onEpisodesReady(id, season, eps) {
            var q = win.epq
            if (!q.tmdb || id !== q.tmdb || season !== q.season) return
            if (!q.absolute) {
                for (var i = 0; i < eps.length; i++)
                    if (eps[i].episode === q.want && eps[i].name)
                        win.resolvedSubtitle = win.epCode(season, q.want) + " · " + eps[i].name
                win.nameNext(season, q.want + 1, eps)
                win.epq = { tmdb: 0 }
                return
            }
            if (eps.length === 0 || season > 60) { win.epq = { tmdb: 0 }; return }
            if (q.before + eps.length >= q.want) {
                var n = q.want - q.before
                var name = ""
                for (var j = 0; j < eps.length; j++) if (eps[j].episode === n) name = eps[j].name || ""
                win.resolvedSubtitle = win.epCode(season, n) + (name.length > 0 ? " · " + name : "")
                win.nameNext(season, n + 1, eps)
                win.epq = { tmdb: 0 }
                return
            }
            win.epq = { tmdb: q.tmdb, absolute: true, want: q.want, season: season + 1, before: q.before + eps.length }
            discovery.fetchEpisodes(q.tmdb, season + 1)
        }
    }
    property string nextPoster: ""
    property string nextTitle: ""
    property string nextSubtitle: ""

    readonly property bool stillDownloading: runway.stillDownloading
    readonly property real downloadedToMs: runway.downloadedToMs
    readonly property real bufferedAheadMs: runway.bufferedAheadMs
    readonly property var streamStats: runway.streamStats
    readonly property real bufferedTo: runway.bufferedTo
    readonly property real bufferedFrom: runway.bufferedFrom
    readonly property bool starved: runway.starved
    readonly property bool showEndCard: endCard.showEndCard

    function openMedia(url, title, hash, fileIdx) {
        win.waiting = null
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
        win.resolveEpisode(pt)
        resume.prepareOpen()
        runway.reset()
        retry.reset()
        win.chapters = (typeof session !== "undefined") ? session.mkvChapters(hash, fileIdx) : []
        win.autoSkip = typeof settings !== "undefined" && settings.getBool("autoSkipIntro", false)
        // no opening/ending in the file's own chapters: ask AniSkip (anime only)
        if (typeof session !== "undefined" && !win.chapters.some(function (c) { return c.kind }))
            session.fetchSkipTimes(hash, fileIdx)
        win.nextIdx = (typeof session !== "undefined") ? session.nextEpisode(hash, fileIdx) : -1
        win.externalNext = (win.nextIdx < 0 && win.nextProvider)
                           ? win.nextProvider.nextEpisodeFor(hash, fileIdx) : null
        // a film: the next one of its TMDB collection, the library's copy first
        var filmAsked = false
        if (win.nextIdx < 0 && !win.externalNext && typeof session !== "undefined") {
            var nf = session.nextFilmInLibrary(hash)
            if ((nf.infoHash || "").length > 0) win.externalNext = { film: nf, owned: true }
            else if ((nf.collectionId || 0) > 0 && typeof search !== "undefined") {
                search.findNextFilm(hash, nf.collectionId, nf.tmdbId)
                filmAsked = true
            }
        }
        // not in this torrent and no series page to ask: the stream addons
        if (win.nextIdx < 0 && !win.externalNext && !filmAsked && typeof search !== "undefined")
            search.findNextEpisode(hash, fileIdx)
        endCard.reset()
        if (win.nextIdx >= 0 && typeof session !== "undefined") {
            var np = session.playerTitle(hash, win.nextIdx)
            win.nextTitle = np.title || ""
            win.nextSubtitle = np.subtitle || ""
            win.nextPoster = session.posterForHash(hash)
        } else { win.nextTitle = ""; win.nextSubtitle = ""; win.nextPoster = "" }
        endCard.nextMeta = ""
        if (win.externalNext && win.externalNext.film) win.nameNextFilm()
        win.tracksRestored = false
        win.autoSubPending = false
        win.loadSubStyle()
        win.clearExternalSubs()
        if (typeof session !== "undefined") {
            var sc = session.findSidecarSubtitle(hash, fileIdx)
            if (sc.length > 0) win.loadExternalSubs(sc)
        }
        player.play()
    }

    function playNext() {
        if (win.nextIdx >= 0) {
            // Same window, next file: the pack's other episodes sit at priority
            // 0 until fileStreamUrl wants this one.
            if (typeof session === "undefined") return
            var idx = win.nextIdx
            var url = session.fileStreamUrl(win.infoHash, idx)
            if (url.length > 0) win.openMedia(url, win.mediaTitle, win.infoHash, idx)
            return
        }
        var next = win.externalNext
        if (!next) return
        resume.save()
        // the next film of the collection: played when the library has it, else
        // its page in Find, to pick a release
        if (next.film) {
            var film = next.film
            var provider0 = win.nextProvider
            if (next.owned && typeof session !== "undefined") { win.close(); session.playByHash(film.infoHash); return }
            if (provider0) { win.close(); provider0.showFilm(film) }
            return
        }
        // a release the addons found: added and watched like a pick on the page
        if (next.magnet && typeof search !== "undefined") {
            win.close()
            search.playFoundEpisode(next)
            return
        }
        if (!win.nextProvider) return
        var provider = win.nextProvider
        win.close()
        provider.playNextEpisode(next)
    }

    PlayerBackend {
        id: backend
        source: win.streamUrl
        videoOutput: videoOut
        volume: win.volume
        muted: win.muted
    }
    readonly property var player: backend.player
    property bool decoderCrashed: false

    Connections {
        target: win.player
        function onPositionChanged() { win.updateCue(win.player.position) }
        function onDurationChanged() { resume.tryApply() }
        function onSeekableChanged() { resume.tryApply() }
        function onPlaybackStateChanged() {
            if (win.player.playbackState === MediaPlayer.PlayingState) resume.tryApply()
            else win.showControls()
        }
        function onMediaStatusChanged() {
            var st = win.player.mediaStatus
            if (st === MediaPlayer.LoadedMedia || st === MediaPlayer.BufferedMedia) resume.tryApply()
            else if (st === MediaPlayer.EndOfMedia) {
                resume.save()
                if (win.nextIdx < 0 && win.externalNext && win.autoplayNext && win.nextAutoplays) win.playNext()
                else endCard.maybePlayNext()
            }
        }
        function onTracksChanged() { win.restoreTracks() }
    }
    Connections {
        target: win.player
        ignoreUnknownSignals: true   // only the isolated player has it
        function onDecoderCrashed() { win.decoderCrashed = true }
        function onSourceChanged() { win.decoderCrashed = false }
    }

    PlayerRetry {
        id: retry
        mediaPlayer: player
        stillDownloading: win.stillDownloading
        // Rebound, not assigned: the next episode still has to reach the player.
        onReloadRequested: {
            backend.source = ""
            backend.source = Qt.binding(function () { return win.streamUrl })
            player.play()
        }
    }

    signal closed()
    onClosing: {
        // Esc and ✕ while waiting: the same as ←, back to the releases
        if (win.waiting) {
            if (typeof session !== "undefined") session.cancelWatch(win.waiting.hash)
            win.waiting = null
            win.waitCancelled(false)
        }
        resume.save(); player.stop(); win.closed()
    }

    Rectangle { anchors.fill: parent; color: "#000000" }

    // ambient glow: a blurred, upscaled copy of the frame bleeding into the
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

    // ---- picture: aspect, rotation, screenshot, always on top ----
    // fit/fill/stretch use the output's own modes; a forced ratio sizes the
    // output to that ratio inside the window and stretches the frame into it
    readonly property var aspectModes: ["fit", "fill", "stretch", "16:9", "4:3", "21:9"]
    property string aspectMode: "fit"
    property int videoRotation: 0
    property bool stayOnTop: false
    function cycleAspect() {
        var i = aspectModes.indexOf(aspectMode)
        aspectMode = aspectModes[(i + 1) % aspectModes.length]
        win.notice((i18n.language, i18n.t("player_aspect")).arg(win.aspectLabel(aspectMode)))
    }
    function aspectLabel(m) {
        if (m === "fit") return i18n.t("player_aspect_fit")
        if (m === "fill") return i18n.t("player_aspect_fill")
        if (m === "stretch") return i18n.t("player_aspect_stretch")
        return m
    }
    function rotate() { videoRotation = (videoRotation + 90) % 360 }
    function toggleOnTop() {
        stayOnTop = !stayOnTop
        if (!win.pipMode) win.flags = stayOnTop ? (win.baseFlags | Qt.WindowStaysOnTopHint) : win.baseFlags
    }
    function screenshot() {
        var dir = String(Platform.StandardPaths.writableLocation(Platform.StandardPaths.PicturesLocation))
                  .replace(/^file:\/\/\/?/, Qt.platform.os === "windows" ? "" : "/")
        var name = (win.headerTitle || "BATorrent").replace(/[\\/:*?"<>|]/g, " ").trim()
        var ms = player.position, sec = Math.floor(ms / 1000)
        var stamp = Math.floor(sec / 3600) + "h" + ("0" + Math.floor(sec % 3600 / 60)).slice(-2) + "m"
                    + ("0" + sec % 60).slice(-2) + "s"
        var path = decodeURIComponent(dir) + "/" + name + " " + stamp + ".png"
        videoOut.grabToImage(function (result) {
            win.notice(result.saveToFile(path)
                       ? (i18n.language, i18n.t("player_shot_saved")).arg(path)
                       : (i18n.language, i18n.t("player_shot_failed")))
        })
    }
    // a line of feedback over the video for actions with no other trace
    property string noticeText: ""
    function notice(t) { noticeText = t; noticeTimer.restart() }
    Timer { id: noticeTimer; interval: 2600; onTriggered: win.noticeText = "" }

    VideoOutput {
        id: videoOut
        readonly property real forced: {
            var m = win.aspectMode
            if (m === "16:9") return 16 / 9
            if (m === "4:3") return 4 / 3
            if (m === "21:9") return 21 / 9
            return 0
        }
        // a quarter turn swaps which side the ratio is measured along
        readonly property bool sideways: win.videoRotation % 180 !== 0
        anchors.centerIn: parent
        readonly property real ratio: forced > 0 ? (sideways ? 1 / forced : forced) : 0
        width: ratio > 0 ? Math.min(parent.width, parent.height * ratio) : parent.width
        height: ratio > 0 ? width / ratio : parent.height
        orientation: win.videoRotation
        // video fills the window; the chrome floats over it (Stremio-style) so
        // the gradient scrims read as depth, not a flat deck on a black frame
        fillMode: win.aspectMode === "fill" ? VideoOutput.PreserveAspectCrop
                : (win.aspectMode === "fit" ? VideoOutput.PreserveAspectFit : VideoOutput.Stretch)
    }
    Rectangle {
        z: 58
        visible: win.noticeText.length > 0
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top; anchors.topMargin: 72
        width: Math.min(parent.width - 48, noticeLbl.implicitWidth + 28)
        height: noticeLbl.implicitHeight + 16
        radius: 8
        color: "#e6101014"
        border.color: Theme.hair; border.width: 1
        Text {
            id: noticeLbl
            anchors.centerIn: parent
            width: Math.min(implicitWidth, parent.parent.width - 76)
            text: win.noticeText
            color: Theme.t1; font.pixelSize: 13; font.family: Theme.fontSans
            elide: Text.ElideMiddle
        }
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
        controlsShown: win.controlsShown
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
        BusyIndicator { Layout.alignment: Qt.AlignHCenter; running: !retry.failed }
        Text {
            Layout.alignment: Qt.AlignHCenter
            color: "#e8e8ea"; font.pixelSize: 14; font.family: Theme.fontSans
            text: !retry.failed ? (i18n.language, i18n.t("player_buffering"))
                  : win.decoderCrashed ? (i18n.language, i18n.t("player_decoder_crashed"))
                  : retry.formatProblem ? (i18n.language, i18n.t("player_error"))
                  : (i18n.language, i18n.t("player_stream_failed"))
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            visible: retry.failed
            spacing: 10
            BtnFlat {
                visible: !retry.formatProblem
                primary: true
                text: (i18n.language, i18n.t("player_try_again"))
                onClicked: retry.retryNow()
            }
            BtnFlat {
                primary: retry.formatProblem
                text: (i18n.language, i18n.t("player_open_external"))
                onClicked: { resume.save(); win.openExternal(); win.close() }
            }
        }
    }

    // Subtitle overlay for both kinds: an external file's cue, else the
    // file's own track (read as ASS by the isolated player). No box behind it
    // unless the user asked for one: an outline and a soft shadow. It rises
    // above the controls when they show; an \an7-9 line sits at the top, as
    // the subtitle asks.
    readonly property string subLine: win.extCueText.length > 0 ? win.extCueText
                                      : (player.subtitleHtml !== undefined ? player.subtitleHtml : "")
    readonly property bool subTop: win.extCueText.length === 0 && player.subtitleAlign !== undefined
                                   && player.subtitleAlign >= 7
    Item {
        id: subBox
        visible: win.subLine.length > 0
        width: parent.width
        height: subText.implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter
        y: win.subTop ? Math.max(videoOut.y, 0) + 64
                      : Math.min(videoOut.y + videoOut.height, win.height) - height - (win.controlsShown ? 128 : 48)
        Behavior on y { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
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
            text: win.subLine
            textFormat: Text.StyledText
            color: win.subColor
            style: win.subBgOpacity > 0 ? Text.Normal : Text.Outline
            styleColor: "#000000"
            font.pixelSize: Math.max(14, Math.round(win.height * 0.042 * win.subScale))
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            width: Math.min(win.width * 0.86, 1100)
            layer.enabled: win.subBgOpacity === 0
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: "#000000"
                shadowOpacity: 0.9
                shadowBlur: 0.35
                shadowVerticalOffset: 2
                blurMax: 8
            }
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
    // the chrome turns the matching skip ring, for the keys as well as the buttons
    signal skipped(int ms)
    function seekBy(ms) {
        if (!player.seekable) return
        player.position = Math.max(0, Math.min(player.duration, player.position + ms))
        win.skipped(ms)
    }
    function bumpVolume(d) { win.muted = false; win.volume = Math.max(0, Math.min(1, win.volume + d)) }
    function seekFraction(f) { if (player.seekable && player.duration > 0) player.position = Math.round(player.duration * f) }
    // One frame at 24 fps; the frame rate isn't exposed, and this lands on the
    // next or previous frame of anything from 24 to 30.
    function frameStep(dir) {
        if (!player.seekable) return
        if (player.playbackState === MediaPlayer.PlayingState) player.pause()
        player.position = Math.max(0, Math.min(player.duration, player.position + dir * 42))
    }
    function subtitleTrackCount() { return player.subtitleTracks.length }
    function audioTrackCount() { return player.audioTracks.length }
    function activeSubtitle() { return player.activeSubtitleTrack }
    function activeAudio() { return Math.max(0, player.activeAudioTrack) }
    function trackLabel(kind) {
        if (kind === "audio")
            return i18n.t("player_audio") + ": " + win.trackName(player.audioTracks[win.activeAudio()],
                                                              i18n.t("player_audio") + " " + (win.activeAudio() + 1))
        var s = player.activeSubtitleTrack
        return i18n.t("player_subs") + ": " + (s < 0 ? i18n.t("player_subs_off")
               : win.trackName(player.subtitleTracks[s], i18n.t("player_subs") + " " + (s + 1)))
    }
    property bool helpOpen: false
    function showControls() { win.controlsShown = true; idle.restart() }

    // auto-hide the chrome after inactivity (both windowed and fullscreen, like
    // Stremio): but never while paused, hovering the bar, or with a panel open.
    Timer {
        id: idle; interval: 3000
        onTriggered: {
            if (optsPanel.open || morePanel.open || chrome.barHovered) return
            if (player.playbackState !== MediaPlayer.PlayingState) return
            win.controlsShown = false
        }
    }
    onFullscreenChanged: { win.controlsShown = true; idle.restart() }

    // audio · subtitles · speed live in one options panel (see below); the
    // "…": picture, window and the odd actions, in sections
    PlayerMorePanel {
        id: morePanel
        anchors.fill: parent
        z: 56
        pw: win
    }
    function openExternalSaved() { resume.save(); win.openExternal() }

    FileDialog {
        id: subFileDlg
        title: (i18n.language, i18n.t("player_load_subtitle"))
        nameFilters: [(i18n.language, i18n.t("filter_subtitle_files"))]
        onAccepted: win.loadExternalSubs(selectedFile.toString())
    }

    // subtitles and audio: one popover over the CC button, in two screens
    PlayerSubsPopover {
        id: optsPanel
        anchors.fill: parent
        z: 60
        pw: win
        mediaPlayer: player
        onLoadFile: subFileDlg.open()
    }

    PlayerSkipChip {
        id: skipChip
        mediaPlayer: player
        chapters: win.chapters
        autoSkip: win.autoSkip
        endCardVisible: win.showEndCard
        controlsShown: win.controlsShown
        onSkipped: win.showControls()
    }

    PlayerEndCard {
        id: endCard
        mediaPlayer: player
        infoHash: win.infoHash
        nextIdx: win.nextIdx
        hasNext: win.hasNext
        autoplayNext: win.autoplayNext
        nextPoster: win.nextPoster
        nextTitle: win.nextTitle
        nextSubtitle: win.nextSubtitle
        canAutoplay: win.nextAutoplays
        actionLabel: win.nextAutoplays ? "" : (i18n.language, i18n.t("player_find_next_film"))
        chapters: win.chapters
        formatHelper: playerFmt
        onPlayNextRequested: win.playNext()
    }

    PlayerWaiting {
        anchors.fill: parent
        pw: win
    }

    PlayerChrome {
        id: chrome
        pw: win
        mediaPlayer: player
        formatHelper: playerFmt
        optionsPanel: optsPanel
        overflowMenu: morePanel
    }

    // ---- keyboard help ----
    Rectangle {
        anchors.fill: parent
        z: 70
        visible: win.helpOpen
        color: "#b3000000"
        MouseArea { anchors.fill: parent; onClicked: win.helpOpen = false }
        Rectangle {
            anchors.centerIn: parent
            width: Math.min(parent.width - 48, 560)
            height: Math.min(parent.height - 48, helpCol.implicitHeight + 40)
            radius: 12
            color: "#f50a0a0c"
            border.color: Theme.hair; border.width: 1
            clip: true
            MouseArea { anchors.fill: parent }
            ColumnLayout {
                id: helpCol
                anchors.fill: parent
                anchors.margins: 20
                spacing: 8
                Text {
                    text: (i18n.language, i18n.t("help_title"))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    Layout.bottomMargin: 6
                }
                GridLayout {
                    columns: 2
                    columnSpacing: 24
                    rowSpacing: 7
                    Repeater {
                        model: [
                            ["Space", "help_play"], ["←  →", "help_seek"], ["Shift + ←  →", "help_seek_big"],
                            ["0 – 9", "help_jump"], [",  .", "help_frame"], ["↑  ↓", "help_volume"],
                            ["M", "help_mute"], ["N", "player_next"], ["S", "help_sub_cycle"], ["A", "help_audio_cycle"],
                            ["[  ]", "help_sub_delay"], ["Z", "help_aspect"], ["R", "player_rotate"],
                            ["Ctrl + S", "player_screenshot"], ["T", "player_on_top"], ["P", "player_pip"],
                            ["F", "player_fullscreen"], ["Esc", "help_close"]
                        ]
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 10
                            Rectangle {
                                Layout.preferredWidth: 104
                                Layout.preferredHeight: 24
                                radius: 6
                                color: Theme.field
                                border.color: Theme.hair; border.width: 1
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData[0]
                                    color: Theme.t1; font.pixelSize: 12; font.weight: Font.DemiBold
                                    font.family: Theme.fontSans
                                }
                            }
                            Text {
                                Layout.fillWidth: true
                                text: (i18n.language, i18n.t(modelData[1]))
                                color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                        }
                    }
                }
            }
        }
    }

    PlayerShortcuts {
        pw: win
        morePanel: morePanel
        optionsPanel: optsPanel
        skipChip: skipChip
        extSubsActive: win.extSubsActive
    }

    Component.onCompleted: if (win.streamUrl.length > 0) player.play()
}
