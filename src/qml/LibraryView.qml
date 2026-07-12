// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The Downloads library: the torrent grid or list (+ empty state). Carved out of
// Main.qml; reads window state via `win`, exposes the grid/list views by alias so
// the parent's keyboard navigation (gridCols/moveSel) can reach them, and signals
// add-magnet intent. HCol is its private sortable list-header column.
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: libraryView
    property var win
    property alias grid: grid
    property alias list: list
    signal addMagnetRequested()

    component HCol: Item {
        id: hc
        property string label
        property string col
        property bool fill: false
        property int w: 78
        property bool alignRight: false
        Layout.fillWidth: fill
        Layout.preferredWidth: fill ? -1 : w
        Layout.fillHeight: true
        Row {
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: hc.alignRight ? undefined : parent.left
            anchors.right: hc.alignRight ? parent.right : undefined
            spacing: 4
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: hc.label
                // anime art sits behind the right columns — lift the weak grey + a contrasting
                // outline so headers stay legible over both dark and bright parts of the art
                color: win.sortColumn === hc.col ? Theme.t2 : (hcMa.containsMouse ? Theme.t3 : (Theme.hasAnime ? Theme.t2 : Theme.t4))
                style: Theme.hasAnime ? Text.Outline : Text.Normal
                styleColor: Theme.isLight ? "#ffffff" : "#000000"
                font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: win.sortColumn === hc.col
                text: win.sortAsc ? "▲" : "▼"
                color: Theme.accent
                font.pixelSize: 7
            }
        }
        MouseArea { id: hcMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.toggleSort(hc.col) }
    }

    Layout.fillWidth: true
    Layout.fillHeight: true
    clip: true

    readonly property bool empty: typeof session !== "undefined" && session.torrentCount === 0

    // full custom background image (z:-1, behind anime art and grid/list).
    // A theme-bg scrim at user-controlled opacity sits on top for legibility.
    Item {
        id: bgImageWrap
        anchors.fill: parent
        visible: Theme.hasBgImage && !parent.empty
        z: -1
        Image {
            anchors.fill: parent
            source: Theme.bgImageSource
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            sourceSize.width: parent.width
            sourceSize.height: parent.height
            cache: false
        }
        Rectangle {
            anchors.fill: parent
            color: Theme.bg
            opacity: Theme.bgImageOpacity
        }
    }

    // empty state (no torrents)
    EmptyState {
        anchors.centerIn: parent
        visible: parent.empty
        onOpenClicked: openFileDlg.open()
        onMagnetClicked: libraryView.addMagnetRequested()
    }


    // anime art (eyes top-right / spider bottom-right). Ported 1:1 from .eyes-accent:
    // the CSS fades the edges via two intersected linear masks; since only Theme.bg sits
    // behind the art, we reproduce it with two bg-colored gradient scrims (left + bottom/top).
    Item {
        id: animeArtWrap
        visible: Theme.hasAnime && !parent.empty
        width: Math.min(Theme.animeBottom ? 560 : 460, parent.width * 0.46)
        height: animeArt.implicitWidth > 0 ? animeArt.implicitHeight * (width / animeArt.implicitWidth) : 0
        anchors.right: parent.right
        anchors.top: Theme.animeBottom ? undefined : parent.top
        anchors.bottom: Theme.animeBottom ? parent.bottom : undefined
        anchors.bottomMargin: Theme.animeBottom ? -80 : 0   // spider sits lower (peeks from bottom)
        z: 0

        Image {
            id: animeArt
            anchors.fill: parent
            source: Theme.hasAnime ? Theme.animeSource : ""
            fillMode: Image.PreserveAspectFit
            // list rows put state/peer columns right on top of the art —
            // drop it to a watermark there so data wins the contrast fight
            opacity: win.gridView ? 0.9 : 0.25
            Behavior on opacity { NumberAnimation { duration: Theme.durSlow; easing.type: Easing.OutCubic } }
        }
        // fade left edge (mask: linear-gradient(90deg, transparent, #000 55%))
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Theme.bg }
                GradientStop { position: 0.55; color: "transparent" }
            }
        }
        // fade bottom (eyes) / top (spider) — mask: linear-gradient(180deg, #000 60%, transparent)
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: Theme.animeBottom ? Theme.bg : "transparent" }
                GradientStop { position: Theme.animeBottom ? 0.40 : 0.60; color: "transparent" }
                GradientStop { position: 1.0; color: Theme.animeBottom ? "transparent" : Theme.bg }
            }
        }
    }

    // ----- GRID -----
    GridView {
        id: grid
        opacity: (win.gridView && !parent.empty) ? 1 : 0
        visible: opacity > 0.01
        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
        anchors.fill: parent
        topMargin: Theme.sp5
        bottomMargin: Theme.sp5
        leftMargin: Theme.sp4
        rightMargin: Theme.sp4
        cellWidth: 178 + Theme.sp4
        cellHeight: 286
        WheelScroller { flick: grid }
        // No `populate` transition: it re-runs (fading every tile from 0)
        // when the filter proxy reports a reorder as a re-layout, which read
        // as the whole grid flashing. The container's opacity Behavior already
        // covers the initial fade-in. (List view has no populate, never flashed.)
        add: Transition {
            NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 180; easing.type: Easing.OutCubic }
            NumberAnimation { properties: "scale"; from: 0.9; to: 1; duration: 180; easing.type: Easing.OutCubic }
        }
        // deleting several selected torrents at once fires these back-to-back
        // with no time to settle between them — a known Qt Quick view-recycling
        // risk — so a batch removal (bulkRemoveInProgress) skips the animation
        // instead of stacking transitions; a single remove keeps it.
        readonly property bool bulkRemove: typeof session !== "undefined" && session.bulkRemoveInProgress
        remove: Transition {
            NumberAnimation { properties: "opacity"; to: 0; duration: grid.bulkRemove ? 0 : 160; easing.type: Easing.OutCubic }
            NumberAnimation { properties: "scale"; to: 0.85; duration: grid.bulkRemove ? 0 : 160; easing.type: Easing.OutCubic }
        }
        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: grid.bulkRemove ? 0 : 280; easing.type: Easing.OutBack; easing.overshoot: 0.9 }
            NumberAnimation { properties: "scale"; to: 1; duration: grid.bulkRemove ? 0 : 280; easing.type: Easing.OutCubic }
        }
        move: Transition {
            NumberAnimation { properties: "x,y"; duration: 300; easing.type: Easing.OutBack; easing.overshoot: 1.1 }
        }
        moveDisplaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 280; easing.type: Easing.OutBack; easing.overshoot: 0.9 }
        }
        clip: true
        model: win.model
        interactive: true
        z: 1

        delegate: PosterTile { win: libraryView.win }
    }

    // Empty-grid click → clear selection. Sits OVER the grid (z:2) and
    // propagates clicks that land on a tile so tileMa still handles them;
    // only clicks on blank space (grid.indexAt == -1) deselect. A plain
    // MouseArea inside the Flickable never received these.
    MouseArea {
        anchors.fill: grid
        visible: win.gridView && !parent.empty
        enabled: visible
        z: 2
        acceptedButtons: Qt.LeftButton
        propagateComposedEvents: true
        onClicked: function(mouse) {
            var idx = grid.indexAt(mouse.x + grid.contentX, mouse.y + grid.contentY)
            if (idx < 0) {
                if (win.selectedRows.length > 0) {
                    win.selectedRows = []; win.selected = -1; win._commitSel()
                }
            } else {
                mouse.accepted = false   // let the tile's MouseArea handle it
            }
        }
        onPressed: function(mouse) {
            // don't swallow the press over a tile, or scrolling/clicks break
            if (grid.indexAt(mouse.x + grid.contentX, mouse.y + grid.contentY) >= 0)
                mouse.accepted = false
        }
    }

    // ----- LIST -----
    ListView {
        id: list
        opacity: (!win.gridView && !parent.empty) ? 1 : 0
        visible: opacity > 0.01
        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
        anchors.fill: parent
        clip: true
        model: win.model
        interactive: true
        z: 1
        WheelScroller { flick: list }
        readonly property bool bulkRemove: typeof session !== "undefined" && session.bulkRemoveInProgress
        add: Transition { NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 160; easing.type: Easing.OutCubic } }
        remove: Transition { NumberAnimation { properties: "opacity"; to: 0; duration: list.bulkRemove ? 0 : 120; easing.type: Easing.OutCubic } }
        displaced: Transition { NumberAnimation { properties: "x,y"; duration: list.bulkRemove ? 0 : 180; easing.type: Easing.OutCubic } }

        header: Rectangle {
            width: ListView.view.width
            height: 36
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp4

                HCol { label: (i18n.language, i18n.t("col_name")); col: "name"; fill: true }
                HCol { label: (i18n.language, i18n.t("col_size")); col: "size"; w: 72; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_progress")); col: "progress"; w: 96 }
                HCol { label: (i18n.language, i18n.t("col_state")); col: "state"; w: 104 }
                HCol { label: (i18n.language, i18n.t("col_seeds")); col: "seeds"; w: 44; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_peers")); col: "peers"; w: 44; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_down")); col: "down"; w: 74; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_up")); col: "up"; w: 74; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_ratio")); col: "ratio"; w: 50; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_eta")); col: "eta"; w: 66; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_category")); col: "category"; w: 84 }
            }
        }

        delegate: TorrentRow { win: libraryView.win }
    }

    // ----- marquee + click/hover overlay for the list -----
    MouseArea {
        id: listArea
        anchors.fill: list
        visible: !win.gridView && !parent.empty
        enabled: visible
        hoverEnabled: true
        preventStealing: true   // don't let the ListView steal the gesture
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        z: 2

        property int hoveredRow: -1
        readonly property int rowH: 56
        property bool dragging: false
        property real startX: 0
        property real startY: 0
        property int pressRow: -1

        // "stalled why" tooltip over the State cell — owned here because
        // this hover-exclusive MouseArea starves in-delegate handlers
        property string tipText: ""
        property point tipPos: Qt.point(0, 0)
        Timer {
            id: stateTipDelay
            interval: 350
            onTriggered: if (listArea.tipText.length > 0) stateTip.visible = true
        }
        function updateStateTip(mx, my) {
            var row = rowAt(my, mx)
            // itemAtIndex can return null (row not instantiated) or a pooled
            // delegate mid-teardown — guard stateCell too before mapToItem.
            var d = row >= 0 ? list.itemAtIndex(row) : null
            if (d && d.stateCell && d.stateDetail !== undefined && d.stateDetail.length > 0 && !dragging) {
                var p = d.stateCell.mapToItem(listArea, 0, 0)
                if (mx >= p.x - 6 && mx <= p.x + d.stateCell.width + 6) {
                    tipText = d.stateDetail
                    tipPos = Qt.point(p.x, p.y + 26)
                    if (!stateTip.visible) stateTipDelay.restart()
                    return
                }
            }
            tipText = ""
            stateTipDelay.stop()
            stateTip.visible = false
        }
        Rectangle {
            id: stateTip
            visible: false
            x: Math.max(8, Math.min(listArea.tipPos.x, listArea.width - width - 8))
            y: listArea.tipPos.y
            z: 10
            radius: 7
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            width: stateTipText.implicitWidth + 20
            height: stateTipText.implicitHeight + 12
            Text {
                id: stateTipText
                anchors.centerIn: parent
                text: listArea.tipText
                color: Theme.t1
                font.pixelSize: 11
                font.family: Theme.fontSans
            }
        }

        function rowAt(my, mx) {
            if (!win.model || list.count === 0) return -1
            // use the ListView's own layout so detection lines up exactly
            // with where each delegate is drawn (header returns -1).
            return list.indexAt((mx === undefined ? width / 2 : mx) + list.contentX,
                                my + list.contentY)
        }

        onPositionChanged: function(mouse) {
            hoveredRow = rowAt(mouse.y, mouse.x)
            updateStateTip(mouse.x, mouse.y)
            if (pressed && !dragging &&
                (Math.abs(mouse.x - startX) > 8 || Math.abs(mouse.y - startY) > 8))
                dragging = true
            if (dragging) {
                marquee.x = Math.min(startX, mouse.x)
                marquee.y = Math.min(startY, mouse.y)
                marquee.width = Math.abs(mouse.x - startX)
                marquee.height = Math.abs(mouse.y - startY)
            }
        }
        onExited: { hoveredRow = -1; tipText = ""; stateTipDelay.stop(); stateTip.visible = false }
        onPressed: function(mouse) {
            startX = mouse.x; startY = mouse.y
            pressRow = rowAt(mouse.y, mouse.x)
            dragging = false
        }
        onReleased: function(mouse) {
            // a real marquee = dragged past threshold AND box big enough
            if (dragging && (marquee.width > 6 || marquee.height > 6)) {
                var top = marquee.y, bot = marquee.y + marquee.height
                var rows = []
                for (var i = 0; i < list.count; ++i) {
                    var ry = list.headerItem.height + i * rowH - list.contentY
                    if (ry + rowH > top && ry < bot) rows.push(i)
                }
                win.selectedRows = rows
                win.selected = rows.length > 0 ? rows[rows.length - 1] : -1
                win.anchorRow = rows.length > 0 ? rows[0] : -1
                win._commitSel()
                dragging = false
                return
            }
            dragging = false
            // otherwise treat as a click (use release position, robust to jitter)
            var clickRow = rowAt(mouse.y, mouse.x)
            if (clickRow < 0) {
                if (mouse.button === Qt.LeftButton) {
                    win.selectedRows = []; win.selected = -1; win._commitSel()
                }
                return
            }
            if (mouse.button === Qt.RightButton) {
                if (!win.isRowSelected(clickRow)) win.selectRow(clickRow, 0)
                win.openContext(clickRow)
            } else {
                win.selectRow(clickRow, mouse.modifiers)
            }
        }
        onDoubleClicked: function(mouse) {
            var r = rowAt(mouse.y, mouse.x)
            // select the clicked row first so we reveal *that* torrent,
            // not whatever was selected before, then open its folder
            // with the file highlighted.
            if (r >= 0) { win.selectRow(r, 0); session.openSelectedFile() }
        }

        Rectangle {
            id: marquee
            visible: listArea.dragging
            color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12)
            border.color: Theme.accent
            border.width: 1
            radius: 2
        }
    }
}

