// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Next-episode end card + autoplay countdown. Host owns nextIdx / titles;
// this leaf owns lead-window visibility, countdown, and the card chrome.
import QtQuick
import "../theme"
import "../widgets"

Item {
    id: root
    anchors.fill: parent
    z: 58

    property var mediaPlayer
    property string infoHash: ""
    property int nextIdx: -1
    // a next episode in this torrent or another one (see PlayerWindow.playNext)
    property bool hasNext: nextIdx >= 0
    property bool autoplayNext: true
    property string nextPoster: ""
    property string nextTitle: ""
    property string nextSubtitle: ""
    property var formatHelper

    property bool endCardDismissed: false
    property int countdownSec: 0
    readonly property real endCardLeadMs: 28000
    readonly property bool inLeadWindow: mediaPlayer && mediaPlayer.duration > 0 && mediaPlayer.position > 0
        && (mediaPlayer.duration - mediaPlayer.position) <= endCardLeadMs
    readonly property bool showEndCard: root.hasNext && !root.endCardDismissed
        && root.autoplayNext && (root.inLeadWindow || root.countdownSec > 0)

    // The window loads it in place; a playFile here opened a second player.
    signal playNextRequested()
    function playNextNow() {
        countdown.stop(); root.countdownSec = 0
        if (root.hasNext) root.playNextRequested()
    }

    function maybePlayNext() {
        if (typeof session === "undefined" || !root.hasNext) return
        if (!root.autoplayNext || root.endCardDismissed) return
        root.countdownSec = 8
        countdown.restart()
    }

    function reset() {
        root.endCardDismissed = false
        root.countdownSec = 0
        countdown.stop()
    }

    function dismiss() {
        countdown.stop()
        root.countdownSec = 0
        root.endCardDismissed = true
    }

    Timer {
        id: countdown; interval: 1000; repeat: true
        onTriggered: { root.countdownSec -= 1; if (root.countdownSec <= 0) root.playNextNow() }
    }

    // "Up next · in 8 s": the episode's name, the show and how long it is,
    // and Watch now filling up as the countdown runs out.
    property string nextMeta: ""
    Rectangle {
        visible: opacity > 0
        opacity: root.showEndCard ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 24; anchors.bottomMargin: 120
        width: 360; height: 114
        radius: 12
        color: Qt.rgba(18 / 255, 18 / 255, 20 / 255, 0.92)
        border.color: Qt.rgba(1, 1, 1, 0.1); border.width: 1
        transform: Translate {
            y: root.showEndCard ? 0 : 10
            Behavior on y { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        }

        Row {
            anchors.fill: parent; anchors.margins: 12; spacing: 12

            Rectangle {
                width: 64; height: 90; radius: 6; clip: true
                color: "#1c1c20"; border.color: Qt.rgba(1, 1, 1, 0.08); border.width: 1
                Image {
                    anchors.fill: parent
                    source: root.nextPoster.length > 0 && formatHelper ? formatHelper.fileUrl(root.nextPoster) : ""
                    fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true
                }
            }
            Column {
                width: parent.width - 64 - 12
                height: parent.height
                spacing: 3
                Text {
                    text: (i18n.language, root.countdownSec > 0
                           ? i18n.t("player_up_next_in").arg(root.countdownSec)
                           : i18n.t("player_up_next"))
                    color: Theme.accentText; font.pixelSize: 11; font.weight: Font.Bold
                    font.letterSpacing: 1.0; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                }
                Text {
                    width: parent.width
                    text: root.nextSubtitle.length > 0 ? root.nextSubtitle : root.nextTitle
                    color: "#ffffff"; font.pixelSize: 15; font.weight: Font.DemiBold
                    font.family: Theme.fontSans; elide: Text.ElideRight
                }
                Text {
                    width: parent.width
                    visible: text.length > 0
                    text: root.nextMeta.length > 0 ? root.nextMeta : (root.nextSubtitle.length > 0 ? root.nextTitle : "")
                    color: Theme.stageMuted; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight
                }
            }
        }
        Row {
            anchors.left: parent.left; anchors.leftMargin: 12 + 64 + 12
            anchors.bottom: parent.bottom; anchors.bottomMargin: 12
            spacing: 6
            Rectangle {
                width: playNowTxt.width + 24; height: 30; radius: 7
                clip: true
                color: "#3a1514"
                // the countdown, as the button filling up
                Rectangle {
                    anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                    width: parent.width * (root.countdownSec > 0 ? 1 - (root.countdownSec - 1) / 8 : 1)
                    color: pnMa.containsMouse ? Qt.lighter(Theme.accent, 1.1) : Theme.accent
                    Behavior on width { NumberAnimation { duration: 950; easing.type: Easing.Linear } }
                }
                Text {
                    id: playNowTxt; anchors.centerIn: parent
                    text: (i18n.language, i18n.t("player_watch_now"))
                    color: "#ffffff"; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
                MouseArea { id: pnMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.playNextNow() }
            }
            Rectangle {
                width: cancelTxt.width + 24; height: 30; radius: 7
                color: cnMa.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
                border.color: Qt.rgba(1, 1, 1, 0.15); border.width: 1
                Text {
                    id: cancelTxt; anchors.centerIn: parent
                    text: (i18n.language, i18n.t("btn_cancel"))
                    color: "#ffffff"; font.pixelSize: 13; font.family: Theme.fontSans
                }
                MouseArea { id: cnMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.dismiss() }
            }
        }
    }
}
