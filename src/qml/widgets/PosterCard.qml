// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Cover-art card for Discover (and reusable in Search/HUB). 2:3 poster with a
// rounded mask, hover scale + ring, title + year/rating below. Emits activated().
import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    id: card
    property string title: ""
    property string poster: ""
    property string year: ""
    property real rating: 0
    property string type: ""
    signal activated()

    property int posterW: 150
    readonly property int posterH: Math.round(posterW * 1.5)

    implicitWidth: posterW
    implicitHeight: posterH + 42

    // poster (scales on hover)
    Item {
        id: art
        width: card.posterW
        height: card.posterH
        scale: ma.containsMouse ? 1.04 : 1.0
        Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

        // placeholder bg + dimmed logo
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "#161618"
            border.color: ma.containsMouse ? Theme.accent : Theme.hair
            border.width: 1
            Behavior on border.color { ColorAnimation { duration: 140 } }
            Image {
                anchors.centerIn: parent
                visible: imgSrc.status !== Image.Ready
                width: parent.width * 0.4
                height: width
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(width * 2, width * 2)
                fillMode: Image.PreserveAspectFit
                opacity: 0.4
            }
        }

        Image {
            id: imgSrc
            anchors.fill: parent
            source: card.poster
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            visible: false
            sourceSize: Qt.size(card.posterW * 2, card.posterH * 2)
        }
        Rectangle { id: imgMask; anchors.fill: parent; radius: 10; visible: false; layer.enabled: true }
        MultiEffect {
            anchors.fill: parent
            source: imgSrc
            maskEnabled: true
            maskSource: imgMask
            visible: imgSrc.status === Image.Ready
        }

        // rating pill
        Rectangle {
            visible: card.rating > 0
            anchors.top: parent.top; anchors.right: parent.right
            anchors.topMargin: 6; anchors.rightMargin: 6
            radius: 6
            color: "#cc000000"
            implicitWidth: rt.width + 12
            implicitHeight: 18
            Text {
                id: rt
                anchors.centerIn: parent
                text: "★ " + card.rating.toFixed(1)
                color: "#ffd24a"
                font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
        }
    }

    // title
    Text {
        anchors.top: art.bottom; anchors.topMargin: 8
        width: card.posterW
        text: card.title
        color: ma.containsMouse ? Theme.t1 : Theme.t2
        font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
        elide: Text.ElideRight
        maximumLineCount: 1
        Behavior on color { ColorAnimation { duration: 140 } }
    }
    Text {
        anchors.top: art.bottom; anchors.topMargin: 25
        width: card.posterW
        visible: card.year.length > 0
        text: card.year
        color: Theme.t4
        font.pixelSize: 10; font.family: Theme.fontSans
        elide: Text.ElideRight
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: card.activated()
    }
}
