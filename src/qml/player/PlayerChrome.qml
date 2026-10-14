// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The player's chrome, floating over the picture and fading with it: the
// title at the top left (show, episode, quality, how much is still coming),
// the window buttons at the top right when the window draws its own frame,
// and at the bottom the seek row and one line of controls. Left: back 10 s,
// play, forward 10 s, next and what it is. Right: speed (only the 1× button),
// subtitles and audio, volume, more, fullscreen.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import QtMultimedia
import "../theme"
import "../widgets"

Item {
    id: root
    anchors.fill: parent

    property var pw
    property var mediaPlayer
    property var formatHelper
    // Named differently from host ids: same-name bindings self-shadow to undefined.
    property var optionsPanel
    property var overflowMenu

    readonly property bool shown: !!pw && pw.controlsShown
    readonly property bool topVisible: topBar.visible
    readonly property real topHeight: 64
    readonly property bool barVisible: bar.visible
    // where the bottom chrome ends, for the subtitle and the chips above it
    readonly property real barHeight: 112
    readonly property bool barHovered: barHover.containsMouse || speedPop.visible
    readonly property alias titleStrip: topStrip
    readonly property alias minButton: minBtn
    readonly property alias maxButton: maxBtn
    readonly property alias closeButton: closeBtn

    readonly property string subBadge: {
        if (!pw || !mediaPlayer) return ""
        if (pw.extSubsActive) return pw.extSubLang.length > 0 ? pw.extSubLang.toUpperCase() : "ON"
        var s = mediaPlayer.activeSubtitleTrack
        // the decoder can show a file's default track while reporting none
        if (s < 0 && pw.subLine.length > 0) return "ON"
        if (s < 0) return (i18n.language, i18n.t("player_subs_off_short"))
        var c = pw.trackLang(mediaPlayer.subtitleTracks[s])
        return c.length > 0 ? c.toUpperCase() : String(s + 1)
    }

    component CtlBtn: Item {
        id: cb
        property string icon
        property int s: 20
        property string tip
        property bool on: false
        signal clicked()
        implicitWidth: 40; implicitHeight: 40
        Rectangle {
            anchors.fill: parent
            radius: 8
            color: cb.on ? Qt.rgba(1, 1, 1, 0.14) : (cbMa.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent")
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
        }
        IconImg { anchors.centerIn: parent; src: cb.icon; s: cb.s; tint: "#ffffff" }
        scale: cbMa.pressed ? 0.92 : 1
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
        MouseArea { id: cbMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: cb.clicked() }
        ToolTip.visible: cbMa.containsMouse && cb.tip.length > 0
        ToolTip.text: cb.tip
        ToolTip.delay: 500
        Accessible.role: Accessible.Button
        Accessible.name: cb.tip
        Accessible.onPressAction: cb.clicked()
    }
    component WinBtn: Rectangle {
        id: wb
        property string glyph
        signal clicked()
        width: 46; height: 32
        color: wbMa.containsMouse ? (glyph === "close" ? Theme.winClose : Qt.rgba(1, 1, 1, 0.12)) : "transparent"
        Rectangle { visible: wb.glyph === "min"; anchors.centerIn: parent; width: 10; height: 1; color: "#ffffff" }
        Rectangle { visible: wb.glyph === "max"; anchors.centerIn: parent; width: 10; height: 10; radius: 1; color: "transparent"; border.width: 1; border.color: "#ffffff" }
        Item {
            visible: wb.glyph === "restore"
            anchors.centerIn: parent
            width: 10; height: 10
            Rectangle { x: 2; width: 8; height: 8; radius: 1; color: "transparent"; border.width: 1; border.color: "#ffffff" }
            Rectangle { y: 2; width: 8; height: 8; radius: 1; color: "#000000"; border.width: 1; border.color: "#ffffff" }
        }
        Item {
            visible: wb.glyph === "close"
            anchors.centerIn: parent
            width: 13; height: 13
            Rectangle { anchors.centerIn: parent; width: 14; height: 1; rotation: 45; color: "#ffffff"; antialiasing: true }
            Rectangle { anchors.centerIn: parent; width: 14; height: 1; rotation: -45; color: "#ffffff"; antialiasing: true }
        }
        MouseArea { id: wbMa; anchors.fill: parent; hoverEnabled: true; onClicked: wb.clicked() }
    }

    // ---- top: title, and the window buttons when the frame is ours ----
    Item {
        id: topBar
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        height: 120
        opacity: root.shown ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.7) }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }
    }
    // The strip that drags the window (double click maximizes). With our own
    // frame the window agent owns it; otherwise Qt moves the window.
    Item {
        id: topStrip
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        height: 56
        MouseArea {
            anchors.fill: parent
            enabled: !!pw && !pw.fullscreen && !pw.frameless
            acceptedButtons: Qt.LeftButton
            onPressed: if (pw) pw.startSystemMove()
            onDoubleClicked: if (pw) pw.toggleMaximized()
        }
    }
    RowLayout {
        id: titleRow
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: 24; anchors.rightMargin: winButtons.visible ? 150 + 24 : 24
        y: 18
        spacing: 12
        opacity: topBar.opacity
        visible: topBar.visible && !!pw && pw.headerTitle.length > 0
        Text {
            text: pw ? pw.headerTitle.replace(/\s*\(\d{4}\)\s*$/, "") : ""
            color: "#ffffff"; font.pixelSize: 16; font.weight: Font.DemiBold; font.family: Theme.fontSans
            elide: Text.ElideRight
            Layout.maximumWidth: root.width * 0.35
        }
        Text {
            visible: text.length > 0
            text: pw ? pw.resolvedSubtitle : ""
            color: Theme.stageT2; font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum
            elide: Text.ElideRight
            Layout.maximumWidth: root.width * 0.35
        }
        Rectangle {
            visible: !!pw && pw.mediaQuality.length > 0
            radius: 4; color: "transparent"; border.color: Qt.rgba(1, 1, 1, 0.3); border.width: 1
            implicitWidth: qB.implicitWidth + 12; implicitHeight: 20
            Text { id: qB; anchors.centerIn: parent; text: pw ? pw.mediaQuality : ""; color: Theme.stageInk; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            MouseArea { id: qMa; anchors.fill: parent; hoverEnabled: true }
            ToolTip.visible: qMa.containsMouse && pw && pw.mediaFileName.length > 0
            ToolTip.text: pw ? pw.mediaFileName : ""
            ToolTip.delay: 300
        }
        Item { Layout.fillWidth: true }
        Text {
            readonly property real total: (pw && pw.streamStats && pw.streamStats.totalBytes) || 0
            readonly property real done: (pw && pw.streamStats && pw.streamStats.downloadedBytes) || 0
            visible: !!pw && pw.stillDownloading && total > 0
            text: (i18n.language, i18n.t("player_rest_downloading")).arg(Math.floor(done / Math.max(1, total) * 100))
            color: Theme.stageMuted; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            MouseArea { id: restMa; anchors.fill: parent; hoverEnabled: true }
            ToolTip.visible: restMa.containsMouse
            ToolTip.delay: 300
            ToolTip.text: formatHelper && pw ? formatHelper.fmtAhead(pw.bufferedAheadMs) : ""
        }
    }
    Row {
        id: winButtons
        visible: !!pw && pw.frameless && topBar.visible
        opacity: topBar.opacity
        anchors.top: parent.top; anchors.right: parent.right
        WinBtn { id: minBtn; glyph: "min"; onClicked: if (pw) pw.showMinimized() }
        WinBtn { id: maxBtn; glyph: pw && pw.visibility === Window.Maximized ? "restore" : "max"; onClicked: if (pw) pw.toggleMaximized() }
        WinBtn { id: closeBtn; glyph: "close"; onClicked: if (pw) pw.close() }
    }

    // ---- bottom: seek row and the control line ----
    Item {
        id: bar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 200
        opacity: root.shown ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.85) }
            }
        }
        MouseArea {
            id: barHover
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            height: root.barHeight
            hoverEnabled: true; acceptedButtons: Qt.NoButton
            onContainsMouseChanged: if (containsMouse && pw) pw.showControls()
            onPositionChanged: if (pw) pw.showControls()
        }

        ColumnLayout {
            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
            anchors.leftMargin: 24; anchors.rightMargin: 24; anchors.bottomMargin: 16
            spacing: 10

            PlayerScrubber {
                Layout.fillWidth: true
                mediaPlayer: root.mediaPlayer
                formatHelper: root.formatHelper
                controlsShown: root.shown
                localFile: pw ? pw.localFile : ""
                stillDownloading: pw ? pw.stillDownloading : false
                downloadedToMs: pw ? pw.downloadedToMs : 0
                buffered: pw ? pw.bufferedTo : 0
                bufferedFrom: pw ? pw.bufferedFrom : 0
                chapters: pw ? pw.chapters : []
                onLocalPathRefreshRequested: {
                    if (pw && typeof session !== "undefined")
                        pw.localFile = session.streamLocalPath(pw.infoHash, pw.fileIndex)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                spacing: 6
                CtlBtn { icon: "qrc:/icons/skip-back-10.svg"; tip: (i18n.language, i18n.t("player_back10")); onClicked: if (pw) pw.seekBy(-10000) }
                CtlBtn {
                    readonly property bool playing: !!mediaPlayer && mediaPlayer.playbackState === MediaPlayer.PlayingState
                    icon: playing ? "qrc:/icons/pause.svg" : "qrc:/icons/play.svg"
                    tip: (i18n.language, i18n.t("player_play_pause"))
                    onClicked: if (pw) pw.togglePlay()
                }
                CtlBtn { icon: "qrc:/icons/skip-fwd-10.svg"; tip: (i18n.language, i18n.t("player_fwd10")); onClicked: if (pw) pw.seekBy(10000) }
                CtlBtn {
                    visible: !!pw && pw.hasNext
                    icon: "qrc:/icons/skip-forward.svg"; s: 18
                    tip: (i18n.language, i18n.t("player_next"))
                    onClicked: if (pw) pw.playNext()
                }
                Text {
                    visible: !!pw && pw.hasNext && text.length > 0
                    Layout.leftMargin: 6
                    Layout.maximumWidth: Math.max(0, root.width * 0.3)
                    text: !pw ? "" : (i18n.language, i18n.t("player_next_label")).arg(pw.nextSubtitle.length > 0 ? pw.nextSubtitle : pw.nextTitle)
                    color: Theme.stageT2; font.pixelSize: 13; font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Item { Layout.fillWidth: true }
                // speed lives on this button alone
                Rectangle {
                    id: speedBtn
                    implicitHeight: 30
                    implicitWidth: spTxt.implicitWidth + 20
                    radius: 7
                    color: speedPop.visible ? Qt.rgba(1, 1, 1, 0.18) : (spMa.containsMouse ? Qt.rgba(1, 1, 1, 0.14) : Qt.rgba(1, 1, 1, 0.08))
                    Text {
                        id: spTxt
                        anchors.centerIn: parent
                        text: (mediaPlayer ? mediaPlayer.playbackRate : 1) + "×"
                        color: "#ffffff"; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                    }
                    MouseArea { id: spMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: speedPop.visible = !speedPop.visible }
                    ToolTip.visible: spMa.containsMouse && !speedPop.visible
                    ToolTip.text: (i18n.language, i18n.t("player_speed"))
                    ToolTip.delay: 500
                }
                // subtitles and audio: "CC PT"
                Rectangle {
                    implicitHeight: 40
                    implicitWidth: ccRow.implicitWidth + 20
                    radius: 8
                    readonly property bool on: !!optionsPanel && optionsPanel.open
                    color: on ? Qt.rgba(1, 1, 1, 0.14) : (ccMa.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent")
                    Row {
                        id: ccRow
                        anchors.centerIn: parent
                        spacing: 6
                        Rectangle {
                            anchors.verticalCenter: parent.verticalCenter
                            width: ccT.implicitWidth + 8; height: 18; radius: 3
                            color: "transparent"; border.color: "#ffffff"; border.width: 1.5
                            Text { id: ccT; anchors.centerIn: parent; text: "CC"; color: "#ffffff"; font.pixelSize: 10; font.weight: Font.Bold; font.family: Theme.fontSans }
                        }
                        Text { anchors.verticalCenter: parent.verticalCenter; text: root.subBadge; color: "#ffffff"; font.pixelSize: 13; font.family: Theme.fontSans }
                    }
                    MouseArea {
                        id: ccMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: if (optionsPanel) optionsPanel.open ? optionsPanel.closePanel() : optionsPanel.openPanel()
                    }
                    ToolTip.visible: ccMa.containsMouse && !on
                    ToolTip.text: (i18n.language, i18n.t("player_subs_audio"))
                    ToolTip.delay: 500
                }
                Item {
                    id: volCtl
                    readonly property bool expanded: volHov.hovered || hsl.pressed
                    implicitHeight: 40
                    implicitWidth: 40 + (expanded ? 80 : 0)
                    Behavior on implicitWidth { NumberAnimation { duration: 150; easing.type: Theme.easeOut } }
                    clip: true
                    HoverHandler { id: volHov }
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        CtlBtn {
                            icon: (pw && (pw.muted || pw.volume <= 0)) ? "qrc:/icons/volume-mute.svg" : "qrc:/icons/volume.svg"
                            s: 18
                            tip: (i18n.language, i18n.t("player_volume"))
                            onClicked: if (pw) pw.muted = !pw.muted
                        }
                        Slider {
                            id: hsl
                            width: 76; height: 40
                            opacity: volCtl.expanded ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
                            from: 0; to: 1
                            value: pw && pw.muted ? 0 : (pw ? pw.volume : 0)
                            onMoved: if (pw) { pw.muted = false; pw.volume = value }
                            background: Rectangle {
                                x: hsl.leftPadding; y: hsl.topPadding + hsl.availableHeight / 2 - height / 2
                                width: hsl.availableWidth; height: 4; radius: 2; color: Qt.rgba(1, 1, 1, 0.22)
                                Rectangle { width: hsl.visualPosition * parent.width; height: parent.height; radius: 2; color: "#ffffff" }
                            }
                            handle: Rectangle {
                                x: hsl.leftPadding + hsl.visualPosition * (hsl.availableWidth - width)
                                y: hsl.topPadding + hsl.availableHeight / 2 - height / 2
                                implicitWidth: 12; implicitHeight: 12; radius: 6; color: "#ffffff"
                            }
                        }
                    }
                }
                CtlBtn { icon: "qrc:/icons/ellipsis.svg"; s: 18; tip: (i18n.language, i18n.t("ctx_grp_more")); onClicked: if (overflowMenu) overflowMenu.popup() }
                CtlBtn { icon: "qrc:/icons/maximize.svg"; s: 18; tip: (i18n.language, i18n.t("player_fullscreen")); onClicked: if (pw) pw.toggleFullscreen() }
            }
        }
    }

    // ---- speed popover, above the 1× button ----
    MouseArea {
        anchors.fill: parent
        visible: speedPop.visible
        onClicked: speedPop.visible = false
    }
    Rectangle {
        id: speedPop
        visible: false
        width: 132
        height: speedCol.implicitHeight + 16
        x: Math.min(root.width - width - 24, speedBtn.mapToItem(root, 0, 0).x + speedBtn.width / 2 - width / 2)
        y: root.height - 76 - height
        radius: 12
        color: Qt.rgba(18 / 255, 18 / 255, 20 / 255, 0.96)
        border.color: Qt.rgba(1, 1, 1, 0.1); border.width: 1
        MouseArea { anchors.fill: parent }
        Column {
            id: speedCol
            anchors.fill: parent
            anchors.margins: 8
            Repeater {
                model: [0.5, 0.75, 1.0, 1.25, 1.5, 2.0]
                Rectangle {
                    required property var modelData
                    readonly property bool cur: !!mediaPlayer && mediaPlayer.playbackRate === modelData
                    width: speedCol.width; height: 34; radius: 7
                    color: cur ? Qt.rgba(1, 1, 1, 0.06) : (rMa.containsMouse ? Qt.rgba(1, 1, 1, 0.06) : "transparent")
                    Text { x: 10; anchors.verticalCenter: parent.verticalCenter; width: 14; text: parent.cur ? "✓" : ""; color: Theme.accent; font.pixelSize: 13; font.weight: Font.Bold }
                    Text { x: 30; anchors.verticalCenter: parent.verticalCenter; text: parent.modelData + "×"; color: "#ffffff"; font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum }
                    MouseArea { id: rMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: { mediaPlayer.playbackRate = parent.modelData; speedPop.visible = false } }
                }
            }
        }
    }
}
