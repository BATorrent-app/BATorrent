// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One flat search result: thumb, name + attribute tags, size/seeds/leech
// columns, add button. `sv` is the owning SearchView (filters, api, helpers).
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: row
    property var sv
    signal menuRequested(int srcIndex)

    required property var modelData
    required property int index
    readonly property int srcIndex: modelData._idx
    readonly property string coverHash: modelData.coverHash || ""
    // catalog items and episode-picker rows drill in on click instead of opening the drawer
    readonly property bool activates: sv.isCatalog || sv.isEpisodes
    property string posterSrc: sv.fileUrl(modelData.poster || "")
    width: ListView.view.width
    height: 60
    color: resMa.containsMouse ? Theme.hover : "transparent"
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

    Component.onCompleted: if (sv.showRowThumbs && posterSrc === "" && coverHash !== "" && sv.api) sv.api.resolveCover(srcIndex)
    Connections {
        target: row.sv.api
        ignoreUnknownSignals: true
        function onCoverReady(infoHash, path) {
            if (infoHash !== "" && infoHash === row.coverHash) row.posterSrc = row.sv.fileUrl(path)
        }
    }

    // small attribute tag for the meta row
    component MetaTag: Text {
        property bool accent: false
        visible: text.length > 0
        color: accent ? Theme.accent : Theme.t3
        font.pixelSize: 10
        font.weight: Font.DemiBold
        font.family: Theme.fontSans
    }

    // origin pill (which source/provider returned the row)
    component SourceTag: Rectangle {
        property alias text: srcLabel.text
        visible: srcLabel.text.length > 0
        implicitWidth: srcLabel.implicitWidth + 14
        implicitHeight: 16
        radius: 8
        color: "transparent"
        border.color: Theme.hair
        border.width: 1
        Text {
            id: srcLabel
            anchors.centerIn: parent
            color: Theme.t2
            font.pixelSize: 9
            font.weight: Font.DemiBold
            font.letterSpacing: 0.3
            font.family: Theme.fontSans
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp5
        anchors.rightMargin: Theme.sp5
        spacing: Theme.sp4

        PosterThumb {
            visible: row.sv.showRowThumbs
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: 36
            implicitHeight: 48
            posterUrl: row.posterSrc
            label: row.modelData.name || ""
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            RowLayout {
                Layout.fillWidth: true
                spacing: 8
                Text {
                    Layout.fillWidth: true
                    text: {
                        if (!row.sv.isEpisodes) return row.modelData.name
                        var tag = (i18n.language, i18n.t("search_season_abbr")).arg(row.modelData.season)
                                  + " " + i18n.t("search_episode_abbr").arg(row.modelData.episode)
                        var n = row.modelData.name || ""
                        return n.length > 0 ? tag + "  ·  " + n : tag
                    }
                    color: Theme.t1
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                TChip {
                    visible: (row.modelData.repacker || "").length > 0
                    text: row.modelData.repacker || ""
                    clickable: true
                    red: row.sv.repackerFilter === (row.modelData.repacker || "")
                    onClicked: row.sv.repackerFilter =
                        (row.sv.repackerFilter === row.modelData.repacker ? "" : (row.modelData.repacker || ""))
                }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                SourceTag { text: row.modelData.sub || row.modelData.provider || "" }
                MetaTag {
                    text: {
                        var ls = row.modelData.langs || []
                        if (ls.length === 0) return ""
                        var s = ls.slice(0, 3).join("/")
                        if (ls.length > 3) s += "+" + (ls.length - 3)
                        if (row.modelData.dubbed === true) s += " DUB"
                        return s
                    }
                    accent: row.modelData.native === true
                }
                MetaTag { text: row.modelData.quality || ""; accent: true }
                MetaTag { text: row.modelData.source || "" }
                MetaTag { text: row.modelData.codec || "" }
                MetaTag { text: row.modelData.hdr ? "HDR" : ""; accent: true }
                // parsed episode/pack tag inside a picked series' releases
                MetaTag {
                    accent: row.modelData.pack === true
                    text: {
                        if (row.sv.isEpisodes) return ""
                        if (row.modelData.pack === true)
                            return (row.modelData.season || 0) > 0
                                   ? (i18n.language, i18n.t("search_season_abbr")).arg(row.modelData.season)
                                   : (i18n.language, i18n.t("search_pack_complete"))
                        if ((row.modelData.episode || 0) > 0)
                            return (i18n.language, i18n.t("search_season_abbr")).arg(row.modelData.season)
                                   + " " + i18n.t("search_episode_abbr").arg(row.modelData.episode)
                        return ""
                    }
                }
                Item { Layout.fillWidth: true }
            }
        }
        Text { text: row.modelData.sizeStr || ""; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono }
        Item {   // fixed-width seeds cell: small bar (fill = health) + number
            Layout.preferredWidth: 100
            Layout.alignment: Qt.AlignVCenter
            visible: (row.modelData.seeds || "").length > 0
            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 9
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 50; height: 4; radius: 2; color: Theme.hairSoft
                    Rectangle {
                        width: parent.width * row.sv.seedFill(row.modelData.seedsN || 0)
                        height: parent.height; radius: 2
                        color: row.sv.seedColor(row.modelData.seedsN || 0)
                        Behavior on width { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
                    }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: row.modelData.seeds || ""
                    width: 28; horizontalAlignment: Text.AlignRight
                    color: row.sv.seedColor(row.modelData.seedsN || 0)
                    font.pixelSize: 12; font.family: Theme.fontMono
                }
            }
        }
        Text { visible: (row.modelData.leech || "").length > 0; text: row.modelData.leech || ""; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono }
        Item {
            Layout.preferredWidth: 36
            Rectangle {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 28; height: 28; radius: 7
                color: "transparent"
                border.color: addMa.containsMouse ? Theme.accent : Theme.hair
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: row.activates ? "›" : "+"
                    color: addMa.containsMouse ? Theme.accentText : Theme.t3
                    font.pixelSize: 15
                }
                MouseArea {
                    id: addMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (row.sv.api) row.sv.api.activateResult(row.srcIndex)
                }
            }
        }
    }
    MouseArea {
        id: resMa
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        z: -1
        onClicked: function (mouse) {
            if (!row.sv.api) return
            if (mouse.button === Qt.RightButton) {
                if (!row.activates) row.menuRequested(row.srcIndex)
                return
            }
            if (row.activates) row.sv.api.activateResult(row.srcIndex)
            else row.sv.openDetail(row.modelData)
        }
        onDoubleClicked: if (row.sv.api) row.sv.api.activateResult(row.srcIndex)
    }
}
