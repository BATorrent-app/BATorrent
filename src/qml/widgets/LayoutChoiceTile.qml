// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A miniature of the app's own frame, used to pick where the navigation and the
// detail panel live. One tile serves both questions: it always draws the whole
// layout and dims whichever half is not being asked about, so the answer is
// shown in place instead of described in a sentence.
import QtQuick
import "../theme"

Item {
    id: tile
    property bool navLeft: false
    property bool detailBottom: false
    property bool classic: false       // list rows instead of poster cards
    property string asking: "nav"      // "nav" | "detail" | "view" — the live part
    property string label: ""
    property bool selected: false
    property var uiPalette: Theme
    signal picked()

    implicitWidth: 132
    implicitHeight: frame.height + cap.height + 8
    activeFocusOnTab: true
    Accessible.role: Accessible.RadioButton
    Accessible.name: label
    Accessible.checked: selected

    function pick() {
        forceActiveFocus()
        picked()
    }

    readonly property real navOp: asking === "nav" ? 1 : 0.28
    readonly property real detOp: asking === "detail" ? 1 : 0.28
    // Detail only belongs on the detail question — drawing it (even dimmed)
    // on nav/view tiles muddies what is being chosen.
    readonly property bool showDetail: asking === "detail"

    Rectangle {
        id: frame
        width: parent.width
        height: 86
        radius: 9
        color: tile.uiPalette.bg
        border.width: tile.selected || tile.activeFocus ? 2 : 1
        border.color: tile.selected ? tile.uiPalette.accent
                    : tile.activeFocus ? tile.uiPalette.focusRing
                    : (tileMa.containsMouse ? tile.uiPalette.t4 : tile.uiPalette.hair)
        Behavior on border.color { ColorAnimation { duration: Theme.reduceMotion ? 0 : 140 } }
        clip: true

        // navigation: a bar across the top or a rail down the left side
        Rectangle {
            id: navBar
            visible: !tile.navLeft
            anchors.top: parent.top; anchors.left: parent.left; anchors.right: parent.right
            anchors.margins: 5
            height: 13
            radius: 4
            color: tile.uiPalette.panel
            opacity: tile.navOp
            Behavior on opacity { NumberAnimation { duration: 180 } }
            Row {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left; anchors.leftMargin: 5
                spacing: 4
                Repeater {
                    model: 3
                    Rectangle { width: 12; height: 3; radius: 1.5; color: tile.uiPalette.t4 }
                }
            }
        }
        Rectangle {
            id: navRail
            visible: tile.navLeft
            anchors.top: parent.top; anchors.bottom: parent.bottom; anchors.left: parent.left
            anchors.margins: 5
            width: 26
            radius: 4
            color: tile.uiPalette.panel
            opacity: tile.navOp
            Behavior on opacity { NumberAnimation { duration: 180 } }
            Column {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top; anchors.topMargin: 7
                spacing: 5
                Repeater {
                    model: 3
                    Rectangle { width: 14; height: 3; radius: 1.5; color: tile.uiPalette.t4 }
                }
            }
        }

        // the content area — poster cards or classic rows, whichever this tile
        // is arguing for
        Item {
            id: content
            anchors.left: tile.navLeft ? navRail.right : parent.left
            anchors.right: (tile.showDetail && !tile.detailBottom) ? detailPane.left : parent.right
            anchors.top: tile.navLeft ? parent.top : navBar.bottom
            anchors.bottom: (tile.showDetail && tile.detailBottom) ? detailPane.top : parent.bottom
            anchors.margins: 5

            Grid {
                visible: !tile.classic
                anchors.fill: parent
                columns: 3
                rowSpacing: 4
                columnSpacing: 4
                Repeater {
                    model: 6
                    Rectangle {
                        width: (content.width - 8) / 3
                        height: Math.max(4, (content.height - 4) / 2)
                        radius: 2
                        color: tile.uiPalette.hover
                    }
                }
            }
            Column {
                visible: tile.classic
                anchors.fill: parent
                spacing: 4
                Repeater {
                    model: 5
                    Rectangle {
                        width: content.width
                        height: Math.max(3, (content.height - 16) / 5)
                        radius: 1.5
                        color: tile.uiPalette.hover
                    }
                }
            }
        }

        // detail surface: a right column or a bottom deck
        Rectangle {
            id: detailPane
            visible: tile.showDetail
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.top: tile.detailBottom ? undefined : (tile.navLeft ? parent.top : navBar.bottom)
            anchors.left: tile.detailBottom ? (tile.navLeft ? navRail.right : parent.left) : undefined
            anchors.margins: 5
            width: tile.detailBottom ? undefined : 30
            height: tile.detailBottom ? 24 : undefined
            radius: 4
            color: tile.uiPalette.panel
            border.width: 1
            border.color: tile.uiPalette.hair
            opacity: tile.detOp
            Behavior on opacity { NumberAnimation { duration: 180 } }
            Rectangle {
                x: 4; y: 4
                width: tile.detailBottom ? 13 : 10
                height: tile.detailBottom ? 16 : 14
                radius: 2
                color: tile.uiPalette.hover
            }
            Rectangle {
                x: tile.detailBottom ? 21 : 4
                y: tile.detailBottom ? 6 : 21
                width: tile.detailBottom ? 40 : 22
                height: 3
                radius: 1.5
                color: tile.uiPalette.accent
            }
        }
    }

    Text {
        id: cap
        anchors.top: frame.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: parent.horizontalCenter
        text: tile.label
        color: tile.selected ? tile.uiPalette.t1 : tile.uiPalette.t3
        font.pixelSize: 12
        font.weight: tile.selected ? Font.DemiBold : Font.Normal
        font.family: tile.uiPalette.fontSans
    }

    MouseArea {
        id: tileMa
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: tile.pick()
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
            tile.pick()
            event.accepted = true
        }
    }
}
