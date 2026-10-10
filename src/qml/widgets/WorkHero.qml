// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The top of a title's page: art behind, identity in front. One component for
// every kind of thing the app can show you — a series, a film, a game — and
// for every place it can be reached from: the library, a search, Discover.
//
// Deliberately plain properties rather than an item object. Search knows a
// work by TMDB fields, the library knows it by a torrent, and a game by IGDB;
// making the hero speak one of those three dialects is what left the app with
// three screens answering the same question.
import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: hero

    property string title
    property string artUrl          // backdrop when there is one, poster otherwise
    property string posterUrl
    property string subtitle        // year · type · rating, composed by the caller
    property var genres: []
    property var cast: []
    property string summary
    property bool showBack: true
    signal backRequested()

    // Art, dimmed hard enough that white text is readable over anything. A
    // poster standing in for a backdrop is cropped, not letterboxed: a band of
    // background beside it reads as a loading failure.
    Image {
        id: art
        anchors.fill: parent
        source: hero.artUrl
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        opacity: status === Image.Ready ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlower } }
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.45) }
            GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.68) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.9) }
        }
    }

    Item {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Theme.sp4
        width: 34; height: 34
        z: 2
        visible: hero.showBack
        IconImg {
            anchors.centerIn: parent
            src: "qrc:/icons/chevron-bold.svg"
            rotation: 180
            tint: backMa.containsMouse ? Theme.t1 : Theme.t2
            s: 18
        }
        MouseArea {
            id: backMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: hero.backRequested()
        }
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.sp5
        anchors.rightMargin: Theme.sp5 * 2
        spacing: Theme.sp2

        Text {
            Layout.fillWidth: true
            text: hero.title
            color: "#ffffff"
            font.pixelSize: 30
            font.weight: Font.Bold
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }
        Text {
            Layout.fillWidth: true
            visible: text.length > 0
            text: hero.subtitle
            color: Theme.t2
            font.pixelSize: 13
            font.family: Theme.fontSans
            font.features: Theme.tnum
            elide: Text.ElideRight
        }

        Flow {
            Layout.fillWidth: true
            Layout.topMargin: 2
            spacing: 6
            visible: hero.genres.length > 0 || hero.cast.length > 0
            Repeater {
                // Genres first, then names: both are chips because both are
                // the same kind of fact, "what this is" and "who is in it".
                model: hero.genres.slice(0, 3).concat(hero.cast.slice(0, 3))
                delegate: Rectangle {
                    required property var modelData
                    height: 24
                    width: chipTxt.implicitWidth + 20
                    radius: 12
                    color: Qt.rgba(1, 1, 1, 0.1)
                    Text {
                        id: chipTxt
                        anchors.centerIn: parent
                        text: parent.modelData
                        color: Theme.t1
                        font.pixelSize: 11
                        font.family: Theme.fontSans
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            Layout.topMargin: 2
            visible: text.length > 0
            text: hero.summary
            color: Theme.t2
            font.pixelSize: 13
            font.family: Theme.fontSans
            wrapMode: Text.WordWrap
            maximumLineCount: 4
            elide: Text.ElideRight
        }
    }
}
