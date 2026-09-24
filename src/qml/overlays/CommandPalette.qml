// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Ctrl/⌘+K palette: fuzzy-find actions and torrents from one box. The field
// has focus on the first frame and typing works during the open animation,
// so the animation adds no latency.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: pal
    anchors.fill: parent
    z: 300
    visible: opened || shown > 0
    property bool opened: false
    // 0..1 presence of the whole overlay; drives scrim and card together
    property real shown: opened ? 1 : 0
    Behavior on shown {
        NumberAnimation {
            duration: pal.opened ? Theme.durBase : Theme.durExit
            easing.type: pal.opened ? Theme.easeOut : Theme.easeIn
        }
    }
    // rows cascade in only right after opening, never while you type
    property bool entering: false
    Timer { id: enterTimer; interval: 260; onTriggered: pal.entering = false }

    // [{label, hint, run}] supplied by Main: keeps every action next to the
    // code that owns it instead of duplicating ids here
    property var actions: []
    property var torrents: []
    property int sel: 0

    // result row activated for a torrent: select + reveal it in Downloads
    signal torrentPicked(int sourceIndex)

    function open() {
        torrents = (typeof session !== "undefined") ? session.torrentPalette() : []
        input.text = ""
        sel = 0
        entering = !Theme.reduceMotion
        enterTimer.restart()
        opened = true
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
        opacity: pal.shown
        color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.45) : Qt.rgba(20/255, 20/255, 28/255, 0.28)
        MouseArea { anchors.fill: parent; onClicked: pal.close()
            onWheel: function(wheel) { wheel.accepted = true } }
    }

    Rectangle {
        id: card
        anchors.horizontalCenter: parent.horizontalCenter
        y: Math.round(parent.height * 0.14) - Theme.travel(10) * (1 - pal.shown)
        width: Math.min(parent.width - 120, 580)
        height: list.visible ? input.height + list.contentHeight + 22 : input.height + 14
        Behavior on height {
            enabled: pal.opened && pal.shown === 1
            NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut }
        }
        clip: true
        opacity: pal.shown
        scale: Theme.grow(0.965) + (1 - Theme.grow(0.965)) * pal.shown
        transformOrigin: Item.Top
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
            // one highlight that slides between rows instead of each row
            // switching its own colour
            highlightFollowsCurrentItem: false
            currentIndex: pal.sel
            highlight: Rectangle {
                width: list.width
                height: 36
                radius: 7
                color: Theme.sel
                y: list.currentItem ? list.currentItem.y : 0
                Behavior on y {
                    enabled: !Theme.reduceMotion
                    NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut }
                }
            }
            delegate: Rectangle {
                id: row
                required property var modelData
                required property int index
                width: ListView.view.width
                height: 36
                radius: 7
                color: index !== pal.sel && rowMa.containsMouse ? Theme.hover : "transparent"
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
                // opening cascade: each row a little later than the one above
                transform: Translate { id: rowShift }
                // rows outlive the palette being closed, so the cascade is
                // started by the open itself, not by the row being created
                Component.onCompleted: if (pal.entering) rowIn.restart()
                Connections {
                    target: pal
                    function onEnteringChanged() { if (pal.entering) rowIn.restart() }
                }
                SequentialAnimation {
                    id: rowIn
                    PropertyAction { target: row; property: "opacity"; value: 0 }
                    PropertyAction { target: rowShift; property: "y"; value: 6 }
                    PauseAnimation { duration: Math.min(row.index, 8) * Theme.stagger }
                    ParallelAnimation {
                        NumberAnimation { target: row; property: "opacity"; from: 0; to: 1
                            duration: Theme.durBase; easing.type: Theme.easeOut }
                        NumberAnimation { target: rowShift; property: "y"; from: 6; to: 0
                            duration: Theme.durSlow; easing.type: Theme.easeOut }
                    }
                }
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
