// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Year/category chip + download/done/seeding/queue badges + seeding sheen
// for a Library PosterTile face. `tile` is the owning PosterTile.
import QtQuick
import QtQuick.Effects
import "../theme"

Item {
    id: root
    required property var tile
    anchors.fill: parent

    // top-left: year and category in ONE pill
    Rectangle {
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.leftMargin: 8
        anchors.topMargin: 8
        visible: tile.year > 0 || tile.category.length > 0
        radius: 9
        color: "#99000000"
        readonly property int maxW: Math.round(tile.width * 0.62)
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

    // The seeding pulse moved into the progress bar itself: it was a 2px
    // line at the poster's edge while downloading got a 9px pill, so the two
    // states drew the same fact at different sizes.

    // downloading badge (top-right)
    Rectangle {
        visible: tile.progress < 0.999 && tile.stateKey !== "queued"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 8
        anchors.topMargin: 8
        radius: 9
        color: "#cc000000"
        implicitWidth: dlRow.implicitWidth + 14
        implicitHeight: 18
        Row {
            id: dlRow
            anchors.centerIn: parent
            spacing: 4
            // A text arrow, the same way the meta line under the tile does it.
            // The disc-in-a-pill was two nested containers inside 18px, and the
            // glyph it held was a stroked SVG with no room for its own stroke.
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "\u2193"
                color: Theme.accent
                font.pixelSize: 12
                font.weight: Font.Bold
                font.family: Theme.fontSans
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: Math.floor(tile.progress * 100) + "%"
                color: "#ffffff"
                opacity: 0.92
                font.pixelSize: 10
                font.weight: Font.Bold
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
        }
    }

    // done badge (top-right)
    Rectangle {
        visible: tile.progress >= 0.999 && tile.stateKey !== "seeding"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 8
        anchors.topMargin: 8
        radius: 9
        color: "#cc000000"
        implicitWidth: doneRow.implicitWidth + 14
        implicitHeight: 18
        Row {
            id: doneRow
            anchors.centerIn: parent
            spacing: 4
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "✓"
                color: Theme.grn
                font.pixelSize: 12
                font.weight: Font.Bold
                font.family: Theme.fontSans
            }
            Text {
                text: (i18n.language, i18n.t("state_done_badge"))
                color: "#ffffff"
                opacity: 0.92
                font.pixelSize: 9
                font.weight: Font.Bold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.5
                font.family: Theme.fontSans
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // seeding badge (top-right)
    Rectangle {
        visible: tile.stateKey === "seeding"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 8
        anchors.topMargin: 8
        radius: 9
        color: "#cc000000"
        implicitWidth: seedRow.implicitWidth + 14
        implicitHeight: 18
        Row {
            id: seedRow
            anchors.centerIn: parent
            spacing: 4
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: "\u2191"
                color: Theme.amber
                font.pixelSize: 12
                font.weight: Font.Bold
                font.family: Theme.fontSans
            }
            Text {
                text: (i18n.language, i18n.t("state_seeding"))
                color: "#ffffff"
                opacity: 0.92
                font.pixelSize: 9
                font.weight: Font.Bold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.5
                font.family: Theme.fontSans
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    // queue badge (top-right)
    Rectangle {
        visible: tile.stateKey === "queued"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.rightMargin: 8
        anchors.topMargin: 8
        radius: 9
        color: "#cc000000"
        implicitWidth: queueRow.implicitWidth + 14
        implicitHeight: 18
        Row {
            id: queueRow
            anchors.centerIn: parent
            spacing: 4
            Rectangle {
                width: 13
                height: 13
                radius: 6.5
                color: "transparent"
                border.color: Theme.t4
                border.width: 1.5
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    anchors.centerIn: parent
                    text: "⋯"
                    color: Theme.t4
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.family: Theme.fontSans
                }
            }
            Text {
                text: (i18n.language, i18n.t("state_queued").arg(tile.queuePos))
                color: "#ffffff"
                opacity: 0.92
                font.pixelSize: 10
                font.weight: Font.Bold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.5
                font.family: Theme.fontSans
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
