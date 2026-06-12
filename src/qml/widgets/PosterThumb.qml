// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent List Thumbs.html .thumb — poster thumbnail for list rows.
// 30×40 (3:4), radius 5, object-fit cover. Fallback: initial letter centered.
import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    id: thumb
    property string posterUrl: ""
    // title behind the row — drives the generated placeholder (per-title hue +
    // initial) so coverless results don't repeat one identical logo down the list
    property string label: ""
    implicitWidth: 30
    implicitHeight: 40

    readonly property real ph: {
        var h = 0
        for (var i = 0; i < label.length; i++) h = (h * 31 + label.charCodeAt(i)) % 3600
        return (h % 360) / 360
    }

    // base (also the placeholder bg)
    Rectangle {
        anchors.fill: parent
        radius: 5
        border.color: Theme.hair
        border.width: 1
        gradient: Gradient {
            GradientStop { position: 0.0; color: thumb.label === "" ? "#161618" : Qt.hsla(thumb.ph, 0.30, Theme.isLight ? 0.66 : 0.30, 1) }
            GradientStop { position: 1.0; color: thumb.label === "" ? "#161618" : Qt.hsla(thumb.ph, 0.36, Theme.isLight ? 0.52 : 0.17, 1) }
        }
        Image {
            anchors.centerIn: parent
            visible: thumb.posterUrl === "" && thumb.label === ""
            width: parent.width * 0.55
            height: width
            source: "qrc:/images/logo.svg"
            sourceSize: Qt.size(width * 2, width * 2)
            fillMode: Image.PreserveAspectFit
            opacity: 0.5
        }
        Text {
            anchors.centerIn: parent
            visible: thumb.posterUrl === "" && thumb.label !== ""
            text: thumb.label.charAt(0).toUpperCase()
            color: Qt.rgba(1, 1, 1, 0.88)
            font.pixelSize: Math.round(parent.height * 0.42)
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
    }

    // poster image (masked rounded)
    Rectangle {
        id: imgContent
        anchors.fill: parent
        color: "#161618"
        visible: false
        layer.enabled: true
        Image {
            anchors.fill: parent
            source: thumb.posterUrl
            fillMode: Image.PreserveAspectCrop
            asynchronous: true
            cache: true
            sourceSize: Qt.size(64, 86)   // drawn tiny in list view — don't decode full-res
        }
    }
    Rectangle {
        id: imgMask
        anchors.fill: parent
        radius: 5
        color: "white"
        visible: false
        layer.enabled: true
    }
    MultiEffect {
        source: imgContent
        anchors.fill: parent
        maskEnabled: true
        maskSource: imgMask
        visible: thumb.posterUrl !== ""
    }
}
