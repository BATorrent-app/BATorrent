// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Release notes, read from CHANGELOG.md (themeBridge.releaseNotes) into
// versions and sections rather than shown as raw Markdown: the versions on
// the left, the chosen one at a reading measure on the right.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: dlg
    anchors.fill: parent
    z: 200
    visible: opened || anim > 0

    property bool opened: false
    property real anim: 0
    Behavior on anim { NumberAnimation { duration: 150; easing.type: Theme.easeOut } }
    onOpenedChanged: {
        anim = opened ? 1 : 0
        if (opened) { DialogStack.push(dlg); if (versions.length === 0) parse(); pick(0) }
        else DialogStack.pop(dlg)
    }
    Component.onDestruction: DialogStack.pop(dlg)
    function open() { opened = true }
    function close() { opened = false }
    // a version to open on, "4.8" or "4.8.0"; the newest when not found
    function openAt(v) { if (versions.length === 0) parse(); opened = true; pick(Math.max(0, indexOf(v))) }

    property var versions: []          // [{v, name, date, sections: [{kind, items: [{title, body}]}]}]
    property int current: 0
    readonly property var cur: versions.length > current ? versions[current] : null

    function indexOf(v) {
        for (var i = 0; i < versions.length; ++i)
            if (versions[i].v === v || versions[i].v.indexOf(v + ".") === 0) return i
        return -1
    }
    function pick(i) { current = i; body.contentY = 0 }
    // the version before this app's own
    function previousOf(v) {
        if (versions.length === 0) parse()
        var ver = typeof themeBridge !== "undefined" ? themeBridge.appVersion : ""
        var i = indexOf(ver)
        return i >= 0 && i + 1 < versions.length ? versions[i + 1].v : (versions.length > 1 ? versions[1].v : "")
    }

    function plain(s) {
        return s.replace(/\[([^\]]+)\]\([^)]*\)/g, "$1").replace(/\*\*/g, "").replace(/`/g, "").replace(/\s+/g, " ").trim()
    }
    // the first sentence is the title, the rest is the body
    function split(text) {
        var t = plain(text)
        var m = /^(.+?[.!?])\s+(.+)$/.exec(t)
        if (m && m[1].length <= 90) return { title: m[1].replace(/\.$/, ""), body: m[2] }
        return { title: t.length <= 90 ? t.replace(/\.$/, "") : "", body: t.length <= 90 ? "" : t }
    }
    function parse() {
        var src = typeof themeBridge !== "undefined" ? themeBridge.releaseNotes() : ""
        var lines = src.split("\n")
        var out = [], ver = null, sec = null, item = null
        function flush() {
            if (item && sec) sec.items.push(split(item))
            item = null
        }
        for (var i = 0; i < lines.length; ++i) {
            var l = lines[i]
            var h = /^## v(\S+)(?:\s+"([^"]+)")?(?:\s+\((\d{4}-\d\d-\d\d)\))?/.exec(l)
            if (h) {
                flush()
                ver = h[1].indexOf("-") >= 0 ? null : { v: h[1], name: h[2] || "", date: h[3] || "", sections: [] }
                if (ver) out.push(ver)
                sec = null
                continue
            }
            if (!ver) continue
            var s = /^### (\w+)/.exec(l)
            if (s) { flush(); sec = { kind: s[1].toLowerCase(), items: [] }; ver.sections.push(sec); continue }
            if (!sec) continue
            if (/^\s*[-*] /.test(l)) { flush(); item = l.replace(/^\s*[-*] /, ""); continue }
            if (item !== null && /^\s+\S/.test(l)) { item += " " + l.trim(); continue }
            if (l.trim() === "" || l.trim() === "---") flush()
        }
        flush()
        versions = out
    }
    function fmtDate(d, full) {
        if (!d) return ""
        var p = d.split("-")
        var dt = new Date(+p[0], +p[1] - 1, +p[2])
        var loc = Qt.locale(["en_US", "pt_BR", "zh_CN", "ja_JP", "ru_RU", "es_ES", "de_DE", "uk_UA", "tr_TR"][i18n.language] || "en_US")
        return full ? dt.toLocaleDateString(loc, Locale.LongFormat).replace(/^[^,]*,\s*/, "")
                    : dt.toLocaleDateString(loc, "MMM yyyy")
    }
    function sectionLabel(k) {
        return k === "added" ? i18n.t("rn_added") : k === "changed" ? i18n.t("rn_changed")
             : k === "fixed" ? i18n.t("rn_fixed") : k === "security" ? i18n.t("rn_security") : k
    }

    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: dlg.opened && DialogStack.topItem === dlg
        onActivated: dlg.close()
    }

    Rectangle {
        anchors.fill: parent
        opacity: dlg.anim
        color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.7) : Qt.rgba(20/255, 20/255, 28/255, 0.4)
        MouseArea { anchors.fill: parent; onClicked: dlg.close(); onWheel: function (w) { w.accepted = true } }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(900, dlg.width - 32)
        height: Math.min(620, dlg.height - 48)
        opacity: dlg.anim
        scale: Theme.reduceMotion ? 1 : (0.97 + 0.03 * dlg.anim)
        radius: Theme.rad(12)
        color: Theme.elev
        border.color: Theme.hair; border.width: 1
        clip: true
        MouseArea { anchors.fill: parent; onWheel: function (w) { w.accepted = true } }

        ColumnLayout {
            anchors.fill: parent
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                Layout.leftMargin: 20; Layout.rightMargin: 12
                Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("release_notes_title2")); color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                Rectangle {
                    width: 30; height: 30; radius: Theme.rad(6)
                    color: xMa.containsMouse ? Theme.hover : "transparent"
                    IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: 14; tint: xMa.containsMouse ? Theme.t1 : Theme.t3 }
                    MouseArea { id: xMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlg.close() }
                }
            }
            Rectangle { Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hair }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 0

                // versions
                ListView {
                    id: verList
                    Layout.preferredWidth: 150
                    Layout.fillHeight: true
                    Layout.margins: 8
                    clip: true
                    model: dlg.versions
                    spacing: 2
                    boundsBehavior: Flickable.StopAtBounds
                    delegate: Rectangle {
                        readonly property bool on: dlg.current === index
                        width: ListView.view.width
                        height: 44
                        radius: Theme.rad(6)
                        color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.08) : (vMa.containsMouse ? Theme.hover : "transparent")
                        Rectangle { visible: parent.on; width: 2; height: parent.height; color: Theme.t1 }
                        Column {
                            x: 12; anchors.verticalCenter: parent.verticalCenter
                            spacing: 1
                            Text { text: modelData.v.replace(/\.0$/, ""); color: parent.parent.on ? Theme.t1 : Theme.t2; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum }
                            Text { visible: modelData.date !== ""; text: (i18n.language, dlg.fmtDate(modelData.date, false)); color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans }
                        }
                        MouseArea { id: vMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: dlg.pick(index) }
                    }
                }
                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: Theme.hair }

                // the chosen version
                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Flickable {
                        id: body
                        anchors.fill: parent
                        clip: true
                        contentWidth: width
                        contentHeight: notes.implicitHeight + 56
                        boundsBehavior: Flickable.StopAtBounds
                        WheelScroller { flick: body }
                        ColumnLayout {
                            id: notes
                            x: 28; y: 26
                            width: Math.min(600, body.width - 56)
                            spacing: 6
                            Row {
                                spacing: 10
                                Text { text: dlg.cur ? dlg.cur.v : ""; color: Theme.t1; font.pixelSize: 24; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum }
                                Text {
                                    anchors.baseline: parent.children[0].baseline
                                    text: (i18n.language, dlg.cur ? [dlg.cur.name ? "“" + dlg.cur.name + "”" : "", dlg.fmtDate(dlg.cur.date, true)].filter(function (s) { return s !== "" }).join(" · ") : "")
                                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                                }
                            }
                            Repeater {
                                model: dlg.cur ? dlg.cur.sections : []
                                delegate: ColumnLayout {
                                    id: secCol
                                    required property var modelData
                                    Layout.fillWidth: true
                                    Layout.topMargin: 14
                                    spacing: 10
                                    Row {
                                        spacing: 7
                                        Rectangle {
                                            anchors.verticalCenter: parent.verticalCenter
                                            width: 6; height: 6; radius: 3
                                            color: secCol.modelData.kind === "added" ? Theme.grn
                                                 : secCol.modelData.kind === "security" ? Theme.warn : Theme.t3
                                        }
                                        Text {
                                            text: (i18n.language, dlg.sectionLabel(secCol.modelData.kind))
                                            color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9
                                            font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
                                        }
                                    }
                                    Repeater {
                                        model: secCol.modelData.items
                                        delegate: ColumnLayout {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            spacing: 3
                                            Text {
                                                Layout.fillWidth: true
                                                visible: modelData.title !== ""
                                                text: modelData.title
                                                color: Theme.t1; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans
                                                wrapMode: Text.WordWrap
                                            }
                                            Text {
                                                Layout.fillWidth: true
                                                visible: modelData.body !== ""
                                                text: modelData.body
                                                color: Theme.t2; font.pixelSize: 14; font.family: Theme.fontSans
                                                wrapMode: Text.WordWrap; lineHeight: 1.5
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    Rectangle {
                        anchors.bottom: parent.bottom
                        width: parent.width; height: 40
                        visible: body.contentHeight > body.height && !body.atYEnd
                        gradient: Gradient {
                            GradientStop { position: 0; color: "transparent" }
                            GradientStop { position: 1; color: Theme.elev }
                        }
                    }
                }
            }
        }
    }
}
