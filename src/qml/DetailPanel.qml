// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Bottom detail panel: tabs (General/Peers/Files/Trackers/Pieces/Graph) for the
// selected torrent. The pane contents live in widgets/Detail* so the grid-mode
// side inspector renders the same data in a vertical frame.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
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
                color: colMa.containsMouse ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14) : Theme.hover
                border.width: 1; border.color: colMa.containsMouse ? Theme.accent : Theme.hair
                Behavior on color { ColorAnimation { duration: 120 } }
                Text {
                    anchors.centerIn: parent
                    text: "⌄"
                    rotation: detailPanel.win.detailsShownCollapsed ? 180 : 0
                    Behavior on rotation { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
                    color: colMa.containsMouse ? Theme.t1 : Theme.t2
                    font.pixelSize: 18; font.bold: true; font.family: Theme.fontSans
                }
                MouseArea {
                    id: colMa
                    anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: detailPanel.win.toggleDetailsCollapsed()
                }
                ToolTip.visible: colMa.containsMouse
                ToolTip.text: detailPanel.win.detailsShownCollapsed ? i18n.t("detail_expand") : i18n.t("detail_collapse")
                ToolTip.delay: 400
            }

            // lock: pin the panel open/closed, ignoring auto-collapse on deselect
            Item {
                id: lockBtn
                anchors.right: collapseBtn.left; anchors.rightMargin: 2
                anchors.verticalCenter: parent.verticalCenter
                width: 28; height: 28; z: 5
                readonly property color tint: lockMa.containsMouse ? Theme.t1 : (detailPanel.win.detailsLocked ? Theme.accent : Theme.t4)
                IconImg {
                    anchors.centerIn: parent
                    s: 15
                    src: detailPanel.win.detailsLocked ? "qrc:/icons/lock.svg" : "qrc:/icons/lock-open.svg"
                    tint: lockBtn.tint
                }
                MouseArea {
                    id: lockMa
                    anchors.fill: parent; anchors.margins: -4
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: detailPanel.win.toggleDetailsLocked()
                }
                ToolTip.visible: lockMa.containsMouse
                ToolTip.text: detailPanel.win.detailsLocked ? i18n.t("detail_pinned") : i18n.t("detail_pin")
                ToolTip.delay: 400
            }

            DetailTabs {
                anchors.left: parent.left
                anchors.leftMargin: Theme.sp5
                anchors.right: lockBtn.left
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

        // .dbody — stacked panes. Only the open tab gets live data: StackLayout
        // keeps every child's bindings alive, and selectedPieces alone is one
        // entry per piece (huge torrents froze the GUI thread without the guard).
        // `detailPanel.visible` joins the guard because in grid mode the side
        // inspector shows instead and this panel must go fully inert.
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: detailPanel.win.detailTab

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
