// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Cover-art card for Discover (and reusable in Search/HUB). 2:3 poster with a
// rounded mask, hover scale + ring, title + year/rating below. Emits activated().
import QtQuick
import QtQuick.Effects
import QtQuick.Controls.Basic
import "../theme"

Item {
    id: card
    property string title: ""
    property string poster: ""
    property string year: ""
    property real rating: 0
    property string type: ""
    signal activated()
    signal getWatch()       // ▶ one-click Get & Watch (movie/series)

    property int posterW: 150
    readonly property int posterH: Math.round(posterW * 1.5)
    property string synopsis: ""    // optional: shown as a hover tooltip
    // optional "My List" toggle (only shown when watchlistEnabled)
    property bool watchlistEnabled: false
    property bool saved: false
    signal watchlistToggle()

    implicitWidth: posterW
    implicitHeight: posterH + 42

    // poster (scales on hover)
    Item {
        id: art
        width: card.posterW
        height: card.posterH
        scale: ma.containsMouse ? 1.04 : 1.0
        Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

        // skeleton placeholder with a shimmer sweep while the cover downloads;
        // crossfades out as the poster fades in.
        Rectangle {
            id: ph
            anchors.fill: parent
            radius: 10
            color: "#161618"
            clip: true
            border.color: ma.containsMouse ? Theme.accent : Theme.hair
            border.width: 1
            opacity: imgSrc.status === Image.Ready ? 0 : 1
            Behavior on opacity { NumberAnimation { duration: 300 } }
            Behavior on border.color { ColorAnimation { duration: 140 } }
            Image {
                anchors.centerIn: parent
                width: parent.width * 0.4
                height: width
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(width * 2, width * 2)
                fillMode: Image.PreserveAspectFit
                opacity: 0.32
            }
            Rectangle {
                width: parent.width * 0.7
                height: parent.height * 2.2
                rotation: 18
                y: -parent.height * 0.6
                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop { position: 0.0; color: "#00ffffff" }
                    GradientStop { position: 0.5; color: "#16ffffff" }
                    GradientStop { position: 1.0; color: "#00ffffff" }
                }
                x: -parent.width
                NumberAnimation on x {
                    from: -parent.width; to: parent.width * 1.7
                    duration: 1200; loops: Animation.Infinite
                    running: imgSrc.status !== Image.Ready
                }
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
            opacity: imgSrc.status === Image.Ready ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 350; easing.type: Easing.OutCubic } }
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

    // synopsis on hover — a fixed-width card beside the poster. The explicit
    // width is essential: a bare Text reports its full unwrapped line as
    // implicitWidth, so the tooltip would stretch across the whole window.
    ToolTip {
        id: synTip
        parent: card
        visible: card.synopsis.length > 0 && ma.containsMouse
        delay: 550
        width: 300
        padding: 12
        x: card.posterW + 10
        y: (card.posterH - height) / 2   // centered beside the poster (Popup clamps to the window)
        contentItem: Text {
            text: card.synopsis
            width: synTip.availableWidth
            wrapMode: Text.WordWrap
            maximumLineCount: 9
            elide: Text.ElideRight
            color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans; lineHeight: 1.38
        }
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 9 }
    }

    // ▶ Get & Watch — centered play button on hover (movie/series only). On top
    // of the main MouseArea so its own click is intercepted.
    Rectangle {
        visible: card.type !== "game" && (ma.containsMouse || pbMa.containsMouse)
        x: (card.posterW - width) / 2
        y: (card.posterH - height) / 2
        width: 46; height: 46; radius: 23
        // dark glass disc; red only as the hover accent (ring + glyph),
        // never a filled surface — same language as the grid tiles
        color: "#cc101014"
        border.color: pbMa.containsMouse ? Theme.accent : Qt.rgba(1, 1, 1, 0.25)
        border.width: 1
        scale: pbMa.containsMouse ? 1.08 : 1.0
        Behavior on border.color { ColorAnimation { duration: 120 } }
        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        IconImg {
            anchors.centerIn: parent
            anchors.horizontalCenterOffset: 1
            src: "qrc:/icons/play.svg"
            tint: pbMa.containsMouse ? Theme.accent : "#ffffff"
            s: 18
        }
        MouseArea {
            id: pbMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.getWatch()
        }
    }

    // "My List" toggle — on top of the main MouseArea so it gets the click
    Rectangle {
        visible: card.watchlistEnabled && (ma.containsMouse || wlMa.containsMouse || card.saved)
        x: 6; y: 6
        width: 26; height: 26; radius: 13
        color: wlMa.containsMouse ? "#cc000000" : "#99000000"
        IconImg {
            anchors.centerIn: parent
            src: "qrc:/icons/heart.svg"
            tint: card.saved ? Theme.accent : "#ffffff"
            s: 14
        }
        MouseArea {
            id: wlMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.watchlistToggle()
        }
    }
}
