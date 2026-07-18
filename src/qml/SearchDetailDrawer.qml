// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Slide-in detail drawer for a picked search result: cover, attribute chips,
// stats, add/stream action. `sv` is the owning SearchView (selection state).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: root
    property var sv

    // label/value stat for the stats grid
    component DetailStat: ColumnLayout {
        id: stat
        property string label: ""
        property string value: ""
        property color valueColor: Theme.t2
        spacing: 2
        Text { text: stat.label; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.5; font.family: Theme.fontSans }
        Text { text: stat.value; color: stat.valueColor; font.pixelSize: 13; font.family: Theme.fontMono }
    }

    Rectangle {
        anchors.fill: parent
        visible: root.sv.detailOpen
        color: "#80000000"
        MouseArea { anchors.fill: parent; onClicked: root.sv.detailOpen = false }
        Behavior on opacity { NumberAnimation { duration: 140 } }
        opacity: root.sv.detailOpen ? 1 : 0
    }

    Rectangle {
        id: drawer
        width: 360
        height: parent.height
        // animate a 0..1 slide, never x (see HubDetailDrawer — resize ghosting)
        property real slide: root.sv.detailOpen ? 1 : 0
        Behavior on slide { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
        x: parent.width - width * slide
        color: Theme.elev
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.hair }
        MouseArea { anchors.fill: parent }   // swallow clicks so the scrim doesn't close it

        Flickable {
            id: searchDetailFlick
            anchors.fill: parent
            anchors.margins: 20
            contentHeight: detailCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            WheelScroller { flick: searchDetailFlick }

            ColumnLayout {
                id: detailCol
                width: parent.width
                spacing: 14

                // close
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: (i18n.language, i18n.t("search_details"))
                        color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold
                        font.letterSpacing: 0.6; font.family: Theme.fontSans
                    }
                    IconImg {
                        src: "qrc:/icons/close.svg"; tint: closeMa.containsMouse ? Theme.t1 : Theme.t4; s: 16
                        MouseArea { id: closeMa; anchors.fill: parent; anchors.margins: -6; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.sv.detailOpen = false }
                    }
                }

                // cover
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 190; Layout.preferredHeight: 285
                    radius: 10
                    color: "#161618"
                    border.color: Theme.hair; border.width: 1
                    clip: true
                    Image {
                        anchors.centerIn: parent
                        visible: root.sv.detailPoster === ""
                        width: parent.width * 0.5; height: width
                        source: "qrc:/images/logo.svg"
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.4
                    }
                    Image {
                        anchors.fill: parent
                        visible: root.sv.detailPoster !== ""
                        source: root.sv.detailPoster
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                        sourceSize: Qt.size(380, 570)
                    }
                }

                Text {
                    Layout.fillWidth: true
                    text: root.sv.selected ? root.sv.selected.name : ""
                    color: Theme.t1; font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                }
                Text {
                    Layout.fillWidth: true
                    visible: root.sv.selected && (root.sv.selected.sub || "").length > 0
                    text: root.sv.selected ? (root.sv.selected.sub || "") : ""
                    color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                }

                // attribute chips
                Flow {
                    Layout.fillWidth: true
                    spacing: 6
                    TChip { visible: root.sv.selected && (root.sv.selected.lang || "").length > 0; text: root.sv.selected ? (root.sv.selected.lang || "") : "" }
                    TChip { visible: root.sv.selected && (root.sv.selected.quality || "").length > 0; text: root.sv.selected ? (root.sv.selected.quality || "") : "" }
                    TChip { visible: root.sv.selected && (root.sv.selected.source || "").length > 0; text: root.sv.selected ? (root.sv.selected.source || "") : "" }
                    TChip { visible: root.sv.selected && (root.sv.selected.codec || "").length > 0; text: root.sv.selected ? (root.sv.selected.codec || "") : "" }
                    TChip { visible: root.sv.selected && root.sv.selected.hdr; text: "HDR" }
                    TChip {
                        visible: root.sv.selected && (root.sv.selected.repacker || "").length > 0
                        text: root.sv.selected ? (root.sv.selected.repacker || "") : ""
                        clickable: true
                        red: root.sv.selected && root.sv.repackerFilter === (root.sv.selected.repacker || "")
                        onClicked: { root.sv.repackerFilter = root.sv.selected.repacker || ""; root.sv.detailOpen = false }
                    }
                }

                // stills / screenshots of the picked work (backdrops for movies
                // and series, IGDB screenshots for games)
                Text {
                    visible: stillsStrip.count > 0
                    text: (i18n.language, i18n.t("search_stills"))
                    color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold
                    font.letterSpacing: 0.5; font.family: Theme.fontSans
                }
                ListView {
                    id: stillsStrip
                    Layout.fillWidth: true
                    Layout.preferredHeight: visible ? 90 : 0
                    visible: count > 0
                    orientation: ListView.Horizontal
                    spacing: 8
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: root.sv.api ? root.sv.api.workStills : []
                    delegate: Rectangle {
                        required property var modelData
                        width: 160; height: 90
                        radius: 8
                        color: "#161618"
                        border.color: Theme.hair; border.width: 1
                        clip: true
                        Image {
                            anchors.fill: parent
                            source: modelData
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                            cache: true
                            sourceSize: Qt.size(320, 180)
                        }
                    }
                }

                // stats grid
                GridLayout {
                    Layout.fillWidth: true
                    columns: 2
                    columnSpacing: 16
                    rowSpacing: 8
                    visible: root.sv.selected !== null

                    DetailStat { label: (i18n.language, i18n.t("search_col_size2")); value: root.sv.selected ? (root.sv.selected.sizeStr || "—") : "" }
                    DetailStat { label: (i18n.language, i18n.t("search_col_seeds2")); value: root.sv.selected ? (root.sv.selected.seeds || "—") : ""; valueColor: (root.sv.selected && root.sv.selected.hasSeeds) ? Theme.grn : Theme.t2; visible: root.sv.selected && (root.sv.selected.seeds || "").length > 0 }
                    DetailStat { label: (i18n.language, i18n.t("search_col_leech")); value: root.sv.selected ? (root.sv.selected.leech || "—") : ""; visible: root.sv.selected && (root.sv.selected.leech || "").length > 0 }
                }

                BtnFlat {
                    Layout.fillWidth: true
                    primary: true
                    text: root.sv.isCatalog ? (i18n.language, i18n.t("search_view_streams"))
                                            : (i18n.language, i18n.t("search_add"))
                    onClicked: {
                        if (!root.sv.api || root.sv.selectedIdx < 0) return
                        root.sv.api.activateResult(root.sv.selectedIdx)
                        if (!root.sv.isCatalog) root.sv.detailOpen = false
                    }
                }
                Item { Layout.preferredHeight: 4 }
            }
        }
    }
}
