// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One of the two cards on top of the HUB: the thing you were in the middle of,
// on its own art, with the one button that takes you back into it.
import QtQuick
import "../theme"
import "../widgets"

Item {
    id: card
    property string kicker
    property var artSources: []
    property int tmdbId: 0
    property string tmdbType
    property string fixedLogo
    property string title
    property real logoWidth: 220
    property string line
    property string buttonText
    property bool buttonLight: false    // white for watching, the accent for playing
    property bool buttonEnabled: true
    property real progress: -1
    property string note
    signal pressed()
    signal opened()

    height: 300

    Rectangle {
        id: frame
        anchors.fill: parent
        radius: 8
        clip: true
        color: "#0a0a0b"

        ChainImage {
            anchors.fill: parent
            sources: card.artSources
            scale: openMa.containsMouse ? 1.02 : 1
            Behavior on scale { NumberAnimation { duration: Theme.durSlower; easing.type: Theme.easeOut } }
        }
        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: Qt.rgba(10 / 255, 10 / 255, 11 / 255, 0.88) }
                GradientStop { position: 0.55; color: Qt.rgba(10 / 255, 10 / 255, 11 / 255, 0.42) }
                GradientStop { position: 1.0; color: Qt.rgba(10 / 255, 10 / 255, 11 / 255, 0.12) }
            }
        }
        MouseArea {
            id: openMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.opened()
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 32
            anchors.rightMargin: 32
            anchors.bottomMargin: 30
            spacing: 12

            Text {
                text: card.kicker
                color: Theme.stageT2
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.letterSpacing: 1.2
                font.capitalization: Font.AllUppercase
                font.family: Theme.fontSans
            }
            TitleMark {
                width: maxWidth
                height: Math.max(implicitHeight, 24)
                maxWidth: card.logoWidth
                maxHeight: 90
                fontSize: 26
                tmdbId: card.tmdbId
                type: card.tmdbType
                title: card.title
                fixedLogo: card.fixedLogo
            }
            Text {
                visible: text.length > 0
                text: card.line
                color: Theme.stageInk
                font.pixelSize: 14
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
            Row {
                spacing: 14
                Rectangle {
                    id: btn
                    height: 40
                    width: btnRow.implicitWidth + 36
                    radius: 4
                    opacity: card.buttonEnabled ? 1 : 0.6
                    color: card.buttonLight ? (btnMa.containsMouse ? "#ffffff" : Theme.stageT1)
                                            : (btnMa.containsMouse ? Qt.lighter(Theme.accent, 1.08) : Theme.accent)
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                    scale: btnMa.pressed ? Theme.pressScale : 1
                    Behavior on scale { NumberAnimation { duration: Theme.durFast } }
                    Row {
                        id: btnRow
                        anchors.centerIn: parent
                        spacing: 9
                        IconImg {
                            anchors.verticalCenter: parent.verticalCenter
                            src: "qrc:/icons/play.svg"
                            tint: card.buttonLight ? Theme.stageBg : "#ffffff"
                            s: 13
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: card.buttonText
                            color: card.buttonLight ? Theme.stageBg : "#ffffff"
                            font.pixelSize: 14
                            font.weight: Font.DemiBold
                            font.family: Theme.fontSans
                        }
                    }
                    MouseArea {
                        id: btnMa
                        anchors.fill: parent
                        enabled: card.buttonEnabled
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: card.pressed()
                    }
                }
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: card.progress >= 0
                    width: 140
                    height: 3
                    radius: 2
                    color: Qt.rgba(1, 1, 1, 0.25)
                    Rectangle {
                        width: parent.width * Math.max(0, Math.min(1, card.progress))
                        height: parent.height
                        radius: 2
                        color: Theme.accent
                    }
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    visible: text.length > 0
                    text: card.note
                    color: Theme.stageT2
                    font.pixelSize: 12
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
        }
    }
}
