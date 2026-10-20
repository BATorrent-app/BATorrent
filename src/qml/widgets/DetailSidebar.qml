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
    property var controller
    signal renameFileRequested(int idx, string current)

    // re-open when the user selects a different torrent after dismissing
    readonly property string selHash: (typeof session !== "undefined" && win.hasSel) ? session.selectedHash : ""

    // host decides whether the side inspector is allowed (off when the user
    // moved the detail panel to the bottom)
    property bool showInspector: true
    // Present whenever the grid is: selecting a torrent changes what this shows,
    // never whether it is there. A panel that came and went on selection resized
    // the grid under the cursor, which moved every tile on a click (Sherwan #10).
    readonly property bool shown: showInspector && controller.gridView
    // A series group selected: the panel lists its episodes, and the tabs
    // wait for one of them to be picked.
    // what the selected group is: a show, or a TMDB collection of films
    readonly property var groupSummary: groupRow >= 0 && controller && controller.rowModel
                                        ? controller.rowModel.groupSummary(groupRow) : ({})
    readonly property int groupRow: (controller && controller.keyedRows && controller.selectedRows.length === 1
                                     && controller.rowModel.isGroup(controller.selectedRows[0]))
                                    ? controller.selectedRows[0] : -1
    readonly property bool showTabs: win.hasSel && (groupRow < 0 || controller.focusedEpisodeSource >= 0)

    // Same collapse the bottom deck has, and the same state behind it: the two are
    // one feature in two placements, so collapsing one and finding the other open
    // would read as a bug. Sideways it takes width instead of height, down to a
    // rail that still carries the pin and the way back.
    readonly property bool collapsed: win.detailsShownCollapsed

    Layout.fillHeight: true
    Layout.preferredWidth: shown ? (collapsed ? 46 : 340) : 0
    // Slides only to collapse or expand. Coming in with the switch from the
    // list it takes its width at once: sliding, it narrowed the grid frame by
    // frame and the cards could be seen re-flowing into their columns.
    Behavior on Layout.preferredWidth {
        enabled: sidebar.visible
        NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut }
    }
    visible: Layout.preferredWidth > 0
    clip: true
    color: Theme.panel
    Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.hair }

    // collapsed rail: the panel has to be reachable from its own edge
    ColumnLayout {
        visible: sidebar.collapsed
        width: 46
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        spacing: 2

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 45
            Rectangle {
                anchors.centerIn: parent
                width: 30; height: 30; radius: Theme.rad(8)
                color: expandMa.containsMouse ? Theme.hover : "transparent"
                IconImg {
                    anchors.centerIn: parent
                    s: 17
                    src: "qrc:/icons/chevron-bold.svg"
                    rotation: 90            // a base do chevron aponta para baixo
                    tint: expandMa.containsMouse ? Theme.t1 : Theme.t3
                }
                MouseArea {
                    id: expandMa
                    anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: sidebar.win.toggleDetailsCollapsed()
                }
                ToolTip.visible: expandMa.containsMouse
                ToolTip.text: (i18n.language, i18n.t("detail_expand"))
                ToolTip.delay: 400
            }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }
        Item { Layout.fillHeight: true }
    }

    // content keeps its full width during the slide so text doesn't reflow
    Item {
        visible: !sidebar.collapsed
        width: 340
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            // header: title + collapse
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                Text {
                    anchors.left: parent.left; anchors.leftMargin: Theme.sp4
                    anchors.right: collapseBtn.left; anchors.rightMargin: Theme.sp2
                    anchors.verticalCenter: parent.verticalCenter
                    // A label, not the name: the identity block right below
                    // already says which torrent this is, in the size that
                    // question deserves. Two of the same title, one small and
                    // one large, just read as a mistake.
                    text: (i18n.language, sidebar.groupRow >= 0
                           ? i18n.t(sidebar.groupSummary.collection ? "insp_collection_n" : "insp_series_n").arg(sidebar.groupSummary.count || 0)
                           : sidebar.win.hasSel ? i18n.t("detail_selected_torrent") : i18n.t("insp_overview"))
                    color: Theme.t4
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 1.0
                    font.capitalization: Font.AllUppercase
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                Rectangle {
                    id: collapseBtn
                    anchors.right: parent.right; anchors.rightMargin: Theme.sp3
                    anchors.verticalCenter: parent.verticalCenter
                    width: 34; height: 34; radius: Theme.rad(8)
                    color: collMa.containsMouse ? Theme.hover : "transparent"
                    IconImg {
                        anchors.centerIn: parent
                        s: 17
                        src: "qrc:/icons/chevron-bold.svg"
                        rotation: -90
                        tint: collMa.containsMouse ? Theme.t1 : Theme.t3
                    }
                    MouseArea {
                        id: collMa
                        anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: sidebar.win.toggleDetailsCollapsed()
                    }
                    ToolTip.visible: collMa.containsMouse
                    ToolTip.text: (i18n.language, i18n.t("detail_collapse"))
                    ToolTip.delay: 400
                }
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
            }

            // Fixed above the tabs: which torrent you are looking at should not
            // disappear when you go read its peers or its files.
            DetailIdentity {
                visible: sidebar.win.hasSel && sidebar.groupRow < 0
                win: sidebar.win
                vertical: true
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp4
                Layout.rightMargin: Theme.sp4
                Layout.topMargin: Theme.sp4
                Layout.bottomMargin: Theme.sp4
            }
            DetailGroup {
                visible: sidebar.groupRow >= 0
                win: sidebar.win
                controller: sidebar.controller
                groupRow: sidebar.groupRow
                Layout.fillWidth: true
                Layout.bottomMargin: Theme.sp3
            }
            Rectangle {
                visible: sidebar.win.hasSel
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.hair
            }
            // a group with no episode picked yet: say how to get the details
            Text {
                visible: sidebar.groupRow >= 0 && !sidebar.showTabs
                Layout.fillWidth: true
                Layout.topMargin: 28
                Layout.leftMargin: 24
                Layout.rightMargin: 24
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                text: (i18n.language, i18n.t(sidebar.groupSummary.collection ? "detail_pick_film" : "detail_pick_episode"))
                color: Theme.t4
                font.pixelSize: 13
                font.family: Theme.fontSans
                lineHeight: 1.4
            }
            Item { visible: sidebar.groupRow >= 0 && !sidebar.showTabs; Layout.fillHeight: true }

            // Nothing selected: what needs a hand, what is ready, the disk
            InspectorOverview {
                visible: !sidebar.win.hasSel
                win: sidebar.win
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
            DetailTabs {
                visible: sidebar.showTabs
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                Layout.leftMargin: Theme.sp4
                Layout.rightMargin: Theme.sp3
                compact: true
                current: sidebar.win.detailTab
                onSelect: function(idx) { sidebar.win.detailTab = idx }
            }
            Rectangle { visible: sidebar.showTabs; Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hair }

            // panes: same guard discipline as the bottom panel: only the open
            // tab of the VISIBLE surface binds live data
            StackLayout {
                id: sidePanes
                visible: sidebar.showTabs
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: sidebar.win.detailTab
                // the new tab fades in from the side it sits on
                transform: Translate { id: sideShift }
                property int prev: 0
                onCurrentIndexChanged: {
                    sideShift.x = Theme.travel(currentIndex > prev ? 14 : -14)
                    prev = currentIndex
                    sidePanes.opacity = 0
                    sideIn.restart()
                }
                ParallelAnimation {
                    id: sideIn
                    NumberAnimation { target: sidePanes; property: "opacity"; to: 1; duration: Theme.durBase; easing.type: Theme.easeOut }
                    NumberAnimation { target: sideShift; property: "x"; to: 0; duration: Theme.durSlow; easing.type: Theme.easeOut }
                }

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
