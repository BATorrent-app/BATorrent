// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

// Drag-and-drop surface for .torrent / magnet. Emits torrentUrlsDropped so the
// host can enqueue through its add-dialog queue (no sibling-id walks).
Item {
    id: root
    anchors.fill: parent
    z: 150

    signal torrentUrlsDropped(var urls)

    DropArea {
        id: dropZone
        anchors.fill: parent
        function isMagnetLike(s) {
            var u = s.toLowerCase()
            return u.indexOf("magnet:") === 0 || u.indexOf("bittorrent:") === 0
        }
        function accepts(drag) {
            if (drag.hasUrls) {
                for (var i = 0; i < drag.urls.length; ++i) {
                    var u = drag.urls[i].toString().toLowerCase()
                    if (u.endsWith(".torrent") || dropZone.isMagnetLike(u)) return true
                }
            }
            if (drag.hasText && dropZone.isMagnetLike(drag.text)) return true
            return false
        }
        onEntered: function(drag) { drag.accepted = accepts(drag) }
        onDropped: function(drop) {
            if (typeof session === "undefined") return
            var torrentUrls = []
            var addedMagnet = false
            if (drop.hasUrls) {
                for (var i = 0; i < drop.urls.length; ++i) {
                    var u = drop.urls[i].toString()
                    if (dropZone.isMagnetLike(u)) { session.addMagnetUri(u); addedMagnet = true }
                    else torrentUrls.push(u)
                }
            }
            if (torrentUrls.length > 0) root.torrentUrlsDropped(torrentUrls)
            if (!addedMagnet && drop.hasText && dropZone.isMagnetLike(drop.text)) session.addMagnetUri(drop.text)
            drop.accept()
        }
    }

    Rectangle {
        anchors.fill: parent
        z: 1
        color: Qt.rgba(0, 0, 0, 0.65)
        visible: opacity > 0.01
        opacity: dropZone.containsDrag ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 150; easing.type: Easing.OutCubic } }
        Rectangle {
            anchors.centerIn: parent
            width: 360; height: 200; radius: 16
            color: Theme.panel
            border.color: Theme.accent
            border.width: 2
            scale: dropZone.containsDrag ? 1.0 : 0.95
            Behavior on scale { NumberAnimation { duration: 180; easing.type: Easing.OutBack } }
            ColumnLayout {
                anchors.centerIn: parent
                spacing: 12
                IconImg { Layout.alignment: Qt.AlignHCenter; src: "qrc:/icons/magnet.svg"; tint: Theme.accentText; s: 52 }
                Text { Layout.alignment: Qt.AlignHCenter; text: (i18n.language, i18n.t("dnd_drop_title")); color: Theme.t1; font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans }
                Text { Layout.alignment: Qt.AlignHCenter; text: (i18n.language, i18n.t("dnd_drop_sub")); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
            }
        }
    }
}
