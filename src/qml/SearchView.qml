// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Search page (4.0 step ②). Placeholder — promotes the SearchWindow into a
// full content page (poster grid, repacker chips, filters), reusing QmlSearchBridge.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: page
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 14
        IconImg { Layout.alignment: Qt.AlignHCenter; src: "qrc:/icons/search.svg"; tint: Theme.t4; s: 44 }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Search"
            color: Theme.t1; font.pixelSize: 22; font.weight: Font.Bold; font.family: Theme.fontSans
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Em construção — busca rica com pôsteres, repacks e filtros."
            color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
        }
    }
}
