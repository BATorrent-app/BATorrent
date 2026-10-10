// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Top of the series screen: backdrop, poster, title block, synopsis, and the
// season picker. Split out of HubSeriesPage so the page stays a composer and
// this keeps the one piece with real layout in it.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: hero
    property var hub
    property var show
    signal closeRequested()

    // The poster, blurred and dimmed, standing in for a backdrop we have not
    // fetched. Cheap, always available, and it carries the show's own colour.
    Image {
        id: art
        anchors.fill: parent
        source: hero.show && hero.show.poster ? hero.hub.fileUrl(hero.show.poster) : ""
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: true
        opacity: status === Image.Ready ? 0.22 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlower } }
    }
    // Dark enough to read white text over any poster, and darkest at the
    // bottom where the summary sits.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.35) }
            GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.6) }
            GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.88) }
        }
    }

    Item {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.margins: Theme.sp4
        width: 34
        height: 34
        z: 2
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
            onClicked: hero.closeRequested()
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.sp5
        anchors.topMargin: 64
        spacing: Theme.sp5

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.alignment: Qt.AlignVCenter
            spacing: Theme.sp2

            Text {
                Layout.fillWidth: true
                text: hero.show ? (hero.show.title || "") : ""
                color: Theme.t1
                font.pixelSize: 26
                font.weight: Font.Bold
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text: {
                    if (!hero.show) return ""
                    var parts = []
                    if (hero.show.year) parts.push(hero.show.year)
                    var n = hero.show.seasons ? hero.show.seasons.length : 0
                    if (n > 0) parts.push((i18n.language, i18n.t(n === 1 ? "hub_n_season" : "hub_n_seasons")).arg(n))
                    if (hero.show.genres && hero.show.genres.length > 0)
                        parts.push(hero.show.genres.slice(0, 3).join(" · "))
                    return parts.join("  ·  ")
                }
                color: Theme.t3
                font.pixelSize: 13
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Text {
                Layout.fillWidth: true
                Layout.maximumHeight: 90
                visible: text.length > 0
                text: hero.show ? (hero.show.description || "") : ""
                color: Theme.t2
                font.pixelSize: 13
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
            }

        }

    }
}
