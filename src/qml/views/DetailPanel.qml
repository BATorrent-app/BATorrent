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
    signal renameFileRequested(int idx, string current)
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
                width: 30; height: 26; radius: 7
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

            DetailTabs {
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
                DetailIdentity {
                    win: detailPanel.win
                    vertical: false
                    x: 24; y: 18
                    width: parent.width - 48
                    height: 142
                }
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hairSoft }
            }

            StackLayout {
                id: panes
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
