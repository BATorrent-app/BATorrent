// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Title scrim + bottom controls (scrubber, transport, volume, options). Host
// passes pw / mediaPlayer / panel refs; chrome exposes bar/top metrics for
// resume inset, subtitle margin, and idle auto-hide.
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
    // Named differently from host ids — same-name bindings self-shadow to undefined.
    property var optionsPanel
    property var overflowMenu

    readonly property bool topVisible: topBar.visible
    readonly property real topHeight: topBar.height
    readonly property bool barVisible: bar.visible
    readonly property real barHeight: bar.height
    readonly property bool barHovered: barHover.containsMouse

    // ---- title bar ----
    Item {
        id: topBar
        anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
        height: 52
        opacity: pw && pw.controlsShown ? 1 : 0
        visible: opacity > 0 && !!pw && pw.mediaTitle.length > 0
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#e6070708" }
                GradientStop { position: 1.0; color: "#00000000" }
            }
        }

        MouseArea {
            anchors.fill: parent
            enabled: !!pw && !pw.fullscreen
            onPressed: if (pw) pw.startSystemMove()
            onDoubleClicked: if (pw) pw.toggleFullscreen()
        }

        Row {
            anchors.centerIn: parent
            spacing: 9
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: pw ? pw.headerTitle.replace(/\s*\(\d{4}\)\s*$/, "") : ""
                color: "#f3f3f4"; font.pixelSize: 15; font.weight: Font.DemiBold
                font.letterSpacing: -0.2; font.family: Theme.fontSans
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: !!pw && pw.resolvedSubtitle.length > 0
                text: pw ? pw.resolvedSubtitle : ""
                color: "#6f7077"; font.pixelSize: 15; font.weight: Font.Medium; font.family: Theme.fontSans
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: !!pw && pw.mediaQuality.length > 0
                radius: 5; color: "transparent"; border.color: "#24ffffff"; border.width: 1
                implicitWidth: qB.implicitWidth + 12; implicitHeight: 18
                Text { id: qB; anchors.centerIn: parent; text: pw ? pw.mediaQuality : ""; color: "#b4b5ba"; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                visible: !!pw && pw.mediaAudio.length > 0
                radius: 5; color: "transparent"; border.color: "#24ffffff"; border.width: 1
                implicitWidth: aB.implicitWidth + 12; implicitHeight: 18
                Text { id: aB; anchors.centerIn: parent; text: pw ? pw.mediaAudio : ""; color: "#b4b5ba"; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 15; height: 15; radius: 7.5; color: "transparent"
                border.color: infoMa.containsMouse ? "#7affffff" : "#38ffffff"; border.width: 1
                Text { anchors.centerIn: parent; text: "i"; color: infoMa.containsMouse ? "#b4b5ba" : "#818288"; font.pixelSize: 9; font.weight: Font.Bold; font.family: Theme.fontSans }
                MouseArea { id: infoMa; anchors.fill: parent; anchors.margins: -4; hoverEnabled: true }
                ToolTip.visible: infoMa.containsMouse && pw && pw.mediaFileName.length > 0
                ToolTip.text: pw ? pw.mediaFileName : ""
                ToolTip.delay: 250
            }
        }
    }

    // ---- controls bar ----
    Item {
        id: bar
        anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
        height: 124
        opacity: pw && pw.controlsShown ? 1 : 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }

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
            onContainsMouseChanged: if (containsMouse && pw) pw.showControls()
            onPositionChanged: if (pw) pw.showControls()
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.leftMargin: 20; anchors.rightMargin: 20
            anchors.topMargin: 6; anchors.bottomMargin: 14
            spacing: 4

            PlayerScrubber {
                Layout.fillWidth: true
                mediaPlayer: root.mediaPlayer
                formatHelper: root.formatHelper
                controlsShown: pw ? pw.controlsShown : false
                localFile: pw ? pw.localFile : ""
                stillDownloading: pw ? pw.stillDownloading : false
                downloadedToMs: pw ? pw.downloadedToMs : 0
                buffered: (pw && pw.streamStats && pw.streamStats.buffered) || 0
                onLocalPathRefreshRequested: {
                    if (pw && typeof session !== "undefined")
                        pw.localFile = session.streamLocalPath(pw.infoHash, pw.fileIndex)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                visible: !!pw && pw.stillDownloading && pw.downloadedToMs > 0
                spacing: 12
                Item { Layout.minimumWidth: 52 }
                Row { spacing: 6
                    Rectangle { width: 9; height: 3; radius: 2; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                    Text { text: "watched"; color: "#9a9aa0"; font.pixelSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                }
                Row { spacing: 6
                    Rectangle { width: 9; height: 3; radius: 2; color: "#80ffffff"; anchors.verticalCenter: parent.verticalCenter }
                    Text {
                        text: "downloaded to " + (formatHelper && pw ? formatHelper.fmt(pw.downloadedToMs) : "")
                        color: "#9a9aa0"; font.pixelSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter
                    }
                }
                Item { Layout.fillWidth: true }
            }

            Item { Layout.fillHeight: true }

            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 48

                RowLayout {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    Rectangle {
                        Layout.alignment: Qt.AlignVCenter
                        readonly property real total: (pw && pw.streamStats && pw.streamStats.totalBytes) || 0
                        readonly property bool low: !!pw && pw.bufferedAheadMs < 30000
                        visible: total > 0 && !!pw && pw.stillDownloading
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
                                text: "+" + (formatHelper && pw ? formatHelper.fmtRunway(pw.bufferedAheadMs) : "")
                                color: parent.parent.low ? Theme.amber : Theme.t2
                                font.pixelSize: 12; font.family: Theme.fontMono
                            }
                        }
                        MouseArea { id: dlMa; anchors.fill: parent; hoverEnabled: true }
                        ToolTip.visible: dlMa.containsMouse
                        ToolTip.delay: 250
                        ToolTip.text: {
                            if (!formatHelper || !pw) return ""
                            return formatHelper.fmtAhead(pw.bufferedAheadMs) + "  ·  "
                                  + formatHelper.fmtBytes((pw.streamStats && pw.streamStats.downloadedBytes) || 0)
                                  + " / " + formatHelper.fmtBytes((pw.streamStats && pw.streamStats.totalBytes) || 0)
                        }
                    }
                }

                RowLayout {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

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
                            anchors.centerIn: parent; src: "qrc:/icons/skip-back-10.svg"; s: 24
                            tint: rwMa.containsMouse ? Theme.t1 : Theme.t2
                            Behavior on tint { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                        }
                        MouseArea { id: rwMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: if (pw) pw.seekBy(-10000) }
                    }
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
                            anchors.horizontalCenterOffset: mediaPlayer && mediaPlayer.playbackState === MediaPlayer.PlayingState ? 0 : 2
                            src: mediaPlayer && mediaPlayer.playbackState === MediaPlayer.PlayingState ? "qrc:/icons/pause.svg" : "qrc:/icons/play.svg"
                            tint: playMa.containsMouse ? Theme.accent : Theme.t1; s: 26
                            Behavior on tint { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                        }
                        MouseArea { id: playMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: if (pw) pw.togglePlay() }
                    }
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
                            anchors.centerIn: parent; src: "qrc:/icons/skip-fwd-10.svg"; s: 24
                            tint: fwMa.containsMouse ? Theme.t1 : Theme.t2
                            Behavior on tint { ColorAnimation { duration: 140; easing.type: Easing.OutCubic } }
                        }
                        MouseArea { id: fwMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: if (pw) pw.seekBy(10000) }
                    }
                    PIconBtn {
                        Layout.alignment: Qt.AlignVCenter
                        visible: !!pw && pw.nextIdx >= 0
                        src: "qrc:/icons/skip-forward.svg"
                        tip: (i18n.language, i18n.t("player_next"))
                        onClicked: if (pw && typeof session !== "undefined") session.playFile(pw.infoHash, pw.nextIdx)
                    }
                }

                RowLayout {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8

                    PChip {
                        Layout.alignment: Qt.AlignVCenter
                        active: !!(mediaPlayer && mediaPlayer.playbackRate !== 1.0)
                        label: (mediaPlayer ? mediaPlayer.playbackRate : 1) + "×"
                        onClicked: if (optionsPanel) optionsPanel.open ? optionsPanel.closePanel() : optionsPanel.openPanel()
                    }
                    PIconBtn {
                        Layout.alignment: Qt.AlignVCenter
                        src: "qrc:/icons/subtitles.svg"; s: 19
                        active: !!(mediaPlayer && mediaPlayer.activeSubtitleTrack >= 0) || !!(pw && pw.extSubsActive)
                        tip: (i18n.language, i18n.t("player_subs"))
                        onClicked: if (optionsPanel) optionsPanel.open ? optionsPanel.closePanel() : optionsPanel.openPanel()
                    }
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
                                    src: (pw && (pw.muted || pw.volume <= 0)) ? "qrc:/icons/volume-mute.svg" : "qrc:/icons/volume.svg"
                                    tint: volCtl.expanded ? Theme.t1 : Theme.t2
                                    s: 18
                                }
                                MouseArea {
                                    id: volMa; anchors.fill: parent
                                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    onClicked: if (pw) pw.muted = !pw.muted
                                }
                            }
                            Slider {
                                id: hsl
                                width: 72; height: 32
                                anchors.verticalCenter: parent.verticalCenter
                                opacity: volCtl.expanded ? 1 : 0
                                Behavior on opacity { NumberAnimation { duration: 120 } }
                                from: 0; to: 1
                                value: pw && pw.muted ? 0 : (pw ? pw.volume : 0)
                                onMoved: if (pw) { pw.muted = false; pw.volume = value }
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
                    PIconBtn { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/ellipsis.svg"; tip: (i18n.language, i18n.t("ctx_grp_more")); onClicked: if (overflowMenu) overflowMenu.popup() }
                    PIconBtn { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/pip.svg"; active: !!pw && pw.pipMode; tip: (i18n.language, i18n.t("player_pip")); onClicked: if (pw) pw.togglePip() }
                    PIconBtn { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/maximize.svg"; tip: (i18n.language, i18n.t("player_fullscreen")); onClicked: if (pw) pw.toggleFullscreen() }
                }
            }
        }
    }
}
