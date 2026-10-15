// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Compact menu item used in the category menus (context menu + filter bar).
// The chosen one is marked by a check and its weight, never in red: red is
// for downloading and for problems.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"
import "."

MenuItem {
    id: cmi
    // -1 keeps the right column empty, for the menus that only assign a
    // category and have no list to count.
    property int count: -1
    property bool current: false
    // A category with nothing in it cannot be filtered to anything: in the
    // filter menu it is left out. The assign menu passes no count and keeps
    // every category, since that is where a first item gets filed.
    readonly property bool empty: cmi.count === 0
    visible: !empty
    implicitHeight: empty ? 0 : 36
    height: implicitHeight
    padding: 0
    contentItem: Item {
        IconImg {
            id: cmiCheck
            x: 12
            anchors.verticalCenter: parent.verticalCenter
            visible: cmi.current
            src: "qrc:/icons/check.svg"
            tint: Theme.t1
            s: 14
        }
        Text {
            id: cmiLabel
            anchors.left: parent.left
            anchors.right: cmiCount.left
            anchors.verticalCenter: parent.verticalCenter
            leftPadding: 36
            rightPadding: 8
            text: cmi.text
            color: cmi.current || cmi.highlighted ? Theme.t1 : Theme.t2
            font.pixelSize: 14
            font.weight: cmi.current ? Font.DemiBold : Font.Normal
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
        Text {
            id: cmiCount
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            rightPadding: 12
            visible: cmi.count >= 0
            text: cmi.count
            color: Theme.t4
            font.pixelSize: 13
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
    }
    background: Rectangle {
        color: cmi.current || cmi.highlighted ? Theme.hover : "transparent"
        radius: Theme.rad(6)
    }
}
