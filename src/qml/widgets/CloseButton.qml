// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The ✕ at the top right of a modal or a card.
import QtQuick
import "../theme"
import "."

Rectangle {
    id: cb
    property int size: 30
    property int iconSize: 14
    signal clicked()
    width: size; height: size
    radius: Theme.rad(6)
    color: ma.containsMouse ? Theme.hover : "transparent"
    IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: cb.iconSize; tint: ma.containsMouse ? Theme.t1 : Theme.t3 }
    MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: cb.clicked() }
}
