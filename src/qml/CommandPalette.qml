// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Ctrl/⌘+K palette: fuzzy-find actions and torrents from one box. Opens with
// no animation on purpose — it's a frequency-100x feature, motion would only
// add latency.
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: pal
    anchors.fill: parent
    z: 300
    visible: opened
    property bool opened: false

    // [{label, hint, run}] supplied by Main — keeps every action next to the
    // code that owns it instead of duplicating ids here
    property var actions: []
    property var torrents: []
    property int sel: 0

    // result row activated for a torrent: select + reveal it in Downloads
    signal torrentPicked(int sourceIndex)

    function open() {
        torrents = (typeof session !== "undefined") ? session.torrentPalette() : []
        opened = true
        input.text = ""
        sel = 0
        Qt.callLater(function() { input.field.forceActiveFocus() })
    }
    function close() { opened = false }
    function toggle() { if (opened) close(); else open() }

    // substring beats subsequence; earlier matches beat later ones
    function score(label, q) {
        if (q.length === 0) return 1
        var l = label.toLowerCase()
        var idx = l.indexOf(q)
        if (idx >= 0) return 1000 - idx
        var qi = 0
        for (var i = 0; i < l.length && qi < q.length; ++i)
            if (l[i] === q[qi]) ++qi
        return qi === q.length ? 100 : -1
    }

    readonly property var results: {
        var q = input.text.trim().toLowerCase()
        var out = []
        for (var a = 0; a < actions.length; ++a) {
            var sA = score(actions[a].label, q)
            if (sA >= 0) out.push({ type: "action", label: actions[a].label, hint: actions[a].hint || "", run: actions[a].run,
                                    s: sA + 1 + (actions.length - a) * 0.001 })   // definition order breaks ties
        }
        for (var t = 0; t < torrents.length; ++t) {
            var sT = score(torrents[t].name, q)
            if (sT >= 0) out.push({ type: "torrent", label: torrents[t].name, hint: "", source: torrents[t].source, s: sT })
        }
        out.sort(function (x, y) { return y.s - x.s })
        return out.slice(0, 12)
    }
    onResultsChanged: if (sel >= results.length) sel = Math.max(0, results.length - 1)

    function activate(idx) {
        if (idx < 0 || idx >= results.length) return
        var r = results[idx]
        close()
        if (r.type === "action") r.run()
        else pal.torrentPicked(r.source)
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.45) : Qt.rgba(20/255, 20/255, 28/255, 0.28)
        MouseArea { anchors.fill: parent; onClicked: pal.close()
            onWheel: function(wheel) { wheel.accepted = true } }
    }

    Rectangle {
        id: card
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.round(parent.height * 0.14)
        width: Math.min(parent.width - 120, 580)
        height: list.visible ? input.height + list.contentHeight + 22 : input.height + 14
        radius: 13
        color: Theme.bg
        border.color: Theme.isDark ? Qt.rgba(1, 1, 1, 0.09) : Qt.rgba(0, 0, 0, 0.14)
        border.width: 1
        MouseArea { anchors.fill: parent; onWheel: function(wheel) { wheel.accepted = true } }

        TFld {
            id: input
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 7
            implicitHeight: 40
            icon: "qrc:/icons/search.svg"
            placeholder: (i18n.language, i18n.t("palette_placeholder"))
            Keys.onDownPressed: pal.sel = Math.min(pal.results.length - 1, pal.sel + 1)
            Keys.onUpPressed: pal.sel = Math.max(0, pal.sel - 1)
            Keys.onEscapePressed: pal.close()
            // the inner TextField consumes Enter → activate via its accepted signal
            Connections {
                target: input.field
                function onAccepted() { pal.activate(pal.sel) }
            }
        }

        ListView {
            id: list
            visible: pal.results.length > 0
            anchors.top: input.bottom
            anchors.topMargin: 6
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 7
            height: contentHeight
            interactive: false
            model: pal.results
            delegate: Rectangle {
                required property var modelData
                required property int index
                width: ListView.view.width
                height: 36
                radius: 7
                color: index === pal.sel ? Theme.sel : (rowMa.containsMouse ? Theme.hover : "transparent")
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10
                    IconImg {
                        src: modelData.type === "action" ? "qrc:/icons/chevron.svg" : "qrc:/icons/download.svg"
                        rotation: modelData.type === "action" ? -90 : 0
                        tint: index === pal.sel ? Theme.t1 : Theme.t4
                        s: 13
                    }
                    Text {
                        Layout.fillWidth: true
                        text: modelData.label
                        color: index === pal.sel ? Theme.t1 : Theme.t2
                        font.pixelSize: 13
                        font.family: Theme.fontSans
                        elide: Text.ElideMiddle
                    }
                    Text {
                        visible: modelData.hint.length > 0
                        text: modelData.hint
                        color: Theme.t4
                        font.pixelSize: 11
                        font.family: Theme.fontSans
                    }
                }
                MouseArea {
                    id: rowMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: pal.activate(index)
                }
            }
        }
    }
}
