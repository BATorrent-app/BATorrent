// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One episode's sources: its own releases, then the season packs holding it.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: src
    required property var sv
    required property var logic
    property var index
    property int season
    property int episode
    property string title
    signal backRequested()

    onEpisodeChanged: sources.reset()
    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.bottomMargin: 12
        spacing: 12
        Item {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            Rectangle {
                anchors.fill: parent
                radius: 14
                color: backMa.containsMouse ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
            }
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/chevron.svg"
                rotation: 90
                tint: Theme.stageInk
                s: 13
            }
            MouseArea {
                id: backMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: src.backRequested()
            }
        }
        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t("search_season_abbr")).arg(src.season) + " "
                  + (i18n.language, i18n.t("search_episode_abbr")).arg(src.episode)
                  + (src.title.length > 0 ? "  " + src.title : "")
            color: Theme.stageT1
            font.pixelSize: 15
            font.weight: Font.Medium
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
    }
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 1
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        Layout.bottomMargin: 6
        color: Theme.stageHair
    }

    SearchReleaseList {
        id: sources
        Layout.fillWidth: true
        Layout.fillHeight: true
        sv: src.sv
        logic: src.logic
        forEpisode: true
        sortKey: ""
        rows: src.index ? src.logic.episodeReleases(src.index, src.episode) : []
    }
}
