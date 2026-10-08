// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Compact menu item used in the category menus (context menu + filter bar).
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

MenuItem {
    id: cmi
    // -1 keeps the right column empty, for the menus that only assign a
    // category and have no list to count.
    property int count: -1
    property bool current: false
    implicitHeight: 30
    padding: 0
    // A category with nothing in it cannot be filtered to anything, so it
    // recedes rather than disappearing: it is still somewhere to file a
    // torrent from the right-click menu.
    opacity: cmi.count === 0 ? 0.45 : 1
    contentItem: Item {
        Text {
            id: cmiLabel
            anchors.left: parent.left
            anchors.right: cmiCount.left
            anchors.verticalCenter: parent.verticalCenter
            leftPadding: 14
            rightPadding: 8
            text: cmi.text
            color: cmi.current ? Theme.accentText : (cmi.highlighted ? Theme.t1 : Theme.t2)
            font.pixelSize: 12
            font.weight: cmi.current ? Font.DemiBold : Font.Normal
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
        Text {
            id: cmiCount
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 14
            visible: cmi.count >= 0
            text: cmi.count
            color: Theme.t4
            font.pixelSize: 11
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
    }
    background: Rectangle {
        color: cmi.highlighted ? Theme.hover : "transparent"
        radius: 5
        Rectangle {
            visible: cmi.current
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: 3
            height: parent.height - 10
            radius: 1.5
            color: Theme.accent
        }
    }
}
