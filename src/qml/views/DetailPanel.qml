// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Bottom detail panel: tabs (General/Peers/Files/Trackers/Pieces/Graph) for the
// selected torrent. The pane contents live in widgets/Detail* so the grid-mode
// side inspector renders the same data in a vertical frame.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"
import "../widgets"

Rectangle {
    id: detailPanel
    property var win
    property var controller: null
    signal renameFileRequested(int idx, string current)
    // A series row picked in the list: the show and its episodes, like the
    // grid's inspector, until one episode is picked (then its tabs).
    readonly property int groupRow: (controller && controller.keyedRows && controller.selectedRows.length === 1
                                     && controller.rowModel.isGroup(controller.selectedRows[0]))
                                    ? controller.selectedRows[0] : -1
    readonly property bool groupMode: groupRow >= 0 && controller.focusedEpisodeSource < 0
    Layout.fillWidth: true
    Layout.preferredHeight: win.detailsShownCollapsed ? 42 : 262
    Behavior on Layout.preferredHeight { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
    color: Theme.panel
    clip: true
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair; z: 3 }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // .dtabs (42px) + lock + collapse
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
                width: 30; height: 26; radius: Theme.rad(7)
                color: colMa.containsMouse ? Theme.hover : "transparent"
                border.width: 1; border.color: Theme.hair
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
                IconImg {
                    anchors.centerIn: parent
                    s: 17
                    src: "qrc:/icons/chevron-bold.svg"
                    rotation: detailPanel.win.detailsShownCollapsed ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: Theme.durBase; easing.type: Theme.easeOut } }
                    tint: colMa.containsMouse ? Theme.t1 : Theme.t2
                }
                MouseArea {
                    id: colMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: detailPanel.win.toggleDetailsCollapsed()
                }
                ToolTip.visible: colMa.containsMouse
                ToolTip.text: (i18n.language, detailPanel.win.detailsShownCollapsed
                    ? i18n.t("detail_expand") : i18n.t("detail_collapse"))
                ToolTip.delay: 400
            }

            Text {
                visible: detailPanel.groupMode
                anchors.left: parent.left; anchors.leftMargin: 24
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t("insp_series_n")).arg(panelGroup.summary.count || 0)
                color: Theme.t4
                font.pixelSize: 11; font.weight: Font.Bold; font.letterSpacing: 1.0
                font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
            }
            DetailTabs {
                visible: !detailPanel.groupMode
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.right: collapseBtn.left
                anchors.rightMargin: Theme.sp3
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                current: detailPanel.win.detailTab
                counts: [
                    "",
                    detailPanel.win.hasSel ? String(session.selectedPeers) : "",
                    detailPanel.win.hasSel ? String(session.selectedFiles.length) : "",
                    detailPanel.win.hasSel ? String(session.selectedTrackers.length) : "",
                    "",
                    ""
                ]
                onSelect: function(idx) { detailPanel.win.detailTab = idx }
            }
        }

        // .dbody: stacked panes. Only the open tab gets live data: StackLayout
        // keeps every child's bindings alive, and selectedPieces alone is one
        // entry per piece (huge torrents froze the GUI thread without the guard).
        // `detailPanel.visible` joins the guard because in grid mode the side
        // inspector shows instead and this panel must go fully inert.
        // Identity on the left, tabs on the right: the same split the side
        // inspector has, laid on its side. Before this the poster left the
        // screen the moment you opened Peers, so you lost track of which
        // torrent the numbers belonged to.
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Item {
                visible: detailPanel.win.hasSel
                Layout.preferredWidth: 440
                Layout.fillHeight: true
                // the show: cover, title, the season and a segment per episode
                Item {
                    visible: detailPanel.groupMode
                    x: 24; y: 18
                    width: parent.width - 48
                    height: 142
                    Rectangle {
                        id: grpCover
                        width: 100; height: 142; radius: Theme.rad(7); clip: true
                        color: Theme.tileBlank
                        border.color: Theme.hair; border.width: 1
                        Image {
                            anchors.fill: parent; anchors.margins: 1
                            source: panelGroup.summary.poster ? detailPanel.win.fileUrl(panelGroup.summary.poster) : ""
                            fillMode: Image.PreserveAspectCrop; asynchronous: true
                            sourceSize: Qt.size(200, 284)
                        }
                    }
                    Column {
                        anchors.left: grpCover.right; anchors.leftMargin: 18
                        anchors.right: parent.right
                        spacing: 3
                        Text {
                            width: parent.width
                            text: panelGroup.summary.title || ""
                            color: Theme.t1; font.pixelSize: Theme.displaySize(17); font.weight: Theme.tWeight(Font.DemiBold); font.family: Theme.fontDisplay; font.capitalization: Theme.tCaps; font.italic: Theme.tItalic; font.letterSpacing: Theme.tTrack(17, 0)
                            elide: Text.ElideRight
                        }
                        Text {
                            width: parent.width
                            text: [panelGroup.summary.year > 0 ? String(panelGroup.summary.year) : "", panelGroup.summary.genres || "",
                                   detailPanel.win.catLabel("Series")].filter(function (x) { return x.length > 0 }).join(" · ")
                            color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans
                            elide: Text.ElideRight
                        }
                    }
                    Column {
                        anchors.left: grpCover.right; anchors.leftMargin: 18
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        spacing: 7
                        Item {
                            width: parent.width; height: 16
                            Text {
                                text: [panelGroup.summary.seasons || "", (i18n.language, i18n.t("group_items_n")).arg(panelGroup.summary.count || 0)]
                                      .filter(function (x) { return x.length > 0 }).join(" · ")
                                color: Theme.t1; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                            }
                            Text {
                                anchors.right: parent.right
                                text: panelGroup.summary.size || ""
                                color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                            }
                        }
                        Row {
                            width: parent.width; height: 4; spacing: 3
                            Repeater {
                                model: panelGroup.episodes
                                Rectangle {
                                    required property var modelData
                                    readonly property real p: Math.max(0, Math.min(1, modelData.progress || 0))
                                    width: (parent.width - parent.spacing * (panelGroup.episodes.length - 1)) / Math.max(1, panelGroup.episodes.length)
                                    height: 4; radius: Theme.rad(2); color: Theme.track
                                    Rectangle { width: parent.width * parent.p; height: 4; radius: Theme.rad(2); color: parent.p >= 0.999 ? Theme.doneFill : Theme.down }
                                }
                            }
                        }
                    }
                }
                DetailIdentity {
                    visible: !detailPanel.groupMode
                    win: detailPanel.win
                    vertical: false
                    x: 24; y: 18
                    width: parent.width - 48
                    height: 142
                }
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hairSoft }
            }

            DetailGroup {
                id: panelGroup
                visible: detailPanel.groupMode
                showHeader: false
                win: detailPanel.win
                controller: detailPanel.controller
                groupRow: detailPanel.groupRow
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 12
                Layout.rightMargin: 12
            }
            StackLayout {
                id: panes
                visible: !detailPanel.groupMode
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                currentIndex: detailPanel.win.detailTab
                // the new tab fades in from the side it sits on
                transform: Translate { id: paneShift }
                property int prev: 0
                onCurrentIndexChanged: {
                    paneShift.x = Theme.travel(currentIndex > prev ? 14 : -14)
                    prev = currentIndex
                    panes.opacity = 0
                    paneIn.restart()
                }
                ParallelAnimation {
                    id: paneIn
                    NumberAnimation { target: panes; property: "opacity"; to: 1; duration: Theme.durBase; easing.type: Easing.OutQuart }
                    NumberAnimation { target: paneShift; property: "x"; to: 0; duration: Theme.durSlow; easing.type: Easing.OutQuart }
                }

                DetailGeneral { win: detailPanel.win; vertical: false }
                DetailPeers {
                    peers: (detailPanel.visible && detailPanel.win.hasSel && detailPanel.win.detailTab === 1) ? session.selectedPeerList : []
                    loading: detailPanel.win.peersTabOpen && session.peersLoading
                }
                DetailFiles {
                    files: (detailPanel.visible && detailPanel.win.hasSel && detailPanel.win.detailTab === 2) ? session.selectedFiles : []
                    onRenameFile: function(idx, current) {
                        detailPanel.renameFileRequested(idx, current)
                    }
                }
                DetailTrackers { trackers: (detailPanel.visible && detailPanel.win.hasSel && detailPanel.win.detailTab === 3) ? session.selectedTrackers : [] }
                DetailPieces   { pieces:   (detailPanel.visible && detailPanel.win.hasSel && detailPanel.win.detailTab === 4) ? session.selectedPieces   : ({}) }
                DetailGraph {
                    dl: (detailPanel.visible && detailPanel.win.hasSel) ? session.selectedDownHistory : []
                    ul: (detailPanel.visible && detailPanel.win.hasSel) ? session.selectedUpHistory : []
                    hasData: detailPanel.win.hasSel
                }
            }
        }
    }
}
