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

    property bool dismissed: false
    // re-open when the user selects a different torrent after dismissing
    readonly property string selHash: (typeof session !== "undefined" && win.hasSel) ? session.selectedHash : ""
    onSelHashChanged: dismissed = false

    // host decides whether the side inspector is allowed (off when the user
    // moved the detail panel to the bottom)
    property bool showInspector: true
    // Pinned means the same here as in the bottom panel: stay open regardless of
    // selection, and ignore a previous dismiss. Without this the pin button would
    // be decoration — visibility was keyed only on hasSel && !dismissed.
    readonly property bool shown: showInspector && controller.gridView
                                  && (win.detailsLocked || (win.hasSel && !dismissed))

    // Same collapse the bottom deck has, and the same state behind it: the two are
    // one feature in two placements, so collapsing one and finding the other open
    // would read as a bug. Sideways it takes width instead of height, down to a
    // rail that still carries the pin and the way back.
    readonly property bool collapsed: win.detailsShownCollapsed

    Layout.fillHeight: true
    Layout.preferredWidth: shown ? (collapsed ? 46 : 340) : 0
    Behavior on Layout.preferredWidth { NumberAnimation { duration: 200; easing.type: Easing.OutCubic } }
    visible: Layout.preferredWidth > 0
    clip: true
    color: Theme.panel
    Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.hair }

    // collapsed rail — the panel has to be reachable from its own edge
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
                width: 30; height: 30; radius: 8
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
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            Rectangle {
                anchors.centerIn: parent
                width: 30; height: 30; radius: 8
                color: railPinMa.containsMouse ? Theme.hover : "transparent"
                IconImg {
                    anchors.centerIn: parent
                    s: 16
                    src: sidebar.win.detailsLocked ? "qrc:/icons/lock-solid.svg" : "qrc:/icons/lock-open-solid.svg"
                    tint: railPinMa.containsMouse ? Theme.t1
                          : (sidebar.win.detailsLocked ? Theme.accent : Theme.t3)
                }
                MouseArea {
                    id: railPinMa
                    anchors.fill: parent
                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: sidebar.win.toggleDetailsLocked()
                }
                ToolTip.visible: railPinMa.containsMouse
                ToolTip.text: (i18n.language, sidebar.win.detailsLocked ? i18n.t("detail_pinned") : i18n.t("detail_pin"))
                ToolTip.delay: 400
            }
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

            // header: title + close
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 45
                Text {
                    anchors.left: parent.left; anchors.leftMargin: Theme.sp4
                    anchors.right: pinBtn.left; anchors.rightMargin: Theme.sp2
                    anchors.verticalCenter: parent.verticalCenter
                    // A label, not the name: the identity block right below
                    // already says which torrent this is, in the size that
                    // question deserves. Two of the same title, one small and
                    // one large, just read as a mistake.
                    text: (i18n.language, i18n.t("detail_selected_torrent"))
                    color: Theme.t4
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.letterSpacing: 1.0
                    font.capitalization: Font.AllUppercase
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
                // Pin, same control the bottom panel has. The two panels are the
                // same feature in two placements, so an option present in one and
                // missing in the other reads as the sidebar being the lesser mode.
                Rectangle {
                    id: collapseBtn
                    anchors.right: pinBtn.left; anchors.rightMargin: 2
                    anchors.verticalCenter: parent.verticalCenter
                    width: 34; height: 34; radius: 8
                    color: collMa.containsMouse ? Theme.hover : "transparent"
                    // Os tres controles deste cabecalho sao um conjunto: mesmo
                    // tamanho (17) e, entre os dois de acao, o mesmo traco (2.4).
                    // O cadeado e estado, por isso e o unico solido.
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
                Rectangle {
                    id: pinBtn
                    anchors.right: closeBtn.left; anchors.rightMargin: 2
                    anchors.verticalCenter: parent.verticalCenter
                    width: 34; height: 34; radius: 8
                    color: pinMa.containsMouse ? Theme.hover : "transparent"
                    // The stroked Tabler lock lands near a 1.1px stroke at this
                    // size — thin and faint at once, which is why the pair read
                    // as decoration. The solid body holds its weight instead.
                    IconImg {
                        anchors.centerIn: parent
                        s: 17
                        src: sidebar.win.detailsLocked ? "qrc:/icons/lock-solid.svg" : "qrc:/icons/lock-open-solid.svg"
                        tint: pinMa.containsMouse ? Theme.t1
                              : (sidebar.win.detailsLocked ? Theme.accent : Theme.t3)
                    }
                    MouseArea {
                        id: pinMa
                        anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: sidebar.win.toggleDetailsLocked()
                    }
                    ToolTip.visible: pinMa.containsMouse
                    ToolTip.text: (i18n.language, sidebar.win.detailsLocked ? i18n.t("detail_pinned") : i18n.t("detail_pin"))
                    ToolTip.delay: 400
                }
                Rectangle {
                    id: closeBtn
                    anchors.right: parent.right; anchors.rightMargin: Theme.sp3
                    anchors.verticalCenter: parent.verticalCenter
                    width: 34; height: 34; radius: 8
                    color: closeMa.containsMouse ? Theme.hover : "transparent"
                    IconImg {
                        anchors.centerIn: parent
                        s: 17
                        src: "qrc:/icons/close-bold.svg"
                        tint: closeMa.containsMouse ? Theme.t1 : Theme.t3
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

            // Fixed above the tabs: which torrent you are looking at should not
            // disappear when you go read its peers or its files.
            DetailIdentity {
                visible: sidebar.win.hasSel
                win: sidebar.win
                vertical: true
                Layout.fillWidth: true
                Layout.leftMargin: Theme.sp4
                Layout.rightMargin: Theme.sp4
                Layout.topMargin: Theme.sp4
                Layout.bottomMargin: Theme.sp4
            }
            Rectangle {
                visible: sidebar.win.hasSel
                Layout.fillWidth: true
                Layout.preferredHeight: 1
                color: Theme.hair
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
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hair }

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
