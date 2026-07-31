// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Optional full dump of the Hydra game catalog (repack tabs + pagination).
// Opened from FindBrowse — does not replace the normal billboard/shelves landing.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"
import "../widgets"

Item {
    id: browse
    property var sv
    property string group: ""   // "" = all; else ReleaseGroup display name
    property int pageIndex: 0
    property int pageSize: 48
    signal backRequested()

    readonly property alias scrollY: list.contentY
    readonly property var api: sv && sv.api ? sv.api : null
    readonly property var tabs: {
        var _ = sv ? sv.gameCatalogGen : 0
        return api ? api.gameRepackTabs() : []
    }
    readonly property int total: {
        var _ = sv ? sv.gameCatalogGen : 0
        return api ? api.gameBrowseTotal(group) : 0
    }
    readonly property int pageCount: Math.max(1, Math.ceil(total / pageSize))

    function reload() {
        if (!api) return
        api.ensureGamesIndexed()
        if (api.gameBrowseTotal("") <= 0) return
        api.browseGames(group, pageIndex, pageSize)
    }

    function selectGroup(g) {
        group = g
        pageIndex = 0
        if (sv) sv.groupFilter = g
        reload()
    }

    function goPage(delta) {
        var next = pageIndex + delta
        if (next < 0 || next >= pageCount) return
        pageIndex = next
        reload()
        list.positionViewAtBeginning()
    }

    onVisibleChanged: if (visible) reload()
    Component.onCompleted: if (visible) reload()

    Connections {
        target: browse.api
        ignoreUnknownSignals: true
        function onGameSourcesChanged() {
            if (browse.visible) browse.reload()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            color: "transparent"
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp3
                BtnFlat {
                    text: (i18n.language, i18n.t("search_back2"))
                    onClicked: browse.backRequested()
                }
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, i18n.t("find_catalog_title"))
                    color: Theme.t1
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
            color: "transparent"
            Flickable {
                id: tabFlick
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                contentWidth: tabRow.width
                contentHeight: height
                clip: true
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds

                Row {
                    id: tabRow
                    spacing: 6
                    height: parent.height
                    anchors.verticalCenter: parent.verticalCenter

                    component RepackChip: Rectangle {
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

                    RepackChip {
                        on: browse.group === ""
                        label: (i18n.language, i18n.t("find_repack_all"))
                        onTapped: browse.selectGroup("")
                    }
                    Repeater {
                        model: browse.tabs
                        delegate: RepackChip {
                            required property var modelData
                            on: browse.group === modelData.name
                            label: modelData.name + " · " + modelData.count
                            onTapped: browse.selectGroup(modelData.name)
                        }
                    }
                }
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp4
                Item { Layout.preferredWidth: 46; visible: browse.sv && browse.sv.showRowThumbs }
                Text {
                    text: (i18n.language, i18n.t("search_col_name2"))
                    Layout.fillWidth: true
                    color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold
                    font.letterSpacing: 0.6; font.family: Theme.fontSans
                }
                Text {
                    text: (i18n.language, i18n.t("search_col_size2"))
                    Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight
                    color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold
                    font.letterSpacing: 0.6; font.family: Theme.fontSans
                }
                Item { Layout.preferredWidth: 100 }
                Item { Layout.preferredWidth: 56 }
                Item { Layout.preferredWidth: 36 }
            }
        }

        ListView {
            id: list
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: browse.sv ? browse.sv.viewModel : []
            boundsBehavior: Flickable.StopAtBounds
            cacheBuffer: 240
            WheelScroller { flick: list }

            Text {
                anchors.centerIn: parent
                visible: list.count === 0
                text: browse.api && browse.api.statusText.length > 0
                      ? browse.api.statusText
                      : (i18n.language, i18n.t("find_catalog_empty"))
                color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
            }

            delegate: SearchResultRow {
                sv: browse.sv
                onMenuRequested: function (i) { rowMenu.openFor(i) }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            color: Theme.elev
            visible: browse.total > browse.pageSize
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp3

                BtnFlat {
                    text: "‹"
                    enabled: browse.pageIndex > 0
                    onClicked: browse.goPage(-1)
                }
                Text {
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: (i18n.language, i18n.t("find_catalog_page"))
                          .arg(browse.pageIndex + 1).arg(browse.pageCount)
                    color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans
                }
                Text {
                    text: browse.api ? browse.api.statusText : ""
                    color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
                    visible: text.length > 0
                }
                BtnFlat {
                    text: "›"
                    enabled: browse.pageIndex + 1 < browse.pageCount
                    onClicked: browse.goPage(1)
                }
            }
        }
    }

    BatMenu {
        id: rowMenu
        property int idx: -1
        function openFor(i) { idx = i; popup() }
        implicitWidth: 190
        BatMenuItem {
            text: (i18n.language, i18n.t("search_add"))
            onTriggered: if (browse.api && rowMenu.idx >= 0) browse.api.activateResult(rowMenu.idx)
        }
        BatMenuItem {
            text: (i18n.language, i18n.t("search_copy_magnet"))
            onTriggered: if (browse.api && rowMenu.idx >= 0) browse.api.copyMagnet(rowMenu.idx)
        }
    }
}
