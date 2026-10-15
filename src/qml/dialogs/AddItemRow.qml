// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One line of the Add download window's list: what it is (Torrent, Magnet,
// Direct), its name, what is known about it, its size. Only what it is told,
// so What's new can show the real row with made-up content.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: row
    property string poster: ""
    property string tag: ""            // a file type for an item without a poster ("ISO")
    property bool fetching: false
    property string name: ""
    property bool mono: false          // a raw link, not a name yet
    property string source: ""         // "Torrent", "Magnet", "Direct"
    property string meta: ""
    property bool warn: false          // the meta line is a problem (late, failed)
    property string size: ""
    property bool dup: false
    property bool first: false
    property bool removable: true
    signal removeClicked()
    signal showClicked()

    implicitHeight: 64
    opacity: dup ? 0.55 : 1

    Rectangle { visible: !row.first; width: parent.width; height: 1; color: Theme.hairSoft }
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12; anchors.rightMargin: 8
        spacing: 12
        Rectangle {
            Layout.preferredWidth: 32; Layout.preferredHeight: 44
            radius: Theme.rad(4)
            color: Theme.hover
            Text {
                anchors.centerIn: parent
                visible: row.poster === "" && !row.fetching
                text: row.tag
                color: Theme.t3; font.pixelSize: 11; font.weight: Font.Bold; font.capitalization: Font.AllUppercase; font.family: Theme.fontSans
            }
            Spinner { anchors.centerIn: parent; visible: row.poster === "" && row.fetching; s: 14 }
            RoundedImage { anchors.fill: parent; visible: row.poster !== ""; sources: row.poster !== "" ? [row.poster] : []; radius: Theme.rad(4) }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            Text {
                Layout.fillWidth: true
                text: row.name
                color: row.mono ? Theme.t2 : Theme.t1
                font.pixelSize: row.mono ? 13 : 14; font.weight: row.mono ? Font.Normal : Font.DemiBold
                font.family: row.mono ? Theme.fontMono : Theme.fontSans
                elide: row.mono ? Text.ElideMiddle : Text.ElideRight
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                Text { text: row.source; color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
                Text {
                    Layout.fillWidth: true
                    text: row.meta
                    elide: Text.ElideRight
                    color: row.warn ? Theme.warn : Theme.t3
                    font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
        }
        BtnFlat {
            visible: row.dup
            sm: true
            text: (i18n.language, i18n.t("addw_show"))
            onClicked: row.showClicked()
        }
        Text {
            visible: !row.dup && row.size !== ""
            text: row.size
            color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        CloseButton {
            visible: row.removable
            size: 24; iconSize: 11
            onClicked: row.removeClicked()
        }
    }
}
