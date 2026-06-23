// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Session + all-time statistics. Reads session.statistics() (QmlSessionBridge)
// for the snapshot numbers; the graph and "now" row bind live to the session.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    signal openWrapped()
    Shortcut { sequences: [StandardKey.Cancel]; onActivated: win.close() }
    flags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    width: 620
    height: 560
    minimumWidth: 520
    minimumHeight: 480
    color: Theme.bg
    // unified chrome: the header band below is the only titlebar — the native
    // text would double-draw (and follows the system theme, not the app's)
    title: Theme.unifiedChrome ? "" : (i18n.language, i18n.t("stats_title"))

    readonly property var sess: typeof session !== "undefined" ? session : null
    property var stats: ({})
    onVisibleChanged: if (visible && sess) stats = sess.statistics()

    component KV: RowLayout {
        property string k: ""
        property string v: ""
        Layout.fillWidth: true
        spacing: 12
        Text { Layout.preferredWidth: 110; text: k; color: Theme.t3; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans }
        Text { text: v; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontMono }
        Item { Layout.fillWidth: true }
    }

    component StatChip: Rectangle {
        property string label: ""
        property string value: ""
        property color tint: Theme.t2
        radius: 10
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1
        Layout.fillWidth: true
        implicitHeight: 58
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2
            Text { Layout.alignment: Qt.AlignHCenter; text: value; color: tint; font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontMono }
            Text { Layout.alignment: Qt.AlignHCenter; text: label; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("stats_title")); color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: Theme.sp5
            spacing: Theme.sp2

            Eyebrow { text: (i18n.language, i18n.t("stats_overview")); red: true }
            Text {
                text: (i18n.language, i18n.t("stats_title2"))
                color: Theme.t1
                font.pixelSize: 18; font.weight: Font.Bold; font.letterSpacing: -0.3
                font.family: Theme.fontSans
            }

            // live speed graph + legend
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 130
                Layout.topMargin: Theme.sp3
                radius: 10
                color: Theme.panel
                border.color: Theme.hair
                border.width: 1
                SpeedGraph {
                    anchors.fill: parent
                    anchors.margins: Theme.sp3
                    dl: win.sess ? win.sess.downloadHistory : []
                    ul: win.sess ? win.sess.uploadHistory : []
                }
                Row {
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.topMargin: Theme.sp3
                    anchors.rightMargin: Theme.sp3
                    spacing: Theme.sp4
                    Row {
                        spacing: 6
                        Rectangle { implicitWidth: 6; implicitHeight: 6; radius: 3; color: Theme.accent; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: win.sess ? win.sess.totalDownSpeed : ""; color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                    }
                    Row {
                        spacing: 6
                        Rectangle { implicitWidth: 6; implicitHeight: 6; radius: 3; color: Theme.amber; anchors.verticalCenter: parent.verticalCenter }
                        Text { text: win.sess ? win.sess.totalUpSpeed : ""; color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontMono; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }

            // torrents right now
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.sp2
                spacing: Theme.sp2
                StatChip { label: (i18n.language, i18n.t("filter_all")); value: win.sess ? String(win.sess.torrentCount) : "0"; tint: Theme.t1 }
                StatChip { label: (i18n.language, i18n.t("filter_downloading")); value: win.sess ? String(win.sess.downloadingCount) : "0"; tint: Theme.accentText }
                StatChip { label: (i18n.language, i18n.t("filter_seeding")); value: win.sess ? String(win.sess.seedingCount) : "0"; tint: Theme.up }
                StatChip { label: (i18n.language, i18n.t("filter_completed")); value: win.sess ? String(win.sess.completedCount) : "0"; tint: Theme.grn }
                StatChip { label: (i18n.language, i18n.t("filter_paused")); value: win.sess ? String(win.sess.pausedCount) : "0"; tint: Theme.t3 }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.topMargin: Theme.sp3
                spacing: Theme.sp6

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.sp1
                    Text { text: (i18n.language, i18n.t("stats_alltime2")); color: Theme.t4; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans; Layout.bottomMargin: Theme.sp1 }
                    KV { k: (i18n.language, i18n.t("detail_kv_downloaded")); v: win.stats.totalDownloaded || "—" }
                    KV { k: (i18n.language, i18n.t("detail_kv_uploaded")); v: win.stats.totalUploaded || "—" }
                    KV { k: (i18n.language, i18n.t("stats_ratio")); v: win.stats.totalRatio || "—" }
                    KV { k: (i18n.language, i18n.t("stats_torrents_added")); v: win.stats.torrentsAdded !== undefined ? String(win.stats.torrentsAdded) : "—" }
                    KV { k: (i18n.language, i18n.t("stats_uptime2")); v: win.stats.uptime || "—" }
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignTop
                    spacing: Theme.sp1
                    Text { text: (i18n.language, i18n.t("stats_session2")); color: Theme.t4; font.pixelSize: 10; font.weight: Font.Bold; font.letterSpacing: 1.2; font.family: Theme.fontSans; Layout.bottomMargin: Theme.sp1 }
                    KV { k: (i18n.language, i18n.t("detail_kv_downloaded")); v: win.stats.sessionDownloaded || "—" }
                    KV { k: (i18n.language, i18n.t("detail_kv_uploaded")); v: win.stats.sessionUploaded || "—" }
                    KV { k: (i18n.language, i18n.t("stats_ratio")); v: win.stats.sessionRatio || "—" }
                    KV { k: (i18n.language, i18n.t("stats_uptime2")); v: win.stats.uptime || "—" }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                BtnFlat {
                    text: "🦇  " + (i18n.language, i18n.t("wrapped_title"))
                    onClicked: win.openWrapped()
                }
                Item { Layout.fillWidth: true }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("release_notes_close")); onClicked: win.close() }
            }
        }
    }
}
