// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Filter bar: search box, grid/list toggle, status-filter pills (the rare
// ones behind "More"), category menu.
// Carved out of Main.qml; reads/writes window state via `win`, exposes the search
// field via the `searchInput` alias for the parent's focus shortcut.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"
import "../widgets"

Rectangle {
    id: filterBar
    property var win
    property var controller
    property var countOverride: null
    property alias searchInput: searchInput

    function countFor(key, fallback) {
        return countOverride !== null && countOverride[key] !== undefined
            ? countOverride[key] : fallback
    }

    // The accent ring means "you're typing here". It has to go the moment
    // attention moves elsewhere: a TextInput keeps activeFocus until something
    // else asks for it, so every interaction that isn't typing calls this.
    function clearSearchFocus() { if (searchInput.activeFocus) searchInput.focus = false }

    component Pill: Rectangle {
        id: pi
        property string label
        property string count
        property string filterKey: "all"   // NOT `state`: that shadows Item.state
        property bool on: controller.activeFilter === filterKey
        property bool warn: false          // Problems: a notch brighter when idle
        signal clicked()
        radius: 8
        height: 30
        implicitWidth: pillRow.implicitWidth + 24
        color: on ? Theme.accentTint : (piMa.containsMouse ? Theme.hover : "transparent")

        // announced as a radio: the pills are one exclusive choice, and the count
        // belongs in the name or the reader never says how many matched
        Accessible.role: Accessible.RadioButton
        Accessible.name: pi.count.length > 0 ? (pi.label + ", " + pi.count) : pi.label
        Accessible.checked: pi.on
        Accessible.onPressAction: pi.clicked()
        activeFocusOnTab: true
        Keys.onReturnPressed: pi.clicked()
        Keys.onSpacePressed: pi.clicked()
        scale: piMa.pressed ? Theme.pressScale : 1
        Behavior on scale { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
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
                id: pillLabel
                anchors.verticalCenter: parent.verticalCenter
                text: pi.label
                color: pi.on ? Theme.accentText : (piMa.containsMouse || pi.warn ? Theme.t2 : Theme.t3)
                font.pixelSize: 13
                font.family: Theme.fontSans
                font.weight: pi.on ? Font.Medium : Font.Normal
            }
            Text {
                // baseline, not verticalCenter: the count is a size smaller, and
                // centring two different type sizes aligns their line boxes while
                // leaving the baselines a pixel apart: the number visibly floats
                anchors.baseline: pillLabel.baseline
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

    component SegCell: Rectangle {
        id: sc
        property bool on: false
        property string icon
        property string tip
        signal clicked()
        // icon and name: an icon-only toggle was easy to miss
        width: scRow.implicitWidth + 20; height: 26
        radius: 6
        color: on ? Theme.hover2 : (scMa.containsMouse ? Theme.hover : "transparent")
        Row {
            id: scRow
            anchors.centerIn: parent
            spacing: 6
            IconImg { anchors.verticalCenter: parent.verticalCenter; src: sc.icon; tint: sc.on ? Theme.t1 : Theme.t3; s: 14 }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: sc.tip
                color: sc.on ? Theme.t1 : (scMa.containsMouse ? Theme.t2 : Theme.t3)
                font.pixelSize: 13
                font.weight: sc.on ? Font.Medium : Font.Normal
                font.family: Theme.fontSans
            }
        }
        MouseArea { id: scMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: sc.clicked() }
        Accessible.role: Accessible.RadioButton
        Accessible.name: sc.tip
        Accessible.checked: sc.on
        Accessible.onPressAction: sc.clicked()
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
    Layout.preferredHeight: 48
    color: Theme.panel
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp4
        spacing: Theme.sp3

        // .search (240×34, padding 0 11, gap 8, bg panel): the one
        // element that absorbs the shrink: it gives back width down to
        // 150 so the pills/category never have to clip.
        Rectangle {
            Layout.preferredWidth: 240
            Layout.minimumWidth: 150
            Layout.preferredHeight: 32
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
                        text: (i18n.language, i18n.t("filter_downloads_hint"))
                        color: Theme.t4
                        font: searchInput.font
                        visible: searchInput.text.length === 0 && !searchInput.activeFocus
                    }
                }
            }
        }

        // Grid / List
        Rectangle {
            Layout.alignment: Qt.AlignVCenter
            Layout.preferredHeight: 32
            Layout.minimumWidth: implicitWidth
            implicitWidth: segRow.implicitWidth + 6
            color: "transparent"
            border.color: Theme.hair
            border.width: 1
            radius: 8
            Row {
                id: segRow
                anchors.centerIn: parent
                spacing: 2
                SegCell {
                    on: controller.gridView && !controller.classicMode
                    icon: "qrc:/icons/grid.svg"
                    tip: (i18n.language, i18n.t("view_grid"))
                    onClicked: { controller.classicMode = false; controller.gridView = true }
                }
                SegCell {
                    on: controller.classicMode
                    icon: "qrc:/icons/list.svg"
                    tip: (i18n.language, i18n.t("view_list"))
                    onClicked: { controller.classicMode = true; controller.gridView = false }
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
                    duration: Theme.durSlower
                    easing.type: Theme.easeOut
                }

                Row {
                    id: pillsContent
                    height: pillsFlick.height
                    spacing: Theme.sp3

                    // the four everyone uses, Problems, and the rest behind More
                    Row {
                        id: pillsRow
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: Theme.sp1
                        Pill { label: (i18n.language, i18n.t("filter_all")); filterKey: "all"; count: filterBar.countFor("all", typeof session !== "undefined" ? session.torrentCount : 0); onClicked: controller.setFilter("all") }
                        Pill { label: (i18n.language, i18n.t("filter_downloading")); filterKey: "downloading"; count: filterBar.countFor("downloading", typeof session !== "undefined" ? session.downloadingCount : 0); onClicked: controller.setFilter("downloading") }
                        Pill { label: (i18n.language, i18n.t("filter_done")); filterKey: "completed"; count: filterBar.countFor("completed", typeof session !== "undefined" ? session.completedCount : 0); onClicked: controller.setFilter("completed") }
                        Pill { label: (i18n.language, i18n.t("filter_paused")); filterKey: "paused"; count: filterBar.countFor("paused", typeof session !== "undefined" ? session.pausedCount : 0); onClicked: controller.setFilter("paused") }
                        Pill {
                            readonly property int n: filterBar.countFor("problems", typeof session !== "undefined" ? session.problemCount : 0)
                            visible: n > 0 || on
                            warn: n > 0
                            label: (i18n.language, i18n.t("filter_problems")); filterKey: "problems"; count: n
                            onClicked: controller.setFilter("problems")
                        }
                        Pill {
                            id: morePill
                            readonly property var keys: ["active", "seeding", "queued"]
                            readonly property int idx: keys.indexOf(controller.activeFilter)
                            on: idx >= 0
                            filterKey: "_more"
                            label: (i18n.language, idx === 0 ? i18n.t("filter_all_active")
                                   : idx === 1 ? i18n.t("filter_seeding")
                                   : idx === 2 ? i18n.t("filter_queued") : i18n.t("filter_more"))
                            count: idx === 0 ? filterBar.countFor("active", typeof session !== "undefined" ? session.activeCount : 0)
                                 : idx === 1 ? filterBar.countFor("seeding", typeof session !== "undefined" ? session.seedingCount : 0)
                                 : idx === 2 ? filterBar.countFor("queued", typeof session !== "undefined" ? session.queuedCount : 0) : ""
                            onClicked: moreMenu.opened ? moreMenu.close() : moreMenu.popup(morePill, 0, morePill.height + 4)
                            DMenu {
                                id: moreMenu
                                implicitWidth: 200
                                DMenuItem { text: (i18n.language, i18n.t("filter_all_active")); shortcutText: typeof session !== "undefined" ? String(session.activeCount) : ""; checkable: true; checked: controller.activeFilter === "active"; onTriggered: controller.setFilter("active") }
                                DMenuItem { text: (i18n.language, i18n.t("filter_seeding")); shortcutText: typeof session !== "undefined" ? String(session.seedingCount) : ""; checkable: true; checked: controller.activeFilter === "seeding"; onTriggered: controller.setFilter("seeding") }
                                DMenuItem { text: (i18n.language, i18n.t("filter_queued")); shortcutText: typeof session !== "undefined" ? String(session.queuedCount) : ""; checkable: true; checked: controller.activeFilter === "queued"; onTriggered: controller.setFilter("queued") }
                            }
                        }
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
                        height: 32
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
                                text: controller.catFilter.length > 0 ? win.catLabel(controller.catFilter) : (i18n.language, i18n.t("filter_all_categories"))
                                color: controller.catFilter.length > 0 ? Theme.t1 : Theme.t2
                                font.pixelSize: 13
                                font.family: Theme.fontSans
                            }
                            IconImg {
                                anchors.verticalCenter: parent.verticalCenter
                                src: "qrc:/icons/chevron.svg"
                                tint: Theme.t4
                                s: 11
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: catFilterMenu.opened ? catFilterMenu.close()
                                                            : catFilterMenu.open()
                        }
                        Menu {
                            id: catFilterMenu
                            y: parent.height + 4
                            // Never narrower than the control it drops from: a
                            // menu that is wider on one side and shorter on the
                            // other does not read as belonging to it.
                            implicitWidth: Math.max(230, parent.width)
                            topPadding: 6
                            bottomPadding: 6
                            // Pressing the trigger must reach onClicked and
                            // toggle; the default also closes on a press over
                            // the parent, so the menu shut and reopened in one
                            // click and never looked like it closed at all.
                            closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent
                            transformOrigin: Item.TopLeft
                            // The same open and close BatMenu uses.
                            enter: Transition {
                                ParallelAnimation {
                                    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.durFast; easing.type: Theme.easeOut }
                                    NumberAnimation { property: "scale"; from: Theme.grow(0.96); to: 1.0; duration: Theme.durBase; easing.type: Theme.easeOut }
                                }
                            }
                            exit: Transition {
                                NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: Theme.durExit; easing.type: Theme.easeIn }
                            }
                            delegate: CatItem {}
                            // Counted once per opening, from the proxy that does
                            // the filtering, so the number and the list it opens
                            // are the same answer.
                            property var counts: ({})
                            function countOf(c) { return counts[c] === undefined ? 0 : counts[c] }
                            background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
                            // The four built-ins are static; user-created ones are
                            // appended on open: the same rule the right-click menu
                            // follows. This list was hardcoded too, so a category
                            // you created could be assigned but never filtered by.
                            onAboutToShow: {
                                catFilterMenu.counts = (typeof torrentFilter !== "undefined")
                                    ? torrentFilter.categoryCounts() : ({})
                                catFilterCustoms.model = win.customCategories()
                            }
                            CatItem {
                                text: (i18n.language, i18n.t("filter_all_categories"))
                                count: typeof session !== "undefined" ? session.torrentCount : 0
                                current: controller.catFilter === ""
                                onTriggered: controller.applyCatFilter("")
                            }
                            MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
                            CatItem { text: win.catLabel("Apps");   count: catFilterMenu.countOf("Apps");   current: controller.catFilter === "Apps";   onTriggered: controller.applyCatFilter("Apps") }
                            CatItem { text: win.catLabel("Games");  count: catFilterMenu.countOf("Games");  current: controller.catFilter === "Games";  onTriggered: controller.applyCatFilter("Games") }
                            CatItem { text: win.catLabel("Movies"); count: catFilterMenu.countOf("Movies"); current: controller.catFilter === "Movies"; onTriggered: controller.applyCatFilter("Movies") }
                            CatItem { text: win.catLabel("Series"); count: catFilterMenu.countOf("Series"); current: controller.catFilter === "Series"; onTriggered: controller.applyCatFilter("Series") }
                            CatItem { text: win.catLabel("Compressed"); count: catFilterMenu.countOf("Compressed"); current: controller.catFilter === "Compressed"; onTriggered: controller.applyCatFilter("Compressed") }
                            Instantiator {
                                id: catFilterCustoms
                                model: []
                                delegate: CatItem {
                                    required property var modelData
                                    text: modelData
                                    count: catFilterMenu.countOf(modelData)
                                    current: controller.catFilter === modelData
                                    onTriggered: controller.applyCatFilter(modelData)
                                }
                                onObjectAdded: function(index, object) { catFilterMenu.insertItem(6 + index, object) }
                                onObjectRemoved: function(index, object) { catFilterMenu.removeItem(object) }
                            }
                        }
                    }
                }
            }

            EdgeScroller { rightSide: true;  active: pillsFlick.canScrollRight; anchors.right: parent.right }
            EdgeScroller { rightSide: false; active: pillsFlick.canScrollLeft;  anchors.left: parent.left }
        }

    }
}
