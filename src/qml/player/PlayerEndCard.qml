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
    readonly property bool showEndCard: root.nextIdx >= 0 && !root.endCardDismissed
        && root.autoplayNext && (root.inLeadWindow || root.countdownSec > 0)

    function playNextNow() {
        countdown.stop(); root.countdownSec = 0
        if (typeof session !== "undefined" && root.nextIdx >= 0)
            session.playFile(root.infoHash, root.nextIdx)
    }

    function maybePlayNext() {
        if (typeof session === "undefined" || root.nextIdx < 0) return
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

    Rectangle {
        visible: opacity > 0
        opacity: root.showEndCard ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 24; anchors.bottomMargin: 134
        width: 348; height: 116
        radius: 12
        color: "#f2101014"
        border.color: Theme.hair; border.width: 1
        transform: Translate {
            y: root.showEndCard ? 0 : 10
            Behavior on y { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        }

        Row {
            anchors.fill: parent; anchors.margins: 12; spacing: 12

            Item {
                width: 62; height: 92; anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    anchors.fill: parent; radius: 7; clip: true
                    color: "#1c1c20"; border.color: Theme.hairSoft; border.width: 1
                    Image {
                        anchors.fill: parent
                        source: root.nextPoster.length > 0 && formatHelper
                                ? formatHelper.fileUrl(root.nextPoster) : ""
                        fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true
                    }
                }
                Canvas {
                    id: ring
                    anchors.centerIn: parent; width: 40; height: 40
                    visible: root.countdownSec > 0
                    property real frac: root.countdownSec / 8
                    onFracChanged: requestPaint()
                    onPaint: {
                        var ctx = getContext("2d"); ctx.reset()
                        var cx = width / 2, cy = height / 2, r = 17
                        ctx.beginPath(); ctx.arc(cx, cy, r, 0, Math.PI * 2)
                        ctx.lineWidth = 3; ctx.strokeStyle = "#40000000"; ctx.stroke()
                        ctx.beginPath(); ctx.arc(cx, cy, r, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * frac)
                        ctx.lineWidth = 3; ctx.lineCap = "round"; ctx.strokeStyle = "#e5332b"; ctx.stroke()
                    }
                    Text {
                        anchors.centerIn: parent; text: root.countdownSec
                        color: "#fff"; font.pixelSize: 15; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                }
            }

            Column {
                width: parent.width - 62 - 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 3
                Text {
                    text: (root.countdownSec > 0)
                          ? ((i18n.language, i18n.t("player_next_ep")) + " · " + root.countdownSec + "s")
                          : (i18n.language, i18n.t("player_up_next"))
                    color: Theme.accent; font.pixelSize: 10; font.weight: Font.Bold
                    font.letterSpacing: 0.6; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                }
                Text {
                    width: parent.width; text: root.nextTitle
                    color: "#f3f3f4"; font.pixelSize: 14; font.weight: Font.DemiBold
                    font.family: Theme.fontSans; elide: Text.ElideRight
                }
                Text {
                    width: parent.width; visible: root.nextSubtitle.length > 0
                    text: root.nextSubtitle
                    color: "#8a8b90"; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight
                }
                Row {
                    spacing: 8; topPadding: 4
                    Rectangle {
                        width: playNowRow.width + 22; height: 28; radius: 7
                        color: pnMa.containsMouse ? "#ff2e37" : Theme.accent
                        Behavior on color { ColorAnimation { duration: 100 } }
                        Row {
                            id: playNowRow; anchors.centerIn: parent; spacing: 6
                            IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/play.svg"; tint: "#fff"; s: 13 }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: (i18n.language, i18n.t("player_watch_now"))
                                color: "#fff"; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                            }
                        }
                        MouseArea {
                            id: pnMa; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: root.playNextNow()
                        }
                    }
                    Rectangle {
                        width: cancelTxt.width + 22; height: 28; radius: 7
                        color: cnMa.containsMouse ? "#1affffff" : "transparent"
                        border.color: Theme.hair; border.width: 1
                        Text {
                            id: cancelTxt; anchors.centerIn: parent
                            text: (i18n.language, i18n.t("btn_cancel"))
                            color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans
                        }
                        MouseArea {
                            id: cnMa; anchors.fill: parent
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            onClicked: root.dismiss()
                        }
                    }
                }
            }
        }
    }
}
