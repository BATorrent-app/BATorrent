// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The mode-specific segmented bars of the search results: audio mode
// (dub/sub/original — hidden for English UIs), season tabs of a picked
// series (or the episode picker), and the per-season episode tabs.
import QtQuick
import QtQuick.Layouts
import "theme"

ColumnLayout {
    id: bars
    property var sv
    spacing: 0
    Layout.fillWidth: true

    // segmented-control chip (audio mode, season and episode bars)
    component SegChip: Rectangle {
        id: chip
        property bool on: false
        property alias label: chipTxt.text
        signal tapped()
        implicitWidth: chipTxt.implicitWidth + 26
        implicitHeight: 30
        radius: 8
        color: on ? Theme.accent : (chipMa.containsMouse ? Theme.hover : Theme.field)
        border.color: on ? Theme.accent : Theme.hair
        border.width: 1
        Behavior on color { ColorAnimation { duration: 120 } }
        Text {
            id: chipTxt
            anchors.centerIn: parent
            color: chip.on ? "#ffffff" : Theme.t2
            font.pixelSize: 12
            font.weight: chip.on ? Font.DemiBold : Font.Medium
            font.family: Theme.fontSans
        }
        MouseArea {
            id: chipMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: chip.tapped()
        }
    }

    // audio-mode segmented control — the thing a viewer actually chooses on:
    // dubbed / subtitled / original, in their own language. Prominent, not a
    // dropdown; hidden for games, title-picking, and English UIs.
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 44
        visible: !bars.sv.isTitles && !bars.sv.isGames && !bars.sv.isEpisodes && bars.sv.showAudioModes && !bars.sv.browse
        color: "transparent"
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp5
            anchors.rightMargin: Theme.sp5
            spacing: Theme.sp3

            Row {
                spacing: 3
                Layout.alignment: Qt.AlignVCenter
                Repeater {
                    model: [
                        { k: "",         t: i18n.t("search_audio_all") },
                        { k: "dub",      t: "🎙  " + i18n.t("search_audio_dub") },
                        { k: "sub",      t: "💬  " + i18n.t("search_audio_sub") },
                        { k: "original", t: "🌐  " + i18n.t("search_audio_original") }
                    ]
                    delegate: SegChip {
                        required property var modelData
                        on: bars.sv.audioModeFilter === modelData.k
                        label: (i18n.language, modelData.t)
                        onTapped: bars.sv.audioModeFilter = modelData.k
                    }
                }
            }
            Item { Layout.fillWidth: true }
            Text {
                visible: bars.sv.audioModeFilter !== ""
                text: bars.sv.viewModel.length + " " + i18n.t("search_audio_matches")
                color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
    }

    // season bar — a picked series' releases grouped by parsed season, or the
    // Stremio episode picker's season tabs
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 42
        visible: !bars.sv.browse && (bars.sv.isSeriesDrill || bars.sv.isEpisodes)
                 && (bars.sv.seasonTabs.length > 0 || bars.sv.packCount > 0)
        color: "transparent"
        Flickable {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp5
            anchors.rightMargin: Theme.sp5
            contentWidth: seasonRow.width
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Row {
                id: seasonRow
                height: parent.height
                spacing: 3
                SegChip {
                    anchors.verticalCenter: parent.verticalCenter
                    on: bars.sv.seasonFilter === -2
                    label: (i18n.language, i18n.t("search_filter_all"))
                    onTapped: { bars.sv.seasonFilter = -2; bars.sv.episodeFilter = -1 }
                }
                SegChip {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: bars.sv.isSeriesDrill && bars.sv.packCount > 0
                    on: bars.sv.seasonFilter === -1
                    label: (i18n.language, i18n.t("search_packs")) + " (" + bars.sv.packCount + ")"
                    onTapped: { bars.sv.seasonFilter = -1; bars.sv.episodeFilter = -1 }
                }
                Repeater {
                    model: bars.sv.seasonTabs
                    delegate: SegChip {
                        required property var modelData
                        anchors.verticalCenter: parent.verticalCenter
                        on: bars.sv.seasonFilter === modelData
                        label: (i18n.language, i18n.t("search_season_abbr")).arg(modelData)
                        onTapped: { bars.sv.seasonFilter = modelData; bars.sv.episodeFilter = -1 }
                    }
                }
            }
        }
    }

    // episode bar — appears once a season is picked in the series drill-down
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 38
        visible: !bars.sv.browse && bars.sv.isSeriesDrill && bars.sv.seasonFilter > 0
                 && bars.sv.episodeTabs.length > 0
        color: "transparent"
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        Flickable {
            anchors.fill: parent
            anchors.leftMargin: Theme.sp5
            anchors.rightMargin: Theme.sp5
            contentWidth: episodeRow.width
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            Row {
                id: episodeRow
                height: parent.height
                spacing: 3
                SegChip {
                    anchors.verticalCenter: parent.verticalCenter
                    on: bars.sv.episodeFilter === -1
                    label: (i18n.language, i18n.t("search_filter_all"))
                    onTapped: bars.sv.episodeFilter = -1
                }
                Repeater {
                    model: bars.sv.episodeTabs
                    delegate: SegChip {
                        required property var modelData
                        anchors.verticalCenter: parent.verticalCenter
                        on: bars.sv.episodeFilter === modelData
                        label: (i18n.language, i18n.t("search_episode_abbr")).arg(modelData)
                        onTapped: bars.sv.episodeFilter = modelData
                    }
                }
            }
        }
    }
}
