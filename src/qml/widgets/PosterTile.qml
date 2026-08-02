// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    property var win
    property var controller
    id: tile
    width: 178
    height: 286

    required property int index
    required property string torrentName
    required property string metaTitle
    required property string stateKey
    required property real progress
    required property string posterPath
    required property string stateString
    required property string stateDetail
    required property string fileKind
    required property string category
    required property string size
    required property string downSpeed
    required property string upSpeed
    required property real downRate
    required property real upRate
    required property var sizeBytes
    required property string infoHash
    required property bool playable
    required property string downloaded
    required property int year
    required property string genres
    required property int queuePos

    readonly property bool isDownloading: stateKey !== "seeding" && stateKey !== "finished"
        && stateKey !== "completed" && stateKey !== "paused" && stateKey !== "queued"
        && stateKey !== "missing"
    readonly property int etaSec: (downRate > 0 && progress < 1.0 && sizeBytes > 0)
        ? Math.round(sizeBytes * (1 - progress) / downRate) : -1

    readonly property bool hasBadge: stateKey === "seeding" || stateKey === "queued"
        || progress >= 0.999
    readonly property string metaLine: genres
    readonly property string posterUrl: win.fileUrl(posterPath)

    Rectangle {
        z: -1
        width: 178 * 0.84
        x: (178 - width) / 2
        y: 237 - 10
        height: 22
        radius: 11
        color: "#000000"
        opacity: tileMa.containsMouse ? (Theme.isLight ? 0.22 : 0.5) : 0
        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
        layer.enabled: true
        layer.effect: MultiEffect { blurEnabled: true; blur: 1.0; blurMax: 28 }
    }

    Item {
        id: posterWrap
        width: 178
        height: 237

        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "#161618"
            visible: tile.posterUrl === ""
            // Last resort only: no usable extension, no resolved type, nothing
            // to say. The bat earns the middle when it is genuinely the only
            // thing we know — as a permanent backdrop it was just noise.
            Image {
                anchors.centerIn: parent
                width: parent.width * 0.5
                height: width
                visible: tile.fileKind.length === 0
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(width * 2, width * 2)
                fillMode: Image.PreserveAspectFit
                opacity: 0.06
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
            }

            // A typographic cover rather than a centred watermark. Centred and
            // symmetrical is what reads as a placeholder; a hero set high and
            // left, with air under it, reads as a decision. The extension is
            // also the most useful thing we know about a torrent with no art.
            Column {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.leftMargin: 13
                anchors.rightMargin: 13
                anchors.topMargin: Math.round(parent.height * 0.17)
                spacing: 7
                visible: tile.fileKind.length > 0

                Text {
                    id: kindText
                    text: tile.fileKind
                    color: "#f5f5f6"
                    // HorizontalFit so a long one (M2TS, WEBM) shrinks to the
                    // tile instead of being clipped or eliding to nonsense.
                    font.pixelSize: Math.round(tile.width * 0.30)
                    fontSizeMode: Text.HorizontalFit
                    minimumPixelSize: 20
                    width: parent.width
                    horizontalAlignment: Text.AlignLeft
                    font.weight: Font.Bold
                    font.letterSpacing: -1.5
                    font.family: Theme.fontSans
                }
                // Red only here, and only under the hero: colour is signal, and
                // this is the one place on a blank tile that earns it.
                Rectangle {
                    width: Math.min(kindText.contentWidth, parent.width)
                    height: 3
                    radius: 1.5
                    color: Theme.accent
                }
            }
            Text {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 13
                anchors.rightMargin: 13
                anchors.bottomMargin: 15
                text: tile.metaTitle || tile.torrentName
                color: "#f5f5f6"
                font.pixelSize: 18
                font.weight: Font.Bold
                font.letterSpacing: -0.3
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
        }

        Rectangle {
            id: posterBg
            anchors.fill: parent
            color: "#161618"
            visible: false
            layer.enabled: true
            Image {
                anchors.fill: parent
                source: tile.posterUrl
                fillMode: Image.PreserveAspectCrop
                asynchronous: true
                sourceSize: Qt.size(356, 474)
                cache: true
            }
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: parent.height * 0.6
                gradient: Gradient {
                    GradientStop { position: 0.0; color: "transparent" }
                    GradientStop { position: 0.55; color: Qt.rgba(0, 0, 0, 0.45) }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.92) }
                }
            }
        }
        Rectangle {
            id: posterMask
            anchors.fill: parent
            radius: 10
            color: "white"
            visible: false
            layer.enabled: true
        }
        MultiEffect {
            source: posterBg
            anchors.fill: parent
            maskEnabled: true
            maskSource: posterMask
            visible: tile.posterUrl !== ""
        }
        Text {
            visible: tile.posterUrl !== ""
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            // Clears the progress bar's band (8 + 9 high) instead of sitting in
            // it. Drops back down when the bar goes away on completion.
            anchors.bottomMargin: progBar.visible ? 25 : 12
            Behavior on anchors.bottomMargin { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
            text: tile.metaTitle || tile.torrentName
            color: "#f5f5f6"
            font.pixelSize: 15
            font.weight: Font.Bold
            font.letterSpacing: -0.2
            font.family: Theme.fontSans
            elide: Text.ElideRight
            maximumLineCount: 2
            wrapMode: Text.WordWrap
        }

        Rectangle {
            id: progBar
            visible: tile.progress < 0.999
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            anchors.bottomMargin: 8
            height: 9
            radius: 4.5
            color: Qt.rgba(0, 0, 0, 0.78)
            border.color: Qt.rgba(1, 1, 1, 0.10)
            border.width: 1
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 1
                height: parent.height - 2
                radius: (parent.height - 2) / 2
                width: Math.max(height, (parent.width - 2) * tile.progress)
                color: win.fillFor(tile.stateKey)
                Behavior on width { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
            }
        }

        Rectangle {
            visible: tile.playable && tile.progress > 0.02
                     && (tileMa.containsMouse || ptMa.containsMouse || controller.isRowSelected(tile.index))
            x: (parent.width - width) / 2
            y: (parent.height - height) / 2
            width: 46
            height: 46
            radius: 23
            z: 5
            color: "#cc101014"
            border.color: ptMa.containsMouse ? Theme.accent : Qt.rgba(1, 1, 1, 0.25)
            border.width: 1
            scale: ptMa.containsMouse ? 1.08 : 1.0
            Behavior on border.color { ColorAnimation { duration: 120 } }
            Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            IconImg {
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 1
                src: "qrc:/icons/play.svg"
                tint: ptMa.containsMouse ? Theme.accent : "#ffffff"
                s: 18
            }
            MouseArea {
                id: ptMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (typeof session !== "undefined") session.playByHash(tile.infoHash)
            }
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: -2
            radius: 12
            color: "transparent"
            visible: controller.isRowSelected(tile.index)
            border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.28)
            border.width: 4
        }
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "transparent"
            border.color: controller.isRowSelected(tile.index) ? Theme.accent
                          : (tileMa.containsMouse ? Qt.rgba(1, 1, 1, 0.2) : Theme.hair)
            border.width: controller.isRowSelected(tile.index) ? 2 : 1
            Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        PosterTileBadges { tile: tile }

        MouseArea {
            id: tileMa
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            onClicked: function(mouse) {
                if (mouse.button === Qt.RightButton) {
                    if (!controller.isRowSelected(tile.index)) controller.selectRow(tile.index, 0)
                    win.openContext(tile.index)
                } else {
                    controller.selectRow(tile.index, mouse.modifiers)
                }
            }
            onDoubleClicked: function(mouse) {
                if (mouse.button !== Qt.RightButton) {
                    controller.selectRow(tile.index, 0)
                    session.openSelectedFile()
                }
            }
        }
    }

    PosterTileMeta {
        id: meta
        tile: tile
        win: tile.win
        anchors.top: posterWrap.bottom
        anchors.topMargin: 10
        anchors.left: posterWrap.left
        anchors.right: posterWrap.right
    }
}
