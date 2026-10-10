// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A game's banner: key art behind, the game's own logo and a one-line
// identity in front. Steam's art and logo when the game has them, IGDB's art
// otherwise; the title is set in type only when no logo loads.
import QtQuick
import "../theme"
import "../widgets"

Item {
    id: hero
    property string title
    property string steamHero
    property string steamLogo
    property string fallbackArt      // IGDB key art, then the poster
    property var facts: []           // year, maker, genre: in that order
    signal backRequested()

    readonly property var artChain: (steamHero ? [steamHero] : []).concat(fallbackArt ? [fallbackArt] : [])

    clip: true
    Rectangle { anchors.fill: parent; color: "#0a0a0b" }

    ChainImage {
        anchors.fill: parent
        sources: hero.artChain
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.rgba(10 / 255, 10 / 255, 11 / 255, 0.85) }
            GradientStop { position: 0.4; color: Qt.rgba(10 / 255, 10 / 255, 11 / 255, 0.35) }
            GradientStop { position: 0.7; color: Qt.rgba(10 / 255, 10 / 255, 11 / 255, 0) }
        }
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.45; color: Qt.rgba(Theme.bg.r, Theme.bg.g, Theme.bg.b, 0) }
            GradientStop { position: 0.8; color: Qt.rgba(Theme.bg.r, Theme.bg.g, Theme.bg.b, 0.75) }
            GradientStop { position: 1.0; color: Theme.bg }
        }
    }

    StageRoundButton {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Theme.sp5
        size: 34
        icon: "qrc:/icons/chevron-bold.svg"
        rotation: 180
        onClicked: hero.backRequested()
    }

    Column {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 48
        anchors.bottomMargin: 36
        width: Math.min(hero.width - 96, 560)
        spacing: 14

        Image {
            id: logo
            width: 340
            height: 140
            source: hero.steamLogo
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignLeft
            verticalAlignment: Image.AlignBottom
            asynchronous: true
            cache: true
            smooth: true
            visible: status === Image.Ready
        }
        Text {
            width: parent.width
            // Only without lettering: both at once reads as the title printed twice.
            visible: !logo.visible
            text: hero.title
            color: Theme.stageT1
            font.pixelSize: 40
            font.weight: Font.Bold
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
        Row {
            spacing: 14
            Repeater {
                model: hero.facts
                delegate: Text {
                    required property string modelData
                    text: modelData
                    color: Theme.stageInk
                    font.pixelSize: 14
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: pcTxt.implicitWidth + 12
                height: 19
                radius: 3
                color: "transparent"
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, 0.35)
                Text {
                    id: pcTxt
                    anchors.centerIn: parent
                    text: "PC"
                    color: Theme.stageInk
                    font.pixelSize: 11
                    font.family: Theme.fontSans
                }
            }
        }
    }
}
