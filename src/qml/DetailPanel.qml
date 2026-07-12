// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Bottom detail panel: tabs (General/Peers/Files/Trackers/Pieces/Graph) for the
// selected torrent. Decomposed out of Main.qml; reads window state via `win` and
// signals rename intent back instead of reaching for the shared prompt dialog.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Shapes
import "theme"
import "widgets"

Rectangle {
    id: detailPanel
    property var win
    signal renameFileRequested(int idx, string current)
    Layout.fillWidth: true
    Layout.preferredHeight: win.detailsShownCollapsed ? 42 : 270
    Behavior on Layout.preferredHeight { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
    color: Theme.panel
    clip: true
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair; z: 3 }
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // .dtabs (42px, 5 tabs)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 42
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            // collapse / expand the whole detail panel (state persists)
            Rectangle {
                id: collapseBtn
                anchors.right: parent.right; anchors.rightMargin: Theme.sp4
                anchors.verticalCenter: parent.verticalCenter
                width: 30; height: 26; radius: 7
                color: colMa.containsMouse ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14) : Theme.hover
                border.width: 1; border.color: colMa.containsMouse ? Theme.accent : Theme.hair
                Behavior on color { ColorAnimation { duration: 120 } }
                Text {
                    anchors.centerIn: parent
                    text: "⌄"
                    rotation: win.detailsShownCollapsed ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                    color: colMa.containsMouse ? Theme.t1 : Theme.t2
                    font.pixelSize: 18; font.bold: true; font.family: Theme.fontSans
                }
                MouseArea {
                    id: colMa
                    anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: win.toggleDetailsCollapsed()
                }
                ToolTip.visible: colMa.containsMouse
                ToolTip.text: win.detailsShownCollapsed ? i18n.t("detail_expand") : i18n.t("detail_collapse")
                ToolTip.delay: 400
            }

            // lock: pin the panel open/closed, ignoring auto-collapse on deselect
            Item {
                id: lockBtn
                anchors.right: collapseBtn.left; anchors.rightMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                width: 28; height: 28; z: 5
                readonly property color tint: lockMa.containsMouse ? Theme.t1 : (win.detailsLocked ? Theme.accent : Theme.t4)
                IconImg {
                    anchors.centerIn: parent
                    s: 15
                    src: win.detailsLocked ? "qrc:/icons/lock.svg" : "qrc:/icons/lock-open.svg"
                    tint: lockBtn.tint
                }
                MouseArea {
                    id: lockMa
                    anchors.fill: parent; anchors.margins: -4
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: win.toggleDetailsLocked()
                }
                ToolTip.visible: lockMa.containsMouse
                ToolTip.text: win.detailsLocked ? i18n.t("detail_pinned") : i18n.t("detail_pin")
                ToolTip.delay: 400
            }

            Row {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                spacing: Theme.sp5

                Repeater {
                    model: [
                        { label: (i18n.language, i18n.t("detail_general")),    ct: "" },
                        { label: (i18n.language, i18n.t("detail_peers")),    ct: win.hasSel ? String(session.selectedPeers) : "" },
                        { label: (i18n.language, i18n.t("detail_files")), ct: win.hasSel ? String(session.selectedFiles.length) : "" },
                        { label: (i18n.language, i18n.t("detail_trackers")), ct: win.hasSel ? String(session.selectedTrackers.length) : "" },
                        { label: (i18n.language, i18n.t("detail_pieces")),  ct: "" },
                        { label: (i18n.language, i18n.t("detail_graph")),  ct: "" }
                    ]
                    delegate: Item {
                        height: 42
                        width: dtabRow.implicitWidth
                        readonly property bool on: win.detailTab === index

                        Row {
                            id: dtabRow
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 7
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                color: parent.parent.on ? Theme.t1 : (dtabMa.containsMouse ? Theme.t2 : Theme.t3)
                                font.pixelSize: 13
                                font.weight: parent.parent.on ? Font.DemiBold : Font.Medium
                                font.family: Theme.fontSans
                            }
                            Text {
                                visible: modelData.ct !== ""
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.ct
                                color: Theme.t4
                                font.pixelSize: 11
                                font.family: Theme.fontSans
                                font.features: Theme.tnum
                            }
                        }
                        Rectangle {
                            visible: parent.on
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.bottom: parent.bottom
                            height: 2
                            color: Theme.accent
                        }
                        MouseArea {
                            id: dtabMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: win.detailTab = index
                        }
                    }
                }
            }
        }

        // .dbody — stacked panes (Geral / Peers / Arquivos / Trackers / Pedaços)
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: win.detailTab

          // --- 0: Geral ---
          Item {
          RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.sp5
            spacing: Theme.sp6

            // .dcover (radius 8 — mask via MultiEffect)
            Item {
                Layout.preferredWidth: 104
                Layout.preferredHeight: 146
                Layout.alignment: Qt.AlignTop

                Rectangle {
                    id: dcoverContent
                    anchors.fill: parent
                    color: "#161618"
                    visible: false
                    layer.enabled: true
                    Image {
                        anchors.fill: parent
                        source: win.fileUrl(win.hasSel ? session.selectedPoster : "")
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                        sourceSize: Qt.size(208, 292)
                    }
                }
                Rectangle {
                    id: dcoverMask
                    anchors.fill: parent
                    radius: 8
                    color: "white"
                    visible: false
                    layer.enabled: true
                }
                MultiEffect {
                    source: dcoverContent
                    anchors.fill: parent
                    maskEnabled: true
                    maskSource: dcoverMask
                    visible: win.hasSel && session.selectedPoster.length > 0
                }
                // placeholder logo (no poster / no selection)
                Image {
                    anchors.centerIn: parent
                    visible: !(win.hasSel && session.selectedPoster.length > 0)
                    width: 52; height: 52
                    source: "qrc:/images/logo.svg"
                    sourceSize: Qt.size(104, 104)
                    fillMode: Image.PreserveAspectFit
                    opacity: 0.4
                    layer.enabled: Theme.isLight
                    layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
                }
                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: "transparent"
                    border.color: Theme.hair
                    border.width: 1
                }
            }

            // .dmain max-width 460
            ColumnLayout {
                Layout.preferredWidth: 460
                Layout.maximumWidth: 460
                Layout.alignment: Qt.AlignTop
                spacing: 6

                Text {
                    text: win.hasSel ? (session.selectedMetaTitle.length > 0 ? session.selectedMetaTitle : session.selectedName) : (i18n.language, i18n.t("empty_no_selection"))
                    color: Theme.t1
                    font.pixelSize: 17
                    font.weight: Font.DemiBold
                    font.letterSpacing: -0.2
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
                Text {
                    visible: win.hasSel && session.selectedMetaInfo.length > 0
                    text: win.hasSel ? session.selectedMetaInfo : ""
                    color: Theme.t3
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                }
                // progress bar — % (left) + downloaded / total (right)
                ColumnLayout {
                    visible: win.hasSel
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    spacing: 6
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: Math.round((win.hasSel ? session.selectedProgress : 0) * 100) + "%"
                            color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            text: win.hasSel ? (session.selectedDownloaded.split(" (")[0] + "  /  " + session.selectedSize) : ""
                            color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true; height: 5; radius: 2; color: Theme.track
                        Rectangle {
                            height: parent.height; radius: 2
                            width: parent.width * (win.hasSel ? session.selectedProgress : 0)
                            color: (win.hasSel && session.selectedCompleted) ? Theme.grn : Theme.accent
                            Behavior on width { NumberAnimation { duration: 220; easing.type: Easing.OutCubic } }
                        }
                    }
                }

                // big live transfer: DOWN (accent) · UP (amber) · ETA — the one
                // place per-direction color earns its keep (single instance)
                RowLayout {
                    visible: win.hasSel
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    spacing: Theme.sp6
                    Repeater {
                        model: [
                            { lbl: (i18n.language, i18n.t("graph_download")), arrow: "↓ ", v: win.hasSel ? session.selectedDownSpeed : "—", c: Theme.accent },
                            { lbl: (i18n.language, i18n.t("graph_upload")),   arrow: "↑ ", v: win.hasSel ? session.selectedUpSpeed   : "—", c: Theme.amber  },
                            { lbl: (i18n.language, i18n.t("col_eta")),        arrow: "",   v: win.hasSel ? session.selectedEta       : "—", c: Theme.t1     }
                        ]
                        delegate: Column {
                            spacing: 3
                            Text { text: modelData.lbl; color: Theme.t4; font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 0.8; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                            Text {
                                text: modelData.arrow + modelData.v
                                color: modelData.c
                                font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
                            }
                        }
                    }
                }

            }

            Item { Layout.fillWidth: true }

            // .dcols (3 colunas KV)
            RowLayout {
                Layout.alignment: Qt.AlignTop
                spacing: Theme.sp6

                // STORAGE
                ColumnLayout {
                    Layout.preferredWidth: 168
                    Layout.alignment: Qt.AlignTop
                    spacing: 5

                    Text {
                        text: (i18n.language, i18n.t("detail_section_storage"))
                        color: Theme.t4
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                        font.capitalization: Font.AllUppercase
                        font.family: Theme.fontSans
                        Layout.bottomMargin: Theme.sp2
                    }
                    Repeater {
                        model: [
                            { kk: "detail_kv_size",  v: win.hasSel ? session.selectedSize : "—" },
                            { kk: "detail_kv_hash",  v: win.hasSel ? session.selectedHash : "—" },
                            { kk: "detail_kv_added", v: win.hasSel ? session.selectedAdded : "—" },
                            { kk: "detail_kv_path",  v: win.hasSel ? session.selectedPath : "—" }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            Text { text: (i18n.language, i18n.t(modelData.kk)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                            Item { Layout.fillWidth: true }
                            Text { text: modelData.v; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum; elide: Text.ElideMiddle; Layout.maximumWidth: 110; horizontalAlignment: Text.AlignRight }
                        }
                    }
                }

                // TRANSFER
                ColumnLayout {
                    Layout.preferredWidth: 168
                    Layout.alignment: Qt.AlignTop
                    spacing: 5

                    Text {
                        text: (i18n.language, i18n.t("detail_section_transfer"))
                        color: Theme.t4
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                        font.capitalization: Font.AllUppercase
                        font.family: Theme.fontSans
                        Layout.bottomMargin: Theme.sp2
                    }
                    Repeater {
                        model: [
                            { kk: "detail_kv_downloaded", v: win.hasSel ? session.selectedDownloaded.split(" (")[0] : "—" },
                            { kk: "detail_kv_uploaded",   v: win.hasSel ? session.selectedUploaded : "—" },
                            { kk: "detail_kv_ratio",      v: win.hasSel ? session.selectedRatio : "—" }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            Text { text: (i18n.language, i18n.t(modelData.kk)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                            Item { Layout.fillWidth: true }
                            Text { text: modelData.v; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum }
                        }
                    }
                }

                // HEALTH
                ColumnLayout {
                    Layout.preferredWidth: 168
                    Layout.alignment: Qt.AlignTop
                    spacing: 5

                    Text {
                        text: (i18n.language, i18n.t("detail_section_health"))
                        color: Theme.t4
                        font.pixelSize: 10
                        font.weight: Font.Bold
                        font.letterSpacing: 0.8
                        font.capitalization: Font.AllUppercase
                        font.family: Theme.fontSans
                        Layout.bottomMargin: Theme.sp2
                    }
                    Repeater {
                        model: [
                            { kk: "detail_kv_seeds",        v: win.hasSel ? String(session.selectedSeeds) : "—" },
                            { kk: "detail_kv_peers",        v: win.hasSel ? String(session.selectedPeers) : "—" },
                            { kk: "detail_kv_availability", v: win.hasSel ? session.selectedAvailability : "—" }
                        ]
                        delegate: RowLayout {
                            Layout.fillWidth: true
                            Text { text: (i18n.language, i18n.t(modelData.kk)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                            Item { Layout.fillWidth: true }
                            Text { text: modelData.v; color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum }
                        }
                    }
                }
            }
          }
          }
          // --- 1: Peers · 2: Arquivos · 3: Trackers · 4: Pedaços ---
          // Only build the heavy list for the tab that's actually open.
          // StackLayout instantiates every child and keeps its bindings
          // live, so without the tab guard, clicking a torrent rebuilt
          // ALL of these at once — and selectedPieces alone is one entry
          // per piece (hundreds of thousands on a big torrent), which
          // froze the GUI thread on every single selection.
          DetailPeers    { peers:    (win.hasSel && win.detailTab === 1) ? session.selectedPeerList : []
                           loading:  win.peersTabOpen && session.peersLoading }
          DetailFiles {
              files: (win.hasSel && win.detailTab === 2) ? session.selectedFiles : []
              onRenameFile: function(idx, current) {
                  detailPanel.renameFileRequested(idx, current)
              }
          }
          DetailTrackers { trackers: (win.hasSel && win.detailTab === 3) ? session.selectedTrackers : [] }
          DetailPieces   { pieces:   (win.hasSel && win.detailTab === 4) ? session.selectedPieces   : ({}) }

          // --- 5: Graph (moved out of General into its own tab) ---
          Item {
                    // per-torrent speed history — readable, contained graph
                    // (down = accent, up = amber). Fills in as samples accrue.
                    Item {
                        id: spBox
                        visible: win.hasSel
                        anchors.fill: parent
                        anchors.margins: Theme.sp5
    
                        readonly property var dl: typeof session !== "undefined" ? session.selectedDownHistory : []
                        readonly property var ul: typeof session !== "undefined" ? session.selectedUpHistory : []
                        readonly property int slots: 60
                        readonly property int scaledMax: {
                            var m = 1
                            for (var i = 0; i < dl.length; ++i) if (dl[i] > m) m = dl[i]
                            for (var j = 0; j < ul.length; ++j) if (ul[j] > m) m = ul[j]
                            return Math.round(m * 1.15)
                        }
                        function scaleText(b) {
                            if (b >= 1048576) return (b / 1048576).toFixed(1) + " MB/s"
                            return Math.round(b / 1024) + " KB/s"
                        }
                        function _path(arr, close) {
                            if (!arr || arr.length === 0) return ""
                            var n = arr.length, step = spGraph.width / (slots - 1), off = (slots - n) * step, h = spGraph.height
                            function yAt(v) { return h - (v / scaledMax) * (h - 2) }
                            var s = close ? ("M " + off.toFixed(1) + "," + h.toFixed(1) + " L " + off.toFixed(1) + "," + yAt(arr[0]).toFixed(1))
                                          : ("M " + off.toFixed(1) + "," + yAt(arr[0]).toFixed(1))
                            for (var i = 1; i < n; ++i) {
                                var px = off + (i-1)*step, py = yAt(arr[i-1]), x = off + i*step, y = yAt(arr[i]), cx = (px+x)/2
                                s += " C " + cx.toFixed(1)+","+py.toFixed(1)+" "+cx.toFixed(1)+","+y.toFixed(1)+" "+x.toFixed(1)+","+y.toFixed(1)
                            }
                            if (close) s += " L " + (off + (n-1)*step).toFixed(1) + "," + h.toFixed(1) + " Z"
                            return s
                        }
    
                        Text {
                            anchors.left: parent.left; anchors.top: parent.top
                            text: spBox.scaleText(spBox.scaledMax)
                            color: Theme.t4; font.pixelSize: 9; font.family: Theme.fontSans
                        }
                        Row {
                            anchors.right: parent.right; anchors.top: parent.top; spacing: 9
                            Text { text: "↓"; color: Theme.accent; font.pixelSize: 11; font.bold: true; font.family: Theme.fontMono }
                            Text { text: "↑"; color: Theme.amber;  font.pixelSize: 11; font.bold: true; font.family: Theme.fontMono }
                        }
                        Rectangle {
                            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                            height: 1; color: Theme.hairSoft
                        }
                        Shape {
                            id: spGraph
                            anchors.left: parent.left; anchors.right: parent.right; anchors.bottom: parent.bottom
                            height: parent.height - 16
                            preferredRendererType: Shape.CurveRenderer
                            antialiasing: true
                            ShapePath {
                                strokeColor: "transparent"; strokeWidth: 0
                                fillGradient: LinearGradient {
                                    x1: 0; y1: 0; x2: 0; y2: spGraph.height
                                    GradientStop { position: 0.0; color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.13) }
                                    GradientStop { position: 1.0; color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.0) }
                                }
                                PathSvg { path: spBox._path(spBox.ul, true) }
                            }
                            ShapePath {
                                strokeColor: "transparent"; strokeWidth: 0
                                fillGradient: LinearGradient {
                                    x1: 0; y1: 0; x2: 0; y2: spGraph.height
                                    GradientStop { position: 0.0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.13) }
                                    GradientStop { position: 1.0; color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.0) }
                                }
                                PathSvg { path: spBox._path(spBox.dl, true) }
                            }
                            ShapePath {
                                strokeColor: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.7); strokeWidth: 1.25
                                fillColor: "transparent"; capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                                PathSvg { path: spBox._path(spBox.ul, false) }
                            }
                            ShapePath {
                                strokeColor: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.85); strokeWidth: 1.25
                                fillColor: "transparent"; capStyle: ShapePath.RoundCap; joinStyle: ShapePath.RoundJoin
                                PathSvg { path: spBox._path(spBox.dl, false) }
                            }
                        }
                    }
              Text {
                  anchors.centerIn: parent; visible: !win.hasSel
                  text: (i18n.language, i18n.t("empty_no_selection"))
                  color: Theme.t4; font.pixelSize: 13; font.family: Theme.fontSans
              }
          }
        }
    }
}

// ================== STATUSBAR ==================
