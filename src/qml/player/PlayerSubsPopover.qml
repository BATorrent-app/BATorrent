// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Subtitles and audio in one popover over the CC button, in two screens.
// First: Subtitle / Audio tabs, the tracks with a tick on the one playing,
// "Search online…" and "Load file…", then size and delay. Second ("Search
// online", with a way back): language chips and the results from the
// subtitle sources, each with Use. Picks are saved per torrent, so the next
// file opens the same way. Replaces the options popover and the drawer.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtMultimedia
import "../theme"
import "../widgets"

Item {
    id: pop
    property var pw           // PlayerWindow root
    property var mediaPlayer
    property bool open: false
    property bool online: false
    property bool audioTab: false
    signal loadFile()

    function openPanel() { online = false; open = true; syncTracksFromPlayer() }
    function closePanel() { open = false }
    function openOnline() {
        open = true
        online = true
        if (typeof subsearch !== "undefined")
            subsearch.searchFor(pw.infoHash, pw.fileIndex, pw.mediaTitle, pop.subLangs)
    }

    visible: opacity > 0
    opacity: open ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }

    // ---- track state: own copy, written on click, synced from the player ----
    property int uiSubTrack: -1
    property int uiAudioTrack: 0
    function syncTracksFromPlayer() {
        if (!mediaPlayer) return
        uiSubTrack = mediaPlayer.activeSubtitleTrack
        uiAudioTrack = mediaPlayer.activeAudioTrack
    }
    function selectSubOff() {
        if (pop.pw) pop.pw.clearExternalSubs()
        if (mediaPlayer) mediaPlayer.activeSubtitleTrack = -1
        uiSubTrack = -1
        if (typeof settings !== "undefined" && pop.pw) settings.set("subTrack_" + pop.pw.infoHash, -1)
    }
    function selectSubTrack(index) {
        if (pop.pw) pop.pw.clearExternalSubs()
        if (mediaPlayer) mediaPlayer.activeSubtitleTrack = index
        uiSubTrack = index
        if (typeof settings !== "undefined" && pop.pw) settings.set("subTrack_" + pop.pw.infoHash, index)
    }
    function selectAudioTrack(index) {
        if (mediaPlayer) mediaPlayer.activeAudioTrack = index
        uiAudioTrack = index
        if (typeof settings !== "undefined" && pop.pw) settings.set("audioTrack_" + pop.pw.infoHash, index)
    }
    // Live on the open player and saved, so the next file opens the same.
    function setSubScale(v) {
        var c = Math.max(0.5, Math.min(2.5, Math.round(v * 10) / 10))
        pop.pw.subScale = c
        if (typeof settings !== "undefined") settings.set("subFontScale", Math.round(c * 100))
    }
    Connections {
        target: mediaPlayer
        enabled: pop.open
        function onActiveSubtitleTrackChanged() { pop.uiSubTrack = mediaPlayer.activeSubtitleTrack }
        function onActiveAudioTrackChanged() { pop.uiAudioTrack = mediaPlayer.activeAudioTrack }
    }

    // a track as a language name, with its title (codec, release) at the right
    function trackParts(meta, fallback) {
        if (!meta) return { l: fallback, k: "" }
        var lang = meta.stringValue(MediaMetaData.Language)
        var title = meta.stringValue(MediaMetaData.Title)
        if (lang.length > 0) return { l: lang, k: title }
        if (title.length > 0) return { l: title, k: "" }
        return { l: fallback, k: "" }
    }
    readonly property string audioCode: {
        if (!mediaPlayer || mediaPlayer.audioTracks.length === 0) return ""
        var c = pw ? pw.trackLang(mediaPlayer.audioTracks[Math.max(0, uiAudioTrack)]) : ""
        return c.toUpperCase()
    }

    // ---- online search ----
    readonly property var langCodes: ["pt", "en", "es", "fr", "de", "it", "ru", "ja", "zh"]
    property var subLangs: {
        var s = (typeof settings !== "undefined") ? settings.get("subtitleLangs") : ""
        if (s && s.length > 0) {
            var parts = ("" + s).split(",").filter(function (x) { return x.length > 0 })
            return parts.length > 0 ? [parts[0]] : ["en"]
        }
        var li = (typeof i18n !== "undefined") ? i18n.language : 0
        return [["en", "pt", "zh", "ja", "ru", "es", "de", "uk"][li] || "en"]
    }
    function selectLang(code) {
        if (!code || (subLangs.length === 1 && subLangs[0] === code)) return
        subLangs = [code]
        if (typeof settings !== "undefined") settings.set("subtitleLangs", code)
        if (typeof subsearch !== "undefined") subsearch.searchFor(pw.infoHash, pw.fileIndex, pw.mediaTitle, [code])
    }
    property string panelError: ""
    property int downloadingIdx: -1
    Connections {
        target: typeof subsearch !== "undefined" ? subsearch : null
        ignoreUnknownSignals: true
        // also the automatic pick when a file has no subtitle in the first
        // language (PlayerWindow starts that search)
        function onSubtitleReady(path) {
            pop.downloadingIdx = -1
            if (pop.online) pop.pw.extSubLang = pop.subLangs[0] || ""
            pop.pw.loadExternalSubs(path)
            if (pop.open) pop.closePanel()
        }
        function onSearchError(message) {
            pop.downloadingIdx = -1
            pop.panelError = message === "no_sources"
                ? (i18n.language, i18n.t("subsearch_no_sources"))
                : ((i18n.language, i18n.t("subsearch_dl_failed")) + (message.length > 0 ? " — " + message : ""))
        }
        function onSearchingChanged() { if (subsearch.searching) pop.panelError = "" }
    }

    // click-away
    MouseArea { anchors.fill: parent; enabled: pop.open; onClicked: pop.closePanel() }

    component Row36: Rectangle {
        id: r
        property string mark: ""
        property string label
        property string key: ""
        property bool chevron: false
        property bool dim: false
        signal activated()
        Layout.fillWidth: true
        implicitHeight: 36
        radius: 7
        color: r.mark.length > 0 ? Qt.rgba(1, 1, 1, 0.06) : (rMa.containsMouse ? Qt.rgba(1, 1, 1, 0.06) : "transparent")
        Text { x: 10; width: 14; anchors.verticalCenter: parent.verticalCenter; text: r.mark; color: Theme.accent; font.pixelSize: 13; font.weight: Font.Bold }
        Text {
            anchors.left: parent.left; anchors.leftMargin: 34
            anchors.right: keyT.left; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: r.label
            color: r.dim ? Theme.stageT4 : "#ffffff"
            font.pixelSize: 14; font.family: Theme.fontSans
            elide: Text.ElideRight
        }
        Text {
            id: keyT
            anchors.right: parent.right; anchors.rightMargin: r.chevron ? 28 : 10
            anchors.verticalCenter: parent.verticalCenter
            text: r.key
            color: Theme.stageT4; font.pixelSize: 12; font.family: Theme.fontSans
        }
        IconImg { visible: r.chevron; anchors.right: parent.right; anchors.rightMargin: 10; anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/chevron.svg"; rotation: -90; s: 10; tint: Theme.stageT4 }
        MouseArea { id: rMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: r.activated() }
    }
    component StepBtn: Rectangle {
        id: sb
        property string glyph
        property bool on: true
        signal clicked()
        implicitWidth: 26; implicitHeight: 26; radius: 6
        color: sbMa.containsMouse && on ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.08)
        opacity: on ? 1 : 0.4
        Text { anchors.centerIn: parent; text: sb.glyph; color: "#ffffff"; font.pixelSize: 14; font.family: Theme.fontSans }
        MouseArea { id: sbMa; anchors.fill: parent; enabled: sb.on; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: sb.clicked() }
    }
    component Hair: Rectangle { Layout.fillWidth: true; Layout.leftMargin: 4; Layout.rightMargin: 4; Layout.topMargin: 6; Layout.bottomMargin: 6; implicitHeight: 1; color: Qt.rgba(1, 1, 1, 0.08) }

    Item {
        id: card
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 24; anchors.bottomMargin: 76
        width: Math.min(340, parent.width - 48)
        height: Math.min(body.implicitHeight + 16, parent.height - 120)
        transform: Translate {
            y: pop.open ? 0 : 6
            Behavior on y { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
        }
        Rectangle {
            id: plate
            anchors.fill: parent
            radius: 12
            color: Qt.rgba(18 / 255, 18 / 255, 20 / 255, 0.96)
            border.color: Qt.rgba(1, 1, 1, 0.1); border.width: 1
        }
        MultiEffect {
            source: plate; anchors.fill: plate; z: -1
            shadowEnabled: true; shadowColor: Qt.rgba(0, 0, 0, 0.6); shadowBlur: 1.0; shadowVerticalOffset: 18; blurMax: 48
        }
        MouseArea { anchors.fill: parent }   // swallow clicks inside

        Flickable {
            anchors.fill: parent
            anchors.margins: 8
            clip: true
            contentWidth: width
            contentHeight: body.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            interactive: contentHeight > height

            ColumnLayout {
                id: body
                width: parent.width
                spacing: 0

                // ===== screen 1: tracks =====
                Rectangle {
                    visible: !pop.online
                    Layout.fillWidth: true
                    Layout.bottomMargin: 6
                    implicitHeight: 38
                    radius: 9
                    color: Qt.rgba(1, 1, 1, 0.05)
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 2
                        Repeater {
                            model: 2
                            Rectangle {
                                required property int index
                                readonly property bool on: (index === 1) === pop.audioTab
                                readonly property bool usable: index === 0 || (pop.mediaPlayer && pop.mediaPlayer.audioTracks.length > 0)
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                radius: 7
                                color: on ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: index === 0 ? (i18n.language, i18n.t("player_subs_tab"))
                                          : (i18n.language, i18n.t("player_audio")) + (pop.audioCode.length > 0 ? " · " + pop.audioCode : "")
                                    color: parent.on ? "#ffffff" : Theme.stageMuted
                                    font.pixelSize: 13; font.weight: parent.on ? Font.DemiBold : Font.Medium; font.family: Theme.fontSans
                                }
                                MouseArea { anchors.fill: parent; enabled: parent.usable; cursorShape: Qt.PointingHandCursor; onClicked: pop.audioTab = index === 1 }
                            }
                        }
                    }
                }
                // subtitles
                Row36 {
                    visible: !pop.online && !pop.audioTab
                    label: (i18n.language, i18n.t("player_subs_off"))
                    mark: pop.uiSubTrack < 0 && !pop.pw.extSubsActive ? "✓" : ""
                    onActivated: pop.selectSubOff()
                }
                Repeater {
                    model: !pop.online && !pop.audioTab && pop.mediaPlayer ? pop.mediaPlayer.subtitleTracks.length : 0
                    Row36 {
                        required property int index
                        readonly property var parts: pop.trackParts(pop.mediaPlayer.subtitleTracks[index], (i18n.language, i18n.t("player_subs")) + " " + (index + 1))
                        label: parts.l
                        key: parts.k
                        mark: !pop.pw.extSubsActive && pop.uiSubTrack === index ? "✓" : ""
                        onActivated: pop.selectSubTrack(index)
                    }
                }
                Row36 {
                    visible: !pop.online && !pop.audioTab && pop.pw.extSubsActive
                    label: pop.pw.extSubName
                    key: (i18n.language, i18n.t("player_subs_external"))
                    mark: "✓"
                    onActivated: pop.selectSubOff()
                }
                // audio
                Repeater {
                    model: !pop.online && pop.audioTab && pop.mediaPlayer ? pop.mediaPlayer.audioTracks.length : 0
                    Row36 {
                        required property int index
                        readonly property var parts: pop.trackParts(pop.mediaPlayer.audioTracks[index], (i18n.language, i18n.t("player_audio")) + " " + (index + 1))
                        label: parts.l
                        key: parts.k
                        mark: pop.uiAudioTrack === index ? "✓" : ""
                        onActivated: pop.selectAudioTrack(index)
                    }
                }
                Hair { visible: !pop.online && !pop.audioTab }
                Row36 {
                    visible: !pop.online && !pop.audioTab
                    label: (i18n.language, i18n.t("player_search_online"))
                    chevron: true
                    onActivated: pop.openOnline()
                }
                Row36 {
                    visible: !pop.online && !pop.audioTab
                    label: (i18n.language, i18n.t("player_load_subtitle"))
                    onActivated: { pop.closePanel(); pop.loadFile() }
                }
                Hair { visible: !pop.online && !pop.audioTab }
                RowLayout {
                    visible: !pop.online && !pop.audioTab
                    Layout.fillWidth: true
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    Layout.topMargin: 6; Layout.bottomMargin: 6
                    spacing: 8
                    Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("player_sub_size")); color: Theme.stageMuted; font.pixelSize: 13; font.family: Theme.fontSans }
                    StepBtn { glyph: "−"; onClicked: pop.setSubScale(pop.pw.subScale - 0.1) }
                    Text { Layout.preferredWidth: 44; horizontalAlignment: Text.AlignHCenter; text: Math.round(pop.pw.subScale * 100) + "%"; color: "#ffffff"; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum }
                    StepBtn { glyph: "+"; onClicked: pop.setSubScale(pop.pw.subScale + 0.1) }
                }
                // QtMultimedia gives no delay for a file's own subtitles; the
                // ones BATorrent loads itself (online or from a file) can shift
                RowLayout {
                    visible: !pop.online && !pop.audioTab
                    Layout.fillWidth: true
                    Layout.leftMargin: 10; Layout.rightMargin: 10
                    Layout.topMargin: 6; Layout.bottomMargin: 6
                    spacing: 8
                    Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("player_sub_delay")); color: Theme.stageMuted; font.pixelSize: 13; font.family: Theme.fontSans }
                    StepBtn { glyph: "−"; on: pop.pw.extSubsActive; onClicked: pop.pw.bumpSubOffset(-250) }
                    Text {
                        Layout.preferredWidth: 44; horizontalAlignment: Text.AlignHCenter
                        text: (pop.pw.extSubOffset / 1000).toLocaleString(Qt.locale(), 'f', 1) + " s"
                        color: pop.pw.extSubsActive ? "#ffffff" : Theme.stageT4; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                    StepBtn { glyph: "+"; on: pop.pw.extSubsActive; onClicked: pop.pw.bumpSubOffset(250) }
                    HoverHandler { id: delayHover }
                    ToolTip.visible: !pop.pw.extSubsActive && delayHover.hovered
                    ToolTip.text: (i18n.language, i18n.t("player_sub_delay_ext"))
                    ToolTip.delay: 300
                }

                // ===== screen 2: search online =====
                RowLayout {
                    visible: pop.online
                    Layout.fillWidth: true
                    Layout.margins: 4
                    Layout.bottomMargin: 8
                    spacing: 8
                    Rectangle {
                        implicitWidth: 30; implicitHeight: 30; radius: 7
                        color: backMa.containsMouse ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.06)
                        IconImg { anchors.centerIn: parent; src: "qrc:/icons/chevron.svg"; rotation: 90; s: 12; tint: "#ffffff" }
                        MouseArea { id: backMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: pop.online = false }
                        ToolTip.visible: backMa.containsMouse
                        ToolTip.text: (i18n.language, i18n.t("btn_back"))
                        ToolTip.delay: 500
                    }
                    Text { text: (i18n.language, i18n.t("player_search_online_title")); color: "#ffffff"; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                    Item { Layout.fillWidth: true }
                    Text { text: pop.pw ? pop.pw.episodeCode : ""; color: Theme.stageT4; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum }
                }
                Flow {
                    visible: pop.online
                    Layout.fillWidth: true
                    Layout.leftMargin: 4; Layout.rightMargin: 4; Layout.bottomMargin: 8
                    spacing: 4
                    Repeater {
                        model: pop.langCodes
                        Rectangle {
                            required property string modelData
                            readonly property bool on: (pop.subLangs || []).indexOf(modelData) >= 0
                            implicitWidth: lc.implicitWidth + 18; implicitHeight: 26; radius: 6
                            color: on ? "#f3f3f4" : (lcMa.containsMouse ? Qt.rgba(1, 1, 1, 0.12) : Qt.rgba(1, 1, 1, 0.06))
                            Text { id: lc; anchors.centerIn: parent; text: modelData.toUpperCase(); color: parent.on ? "#101014" : Theme.stageInk; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                            MouseArea { id: lcMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: pop.selectLang(modelData) }
                        }
                    }
                }
                Spinner {
                    visible: pop.online && typeof subsearch !== "undefined" && subsearch.searching
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8; Layout.bottomMargin: 8
                    s: 22
                }
                Text {
                    visible: pop.online && pop.panelError.length > 0
                    Layout.fillWidth: true; Layout.margins: 10
                    text: pop.panelError; color: Theme.stageT2; font.pixelSize: 12; font.family: Theme.fontSans; wrapMode: Text.WordWrap
                }
                Text {
                    visible: pop.online && typeof subsearch !== "undefined" && !subsearch.searching
                             && subsearch.results.length === 0 && pop.panelError.length === 0
                    Layout.fillWidth: true; Layout.margins: 10
                    text: (i18n.language, i18n.t("subsearch_empty")); color: Theme.stageMuted; font.pixelSize: 12; font.family: Theme.fontSans; wrapMode: Text.WordWrap
                }
                Repeater {
                    model: pop.online && typeof subsearch !== "undefined" ? subsearch.results : []
                    Rectangle {
                        required property var modelData
                        required property int index
                        Layout.fillWidth: true
                        implicitHeight: resCol.implicitHeight + 16
                        radius: 7
                        color: resMa.containsMouse ? Qt.rgba(1, 1, 1, 0.06) : "transparent"
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 10; anchors.rightMargin: 10
                            spacing: 10
                            ColumnLayout {
                                id: resCol
                                Layout.fillWidth: true
                                spacing: 2
                                Text { Layout.fillWidth: true; text: modelData.name.length > 0 ? modelData.name : modelData.provider; color: "#ffffff"; font.pixelSize: 13; font.family: Theme.fontSans; elide: Text.ElideMiddle }
                                Text { Layout.fillWidth: true; text: modelData.provider + (modelData.language ? " · " + modelData.language : ""); color: Theme.stageT4; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight }
                            }
                            Spinner { visible: pop.downloadingIdx === index; s: 14 }
                            Rectangle {
                                visible: pop.downloadingIdx !== index
                                implicitWidth: useT.implicitWidth + 20; implicitHeight: 26; radius: 6
                                color: useMa.containsMouse ? Qt.rgba(1, 1, 1, 0.18) : Qt.rgba(1, 1, 1, 0.1)
                                Text { id: useT; anchors.centerIn: parent; text: (i18n.language, i18n.t("player_sub_use")); color: "#ffffff"; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                                MouseArea { id: useMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { pop.downloadingIdx = index; pop.panelError = ""; subsearch.download(index) } }
                            }
                        }
                        MouseArea { id: resMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                    }
                }
            }
        }
    }
}
