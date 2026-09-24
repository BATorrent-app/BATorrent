// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Alternative-speed (turtle) toggle for the top nav bar.
//
// It also lives in the status strip at the bottom of the Downloads page, but
// that is a 26px icon on a 30px bar that only exists on one page — a mode the
// whole session runs under was effectively hidden. Up here it sits with VPN,
// the other session-wide mode, and reads the same way: tinted when engaged.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../theme"
import "../widgets"

Rectangle {
    id: root
    required property var bar

    visible: typeof session !== "undefined"
    Layout.alignment: Qt.AlignVCenter
    Layout.preferredHeight: 34
    Layout.preferredWidth: turtleRow.implicitWidth + 22
    radius: 9

    readonly property bool on: typeof session !== "undefined" && session.altSpeedsActive

    // Engaged is a filled accent tint, not a colour swap on the glyph: a mode
    // that is throttling every transfer in the app has to be visible without
    // being compared against its own off state.
    color: on ? Theme.accentTint : (turtleMa.containsMouse ? Theme.hover : "transparent")
    Behavior on color { ColorAnimation { duration: 130 } }

    RowLayout {
        id: turtleRow
        anchors.centerIn: parent
        spacing: 8
        IconImg {
            Layout.alignment: Qt.AlignVCenter
            src: "qrc:/icons/turtle.svg"
            tint: root.on ? Theme.accentText : Theme.t3
            s: 16
            Behavior on tint { ColorAnimation { duration: 130 } }
        }
        Text {
            visible: !root.bar.tightChip
            text: (i18n.language, i18n.t("tb_alt_speed"))
            color: root.on ? Theme.accentText : Theme.t3
            font.pixelSize: 12
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
            Behavior on color { ColorAnimation { duration: 130 } }
        }
    }
    MouseArea {
        id: turtleMa
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: if (typeof session !== "undefined")
                       session.setAltSpeedsActive(!session.altSpeedsActive)
    }
    ToolTip.visible: turtleMa.containsMouse
    ToolTip.delay: 400
    // The tint already answers "is it on"; the tip only has to say what it is.
    ToolTip.text: (i18n.language, i18n.t("tb_alt_speed"))
}
