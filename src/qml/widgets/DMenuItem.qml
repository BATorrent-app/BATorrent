// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A row of DMenu: 14px icon, label, the shortcut in grey at the right, or a
// chevron when it opens a submenu. tone "accent" (Donate) and "danger"
// (Remove) colour the row; "primary" makes it the bold first action (Play).
// Disabled rows collapse when hideWhenDisabled, the way a context menu drops
// what does not apply.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"

MenuItem {
    id: mi
    property string iconSrc: subMenu && subMenu.iconSrc !== undefined ? subMenu.iconSrc : ""
    property string shortcutText: ""
    property string tone: ""
    property bool hideWhenDisabled: false
    readonly property bool shown: enabled || !hideWhenDisabled
    visible: shown
    implicitHeight: shown ? 32 : 0
    implicitWidth: 244
    padding: 0
    indicator: null
    readonly property color ink: !mi.enabled ? Theme.t4
                               : tone === "danger" || tone === "accent" ? Theme.accentText
                               : tone === "primary" || mi.highlighted ? Theme.t1 : Theme.tSoft
    readonly property color iconInk: !mi.enabled ? Theme.t4
                                   : tone === "danger" || tone === "accent" ? Theme.accentText
                                   : tone === "primary" ? Theme.accent
                                   : mi.highlighted ? Theme.t2 : Theme.t3
    contentItem: Item {
        IconImg {
            visible: mi.iconSrc !== ""
            x: 10
            anchors.verticalCenter: parent.verticalCenter
            src: mi.iconSrc
            s: 14
            tint: mi.iconInk
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: mi.iconSrc !== "" ? 34 : 10
            anchors.right: keyText.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: (mi.checkable && mi.checked ? "✓  " : "") + mi.text
            color: mi.ink
            font.pixelSize: 13
            font.weight: mi.tone === "primary" ? Font.DemiBold : Font.Normal
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
        Text {
            id: keyText
            anchors.right: parent.right
            anchors.rightMargin: mi.subMenu ? 26 : 10
            anchors.verticalCenter: parent.verticalCenter
            text: mi.shortcutText
            color: Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
        }
    }
    arrow: IconImg {
        visible: mi.subMenu !== null
        x: mi.width - width - 10
        y: (mi.height - height) / 2
        src: "qrc:/icons/chevron.svg"
        rotation: -90
        s: 10
        tint: Theme.t4
    }
    background: Rectangle {
        color: mi.highlighted && mi.enabled ? Theme.hover2 : "transparent"
        radius: 7
    }
}
