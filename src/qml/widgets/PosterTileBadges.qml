// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The two things a PosterTile face says over its artwork: what this release is
// (year/category, top-left) and what it is doing (status, top-right).
// `tile` is the owning PosterTile.
//
// There used to be four near-identical status pills here — downloading, done,
// seeding, queued — each with its own glyph, its own label and its own idea of
// what the state was called. That is how the grid ended up disagreeing with the
// list about the same torrent. One pill now, drawing the engine's own
// stateString through the shared StatusMark, so there is nothing left to drift.
import QtQuick
import "../theme"

Item {
    id: root
    required property var tile
    anchors.fill: parent

    // status (top-right) — laid out first because it gets first call on the
    // width; the year/category pill takes what is left.
    Rectangle {
        id: statusPill
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 8
        anchors.topMargin: 8
        radius: 9
        color: "#cc000000"
        implicitWidth: Math.min(statusMark.implicitWidth + 14,
                                Math.round(tile.width * 0.62))
        implicitHeight: 18
        transformOrigin: Item.Right
        Behavior on implicitWidth {
            NumberAnimation { duration: Theme.durBase; easing.type: Theme.easeOut }
        }

        StatusMark {
            id: statusMark
            anchors.centerIn: parent
            stateKey: tile.stateKey
            // The engine's label, not one rebuilt from the key: the list row
            // renders this exact string, so the two views cannot disagree.
            label: tile.stateString
            onArtwork: true
            stalled: tile.isDownloading && tile.stateDetail.length > 0
            maxLabelWidth: Math.round(tile.width * 0.62) - 14 - symbolSize - spacing
        }

        SequentialAnimation {
            id: statusSwap
            NumberAnimation {
                target: statusMark; property: "opacity"; to: 0
                duration: Theme.durExit; easing.type: Theme.easeIn
            }
            PropertyAction { target: statusPill; property: "scale"; value: Theme.grow(0.72) }
            ParallelAnimation {
                NumberAnimation {
                    target: statusMark; property: "opacity"; to: 1
                    duration: Theme.durFast
                }
                NumberAnimation {
                    target: statusPill; property: "scale"; to: 1
                    duration: 340; easing.type: Theme.easePop; easing.overshoot: 1.3
                }
            }
        }
        Connections {
            target: tile
            function onStateKeyChanged() { statusSwap.restart() }
        }
    }

    // year + category (top-left) in ONE pill
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 8
        anchors.topMargin: 8
        readonly property int maxW: Math.round(tile.width) - 16 - 6 - statusPill.width
        // Below this it is a stub with an ellipsis in it, which says less than
        // showing nothing at all.
        visible: (tile.year > 0 || tile.category.length > 0) && maxW >= 34
        radius: 9
        color: "#99000000"
        implicitWidth: Math.min(tagRow.implicitWidth + 12, maxW)
        implicitHeight: 18

        Row {
            id: tagRow
            anchors.centerIn: parent
            spacing: 5
            Text {
                id: yrTxt
                anchors.verticalCenter: parent.verticalCenter
                visible: tile.year > 0
                text: tile.year
                color: "#ffffff"
                opacity: 0.92
                font.pixelSize: 10
                font.weight: Font.Bold
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                visible: tile.year > 0 && tile.category.length > 0
                text: "·"
                color: "#ffffff"
                opacity: 0.45
                font.pixelSize: 10
                font.family: Theme.fontSans
            }
            Text {
                id: catTxt
                anchors.verticalCenter: parent.verticalCenter
                visible: tile.category.length > 0
                text: tile.category
                width: Math.min(implicitWidth,
                                tagRow.parent.maxW - 12
                                - (yrTxt.visible ? yrTxt.implicitWidth + 10 : 0))
                elide: Text.ElideRight
                color: "#ffffff"
                opacity: 0.88
                font.pixelSize: 9
                font.weight: Font.Bold
                font.letterSpacing: 1.0
                font.capitalization: Font.AllUppercase
                font.family: Theme.fontSans
            }
        }
    }
}
