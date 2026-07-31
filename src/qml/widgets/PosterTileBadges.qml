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

    // seeding pulse along the poster's bottom edge while uploading
    Rectangle {
        id: seedTrack
        visible: tile.stateKey === "seeding" && tile.upRate > 0
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        anchors.bottomMargin: 8
        height: 2
        radius: 1
        color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.10)
        clip: true
        Rectangle {
            id: seedSheen
            width: 44
            height: parent.height
            radius: parent.radius
            color: Qt.rgba(Theme.amber.r, Theme.amber.g, Theme.amber.b, 0.5)
            SequentialAnimation on x {
                running: seedTrack.visible
                loops: Animation.Infinite
                NumberAnimation {
                    from: -seedSheen.width
                    to: seedTrack.width
                    duration: 2600
                    easing.type: Easing.InOutSine
                }
                PauseAnimation { duration: 900 }
            }
        }
    }

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
            Rectangle {
                width: 13
                height: 13
                radius: 6.5
                color: "transparent"
                border.color: Theme.accent
                border.width: 1.5
                anchors.verticalCenter: parent.verticalCenter
                IconImg {
                    anchors.centerIn: parent
                    src: "qrc:/icons/arrow-down.svg"
                    tint: Theme.accent
                    s: 9
                }
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
            Rectangle {
                width: 13
                height: 13
                radius: 6.5
                color: "transparent"
                border.color: Theme.grn
                border.width: 1.5
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: Theme.grn
                    font.pixelSize: 8
                    font.weight: Font.Bold
                    font.family: Theme.fontSans
                }
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
            Rectangle {
                width: 13
                height: 13
                radius: 6.5
                color: "transparent"
                border.color: Theme.amber
                border.width: 1.5
                anchors.verticalCenter: parent.verticalCenter
                IconImg {
                    anchors.centerIn: parent
                    src: "qrc:/icons/arrow-up.svg"
                    tint: Theme.amber
                    s: 9
                }
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
                font.pixelSize: 9
                font.weight: Font.Bold
                font.capitalization: Font.AllUppercase
                font.letterSpacing: 0.5
                font.family: Theme.fontSans
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
