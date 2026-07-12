// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Grid-mode detail surface: a right-side inspector column with the same six
// tabs as the bottom panel, reflowed for 340px. The bottom panel stays the
// list-mode surface; exactly one of the two is ever visible, so they can
// share win.detailTab (which also drives peer polling).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: sidebar
    property var win
    signal renameFileRequested(int idx, string current)

    property bool dismissed: false
    // re-open when the user selects a different torrent after dismissing
    readonly property string selHash: (typeof session !== "undefined" && win.hasSel) ? session.selectedHash : ""
    onSelHashChanged: dismissed = false

    readonly property bool shown: win.gridView && win.hasSel && !dismissed

    Layout.fillHeight: true
    Layout.preferredWidth: shown ? 340 : 0
    Behavior on Layout.preferredWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
    visible: Layout.preferredWidth > 0
    clip: true
    color: Theme.panel
    Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.hair }

    // content keeps its full width during the slide so text doesn't reflow
    Item {
        width: 340
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // header: title + close
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                Text {
                    anchors.left: parent.left; anchors.leftMargin: Theme.sp4
                    anchors.right: closeBtn.left; anchors.rightMargin: Theme.sp2
                    anchors.verticalCenter: parent.verticalCenter
                    text: sidebar.win.hasSel ? (session.selectedMetaTitle.length > 0 ? session.selectedMetaTitle : session.selectedName) : ""
                    color: Theme.t1
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Rectangle {
                    id: closeBtn
                    anchors.right: parent.right; anchors.rightMargin: Theme.sp3
                    anchors.verticalCenter: parent.verticalCenter
                    width: 26; height: 26; radius: 7
                    color: closeMa.containsMouse ? Theme.hover : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: closeMa.containsMouse ? Theme.t1 : Theme.t4
                        font.pixelSize: 12; font.family: Theme.fontSans
                    }
                    MouseArea {
                        id: closeMa
                        anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: sidebar.dismissed = true
                    }
                    ToolTip.visible: closeMa.containsMouse
                    ToolTip.text: (i18n.language, i18n.t("btn_close"))
                    ToolTip.delay: 400
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            }

            DetailTabs {
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                Layout.leftMargin: Theme.sp4
                Layout.rightMargin: Theme.sp3
                compact: true
                current: sidebar.win.detailTab
                onSelect: function(idx) { sidebar.win.detailTab = idx }
            }
            Rectangle { Layout.fillWidth: true; height: 1; color: Theme.hair }

            // panes — same guard discipline as the bottom panel: only the open
            // tab of the VISIBLE surface binds live data
            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: sidebar.win.detailTab

                DetailGeneral { win: sidebar.win; vertical: true }
                DetailPeers {
                    compact: true
                    peers: (sidebar.shown && sidebar.win.detailTab === 1) ? session.selectedPeerList : []
                    loading: sidebar.win.peersTabOpen && session.peersLoading
                }
                DetailFiles {
                    files: (sidebar.shown && sidebar.win.detailTab === 2) ? session.selectedFiles : []
                    onRenameFile: function(idx, current) {
                        sidebar.renameFileRequested(idx, current)
                    }
                }
                DetailTrackers {
                    compact: true
                    trackers: (sidebar.shown && sidebar.win.detailTab === 3) ? session.selectedTrackers : []
                }
                DetailPieces { pieces: (sidebar.shown && sidebar.win.detailTab === 4) ? session.selectedPieces : ({}) }
                DetailGraph {
                    dl: (sidebar.shown && sidebar.win.hasSel) ? session.selectedDownHistory : []
                    ul: (sidebar.shown && sidebar.win.hasSel) ? session.selectedUpHistory : []
                    hasData: sidebar.win.hasSel
                }
            }
        }
    }
}
