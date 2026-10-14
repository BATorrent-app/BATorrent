// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Menu chrome of the 5.0 design: raised surface, hairline edge, radius 10, a
// soft drop shadow and 6px of inset around the rows. Used by the app menu
// behind the logo and the library's right-click menu; submenus inherit it.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import "../theme"

Menu {
    id: menu
    modal: true
    // the icon its row shows when this menu is a submenu
    property string iconSrc: ""
    implicitWidth: 244
    padding: 6
    overlap: 6
    delegate: DMenuItem {}
    background: Item {
        implicitWidth: menu.implicitWidth
        Rectangle {
            id: plate
            anchors.fill: parent
            color: Theme.elev
            border.color: Theme.hair
            border.width: 1
            radius: 10
        }
        MultiEffect {
            source: plate
            anchors.fill: plate
            z: -1
            shadowEnabled: true
            shadowColor: Qt.rgba(0, 0, 0, Theme.isLight ? 0.18 : 0.55)
            shadowBlur: 0.9
            shadowVerticalOffset: 14
            blurMax: 40
        }
    }
    enter: Transition {
        ParallelAnimation {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: Theme.durFast; easing.type: Theme.easeOut }
            NumberAnimation { property: "scale"; from: Theme.grow(0.97); to: 1.0; duration: Theme.durBase; easing.type: Theme.easeOut }
        }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 80; easing.type: Theme.easeIn }
    }
}
