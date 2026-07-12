// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Filter bar: grid/list toggle, status-filter pills, category menu, search box.
// Carved out of Main.qml; reads/writes window state via `win`, exposes the search
// field via the `searchInput` alias for the parent's focus shortcut.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: filterBar
    property var win
    property alias searchInput: searchInput

    component Pill: Rectangle {
        id: pi
        property string label
        property string count
        property string filterKey: "all"   // NOT `state` — that shadows Item.state
        property bool on: win.activeFilter === filterKey
        signal clicked()
        radius: 8
        height: 30
        implicitWidth: pillRow.implicitWidth + 26
        color: on ? Theme.accentTint : (piMa.containsMouse ? Theme.hover : "transparent")

        activeFocusOnTab: true
        Keys.onReturnPressed: pi.clicked()
        Keys.onSpacePressed: pi.clicked()
        scale: piMa.pressed ? Theme.pressScale : 1
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Easing.OutCubic } }
        Rectangle {
            visible: pi.activeFocus
            anchors.fill: parent
            anchors.margins: -2
            radius: 10
            color: "transparent"
            border.color: Theme.focusRing
            border.width: Theme.focusRingWidth
        }

        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: 7
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: pi.label
                color: pi.on ? Theme.accentText : (piMa.containsMouse ? Theme.t2 : Theme.t3)
                font.pixelSize: 12
                font.family: Theme.fontSans
                font.weight: Font.Medium
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: pi.count
                color: pi.on ? Theme.accentText : Theme.t4
                font.pixelSize: 11
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
        }
        MouseArea {
            id: piMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: pi.clicked()
        }
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 54
    color: "transparent"
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp4
        spacing: Theme.sp3

        // .search (240×34, padding 0 11, gap 8, bg panel) — the one
        // element that absorbs the shrink: it gives back width down to
        // 150 so the pills/category never have to clip.
        Rectangle {
            Layout.preferredWidth: 240
            Layout.minimumWidth: 150
            Layout.preferredHeight: 34
            Layout.alignment: Qt.AlignVCenter
            color: Theme.panel
            border.color: searchInput.activeFocus ? Theme.accent : Theme.hair
            border.width: 1
            radius: 8

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 11
                anchors.rightMargin: 11
                spacing: 8
                IconImg {
                    Layout.alignment: Qt.AlignVCenter
                    src: "qrc:/icons/search.svg"
                    tint: Theme.t4
                    s: 14
                }
                TextInput {
                    id: searchInput
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    verticalAlignment: TextInput.AlignVCenter
                    color: Theme.t1
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                    clip: true
                    // debounce: re-filtering the whole list on every
                    // keystroke stutters with a large library
                    onTextChanged: searchDebounce.restart()
                    Keys.onEscapePressed: searchInput.focus = false
                    Timer {
                        id: searchDebounce
                        interval: 150
                        onTriggered: if (typeof torrentFilter !== "undefined") torrentFilter.setSearchText(searchInput.text)
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (i18n.language, i18n.t("search_heading"))
                        color: Theme.t4
                        font: searchInput.font
                        visible: searchInput.text.length === 0 && !searchInput.activeFocus
                    }
                }
            }
        }

        // .seg (toggle Grade/Lista) — padding 2, bg panel
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 32
            Layout.minimumWidth: implicitWidth      // never clip the toggle
            implicitWidth: segRow.implicitWidth + 4
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            radius: 8

            Row {
                id: segRow
                anchors.centerIn: parent
                spacing: 2

                Rectangle {
                    readonly property bool on: win.gridView && !win.classicMode
                    implicitWidth: segGr.implicitWidth + 22
                    height: 28
                    radius: 6
                    color: on ? Qt.rgba(1,1,1,0.08) : "transparent"
                    Row {
                        id: segGr
                        anchors.centerIn: parent
                        spacing: 6
                        IconImg {
                            anchors.verticalCenter: parent.verticalCenter
                            src: "qrc:/icons/grid.svg"
                            tint: parent.parent.on ? Theme.t1 : Theme.t3
                            s: 14
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: (i18n.language, i18n.t("view_grid"))
                            color: parent.parent.on ? Theme.t1 : Theme.t3
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            font.family: Theme.fontSans
                        }
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { win.classicMode = false; win.gridView = true } }
                }
                Rectangle {
                    readonly property bool on: win.classicMode
                    implicitWidth: segCl.implicitWidth + 22
                    height: 28
                    radius: 6
                    color: on ? Qt.rgba(1,1,1,0.08) : "transparent"
                    Row {
                        id: segCl
                        anchors.centerIn: parent
                        spacing: 6
                        IconImg {
                            anchors.verticalCenter: parent.verticalCenter
                            src: "qrc:/icons/list.svg"
                            tint: parent.parent.on ? Theme.t1 : Theme.t3
                            s: 14
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: (i18n.language, i18n.t("view_classic"))
                            color: parent.parent.on ? Theme.t1 : Theme.t3
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            font.family: Theme.fontSans
                        }
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { win.classicMode = true; win.gridView = false } }
                }
            }
        }

        // .pills (gap 4) — 6 pills, counts from session, click sets filter
        Row {
            Layout.alignment: Qt.AlignVCenter
            Layout.minimumWidth: implicitWidth      // pills never clip
            spacing: Theme.sp1
            Pill { label: (i18n.language, i18n.t("filter_all"));     filterKey: "all";         count: typeof session !== "undefined" ? session.torrentCount : 0;     onClicked: win.setFilter("all") }
            Pill { label: (i18n.language, i18n.t("filter_all_active"));    filterKey: "active";      count: typeof session !== "undefined" ? session.activeCount : 0;      onClicked: win.setFilter("active") }
            Pill { label: (i18n.language, i18n.t("filter_downloading"));  filterKey: "downloading"; count: typeof session !== "undefined" ? session.downloadingCount : 0; onClicked: win.setFilter("downloading") }
            Pill { label: (i18n.language, i18n.t("filter_seeding"));  filterKey: "seeding";     count: typeof session !== "undefined" ? session.seedingCount : 0;     onClicked: win.setFilter("seeding") }
            Pill { label: (i18n.language, i18n.t("filter_paused"));   filterKey: "paused";      count: typeof session !== "undefined" ? session.pausedCount : 0;      onClicked: win.setFilter("paused") }
            Pill { label: (i18n.language, i18n.t("filter_completed")); filterKey: "completed";   count: typeof session !== "undefined" ? session.completedCount : 0;   onClicked: win.setFilter("completed") }
        }

        Item { Layout.fillWidth: true }

        // .cat (Todas as categorias + chevron)
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 34
            Layout.minimumWidth: implicitWidth      // never clip the category dropdown
            implicitWidth: catRow.implicitWidth + 24
            color: "transparent"
            border.color: Theme.hair
            border.width: 1
            radius: 8

            Row {
                id: catRow
                anchors.centerIn: parent
                spacing: 8
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: win.catFilter.length > 0 ? win.catLabel(win.catFilter) : (i18n.language, i18n.t("filter_all_categories"))
                    color: win.catFilter.length > 0 ? Theme.t1 : Theme.t2
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                }
                IconImg {
                    anchors.verticalCenter: parent.verticalCenter
                    src: "qrc:/icons/chevron.svg"
                    tint: Theme.t4
                    s: 13
                }
            }
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: catFilterMenu.open()
            }
            Menu {
                id: catFilterMenu
                y: parent.height + 4
                implicitWidth: 200
                delegate: CatItem {}
                background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
                CatItem { text: (i18n.language, i18n.t("filter_all_categories")); onTriggered: win.applyCatFilter("") }
                MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
                CatItem { text: win.catLabel("Apps");   onTriggered: win.applyCatFilter("Apps") }
                CatItem { text: win.catLabel("Games");  onTriggered: win.applyCatFilter("Games") }
                CatItem { text: win.catLabel("Movies"); onTriggered: win.applyCatFilter("Movies") }
                CatItem { text: win.catLabel("Series"); onTriggered: win.applyCatFilter("Series") }
            }
        }

        // donate moved to the nav rail (bottom) — it was cramped here
        // and got clipped when the filter row filled up.
        // (the port indicator now lives in the status bar: it's status,
        // not a filter, and it was the first thing to clip here)
    }
}
