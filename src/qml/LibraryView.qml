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
        remove: Transition {
            NumberAnimation { properties: "opacity"; to: 0; duration: 160; easing.type: Easing.OutCubic }
            NumberAnimation { properties: "scale"; to: 0.85; duration: 160; easing.type: Easing.OutCubic }
        }
        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 280; easing.type: Easing.OutBack; easing.overshoot: 0.9 }
            NumberAnimation { properties: "scale"; to: 1; duration: 280; easing.type: Easing.OutCubic }
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

        delegate: Item {
            id: tile
            width: 178
            height: 286

            required property int index
            required property string torrentName
            required property string metaTitle
            required property string stateKey
            required property real progress
            required property string posterPath
            required property string stateString
            required property string stateDetail
            required property string category
            required property string size
            required property string downSpeed
            required property string upSpeed
            required property real downRate
            required property var sizeBytes

            readonly property bool isDownloading: stateKey !== "seeding" && stateKey !== "finished"
                && stateKey !== "completed" && stateKey !== "paused" && stateKey !== "queued"
            readonly property int etaSec: (downRate > 0 && progress < 1.0 && sizeBytes > 0)
                ? Math.round(sizeBytes * (1 - progress) / downRate) : -1

            readonly property string posterUrl: win.fileUrl(posterPath)

            // soft drop shadow under the cover, fading in on hover
            Rectangle {
                z: -1
                width: 178 * 0.84
                x: (178 - width) / 2
                y: 237 - 10
                height: 22
                radius: 11
                color: "#000000"
                opacity: tileMa.containsMouse ? 0.5 : 0
                Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
                layer.enabled: true
                layer.effect: MultiEffect { blurEnabled: true; blur: 1.0; blurMax: 28 }
            }

            // .poster wrapper (aspect 3:4 ≈ 178:237)
            Item {
                id: posterWrap
                width: 178
                height: 237

                // fallback (no poster): tinted bg + watermark + category + title
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: "#161618"
                    visible: tile.posterUrl === ""
                    // watermark: BATorrent logo (not the title's first letter)
                    Image {
                        anchors.centerIn: parent
                        width: parent.width * 0.5
                        height: width
                        source: "qrc:/images/logo.svg"
                        sourceSize: Qt.size(width * 2, width * 2)
                        fillMode: Image.PreserveAspectFit
                        opacity: 0.06
                        layer.enabled: Theme.isLight
                        layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
                    }
                    Text {
                        anchors.left: parent.left; anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: 13; anchors.rightMargin: 13; anchors.bottomMargin: 15
                        text: tile.metaTitle || tile.torrentName
                        color: "#f5f5f6"
                        font.pixelSize: 18; font.weight: Font.Bold; font.letterSpacing: -0.3
                        font.family: Theme.fontSans
                        wrapMode: Text.WordWrap
                        maximumLineCount: 3
                        elide: Text.ElideRight
                    }
                }

                // poster image (masked rounded) — only when present
                Rectangle {
                    id: posterBg
                    anchors.fill: parent
                    color: "#161618"
                    visible: false
                    layer.enabled: true
                    Image {
                        anchors.fill: parent
                        source: tile.posterUrl
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        // decode at ~2× display size, not the poster's full
                        // resolution — cuts memory and decode time per cover.
                        sourceSize: Qt.size(356, 474)
                        cache: true
                    }
                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height * 0.6
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "transparent" }
                            GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.45) }
                            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.92) }
                        }
                    }
                }
                Rectangle {
                    id: posterMask
                    anchors.fill: parent
                    radius: 10
                    color: "white"
                    visible: false
                    layer.enabled: true
                }
                MultiEffect {
                    source: posterBg
                    anchors.fill: parent
                    maskEnabled: true
                    maskSource: posterMask
                    visible: tile.posterUrl !== ""
                }
                // title over the fade (only when poster present)
                Text {
                    visible: tile.posterUrl !== ""
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    anchors.bottomMargin: 12
                    text: tile.metaTitle || tile.torrentName
                    color: "#f5f5f6"
                    font.pixelSize: 15
                    font.weight: Font.Bold
                    font.letterSpacing: -0.2
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    wrapMode: Text.WordWrap
                }
                // .pbar progress (bottom, over everything)
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 3
                    color: Qt.rgba(0, 0, 0, 0.5)
                    Rectangle {
                        height: parent.height
                        width: parent.width * tile.progress
                        color: win.fillFor(tile.stateKey)
                        Behavior on width { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
                    }
                }
                // border overlay (radius 10, hair / accent when sel)
                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: "transparent"
                    border.color: win.isRowSelected(tile.index) ? Theme.accent : (tileMa.containsMouse ? Qt.rgba(1,1,1,0.2) : Theme.hair)
                    border.width: win.isRowSelected(tile.index) ? 2 : 1
                    Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
                }

                // category chip (top-left) — dark pill so it reads on any cover
                Rectangle {
                    visible: tile.category.length > 0
                    anchors.left: parent.left; anchors.top: parent.top
                    anchors.leftMargin: 8; anchors.topMargin: 8
                    radius: 5; color: "#99000000"
                    implicitWidth: catTxt.implicitWidth + 12; implicitHeight: 18
                    Text {
                        id: catTxt; anchors.centerIn: parent
                        text: tile.category
                        color: "#ffffff"; opacity: 0.88
                        font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0
                        font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                    }
                }
                // download % (top-right) — hidden once complete; tint follows state
                Rectangle {
                    visible: tile.progress < 0.999
                    anchors.right: parent.right; anchors.top: parent.top
                    anchors.rightMargin: 8; anchors.topMargin: 8
                    radius: 9; color: "#cc000000"
                    implicitWidth: pctTxt.implicitWidth + 14; implicitHeight: 18
                    Text {
                        id: pctTxt; anchors.centerIn: parent
                        text: Math.round(tile.progress * 100) + "%"
                        color: "#ffffff"
                        font.pixelSize: 10; font.weight: Font.Bold; font.family: Theme.fontSans
                    }
                }

                MouseArea {
                    id: tileMa
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            if (!win.isRowSelected(tile.index)) win.selectRow(tile.index, 0)
                            win.openContext(tile.index)
                        } else {
                            win.selectRow(tile.index, mouse.modifiers)
                        }
                    }
                    onDoubleClicked: function(mouse) {
                        if (mouse.button !== Qt.RightButton) {
                            win.selectRow(tile.index, 0); session.openSelectedFile()
                        }
                    }
                }
            }

            // meta — line 1: state dot + live info (speed·ETA when downloading,
            // else the status word); line 2: size. No redundant "Downloading":
            // the dot + the % pill + the bar already say it.
            Column {
                id: meta
                anchors.top: posterWrap.bottom
                anchors.topMargin: 10
                anchors.left: posterWrap.left
                anchors.right: posterWrap.right
                spacing: 2

                Item {
                    width: meta.width; height: 16
                    Row {
                        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                        spacing: 6
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            anchors.verticalCenter: parent.verticalCenter
                            // a stalled download reads amber (health), not the state colour
                            color: (tile.isDownloading && tile.stateDetail.length > 0) ? Theme.amber : win.dotFor(tile.stateKey)
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            // stalled reads amber (dot + text); the full reason is on hover —
                            // the grid card is too narrow to show it inline without clipping
                            text: tile.isDownloading ? ("↓ " + tile.downSpeed)
                                  : (tile.stateKey === "seeding" ? ("↑ " + tile.upSpeed) : tile.stateString)
                            color: (tile.isDownloading && tile.stateDetail.length > 0) ? Theme.amber : win.textFor(tile.stateKey)
                            font.pixelSize: 12; font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                    }
                    Text {
                        // ETA while downloading; once there's nothing left to
                        // fetch, the size takes this slot (line 2 collapses) so
                        // it's never left orphaned under an empty ETA.
                        anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                        text: tile.etaSec >= 0 ? win.fmtEta(tile.etaSec) : tile.size
                        color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontMono
                    }
                }
                Text {
                    width: meta.width
                    horizontalAlignment: Text.AlignRight
                    visible: tile.etaSec >= 0
                    text: tile.size
                    color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono
                }
            }
        }
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
        add: Transition { NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 160; easing.type: Easing.OutCubic } }
        remove: Transition { NumberAnimation { properties: "opacity"; to: 0; duration: 120; easing.type: Easing.OutCubic } }
        displaced: Transition { NumberAnimation { properties: "x,y"; duration: 180; easing.type: Easing.OutCubic } }

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
                HCol { label: (i18n.language, i18n.t("col_size")); col: "size"; w: 78; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_progress")); col: "progress"; w: 104 }
                HCol { label: (i18n.language, i18n.t("col_down")); col: "down"; w: 78; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_up")); col: "up"; w: 78; alignRight: true }
                HCol { label: (i18n.language, i18n.t("col_state")); col: "state"; w: 110 }
                HCol { label: (i18n.language, i18n.t("col_category")); col: "category"; w: 90 }
                HCol { label: (i18n.language, i18n.t("col_peers")); col: "peers"; w: 56; alignRight: true }
            }
        }

        delegate: Rectangle {
            id: lrow
            width: ListView.view.width
            height: 56

            required property int index
            required property string torrentName
            required property string metaTitle
            required property string stateKey
            required property real progress
            required property string stateString
            required property string stateDetail
            required property string size
            required property string downSpeed
            required property string upSpeed
            required property int downRate
            required property int upRate
            required property string category
            required property int numPeers
            required property string posterPath

            readonly property string posterUrl: win.fileUrl(posterPath)
            // the "stalled why" tooltip is driven by listArea (its z:2
            // hover-exclusive MouseArea starves any in-delegate handler)
            readonly property Item stateCell: lrowStateText

            color: win.isRowSelected(index) ? Theme.sel : (listArea.hoveredRow === index ? Theme.hover : "transparent")

            // .sel inset 2px barra esquerda
            Rectangle {
                visible: win.isRowSelected(lrow.index)
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 2
                color: Theme.accent
            }
            // border-bottom hairSoft
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp4
                anchors.rightMargin: Theme.sp4
                spacing: Theme.sp4

                // .name: poster thumb + nome
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Theme.sp3
                    PosterThumb {
                        Layout.alignment: Qt.AlignVCenter
                        posterUrl: lrow.posterUrl
                        label: lrow.metaTitle || lrow.torrentName || ""
                    }
                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        text: lrow.metaTitle || lrow.torrentName
                        color: Theme.t1
                        font.pixelSize: 13
                        font.family: Theme.fontSans
                        elide: Text.ElideRight
                    }
                }
                Text {
                    text: lrow.size
                    Layout.preferredWidth: 78
                    horizontalAlignment: Text.AlignRight
                    color: Theme.t2
                    font.pixelSize: 12
                    font.family: Theme.fontMono
                }
                // progress: surfaceAlt track, state-colored fill, centered % (white over fill, t1 over track)
                Item {
                    Layout.preferredWidth: 104
                    Layout.preferredHeight: 18
                    Rectangle {
                        id: pbarTrack
                        anchors.fill: parent
                        radius: 4
                        color: Theme.field
                        clip: true
                        Rectangle {
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            width: Math.max(lrow.progress > 0.001 ? 2 : 0, parent.width * lrow.progress)
                            radius: 4
                            color: win.fillFor(lrow.stateKey)
                        }
                        Text {
                            id: pbarPct
                            anchors.centerIn: parent
                            text: (lrow.progress * 100).toFixed(1) + "%"
                            color: (parent.width / 2) < (parent.width * lrow.progress - 4) ? "#ffffff" : Theme.t1
                            font.pixelSize: 9
                            font.weight: Font.DemiBold
                            font.family: Theme.fontSans
                        }
                    }
                }
                Text {
                    text: lrow.downRate > 0 ? lrow.downSpeed : "—"
                    Layout.preferredWidth: 78
                    horizontalAlignment: Text.AlignRight
                    color: lrow.downRate > 0 ? Theme.accentText : Theme.t4
                    font.pixelSize: 12
                    font.family: Theme.fontMono
                }
                Text {
                    text: lrow.upRate > 0 ? lrow.upSpeed : "—"
                    Layout.preferredWidth: 78
                    horizontalAlignment: Text.AlignRight
                    color: lrow.upRate > 0 ? Theme.up : Theme.t4
                    font.pixelSize: 12
                    font.family: Theme.fontMono
                }
                Text {
                    id: lrowStateText
                    text: lrow.stateString
                    Layout.preferredWidth: 110
                    color: win.textFor(lrow.stateKey)
                    font.pixelSize: 12
                    font.weight: Theme.hasAnime ? Font.DemiBold : Font.Medium
                    font.family: Theme.fontSans
                }
                Text {
                    text: win.catLabel(lrow.category)
                    Layout.preferredWidth: 90
                    color: Theme.hasAnime ? Theme.t1 : Theme.t3
                    style: Theme.hasAnime ? Text.Outline : Text.Normal
                    styleColor: Theme.isLight ? "#ffffff" : "#000000"
                    font.pixelSize: 12
                    font.weight: Theme.hasAnime ? Font.Medium : Font.Normal
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Text {
                    text: lrow.numPeers
                    Layout.preferredWidth: 56
                    horizontalAlignment: Text.AlignRight
                    color: lrow.numPeers === 0 ? (Theme.hasAnime ? Theme.t2 : Theme.t4) : (Theme.hasAnime ? Theme.t1 : Theme.t2)
                    style: Theme.hasAnime ? Text.Outline : Text.Normal
                    styleColor: Theme.isLight ? "#ffffff" : "#000000"
                    font.pixelSize: 12
                    font.weight: Theme.hasAnime ? Font.Medium : Font.Normal
                    font.family: Theme.fontMono
                }
            }
        }
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
            var d = row >= 0 ? list.itemAtIndex(row) : null
            if (d && d.stateDetail !== undefined && d.stateDetail.length > 0 && !dragging) {
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
