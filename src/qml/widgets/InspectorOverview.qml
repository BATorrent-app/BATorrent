// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The grid inspector with nothing selected. Not an icon and a sentence, and
// not a repeat of what the screen already shows (speeds, VPN, progress):
// three blocks, each ending in something to do. What needs you (find the
// files, another source, resume), what finished and was never opened (watch,
// play), and the space left on the disk once everything lands.
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls.Basic
import "../theme"

Item {
    id: ov
    property var win
    property var info: ({ attention: [], ready: [], disk: ({}) })
    function refresh() { if (typeof session !== "undefined") info = session.overview() }
    onVisibleChanged: if (visible) refresh()
    Timer { interval: 3000; repeat: true; running: ov.visible; onTriggered: ov.refresh() }

    function why(a) {
        if (a.reason === "missing") return i18n.t("ov_missing")
        if (a.reason === "error") return a.detail && a.detail.length > 0 ? a.detail : i18n.t("tile_error")
        if (a.reason === "noseeds") return i18n.t("ov_noseeds")
        if (a.reason === "fetching") return i18n.t("ov_fetching").arg(a.minutes)
        return i18n.t("ov_paused").arg(a.pct).arg(a.left)
    }
    function act(a) {
        return i18n.t(a.reason === "missing" ? "ov_act_locate"
                    : a.reason === "noseeds" ? "ov_act_source"
                    : a.reason === "fetching" ? "notif_remove"
                    : a.reason === "error" ? "ov_act_retry" : "ov_act_resume")
    }
    function run(a) {
        if (typeof session === "undefined") return
        win.selectTorrentByHash(a.infoHash)
        if (a.reason === "missing") win.promptSetLocation()
        else if (a.reason === "noseeds") win.openSearch(a.title)
        else if (a.reason === "fetching") win.removeDlg.open()
        else if (a.reason === "error") session.forceRecheckSelected()
        else session.resumeSelected()
    }

    component Eyebrow2: Text {
        color: Theme.t4
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.letterSpacing: 1.0
        font.capitalization: Font.AllUppercase
        font.family: Theme.fontSans
    }
    component Thumb: Item {
        id: th
        property string src
        property string kind
        width: 32; height: 44
        Item {
            id: thArt
            anchors.fill: parent
            visible: false
            layer.enabled: true
            Rectangle { anchors.fill: parent; color: Theme.bg }
            Image { anchors.fill: parent; source: th.src; fillMode: Image.PreserveAspectCrop; asynchronous: true; sourceSize: Qt.size(64, 88) }
            Text {
                visible: th.src.length === 0
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 4
                text: th.kind
                color: Theme.t1
                font.pixelSize: 10; font.weight: Font.Bold; font.family: Theme.fontSans
            }
        }
        Rectangle { id: thMask; anchors.fill: parent; radius: Theme.rad(4); visible: false; layer.enabled: true }
        MultiEffect { source: thArt; anchors.fill: parent; maskEnabled: true; maskSource: thMask }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 22

        // ---- needs you ----
        ColumnLayout {
            visible: ov.info.attention.length > 0
            Layout.fillWidth: true
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                Eyebrow2 { text: (i18n.language, i18n.t("ov_needs_you")) }
                Item { Layout.fillWidth: true }
                Text { text: ov.info.attentionCount || ""; color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum }
            }
            Repeater {
                model: ov.info.attention
                Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    implicitHeight: 60
                    radius: Theme.rad(9)
                    color: Theme.elev
                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 12
                        Thumb { src: modelData.poster; kind: modelData.kind }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2
                            Text { Layout.fillWidth: true; text: modelData.title; color: Theme.t1; font.pixelSize: 13; font.weight: Font.Medium; font.family: Theme.fontSans; elide: Text.ElideRight }
                            Text {
                                Layout.fillWidth: true
                                text: (i18n.language, ov.why(modelData))
                                color: modelData.reason === "paused" ? Theme.t3 : Theme.accentText
                                font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight
                            }
                        }
                        Rectangle {
                            Layout.preferredHeight: 28
                            Layout.preferredWidth: actTxt.implicitWidth + 20
                            radius: Theme.rad(7)
                            color: actMa.containsMouse ? Theme.hover : "transparent"
                            border.color: Theme.hair; border.width: 1
                            Text { id: actTxt; anchors.centerIn: parent; text: (i18n.language, ov.act(modelData)); color: Theme.t1; font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans }
                            MouseArea { id: actMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: ov.run(modelData) }
                        }
                    }
                }
            }
        }

        // ---- ready, never opened ----
        ColumnLayout {
            visible: ov.info.ready.length > 0
            Layout.fillWidth: true
            spacing: 8
            Eyebrow2 { text: (i18n.language, i18n.t("ov_ready")) }
            Repeater {
                model: ov.info.ready
                RowLayout {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.leftMargin: 8
                    Layout.rightMargin: 8
                    spacing: 12
                    Thumb { src: modelData.poster; kind: modelData.kind }
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2
                        Text { Layout.fillWidth: true; text: modelData.title; color: Theme.t1; font.pixelSize: 13; font.weight: Font.Medium; font.family: Theme.fontSans; elide: Text.ElideRight }
                        Text {
                            Layout.fillWidth: true
                            text: (i18n.language, i18n.t(modelData.type === "game" ? "ov_not_played" : "ov_not_watched"))
                            color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; elide: Text.ElideRight
                        }
                    }
                    Rectangle {
                        Layout.preferredHeight: 28
                        Layout.preferredWidth: goRow.implicitWidth + 18
                        radius: Theme.rad(7)
                        color: goMa.containsMouse ? Qt.darker(Theme.t1, 1.08) : Theme.t1
                        Row {
                            id: goRow
                            anchors.centerIn: parent
                            spacing: 6
                            IconImg { anchors.verticalCenter: parent.verticalCenter; src: "qrc:/icons/play.svg"; s: 9; tint: Theme.bg }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: (i18n.language, i18n.t(modelData.type === "game" ? "ov_act_play" : "ov_act_watch"))
                                color: Theme.bg
                                font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                            }
                        }
                        MouseArea {
                            id: goMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (typeof session !== "undefined") {
                                if (modelData.type === "game") session.launchGame(modelData.infoHash)
                                else session.playByHash(modelData.infoHash)
                            }
                        }
                    }
                }
            }
        }

        // ---- the disk, once everything lands ----
        ColumnLayout {
            visible: ov.info.disk && ov.info.disk.name !== undefined
            Layout.fillWidth: true
            spacing: 8
            Eyebrow2 { text: (i18n.language, i18n.t("ov_when_done")) }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 8
                radius: Theme.rad(4)
                color: Theme.track
                clip: true
                Row {
                    anchors.fill: parent
                    Rectangle { width: parent.width * Math.max(0, Math.min(1, ov.info.disk.usedFraction || 0)); height: 8; color: Theme.pausedFill }
                    Rectangle {
                        width: parent.width * Math.max(0, Math.min(1, ov.info.disk.remainingFraction || 0))
                        height: 8
                        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.55)
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, (ov.info.disk.name || "") + " · " + i18n.t("ov_left_to_download").arg(ov.info.disk.remaining || "0 B"))
                    color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum; elide: Text.ElideRight
                }
                Text {
                    text: (i18n.language, i18n.t("status_free_space").arg(ov.info.disk.freeAfter || ""))
                    color: ov.info.disk.short ? Theme.accentText : Theme.t1
                    font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
        }

        Item { Layout.fillHeight: true }
        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t("ov_footnote"))
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
            lineHeight: 1.4
        }
    }
}
