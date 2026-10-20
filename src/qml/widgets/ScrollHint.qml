// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// "There is more below": at the foot of a Flickable whose content runs past
// it, the content fades out and a "More below" pill scrolls on by most of a
// page. Gone once the end is in view. Place it over the Flickable, anchored
// to its bottom.
import QtQuick
import "../theme"

Item {
    id: hint
    required property Flickable flick
    property color background: Theme.elev   // what the content fades into

    readonly property bool more: flick.contentHeight > flick.height + 2
                                 && flick.contentY + flick.height < flick.contentHeight - 8
    width: flick.width
    height: 84
    visible: opacity > 0
    opacity: more ? 1 : 0
    Behavior on opacity { NumberAnimation { duration: Theme.durBase; easing.type: Theme.easeOut } }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: Qt.rgba(hint.background.r, hint.background.g, hint.background.b, 0) }
            GradientStop { position: 0.5; color: Qt.rgba(hint.background.r, hint.background.g, hint.background.b, 0.92) }
            GradientStop { position: 1; color: hint.background }
        }
    }
    // the fade takes the wheel and clicks it covers back to the content
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        onWheel: function (w) { w.accepted = false }
    }
    Rectangle {
        id: pill
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 12
        width: pillRow.implicitWidth + 26
        height: 30
        radius: 15
        color: pillMa.containsMouse ? Theme.hover2 : Theme.panel
        border.color: Theme.hair; border.width: 1
        Row {
            id: pillRow
            anchors.centerIn: parent
            spacing: 6
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: (i18n.language, i18n.t("wn_more_below"))
                color: pillMa.containsMouse ? Theme.t1 : Theme.t2
                font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
            IconImg {
                anchors.verticalCenter: parent.verticalCenter
                src: "qrc:/icons/chevron.svg"
                s: 13
                tint: pillMa.containsMouse ? Theme.t1 : Theme.t2
            }
        }
        MouseArea {
            id: pillMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                scrollOn.to = Math.min(hint.flick.contentHeight - hint.flick.height,
                                       hint.flick.contentY + hint.flick.height * 0.8)
                scrollOn.restart()
            }
        }
        Accessible.role: Accessible.Button
        Accessible.name: (i18n.language, i18n.t("wn_more_below"))
    }
    NumberAnimation {
        id: scrollOn
        target: hint.flick
        property: "contentY"
        duration: Theme.durSlow
        easing.type: Theme.easeOut
    }
}
