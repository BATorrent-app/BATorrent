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

    // Overflow affordance for the pills row: a panel-colored fade over the cut
    // edge, plus a scroll arrow that shows itself on hover (Netflix-style).
    component EdgeScroller: Item {
        id: edge
        property bool rightSide: true
        property bool active: false
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 52
        visible: active

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: edge.rightSide ? "transparent" : Theme.panel }
                GradientStop { position: 1.0; color: edge.rightSide ? Theme.panel : "transparent" }
            }
        }
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: edge.rightSide ? parent.right : undefined
            anchors.left: edge.rightSide ? undefined : parent.left
            anchors.rightMargin: 4
            anchors.leftMargin: 4
            width: 28; height: 28; radius: 14
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            opacity: edgeMa.containsMouse ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: Theme.durFast } }
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/chevron.svg"
                tint: Theme.t1
                s: 14
                rotation: edge.rightSide ? -90 : 90
            }
        }
        MouseArea {
            id: edgeMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: pillsFlick.animateTo(pillsFlick.contentX
                           + (edge.rightSide ? 1 : -1) * pillsFlick.width * 0.7)
        }
    }

    Layout.fillWidth: true
    Layout.preferredHeight: 54
    color: Theme.panel
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
            color: Theme.field
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

        // Pills + category live in a horizontal Flickable: on a narrow window
        // they scroll instead of clipping, with a fade + hover arrow on each
        // overflowing edge. The search box and the grid/classic toggle stay put.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Flickable {
                id: pillsFlick
                anchors.fill: parent
                contentWidth: pillsContent.implicitWidth
                contentHeight: height
                clip: true
                flickableDirection: Flickable.HorizontalFlick
                boundsBehavior: Flickable.StopAtBounds
                interactive: contentWidth > width + 1

                readonly property bool canScrollRight: contentWidth > width + 1
                                                       && contentX < contentWidth - width - 1
                readonly property bool canScrollLeft: contentX > 1

                function animateTo(x) {
                    pillScrollAnim.stop()
                    pillScrollAnim.from = contentX
                    pillScrollAnim.to = Math.max(0, Math.min(x, contentWidth - width))
                    pillScrollAnim.start()
                }
                NumberAnimation {
                    id: pillScrollAnim
                    target: pillsFlick
                    property: "contentX"
                    duration: 280
                    easing.type: Easing.OutCubic
                }

                Row {
                    id: pillsContent
                    height: pillsFlick.height
                    spacing: Theme.sp3

                    // .pills (gap 4) — 7 pills, counts from session, click sets filter
                    Row {
                        id: pillsRow
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.sp1
                        Pill { label: (i18n.language, i18n.t("filter_all"));     filterKey: "all";         count: typeof session !== "undefined" ? session.torrentCount : 0;     onClicked: win.setFilter("all") }
                        Pill { label: (i18n.language, i18n.t("filter_all_active"));    filterKey: "active";      count: typeof session !== "undefined" ? session.activeCount : 0;      onClicked: win.setFilter("active") }
                        Pill { label: (i18n.language, i18n.t("filter_downloading"));  filterKey: "downloading"; count: typeof session !== "undefined" ? session.downloadingCount : 0; onClicked: win.setFilter("downloading") }
                        Pill { label: (i18n.language, i18n.t("filter_seeding"));  filterKey: "seeding";     count: typeof session !== "undefined" ? session.seedingCount : 0;     onClicked: win.setFilter("seeding") }
                        Pill { label: (i18n.language, i18n.t("filter_paused"));   filterKey: "paused";      count: typeof session !== "undefined" ? session.pausedCount : 0;      onClicked: win.setFilter("paused") }
                        // always visible, like the other pills (tester asked for it to stay
                        // put next to Paused/Completed instead of appearing only when a queue
                        // limit is holding torrents back)
                        Pill {
                            label: (i18n.language, i18n.t("filter_queued")); filterKey: "queued"
                            count: typeof session !== "undefined" ? session.queuedCount : 0
                            onClicked: win.setFilter("queued")
                        }
                        Pill { label: (i18n.language, i18n.t("filter_completed")); filterKey: "completed";   count: typeof session !== "undefined" ? session.completedCount : 0;   onClicked: win.setFilter("completed") }
                    }

                    // keeps the category button right-aligned while everything fits
                    Item {
                        width: Math.max(0, pillsFlick.width - pillsRow.implicitWidth
                                           - catBox.implicitWidth - 2 * pillsContent.spacing)
                        height: 1
                    }

                    // .cat (Todas as categorias + chevron)
                    Rectangle {
                        id: catBox
                        anchors.verticalCenter: parent.verticalCenter
                        height: 34
                        implicitWidth: catRow.implicitWidth + 24
                        width: implicitWidth
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
                }
            }

            EdgeScroller { rightSide: true;  active: pillsFlick.canScrollRight; anchors.right: parent.right }
            EdgeScroller { rightSide: false; active: pillsFlick.canScrollLeft;  anchors.left: parent.left }
        }

        // donate moved to the nav rail (bottom) — it was cramped here
        // and got clipped when the filter row filled up.
        // (the port indicator now lives in the status bar: it's status,
        // not a filter, and it was the first thing to clip here)
    }
}
