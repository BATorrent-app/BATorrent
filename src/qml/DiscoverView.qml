// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Discover page (4.0 step ③). Placeholder — rotating hero + poster rows
// (TMDB trending/popular + IGDB) land here. Gated off in store builds.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: page
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 14
        IconImg { Layout.alignment: Qt.AlignHCenter; src: "qrc:/icons/grid.svg"; tint: Theme.t4; s: 44 }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Discover"
            color: Theme.t1; font.pixelSize: 22; font.weight: Font.Bold; font.family: Theme.fontSans
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "Em construção — trending de filmes e jogos vem aqui."
            color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
        }
    }
}
