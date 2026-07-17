// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// A single HUB poster card (movie/show or game). Emits play/context/showDetail
// and leaves the parent to wire them; game-state formatting comes from `host`
// (the HubView, which owns gameState*/cardStatus/fmtTime). Split out of
// HubView.qml verbatim so the view stays under the QML size ceiling.

import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Controls.Basic
import "theme"
import "widgets"

Item {
    id: card
    property var host                       // the HubView: gameState*/cardStatus/fmtTime helpers
    property var item
    property int cardW: 150
    property bool requireDoubleClick: false
    property bool isGame: false
    readonly property int cardH: Math.round(cardW * 1.5)
    signal play()
    signal context()
    signal showDetail()
    implicitWidth: cardW
    implicitHeight: cardH + 38

    Item {
        id: art
        width: card.cardW; height: card.cardH
        scale: ma.containsMouse ? 1.04 : 1.0
        Behavior on scale { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

        Rectangle {
            anchors.fill: parent; radius: 10; color: "#161618"; clip: true
            border.color: ma.containsMouse ? Theme.accent : Theme.hair; border.width: 1
            opacity: img.status === Image.Ready ? 0 : 1
            Behavior on opacity { NumberAnimation { duration: 250 } }
            Behavior on border.color { ColorAnimation { duration: 140 } }
            Image {
                anchors.centerIn: parent; width: parent.width * 0.4; height: width
                source: "qrc:/images/logo.svg"; sourceSize: Qt.size(width * 2, width * 2)
                fillMode: Image.PreserveAspectFit; opacity: 0.3
            }
        }
        Image {
            id: img; anchors.fill: parent
            source: card.item.poster || ""
            fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true
            visible: false; sourceSize: Qt.size(card.cardW * 2, card.cardH * 2)
        }
        Rectangle { id: mask; anchors.fill: parent; radius: 10; visible: false; layer.enabled: true }
        MultiEffect {
            anchors.fill: parent; source: img; maskEnabled: true; maskSource: mask
            opacity: img.status === Image.Ready ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 300; easing.type: Easing.OutCubic } }
        }

        // hover play overlay (movies: glass disc, same language as the grid
        // tiles / Find cards; games use the state button below)
        Rectangle {
            anchors.fill: parent; radius: 10; color: "#66000000"
            opacity: ma.containsMouse ? 1 : 0
            Behavior on opacity { NumberAnimation { duration: 140 } }
            Rectangle {
                visible: !card.isGame
                anchors.centerIn: parent
                width: 46; height: 46; radius: 23
                color: "#cc101014"
                border.color: Qt.rgba(1, 1, 1, 0.25); border.width: 1
                IconImg {
                    anchors.centerIn: parent; anchors.horizontalCenterOffset: 1
                    src: "qrc:/icons/play.svg"; tint: "#ffffff"; s: 18
                }
            }
        }

        // games: state-driven primary action (Install / Extracting / Play / …).
        // The label IS the differentiation — never a blind double-click.
        Rectangle {
            id: stateBtn
            visible: card.isGame && card.item.installState !== 0 && card.item.installState !== 5
            anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
            anchors.margins: 6
            height: 26; radius: 7
            readonly property bool actionable: card.host.gameStateActionable(card.item)
            color: actionable ? (sbma.containsMouse ? Qt.lighter(Theme.accent, 1.12) : Theme.accent)
                              : "#cc1b1b1f"
            border.color: actionable ? "transparent" : Theme.hair
            border.width: actionable ? 0 : 1
            opacity: (ma.containsMouse || actionable) ? 1 : 0.92
            Behavior on opacity { NumberAnimation { duration: 140 } }
            Row {
                anchors.centerIn: parent; spacing: 5
                Spinner {
                    visible: card.item.installState === 2 || card.item.installState === 3
                    s: 13; tint: "#c7c7cc"
                    anchors.verticalCenter: parent.verticalCenter
                }
                Text {
                    text: card.host.gameStateLabel(card.item)
                    // fixed light colors: the button sits on the poster/dark chip in
                    // BOTH themes, and accentText is accent-ON-dark, not text-on-accent
                    color: stateBtn.actionable ? "#ffffff" : "#c7c7cc"
                    font.pixelSize: 11; font.weight: Font.Bold; font.family: Theme.fontSans
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            MouseArea {
                id: sbma; anchors.fill: parent
                hoverEnabled: true
                cursorShape: stateBtn.actionable ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: card.host.gamePrimary(card.item)
            }
        }

        // "playing now" badge
        Rectangle {
            visible: card.item.playing === true
            anchors.top: parent.top; anchors.right: parent.right; anchors.margins: 6
            radius: 6; color: Theme.accent
            implicitWidth: pnl.width + 12; implicitHeight: 18
            Text {
                id: pnl; anchors.centerIn: parent
                text: (i18n.language, i18n.t("hub_playing_now"))
                color: "#ffffff"; font.pixelSize: 9; font.weight: Font.Bold; font.family: Theme.fontSans
            }
        }

        // status pill: Installed (green) / ↓X% (downloading)
        Rectangle {
            anchors.top: parent.top; anchors.left: parent.left; anchors.margins: 6
            radius: 6; color: "#cc000000"
            implicitWidth: dlRow.width + 12; implicitHeight: 18
            Row {
                id: dlRow; anchors.centerIn: parent; spacing: 5
                Rectangle { visible: card.item.completed; width: 6; height: 6; radius: 3; color: Theme.grn; anchors.verticalCenter: parent.verticalCenter }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: card.item.completed ? i18n.t("hub_installed") : ("↓ " + Math.floor((card.item.progress || 0) * 100) + "%")
                    color: card.item.completed ? Theme.grn : Theme.accent
                    font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
                }
            }
        }

        // elapsed / total time (resume)
        Rectangle {
            visible: (card.item.watchedPct || 0) > 0 && (card.item.durMs || 0) > 0
            anchors.bottom: parent.bottom; anchors.right: parent.right
            anchors.bottomMargin: 12; anchors.rightMargin: 6
            radius: 5; color: "#cc000000"
            implicitWidth: tlabel.width + 12; implicitHeight: 17
            Text {
                id: tlabel; anchors.centerIn: parent
                text: card.host.fmtTime(card.item.resumeMs) + " / " + card.host.fmtTime(card.item.durMs)
                color: "#e8e8ec"; font.pixelSize: 9; font.weight: Font.DemiBold; font.family: Theme.fontMono
            }
        }

        // watched-% bar
        Rectangle {
            visible: (card.item.watchedPct || 0) > 0
            anchors.bottom: parent.bottom; anchors.left: parent.left; anchors.right: parent.right
            anchors.margins: 5
            height: 4; radius: 2; color: "#99000000"
            Rectangle {
                anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
                width: parent.width * Math.min(1, card.item.watchedPct)
                radius: 2; color: Theme.accent
            }
        }
    }

    Text {
        id: titleLabel
        anchors.top: art.bottom; anchors.topMargin: 8
        width: card.cardW
        text: card.item.title || ""
        color: ma.containsMouse ? Theme.t1 : Theme.t2
        font.pixelSize: 12; font.weight: Font.Medium; font.family: Theme.fontSans
        elide: Text.ElideRight; maximumLineCount: 1
        Behavior on color { ColorAnimation { duration: 140 } }
    }
    Text {
        anchors.top: titleLabel.bottom; anchors.topMargin: 2
        width: card.cardW
        visible: text.length > 0
        text: (i18n.language, card.host.cardStatus(card.item, card.isGame))
        color: card.item.playing === true ? Theme.accent : Theme.t4
        font.pixelSize: 10; font.family: Theme.fontSans
        elide: Text.ElideRight
    }

    MouseArea {
        id: ma; anchors.fill: parent
        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        // single click → detail drawer; double click → quick-play
        onClicked: function (mouse) {
            if (mouse.button === Qt.RightButton) { card.context(); return }
            card.showDetail()
        }
        onDoubleClicked: card.play()
    }

    // rich hover card: genres + description (games carry IGDB metadata)
    ToolTip {
        id: richTip
        parent: card
        visible: ma.containsMouse && (card.item.description || "").length > 0
        delay: 600
        width: 300
        padding: 12
        x: card.cardW + 10
        y: (card.cardH - height) / 2
        contentItem: ColumnLayout {
            spacing: 6
            Text {
                visible: text.length > 0
                text: ((card.item.genres && card.item.genres.length > 0) ? card.item.genres.join("  ·  ") : "")
                      + (((card.item.rating || 0) > 0) ? ((card.item.genres && card.item.genres.length > 0 ? "   " : "") + "★ " + Number(card.item.rating).toFixed(1)) : "")
                color: Theme.t4; font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
                Layout.maximumWidth: 276; wrapMode: Text.WordWrap
            }
            Text {
                text: card.item.description || ""
                Layout.maximumWidth: 276
                wrapMode: Text.WordWrap; maximumLineCount: 8; elide: Text.ElideRight
                color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontSans; lineHeight: 1.35
            }
        }
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 9 }
    }
}
