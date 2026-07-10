// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Slide-in detail drawer for a clicked library card (movie or game): cover,
// meta lines, play/open/set-exe actions, synopsis. `hub` is the owning
// HubView (detail state + the game-state helpers).
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: root
    property var hub

    Rectangle {
        anchors.fill: parent
        visible: root.hub.detailOpen || detailDrawer.x < root.hub.width
        color: "#88000000"
        opacity: root.hub.detailOpen ? 1 : 0
        Behavior on opacity { NumberAnimation { duration: 150 } }
        MouseArea { anchors.fill: parent; onClicked: root.hub.detailOpen = false }
    }
    Rectangle {
        id: detailDrawer
        width: Math.min(420, root.hub.width)
        height: parent.height
        x: root.hub.detailOpen ? parent.width - width : parent.width
        color: Theme.elev
        Behavior on x { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        Rectangle { anchors.left: parent.left; width: 1; height: parent.height; color: Theme.hair }
        MouseArea { anchors.fill: parent }   // swallow clicks so the scrim doesn't close it

        readonly property var it: root.hub.detailItem
        readonly property bool isGame: root.hub.detailIsGame

        Flickable {
            id: detailFlick
            anchors.fill: parent
            anchors.margins: 24
            contentHeight: dcol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            WheelScroller { flick: detailFlick }

            ColumnLayout {
                id: dcol
                width: parent.width
                spacing: 16
                visible: detailDrawer.it !== null

                // close
                Item {
                    Layout.fillWidth: true; Layout.preferredHeight: 28
                    Rectangle {
                        anchors.right: parent.right; width: 28; height: 28; radius: 14
                        color: dClose.containsMouse ? Theme.hover : "transparent"
                        Text { anchors.centerIn: parent; text: "✕"; color: Theme.t2; font.pixelSize: 14 }
                        MouseArea { id: dClose; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.hub.detailOpen = false }
                    }
                }

                // cover (rounded mask, no shadow)
                Item {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 168; Layout.preferredHeight: 224
                    Rectangle { id: dPC; anchors.fill: parent; color: "#161618"; visible: false; layer.enabled: true
                        Image { anchors.fill: parent; source: detailDrawer.it ? (detailDrawer.it.poster || "") : ""; fillMode: Image.PreserveAspectCrop; asynchronous: true; cache: true } }
                    Rectangle { id: dPM; anchors.fill: parent; radius: 14; color: "white"; visible: false; layer.enabled: true }
                    MultiEffect { source: dPC; anchors.fill: parent; maskEnabled: true; maskSource: dPM }
                    Rectangle { anchors.fill: parent; radius: 14; color: "transparent"; border.color: "#33ffffff"; border.width: 1 }
                }

                Text {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignHCenter
                    text: detailDrawer.it ? (detailDrawer.it.title || "") : ""
                    color: Theme.t1; font.pixelSize: 20; font.weight: Font.Bold; font.family: Theme.fontSans
                    horizontalAlignment: Text.AlignHCenter; wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignHCenter
                    visible: text.length > 0
                    color: Theme.t3; font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight
                    text: {
                        var it = detailDrawer.it; if (!it) return ""
                        var parts = []
                        if ((it.year || "").length > 0) parts.push(it.year)
                        if (it.genres && it.genres.length > 0) parts.push(it.genres.slice(0, 2).join(" · "))
                        if ((it.rating || 0) > 0) parts.push("★ " + Number(it.rating).toFixed(1))
                        return parts.join("   ·   ")
                    }
                }
                Text {
                    Layout.fillWidth: true; Layout.alignment: Qt.AlignHCenter
                    visible: text.length > 0
                    color: detailDrawer.it && detailDrawer.it.playing ? Theme.accent : Theme.t4
                    font.pixelSize: 12; font.family: Theme.fontMono; horizontalAlignment: Text.AlignHCenter
                    text: {
                        var it = detailDrawer.it; if (!it) return ""
                        if (detailDrawer.isGame) return root.hub.cardStatus(it, true)
                        var p = []
                        if ((it.watchedPct || 0) > 0) p.push(Math.round(it.watchedPct * 100) + "% " + i18n.t("hub_watched"))
                        if ((it.durMs || 0) > 0) p.push(root.hub.fmtTime(it.durMs))
                        if ((it.size || 0) > 0) p.push(root.hub.fmtSize(it.size))
                        return p.join("   ·   ")
                    }
                }

                // actions
                BtnFlat {
                    Layout.fillWidth: true; Layout.topMargin: 4
                    primary: true; icon: "qrc:/icons/play.svg"
                    text: detailDrawer.isGame ? (root.hub.gameStateActionable(detailDrawer.it) ? root.hub.gameStateLabel(detailDrawer.it) : i18n.t("hub_gs_play"))
                          : ((detailDrawer.it && (detailDrawer.it.watchedPct || 0) > 0) ? i18n.t("hub_resume") : i18n.t("hub_gs_play"))
                    onClicked: {
                        if (!detailDrawer.it) return
                        if (detailDrawer.isGame) root.hub.gamePrimary(detailDrawer.it)
                        else if (root.hub.api) root.hub.api.playByHashFile(detailDrawer.it.infoHash, detailDrawer.it.fileIndex)
                        root.hub.detailOpen = false
                    }
                }
                BtnFlat {
                    Layout.fillWidth: true
                    icon: "qrc:/icons/open.svg"
                    text: (i18n.language, i18n.t("hub_open_folder"))
                    onClicked: if (root.hub.api && detailDrawer.it) {
                        var f = root.hub.api.gameFolder(detailDrawer.it.infoHash)
                        if (f && f.length > 0) Qt.openUrlExternally(root.hub.fileUrl(f))
                    }
                }
                BtnFlat {
                    Layout.fillWidth: true
                    visible: detailDrawer.isGame
                    icon: "qrc:/icons/settings.svg"
                    text: (i18n.language, i18n.t("hub_set_exe"))
                    onClicked: if (detailDrawer.it) { root.hub.openExePicker(detailDrawer.it.infoHash, false); root.hub.detailOpen = false }
                }

                Text {
                    Layout.fillWidth: true; Layout.topMargin: 4
                    visible: !!detailDrawer.it && (detailDrawer.it.description || detailDrawer.it.overview || "").length > 0
                    text: detailDrawer.it ? (detailDrawer.it.description || detailDrawer.it.overview || "") : ""
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                    wrapMode: Text.WordWrap; lineHeight: 1.35
                }
            }
        }
    }
}
