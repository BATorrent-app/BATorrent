// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Netflix-style skip intro/credits chip. Visible while playhead sits in a
// labelled chapter; yields to the end card; works even when chrome is hidden.
import QtQuick
import "../theme"
import "../widgets"

Item {
    id: root
    anchors.fill: parent
    z: 57

    property var mediaPlayer
    property var chapters: []
    property bool endCardVisible: false
    property bool controlsShown: true
    property bool autoSkip: false
    signal skipped()

    // Skips once per chapter, and only when playback walked into it: someone
    // who seeks back into the opening means to watch it.
    property real autoSkippedAt: -1
    onChaptersChanged: autoSkippedAt = -1
    onActiveSkipChanged: {
        var sk = root.activeSkip
        if (!root.autoSkip || !sk || !mediaPlayer || root.endCardVisible) return
        if (sk.startMs === root.autoSkippedAt || mediaPlayer.position > sk.startMs + 4000) return
        root.autoSkippedAt = sk.startMs
        mediaPlayer.position = sk.endMs
        root.skipped()
    }

    readonly property var activeSkip: {
        if (!mediaPlayer) return null
        for (var i = 0; i < chapters.length; ++i) {
            var c = chapters[i]
            if (c.kind && c.endMs > 0 && mediaPlayer.position >= c.startMs
                && mediaPlayer.position < c.endMs - 1000)
                return c
        }
        return null
    }

    // "Skip opening  S": the words and the key, on a band of its own above
    // the controls (or near the bottom when they are hidden)
    function skipNow() {
        var sk = root.activeSkip
        if (!sk || !mediaPlayer || root.endCardVisible) return false
        mediaPlayer.position = sk.endMs
        root.skipped()
        return true
    }
    Rectangle {
        id: chip
        readonly property var sk: root.activeSkip
        visible: opacity > 0
        opacity: (sk && !root.endCardVisible) ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durBase; easing.type: Theme.easeOut } }
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 24
        anchors.bottomMargin: root.controlsShown ? 184 : 40
        Behavior on anchors.bottomMargin { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        width: skipRow.width + 32; height: 40
        radius: 8
        color: skMa.containsMouse ? "#ffffff" : Qt.rgba(20 / 255, 20 / 255, 22 / 255, 0.85)
        border.color: skMa.containsMouse ? "#ffffff" : Qt.rgba(1, 1, 1, 0.18); border.width: 1
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Row {
            id: skipRow; anchors.centerIn: parent; spacing: 8
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: chip.sk ? (chip.sk.kind === "credits"
                        ? (i18n.language, i18n.t("player_skip_credits"))
                        : (i18n.language, i18n.t("player_skip_intro"))) : ""
                color: skMa.containsMouse ? "#0a0a0c" : "#ffffff"
                font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "S"
                color: skMa.containsMouse ? "#55555c" : Theme.stageMuted
                font.pixelSize: 12; font.family: Theme.fontSans
            }
        }
        MouseArea {
            id: skMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: root.skipNow()
        }
    }
}
