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
        // what is coming in, said before it lands: how many, and their names
        property int count: 0
        property string names: ""
        property bool torrents: true
        onEntered: function(drag) {
            drag.accepted = accepts(drag)
            var list = []
            var allTorrents = true
            if (drag.hasUrls) {
                for (var i = 0; i < drag.urls.length; ++i) {
                    var u = drag.urls[i].toString()
                    if (typeof session !== "undefined" && session.inputKind(u) !== "torrentFile") allTorrents = false
                    list.push(decodeURIComponent(u.split("/").pop()))
                }
            } else allTorrents = false
            count = Math.max(list.length, drag.hasText ? 1 : 0)
            names = list.slice(0, 3).join(", ") + (list.length > 3 ? ", …" : "")
            torrents = allTorrents && list.length > 0
        }
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

    // the whole window takes it: a dashed edge all round, the count in the middle
    Rectangle {
        anchors.fill: parent
        z: 1
        color: Theme.scrim
        visible: opacity > 0.01
        opacity: dropZone.containsDrag ? 1 : 0
        Behavior on opacity { OpacityAnimator { duration: 150; easing.type: Theme.easeOut } }
        Canvas {
            id: edge
            anchors.fill: parent
            anchors.margins: 14
            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()
            onPaint: {
                var ctx = getContext("2d")
                ctx.reset()
                ctx.strokeStyle = String(Theme.t1)
                ctx.lineWidth = 2
                ctx.setLineDash([6, 5])
                var r = Theme.rad(12)
                ctx.beginPath()
                ctx.roundedRect(1, 1, width - 2, height - 2, r, r)
                ctx.stroke()
            }
        }
        Column {
            anchors.centerIn: parent
            width: Math.min(parent.width - 80, 720)
            spacing: 10
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                text: (i18n.language, i18n.t("dnd_drop_here"))
                color: Theme.t1
                font.pixelSize: 28; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
            Text {
                width: parent.width
                horizontalAlignment: Text.AlignHCenter
                textFormat: Text.PlainText
                elide: Text.ElideMiddle
                visible: dropZone.count > 0
                text: {
                    i18n.language
                    var head = dropZone.torrents
                               ? i18n.t(dropZone.count === 1 ? "dnd_n_torrents_1" : "dnd_n_torrents").arg(dropZone.count)
                               : i18n.t(dropZone.count === 1 ? "dnd_n_items_1" : "dnd_n_items").arg(dropZone.count)
                    return dropZone.names.length > 0 ? head + " · " + dropZone.names : head
                }
                color: Theme.t2
                font.pixelSize: 15; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }
    }
}
