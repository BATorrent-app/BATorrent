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
import "../widgets/TimeAgo.js" as TimeAgo

ModalShell {
    id: dlg
    cardWidth: 900
    cardHeight: 620
    onOpenedChanged: if (opened) { if (versions.length === 0) parse(); pick(0) }
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
    function fmtDate(d, full) { return TimeAgo.date(d, i18n.localeName, full) }
    function sectionLabel(k) {
        return k === "added" ? i18n.t("rn_added") : k === "changed" ? i18n.t("rn_changed")
             : k === "fixed" ? i18n.t("rn_fixed") : k === "security" ? i18n.t("rn_security") : k
    }


    ColumnLayout {
        anchors.fill: parent
        spacing: 0
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            Layout.leftMargin: 20; Layout.rightMargin: 12
            Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("release_notes_title2")); color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            CloseButton {
                onClicked: dlg.dismiss()
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
                    color: on ? Theme.selectedFill : (vMa.containsMouse ? Theme.hover : "transparent")
                    Rectangle { visible: parent.on; width: 2; height: parent.height; color: Theme.t1 }
                    Column {
                        x: 12; anchors.verticalCenter: parent.verticalCenter
                        spacing: 1
                        Text { text: modelData.v.replace(/\.0$/, ""); color: parent.parent.on ? Theme.t1 : Theme.t2; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum }
                        Text { visible: modelData.date !== ""; text: (i18n.language, dlg.fmtDate(modelData.date, false)); color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans }
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
                                            color: Theme.t2; font.pixelSize: 15; font.family: Theme.fontSans
                                            wrapMode: Text.WordWrap; lineHeight: 1.6
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
