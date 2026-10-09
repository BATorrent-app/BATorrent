// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

// Drag-and-drop surface for .torrent / magnet / direct http links. Emits
// torrentUrlsDropped so the host can enqueue through its add-dialog queue (no
// sibling-id walks); http links go straight to the download engine.
Item {
    id: root
    anchors.fill: parent
    z: 150

    signal torrentUrlsDropped(var urls)

    DropArea {
        id: dropZone
        anchors.fill: parent
        // What we accept is exactly what we can add, asked of the same
        // function that will do the adding. Two lists drift: this one used to
        // take anything ending in .torrent and then hand local files to a
        // different path than the one that vetted them.
        function accepts(drag) {
            if (typeof session === "undefined") return false
            if (drag.hasUrls) {
                for (var i = 0; i < drag.urls.length; ++i)
                    if (session.inputKind(drag.urls[i].toString()) !== "") return true
            }
            return drag.hasText && session.inputKind(drag.text) !== ""
        }
        onEntered: function(drag) { drag.accepted = accepts(drag) }
        onDropped: function(drop) {
            if (typeof session === "undefined") return
            // session.addAnything works out magnet vs .torrent vs an http
            // .torrent vs an ordinary download. This used to decide for
            // itself, and knew things the command line did not.
            var torrentFiles = []
            var handled = false
            if (drop.hasUrls) {
                for (var i = 0; i < drop.urls.length; ++i) {
                    var u = drop.urls[i].toString()
                    if (session.inputKind(u) === "torrentFile") { torrentFiles.push(u); handled = true }
                    else if (session.addAnything(u)) handled = true
                }
            }
            if (torrentFiles.length > 0) root.torrentUrlsDropped(torrentFiles)
            if (!handled && drop.hasText) session.addAnything(drop.text)
        }
    }

    Rectangle {
        anchors.fill: parent
        z: 1
        color: Qt.rgba(0, 0, 0, 0.65)
        visible: opacity > 0.01
        opacity: dropZone.containsDrag ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 150; easing.type: Theme.easeOut } }
        Rectangle {
            anchors.centerIn: parent
            width: 360; height: 200; radius: 16
            color: Theme.panel
            border.color: Theme.accent
            border.width: 2
            scale: dropZone.containsDrag ? 1.0 : 0.95
            Behavior on scale { NumberAnimation { duration: Theme.durBase; easing.type: Easing.OutBack } }
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
