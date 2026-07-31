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
    signal skipped()

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

    Rectangle {
        id: chip
        readonly property var sk: root.activeSkip
        visible: opacity > 0
        opacity: (sk && !root.endCardVisible) ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.rightMargin: 24
        anchors.bottomMargin: root.controlsShown ? 134 : 40
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
                text: chip.sk ? (chip.sk.kind === "credits"
                        ? (i18n.language, i18n.t("player_skip_credits"))
                        : (i18n.language, i18n.t("player_skip_intro"))) : ""
                color: skMa.containsMouse ? "#0a0a0c" : Theme.t1
                font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
        }
        MouseArea {
            id: skMa; anchors.fill: parent
            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (!chip.sk || !mediaPlayer) return
                mediaPlayer.position = chip.sk.endMs
                root.skipped()
            }
        }
    }
}
