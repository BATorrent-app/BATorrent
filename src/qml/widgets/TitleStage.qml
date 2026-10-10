// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A title's page: its art across the left two thirds, fading into a dark
// column on the right where whatever lists it (episodes, releases) lives.
// The identity sits at the bottom left, on the darkest part of the art.
//
// Plain properties rather than an item, like WorkHero: search, the library and
// a game each know a title in their own fields.
import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: stage
    property string title
    property string artUrl
    property string logoUrl
    property real rating: 0
    property var facts: []          // year, "4 Seasons", a studio — in that order
    property string badge           // HD, 4K, PC
    property string summary
    property var cast: []
    property var genres: []
    property int panelWidth: 520
    default property alias panel: panelHost.data

    readonly property real panelX: width - panelWidth
    // The fade is drawn for a 1440px window; scaled so it always finishes
    // exactly where the panel begins, at any width.
    readonly property real fadeEnd: width > 0 ? panelX / width : 0.64

    Rectangle { anchors.fill: parent; color: Theme.stageBg }

    Image {
        id: art
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: stage.panelX + 30
        source: stage.artUrl
        fillMode: Image.PreserveAspectCrop
        verticalAlignment: Image.AlignTop
        asynchronous: true
        cache: true
        opacity: status === Image.Ready ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: Theme.durSlower } }
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0;                    color: Theme.stageWash(0.88) }
            GradientStop { position: stage.fadeEnd * 0.413;  color: Theme.stageWash(0.45) }
            GradientStop { position: stage.fadeEnd * 0.667;  color: Theme.stageWash(0.15) }
            GradientStop { position: stage.fadeEnd * 0.952;  color: Theme.stageWash(0.95) }
            GradientStop { position: stage.fadeEnd;          color: Theme.stageWash(1) }
        }
    }
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.55; color: Theme.stageWash(0) }
            GradientStop { position: 1.0;  color: Theme.stageWash(0.9) }
        }
    }

    ColumnLayout {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: 56
        anchors.bottomMargin: 56
        width: Math.min(520, stage.panelX - 96)
        spacing: 18

        Image {
            id: logo
            Layout.preferredWidth: Math.min(420, parent.width)
            Layout.preferredHeight: implicitWidth > 0
                ? Layout.preferredWidth * (implicitHeight / implicitWidth) : 0
            Layout.maximumHeight: 190
            source: stage.logoUrl
            fillMode: Image.PreserveAspectFit
            horizontalAlignment: Image.AlignLeft
            asynchronous: true
            cache: true
            smooth: true
            visible: status === Image.Ready
        }
        Text {
            Layout.fillWidth: true
            visible: !logo.visible
            text: stage.title
            color: Theme.stageT1
            font.pixelSize: 52
            font.weight: Font.Bold
            font.letterSpacing: -0.8
            font.family: Theme.fontSans
            lineHeight: 1.0
            wrapMode: Text.WordWrap
            maximumLineCount: 2
            elide: Text.ElideRight
        }

        Row {
            spacing: 14
            visible: stage.rating > 0 || stage.facts.length > 0 || stage.badge.length > 0
            Text {
                visible: stage.rating > 0
                text: "★ " + stage.rating.toFixed(1)
                color: Theme.star
                font.pixelSize: 14; font.weight: Font.DemiBold
                font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Repeater {
                model: stage.facts
                Text {
                    required property var modelData
                    text: modelData
                    color: Theme.stageInk
                    font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
            Rectangle {
                visible: stage.badge.length > 0
                anchors.verticalCenter: parent.verticalCenter
                width: badgeTxt.implicitWidth + 12
                height: 19
                radius: 3
                color: "transparent"
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, 0.35)
                Text {
                    id: badgeTxt
                    anchors.centerIn: parent
                    text: stage.badge
                    color: Theme.stageInk
                    font.pixelSize: 11; font.family: Theme.fontSans
                }
            }
        }

        Text {
            Layout.fillWidth: true
            visible: text.length > 0
            text: stage.summary
            color: Theme.stageInk
            font.pixelSize: 15
            font.family: Theme.fontSans
            lineHeight: 1.6
            wrapMode: Text.WordWrap
            maximumLineCount: 4
            elide: Text.ElideRight
        }

        ColumnLayout {
            spacing: 5
            visible: stage.cast.length > 0 || stage.genres.length > 0
            component Credit: Text {
                property string label
                property var names: []
                Layout.fillWidth: true
                visible: names.length > 0
                textFormat: Text.StyledText
                text: "<font color='" + Theme.stageT4 + "'>" + label + "&nbsp;&nbsp;</font>"
                      + "<font color='" + Theme.stageT2 + "'>"
                      + names.join(", ").replace(/&/g, "&amp;").replace(/</g, "&lt;") + "</font>"
                font.pixelSize: 13
                font.family: Theme.fontSans
                lineHeight: 1.5
                elide: Text.ElideRight
            }
            Credit {
                label: (i18n.language, i18n.t("find_starring"))
                names: stage.cast
            }
            Credit {
                label: (i18n.language, i18n.t("find_genres"))
                names: stage.genres
            }
        }
    }

    Item {
        id: panelHost
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 32
        anchors.rightMargin: 28
        anchors.leftMargin: 20
        width: stage.panelWidth - 48
    }
}
