// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    property var win
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
    required property string category
    required property string size
    required property string downSpeed
    required property string upSpeed
    required property real downRate
    required property var sizeBytes

    readonly property bool isDownloading: stateKey !== "seeding" && stateKey !== "finished"
        && stateKey !== "completed" && stateKey !== "paused" && stateKey !== "queued"
    readonly property int etaSec: (downRate > 0 && progress < 1.0 && sizeBytes > 0)
        ? Math.round(sizeBytes * (1 - progress) / downRate) : -1

    readonly property string posterUrl: win.fileUrl(posterPath)

    // soft drop shadow under the cover, fading in on hover
    Rectangle {
        z: -1
        width: 178 * 0.84
        x: (178 - width) / 2
        y: 237 - 10
        height: 22
        radius: 11
        color: "#000000"
        opacity: tileMa.containsMouse ? 0.5 : 0
        Behavior on opacity { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
        layer.enabled: true
        layer.effect: MultiEffect { blurEnabled: true; blur: 1.0; blurMax: 28 }
    }

    // .poster wrapper (aspect 3:4 ≈ 178:237)
    Item {
        id: posterWrap
        width: 178
        height: 237

        // fallback (no poster): tinted bg + watermark + category + title
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "#161618"
            visible: tile.posterUrl === ""
            // watermark: BATorrent logo (not the title's first letter)
            Image {
                anchors.centerIn: parent
                width: parent.width * 0.5
                height: width
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(width * 2, width * 2)
                fillMode: Image.PreserveAspectFit
                opacity: 0.06
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
            }
            Text {
                anchors.left: parent.left; anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.leftMargin: 13; anchors.rightMargin: 13; anchors.bottomMargin: 15
                text: tile.metaTitle || tile.torrentName
                color: "#f5f5f6"
                font.pixelSize: 18; font.weight: Font.Bold; font.letterSpacing: -0.3
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                maximumLineCount: 3
                elide: Text.ElideRight
            }
        }

        // poster image (masked rounded) — only when present
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
                // decode at ~2× display size, not the poster's full
                // resolution — cuts memory and decode time per cover.
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
        // title over the fade (only when poster present)
        Text {
            visible: tile.posterUrl !== ""
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.bottomMargin: 12
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
        // .pbar progress (bottom, over everything)
        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 3
            color: Qt.rgba(0, 0, 0, 0.5)
            Rectangle {
                height: parent.height
                width: parent.width * tile.progress
                color: win.fillFor(tile.stateKey)
                Behavior on width { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
            }
        }
        // border overlay (radius 10, hair / accent when sel)
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "transparent"
            border.color: win.isRowSelected(tile.index) ? Theme.accent : (tileMa.containsMouse ? Qt.rgba(1,1,1,0.2) : Theme.hair)
            border.width: win.isRowSelected(tile.index) ? 2 : 1
            Behavior on border.color { ColorAnimation { duration: 120; easing.type: Easing.OutCubic } }
        }

        // category chip (top-left) — dark pill so it reads on any cover
        Rectangle {
            visible: tile.category.length > 0
            anchors.left: parent.left; anchors.top: parent.top
            anchors.leftMargin: 8; anchors.topMargin: 8
            radius: 5; color: "#99000000"
            implicitWidth: catTxt.implicitWidth + 12; implicitHeight: 18
            Text {
                id: catTxt; anchors.centerIn: parent
                text: tile.category
                color: "#ffffff"; opacity: 0.88
                font.pixelSize: 9; font.weight: Font.Bold; font.letterSpacing: 1.0
                font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
            }
        }
        // download % (top-right) — hidden once complete; tint follows state
        Rectangle {
            visible: tile.progress < 0.999
            anchors.right: parent.right; anchors.top: parent.top
            anchors.rightMargin: 8; anchors.topMargin: 8
            radius: 9; color: "#cc000000"
            implicitWidth: pctTxt.implicitWidth + 14; implicitHeight: 18
            Text {
                id: pctTxt; anchors.centerIn: parent
                text: Math.round(tile.progress * 100) + "%"
                color: "#ffffff"
                font.pixelSize: 10; font.weight: Font.Bold; font.family: Theme.fontSans
            }
        }

        MouseArea {
            id: tileMa
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            onClicked: function(mouse) {
                if (mouse.button === Qt.RightButton) {
                    if (!win.isRowSelected(tile.index)) win.selectRow(tile.index, 0)
                    win.openContext(tile.index)
                } else {
                    win.selectRow(tile.index, mouse.modifiers)
                }
            }
            onDoubleClicked: function(mouse) {
                if (mouse.button !== Qt.RightButton) {
                    win.selectRow(tile.index, 0); session.openSelectedFile()
                }
            }
        }
    }

    // meta — line 1: state dot + live info (speed·ETA when downloading,
    // else the status word); line 2: size. No redundant "Downloading":
    // the dot + the % pill + the bar already say it.
    Column {
        id: meta
        anchors.top: posterWrap.bottom
        anchors.topMargin: 10
        anchors.left: posterWrap.left
        anchors.right: posterWrap.right
        spacing: 2

        Item {
            width: meta.width; height: 16
            Row {
                anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                spacing: 6
                Rectangle {
                    width: 6; height: 6; radius: 3
                    anchors.verticalCenter: parent.verticalCenter
                    // a stalled download reads amber (health), not the state colour
                    color: (tile.isDownloading && tile.stateDetail.length > 0) ? Theme.amber : win.dotFor(tile.stateKey)
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    // stalled reads amber (dot + text); the full reason is on hover —
                    // the grid card is too narrow to show it inline without clipping
                    text: tile.isDownloading ? ("↓ " + tile.downSpeed)
                          : (tile.stateKey === "seeding" ? ("↑ " + tile.upSpeed) : tile.stateString)
                    color: (tile.isDownloading && tile.stateDetail.length > 0) ? Theme.amber : win.textFor(tile.stateKey)
                    font.pixelSize: 12; font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
            }
            Text {
                // ETA while downloading; once there's nothing left to
                // fetch, the size takes this slot (line 2 collapses) so
                // it's never left orphaned under an empty ETA.
                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                text: tile.etaSec >= 0 ? win.fmtEta(tile.etaSec) : tile.size
                color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontMono
            }
        }
        Text {
            width: meta.width
            horizontalAlignment: Text.AlignRight
            visible: tile.etaSec >= 0
            text: tile.size
            color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono
        }
    }
}
