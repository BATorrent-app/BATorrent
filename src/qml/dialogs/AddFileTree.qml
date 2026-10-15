// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Every file of the Add download window's single item, as a tree. A folder
// ticks everything under it; the ticks live in the window, shared with the
// groups above.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: tree
    property var host: null
    property var rows: []
    property bool divider: false

    // a folder owns every row after it until the depth comes back to its own
    function filesUnder(r) {
        var out = [], d = rows[r].depth
        for (var i = r + 1; i < rows.length && rows[i].depth > d; ++i)
            if (!rows[i].dir) out.push(rows[i].fileIndex)
        return out
    }
    readonly property var under: {
        var out = []
        for (var i = 0; i < rows.length; ++i) out.push(rows[i].dir ? filesUnder(i) : [rows[i].fileIndex])
        return out
    }

    implicitHeight: visible ? Math.min(rows.length * 36, 288) + (divider ? 1 : 0) : 0
    height: implicitHeight

    Rectangle { visible: tree.divider; width: parent.width; height: 1; color: Theme.hairSoft }

    ListView {
        y: tree.divider ? 1 : 0
        width: parent.width
        height: parent.height - y
        clip: true
        model: tree.rows
        boundsBehavior: Flickable.StopAtBounds
        delegate: Item {
            width: ListView.view.width
            height: 36
            readonly property var files: tree.under[index] || []
            readonly property int on: tree.host.selRev >= 0 ? tree.host.countOn(files) : 0
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14 + modelData.depth * 18
                anchors.rightMargin: 14
                spacing: 10
                TChk {
                    Layout.alignment: Qt.AlignVCenter
                    on: files.length > 0 && parent.parent.on === files.length
                    partial: parent.parent.on > 0 && parent.parent.on < files.length
                    onToggled: function (v) { tree.host.setFiles(files, parent.parent.on === 0) }
                }
                IconImg {
                    Layout.alignment: Qt.AlignVCenter
                    src: modelData.dir ? "qrc:/icons/folder.svg" : "qrc:/icons/file.svg"
                    tint: modelData.dir ? Theme.amber : Theme.t3
                    s: 13
                }
                Text {
                    Layout.fillWidth: true
                    text: modelData.path
                    color: Theme.t1
                    font.pixelSize: 12; font.weight: modelData.dir ? Font.DemiBold : Font.Medium
                    font.family: Theme.fontSans
                    elide: Text.ElideMiddle
                }
                Text {
                    visible: !modelData.dir
                    text: modelData.size
                    color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
        }
    }
}
