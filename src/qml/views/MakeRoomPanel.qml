// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Free up space. Everything on one disk, each item with the reason it could
// go (played, watched, paused for months, an episode a season pack already
// holds), a few suggestions that tick several at once, and a meter that says
// whether what is ticked is enough. Opened from the disk gauge, or with a
// target when something did not fit.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: root
    anchors.fill: parent
    z: 210
    visible: open || anim > 0

    property bool open: false
    // bytes still missing for what did not fit, 0 when just browsing
    property double targetBytes: 0
    property string targetName: ""     // what needs the room, when known
    property string targetPath: ""     // the folder it was going to; the default one when empty

    property real anim: 0
    Behavior on anim { NumberAnimation { duration: 150; easing.type: Theme.easeOut } }
    onOpenChanged: {
        anim = open ? 1 : 0
        if (open) {
            DialogStack.push(root)
            picked = ({})
            deleteFiles = true
            reload()
        } else {
            DialogStack.pop(root)
            targetName = ""
            targetPath = ""
        }
    }
    Component.onDestruction: DialogStack.pop(root)

    property var info: ({ disk: "", freeBytes: -1, rows: [] })
    function reload() { if (typeof session !== "undefined") info = session.freeUpSpace(targetPath) }
    readonly property var rows: info.rows || []
    readonly property double freeBytes: info.freeBytes === undefined ? -1 : info.freeBytes

    property var picked: ({})          // hash -> true
    property bool deleteFiles: true
    function isPicked(h) { return picked[h] === true }
    function setPicked(list, v) {
        var p = Object.assign({}, picked)
        for (var i = 0; i < list.length; ++i) { if (v) p[list[i]] = true; else delete p[list[i]] }
        picked = p
    }
    readonly property var pickedRows: rows.filter(function (r) { return root.picked[r.infoHash] === true })
    readonly property double pickedBytes: { var b = 0; for (var i = 0; i < pickedRows.length; ++i) b += pickedRows[i].sizeBytes; return b }
    // only deleting the files gives the room back
    readonly property double freedBytes: deleteFiles ? pickedBytes : 0

    property string sortKey: "size"
    readonly property var sortedRows: {
        var arr = rows.slice()
        if (sortKey === "age") arr.sort(function (a, b) { return (a.addedTime || 0) - (b.addedTime || 0) })
        else arr.sort(function (a, b) { return (b.sizeBytes || 0) - (a.sizeBytes || 0) })
        return arr
    }

    function fmt(b) { return typeof session !== "undefined" ? session.humanSize(b) : "" }
    // "3 days ago", "yesterday", "today", from unix seconds
    function ago(secs) {
        var days = Math.floor((Date.now() / 1000 - secs) / 86400)
        if (days <= 0) return i18n.t("freeup_today")
        if (days === 1) return i18n.t("freeup_yesterday")
        return i18n.t("freeup_days_ago").arg(days)
    }
    function epLabel(r) { var m = /S\d+E\d+/.exec(r.name); return m ? m[0] : r.name }
    // the one reason shown under a name
    function reason(r) {
        i18n.language
        if (r.inPack) return i18n.t("freeup_r_dup")
        if (r.lastPlayed > 0) return i18n.t("freeup_r_played").arg(ago(r.lastPlayed))
        if (r.watchedAt > 0) return i18n.t("freeup_r_watched").arg(ago(r.watchedAt))
        if (r.paused) {
            var pct = Math.floor((r.progress || 0) * 100)
            return r.pausedSince > 0 ? i18n.t("freeup_r_paused").arg(ago(r.pausedSince)).arg(pct)
                                     : i18n.t("freeup_r_paused_now").arg(pct)
        }
        if (r.completed)
            return r.upRate > 0 ? i18n.t("freeup_r_done_up").arg(fmt(r.upRate) + "/s") : i18n.t("freeup_r_done")
        return i18n.t("freeup_r_down").arg(Math.floor((r.progress || 0) * 100))
    }

    // ---- suggestions: only the ones that have something ----
    readonly property var suggestions: {
        i18n.language
        var dup = [], stale = [], used = []
        var now = Date.now() / 1000
        for (var i = 0; i < rows.length; ++i) {
            var r = rows[i]
            if (r.inPack) dup.push(r)
            else if (r.paused && r.pausedSince > 0 && now - r.pausedSince > 90 * 86400) stale.push(r)
            else if (r.completed && (r.lastPlayed > 0 || r.watchedAt > 0)) used.push(r)
        }
        function sum(l) { var b = 0; for (var j = 0; j < l.length; ++j) b += l[j].sizeBytes; return b }
        function names(l) { return l.slice(0, 2).map(function (x) { return x.name }).join(", ") + (l.length > 2 ? " +" + (l.length - 2) : "") }
        var out = []
        if (dup.length > 0) out.push({ key: "dup", title: i18n.t("freeup_s_dup"), rows: dup, bytes: sum(dup),
            desc: i18n.t("freeup_s_dup_d").arg(dup.map(function (x) { return root.epLabel(x) }).join(", ")) })
        if (stale.length > 0) out.push({ key: "stale", title: i18n.t("freeup_s_stale"), rows: stale, bytes: sum(stale),
            desc: stale.length === 1 ? i18n.t("freeup_s_stale_one").arg(stale[0].name).arg(Math.floor((now - stale[0].pausedSince) / 86400))
                                     : i18n.t("freeup_s_stale_d").arg(stale.length) })
        if (used.length > 0) out.push({ key: "used", title: i18n.t("freeup_s_used"), rows: used, bytes: sum(used), desc: names(used) })
        return out
    }

    function commit() {
        if (pickedRows.length === 0 || typeof session === "undefined") return
        session.removeTorrentsByHash(pickedRows.map(function (r) { return r.infoHash }), deleteFiles)
        open = false
    }

    Shortcut {
        sequences: [StandardKey.Cancel]
        enabled: root.open && DialogStack.topItem === root
        onActivated: root.open = false
    }
    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: root.open && DialogStack.topItem === root
        onActivated: root.commit()
    }

    Rectangle {
        anchors.fill: parent
        opacity: root.anim
        color: Theme.isDark ? Qt.rgba(0, 0, 0, 0.7) : Qt.rgba(20/255, 20/255, 28/255, 0.4)
        MouseArea { anchors.fill: parent; onClicked: root.open = false; onWheel: function (w) { w.accepted = true } }
    }

    Rectangle {
        id: card
        anchors.centerIn: parent
        width: Math.min(parent.width - 32, 600)
        height: Math.min(parent.height - 48, col.implicitHeight)
        opacity: root.anim
        scale: Theme.reduceMotion ? 1 : (0.97 + 0.03 * root.anim)
        radius: Theme.rad(12)
        color: Theme.elev
        border.color: Theme.hair
        border.width: 1
        clip: true
        MouseArea { anchors.fill: parent; onWheel: function (w) { w.accepted = true } }

        ColumnLayout {
            id: col
            anchors.fill: parent
            spacing: 0

            // ---- head + meter ----
            ColumnLayout {
                Layout.fillWidth: true
                Layout.margins: 22
                Layout.bottomMargin: 14
                spacing: 14
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Text { text: (i18n.language, i18n.t("freeup_title")); color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                        Text {
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                            color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                            text: {
                                i18n.language
                                var bits = []
                                if (root.info.disk) bits.push(root.info.disk)
                                if (root.freeBytes >= 0) bits.push(i18n.t("freeup_free").arg(root.fmt(root.freeBytes)))
                                if (root.targetBytes > 0 && root.targetName !== "")
                                    bits.push(i18n.t("freeup_needs").arg(root.targetName).arg(root.fmt(Math.max(0, root.freeBytes) + root.targetBytes)))
                                return bits.join(" · ")
                            }
                        }
                    }
                    Rectangle {
                        Layout.alignment: Qt.AlignTop
                        Layout.preferredWidth: 30; Layout.preferredHeight: 30
                        radius: Theme.rad(6)
                        color: xMa.containsMouse ? Theme.hover : "transparent"
                        IconImg { anchors.centerIn: parent; src: "qrc:/icons/close.svg"; s: 14; tint: xMa.containsMouse ? Theme.t1 : Theme.t3 }
                        MouseArea { id: xMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.open = false }
                    }
                }
                // grey = free now, then what is ticked: orange while short, green once it is enough
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    readonly property double free: Math.max(0, root.freeBytes)
                    readonly property double need: root.targetBytes > 0 ? free + root.targetBytes : 0
                    readonly property double scale: Math.max(need, free + root.freedBytes, 1)
                    readonly property bool enough: root.targetBytes <= 0 || root.freedBytes >= root.targetBytes
                    Item {
                        id: meter
                        Layout.fillWidth: true
                        Layout.preferredHeight: 8
                        Rectangle { anchors.fill: parent; radius: 4; color: Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.08) }
                        Rectangle {
                            width: meter.width * parent.free / parent.scale
                            height: parent.height; radius: 4
                            color: Theme.t4
                        }
                        Rectangle {
                            x: meter.width * parent.free / parent.scale
                            width: meter.width * root.freedBytes / parent.scale
                            height: parent.height; radius: 4
                            color: parent.enough ? Theme.grn : Theme.warn
                            Behavior on width { enabled: !Theme.reduceMotion; NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
                        }
                    }
                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: (i18n.language, i18n.t("freeup_selected")) + "  <b>" + root.fmt(root.pickedBytes) + "</b>"
                            textFormat: Text.StyledText
                            color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                        }
                        Item { Layout.fillWidth: true }
                        Text {
                            color: parent.parent.enough ? (root.targetBytes > 0 ? Theme.grn : Theme.t3) : Theme.warn
                            font.pixelSize: 13; font.weight: root.targetBytes > 0 ? Font.DemiBold : Font.Normal
                            font.family: Theme.fontSans; font.features: Theme.tnum
                            text: {
                                i18n.language
                                if (root.targetBytes <= 0)
                                    return i18n.t("freeup_left").arg(root.fmt(Math.max(0, root.freeBytes) + root.freedBytes))
                                if (root.freedBytes < root.targetBytes)
                                    return i18n.t("freeup_short").arg(root.fmt(root.targetBytes - root.freedBytes))
                                return i18n.t("freeup_fits").arg(root.fmt(root.freedBytes - root.targetBytes))
                            }
                        }
                    }
                }
            }

            // ---- suggestions ----
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 22; Layout.rightMargin: 22
                Layout.bottomMargin: 18
                visible: root.suggestions.length > 0
                spacing: 9
                Text { text: (i18n.language, i18n.t("freeup_suggestions")); color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Repeater {
                        model: root.suggestions
                        delegate: Rectangle {
                            id: sg
                            readonly property var hashes: modelData.rows.map(function (r) { return r.infoHash })
                            readonly property bool on: hashes.every(function (h) { return root.picked[h] === true })
                            Layout.fillWidth: true
                            Layout.preferredWidth: 1
                            Layout.alignment: Qt.AlignTop
                            implicitHeight: sgCol.implicitHeight + 22
                            radius: Theme.radCard(10)
                            color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.08) : (sgMa.containsMouse ? Theme.hover : "transparent")
                            border.width: on ? 1.5 : 1
                            border.color: on ? Theme.t1 : Theme.hair
                            ColumnLayout {
                                id: sgCol
                                x: 11; y: 11
                                width: parent.width - 22
                                spacing: 4
                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8
                                    Text { Layout.fillWidth: true; text: modelData.title; color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; wrapMode: Text.WordWrap }
                                    Text { Layout.alignment: Qt.AlignTop; text: root.fmt(modelData.bytes); color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum }
                                }
                                Text { Layout.fillWidth: true; text: modelData.desc; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontSans; wrapMode: Text.WordWrap; maximumLineCount: 3; elide: Text.ElideRight }
                            }
                            MouseArea { id: sgMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: root.setPicked(sg.hashes, !sg.on) }
                        }
                    }
                }
            }

            // ---- everything on this disk ----
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 22; Layout.rightMargin: 22
                Layout.bottomMargin: 8
                Text { Layout.fillWidth: true; text: (i18n.language, i18n.t("freeup_all")); color: Theme.t3; font.pixelSize: 11; font.weight: Font.DemiBold; font.letterSpacing: 0.9; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans }
                Rectangle {
                    implicitWidth: segRow.implicitWidth + 6; implicitHeight: 30
                    radius: Theme.rad(8)
                    color: Theme.field
                    border.color: Theme.hair; border.width: 1
                    Row {
                        id: segRow
                        x: 3; y: 3
                        spacing: 2
                        Repeater {
                            model: [["size", "freeup_sort_size"], ["age", "freeup_sort_age"]]
                            delegate: Rectangle {
                                readonly property bool on: root.sortKey === modelData[0]
                                width: segTxt.implicitWidth + 20; height: 24
                                radius: Math.max(0, Theme.rad(8) - 2)
                                color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.12) : "transparent"
                                Text { id: segTxt; anchors.centerIn: parent; text: (i18n.language, i18n.t(modelData[1])); color: parent.on ? Theme.t1 : Theme.t3; font.pixelSize: 13; font.weight: parent.on ? Font.DemiBold : Font.Normal; font.family: Theme.fontSans }
                                MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.sortKey = modelData[0] }
                            }
                        }
                    }
                }
            }
            Item {
                Layout.fillWidth: true
                Layout.leftMargin: 10; Layout.rightMargin: 10
                Layout.fillHeight: true
                Layout.preferredHeight: Math.min(roomList.contentHeight, 6 * 59)
                Layout.minimumHeight: 120
                ListView {
                    id: roomList
                    anchors.fill: parent
                    clip: true
                    model: root.sortedRows
                    boundsBehavior: Flickable.StopAtBounds
                    WheelScroller { flick: roomList }
                    Text {
                        anchors.centerIn: parent
                        visible: roomList.count === 0
                        text: (i18n.language, i18n.t("make_room_empty"))
                        color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
                    }
                    delegate: Rectangle {
                        id: row
                        required property var modelData
                        required property int index
                        readonly property bool on: root.picked[modelData.infoHash] === true
                        width: ListView.view.width
                        height: 59
                        color: on ? Qt.rgba(Theme.t1.r, Theme.t1.g, Theme.t1.b, 0.08) : (rowMa.containsMouse ? Theme.hover : "transparent")
                        Rectangle { visible: row.index > 0; width: parent.width; height: 1; color: Theme.hairSoft }
                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12; anchors.rightMargin: 12
                            spacing: 12
                            TChk { Layout.alignment: Qt.AlignVCenter; on: row.on; onToggled: function (v) { root.setPicked([row.modelData.infoHash], v) } }
                            Rectangle {
                                Layout.preferredWidth: 30; Layout.preferredHeight: 40
                                radius: Theme.rad(4)
                                color: Theme.hover
                                Text {
                                    anchors.centerIn: parent
                                    visible: !row.modelData.poster
                                    text: row.modelData.name.split(/[\s:·.\-_]+/).filter(function (w) { return w.length > 0 }).slice(0, 2).map(function (w) { return w[0] }).join("").toUpperCase()
                                    color: Theme.t3; font.pixelSize: 10; font.weight: Font.Bold; font.family: Theme.fontSans
                                }
                                RoundedImage {
                                    anchors.fill: parent
                                    visible: !!row.modelData.poster
                                    radius: Theme.rad(4)
                                    sourceWidth: 60
                                    sources: row.modelData.poster ? [(Qt.platform.os === "windows" ? "file:///" : "file://") + encodeURI(row.modelData.poster)] : []
                                }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 3
                                Text { Layout.fillWidth: true; text: row.modelData.name; color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; elide: Text.ElideRight }
                                Text {
                                    Layout.fillWidth: true
                                    text: root.reason(row.modelData)
                                    color: row.modelData.inPack ? Theme.warn : Theme.t3
                                    font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                                    elide: Text.ElideRight
                                }
                            }
                            Text { text: root.fmt(row.modelData.sizeBytes); color: Theme.t1; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum }
                        }
                        MouseArea { id: rowMa; anchors.fill: parent; hoverEnabled: true; z: -1; cursorShape: Qt.PointingHandCursor; onClicked: root.setPicked([row.modelData.infoHash], !row.on) }
                    }
                }
                // more below
                Rectangle {
                    anchors.bottom: parent.bottom
                    width: parent.width; height: 28
                    visible: roomList.contentHeight > roomList.height && !roomList.atYEnd
                    gradient: Gradient {
                        GradientStop { position: 0; color: "transparent" }
                        GradientStop { position: 1; color: Theme.elev }
                    }
                }
            }

            // ---- footer ----
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 64
                Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 22; anchors.rightMargin: 16
                    spacing: 10
                    Row {
                        spacing: 9
                        TChk { anchors.verticalCenter: parent.verticalCenter; on: root.deleteFiles; onToggled: function (v) { root.deleteFiles = v } }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: (i18n.language, i18n.t("freeup_delete_files"))
                            color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.deleteFiles = !root.deleteFiles }
                        }
                    }
                    Item { Layout.fillWidth: true }
                    BtnFlat { text: (i18n.language, i18n.t("btn_cancel")); onClicked: root.open = false }
                    BtnFlat {
                        primary: true
                        opacity: root.pickedRows.length > 0 ? 1 : 0.4
                        text: {
                            i18n.language
                            var n = root.pickedRows.length
                            if (n === 0) return i18n.t("freeup_remove_none")
                            return root.deleteFiles ? i18n.t("freeup_remove_frees").arg(n).arg(root.fmt(root.freedBytes))
                                                    : i18n.t("freeup_remove").arg(n)
                        }
                        onClicked: root.commit()
                    }
                }
            }
        }
    }
}
