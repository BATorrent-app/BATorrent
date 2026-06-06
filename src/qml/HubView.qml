// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// HUB page (4.0 steps ⑤/⑥). Placeholder — your library of completed downloads:
// watch movies (embedded player + resume) and launch games.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: page
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 14
        IconImg { Layout.alignment: Qt.AlignHCenter; src: "qrc:/icons/play.svg"; tint: Theme.t4; s: 44 }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "HUB"
            color: Theme.t1; font.pixelSize: 22; font.weight: Font.Bold; font.family: Theme.fontSans
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Em construção — assista seus filmes e jogue seus jogos daqui."
            color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
        }
    }
}
