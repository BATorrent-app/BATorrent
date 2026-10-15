// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Removing: one choice, three ways out, and the button says exactly what it
// will do. Keeping the files is always the one marked when it opens (Del);
// Shift+Del opens with "delete permanently" marked instead. The choice is
// never remembered: the safe one comes back every time. Only "delete
// permanently" says it can't be undone, because only there is it true.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ModalShell {
    id: dlg
    cardWidth: 460
    cardHeight: col.implicitHeight

    // 0 keep the files, 1 to the Trash, 2 delete permanently
    property int choice: 0
    property var info: ({ count: 0 })
    signal accepted(int choice)

    function open() { openWith(0) }
    function openWith(c) {
        info = typeof session !== "undefined" ? session.removalInfo() : ({ count: 0 })
        if (!info.count) return
        choice = c
        opened = true
    }
    function confirm() { var c = choice; close(); accepted(c) }

    readonly property int count: info.count || 0
    // a season's episodes share one title: name it once
    readonly property var names: {
        var seen = {}, out = []
        var n = info.names || []
        for (var i = 0; i < n.length; ++i) if (!seen[n[i]]) { seen[n[i]] = true; out.push(n[i]) }
        return out
    }
    readonly property string size: info.size || ""

    Shortcut {
        sequences: ["Up"]
        enabled: dlg.opened && DialogStack.topItem === dlg
        onActivated: dlg.choice = Math.max(0, dlg.choice - 1)
    }
    Shortcut {
        sequences: ["Down"]
        enabled: dlg.opened && DialogStack.topItem === dlg
        onActivated: dlg.choice = Math.min(2, dlg.choice + 1)
    }
    Shortcut {
        sequences: ["Return", "Enter"]
        enabled: dlg.opened && DialogStack.topItem === dlg
        onActivated: dlg.confirm()
    }

    ColumnLayout {
        id: col
        width: parent.width
        spacing: 0

        // what goes: the cover, the name, its size and who it is still feeding
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 12
            Layout.topMargin: 20; Layout.bottomMargin: 18
            spacing: 14
            Rectangle {
                Layout.alignment: Qt.AlignTop
                Layout.preferredWidth: 44; Layout.preferredHeight: 62
                radius: Theme.rad(4)
                color: Theme.hover
                RoundedImage {
                    anchors.fill: parent
                    visible: !!dlg.info.poster
                    radius: Theme.rad(4)
                    sourceWidth: 88
                    sources: dlg.info.poster ? [dlg.info.poster] : []
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignTop
                spacing: 5
                Text {
                    Layout.fillWidth: true
                    // names come from the torrent: plain text, never markup
                    textFormat: Text.PlainText
                    text: (i18n.language, dlg.names.length > 1 ? i18n.t("rm_title_n").arg(dlg.count)
                                                        : i18n.t("rm_title_one").arg(dlg.names[0] || ""))
                    color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    wrapMode: Text.WordWrap; maximumLineCount: 2; elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    textFormat: Text.PlainText
                    color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                    wrapMode: Text.WordWrap
                    text: {
                        i18n.language
                        var bits = []
                        if (dlg.count > 1 && dlg.names.length === 1) {
                            bits.push(i18n.t("rm_n_items").arg(dlg.count))
                            bits.push(i18n.t("rm_total").arg(dlg.size))
                        } else if (dlg.count > 1) {
                            var n = dlg.names
                            bits.push(n.length > 3 ? i18n.t("rm_and_more").arg(n.slice(0, 3).join(", ")).arg(n.length - 3)
                                                   : n.join(", "))
                            bits.push(i18n.t("rm_total").arg(dlg.size))
                        } else {
                            bits.push(dlg.size)
                        }
                        var p = dlg.info.peers || 0
                        if (p > 0) bits.push(i18n.t(p === 1 ? "rm_seeding_1" : "rm_seeding").arg(p))
                        return bits.join(" · ")
                    }
                }
            }
            CloseButton { Layout.alignment: Qt.AlignTop; onClicked: dlg.dismiss() }
        }

        // the three ways out
        ColumnLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 20
            spacing: 4
            Option {
                value: 0
                label: (i18n.language, i18n.t("rm_keep"))
                detail: (i18n.language, dlg.count > 1 || !dlg.info.folder
                         ? i18n.t("rm_keep_d_many").arg(dlg.size)
                         : i18n.t("rm_keep_d").arg(dlg.size).arg(dlg.info.folder))
            }
            Option {
                value: 1
                label: (i18n.language, i18n.t("rm_trash"))
                detail: (i18n.language, i18n.t("rm_trash_d").arg(dlg.size))
            }
            Option {
                value: 2
                label: (i18n.language, i18n.t("rm_delete"))
                detail: (i18n.language, i18n.t("rm_delete_d").arg(dlg.size))
            }
        }

        // an installed game in the selection keeps playing
        RowLayout {
            visible: !!dlg.info.game && dlg.choice > 0
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.topMargin: 12
            spacing: 10
            Rectangle { Layout.alignment: Qt.AlignTop; Layout.topMargin: 6; width: 6; height: 6; radius: 3; color: Theme.warn }
            Text {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                wrapMode: Text.WordWrap
                text: (i18n.language, i18n.t("rm_game_stays").arg(dlg.info.game || "").arg(dlg.info.gameSize || ""))
                color: Theme.t2; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }

        // footer: the shortcut that opens this choice, Cancel, and the one red button
        Item {
            Layout.fillWidth: true
            Layout.topMargin: 18
            Layout.preferredHeight: 62
            Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20; anchors.rightMargin: 16
                spacing: 8
                Text {
                    text: dlg.choice === 0 ? "Del" : dlg.choice === 2 ? "Shift+Del" : ""
                    color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans
                }
                Item { Layout.fillWidth: true }
                BtnFlat { text: (i18n.language, i18n.t("btn_cancel")); onClicked: dlg.dismiss() }
                BtnFlat {
                    primary: true
                    text: (i18n.language, dlg.choice === 0 ? i18n.t("rm_btn_keep")
                                        : dlg.choice === 1 ? i18n.t("rm_btn_trash").arg(dlg.size)
                                                           : i18n.t("rm_btn_delete").arg(dlg.size))
                    onClicked: dlg.confirm()
                }
            }
        }
    }

    component Option: Rectangle {
        id: opt
        property int value
        property string label
        property string detail
        readonly property bool on: dlg.choice === value
        Layout.fillWidth: true
        implicitHeight: optCol.implicitHeight + 24
        radius: Theme.rad(8)
        color: on ? Theme.hover : optMa.containsMouse ? Qt.rgba(Theme.hover.r, Theme.hover.g, Theme.hover.b, Theme.hover.a * 0.5) : "transparent"
        border.width: on ? 1 : 0
        border.color: Theme.hair
        Rectangle {
            id: dot
            x: 14; y: 13
            width: 18; height: 18; radius: 9
            color: opt.on ? Theme.t1 : "transparent"
            border.width: opt.on ? 0 : 1.5
            border.color: Theme.t3
            Rectangle { anchors.centerIn: parent; visible: opt.on; width: 7; height: 7; radius: 3.5; color: Theme.elev }
        }
        Column {
            id: optCol
            x: 44; y: 12
            width: parent.width - 44 - 14
            spacing: 3
            Text { text: opt.label; color: Theme.t1; font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Text {
                width: parent.width
                text: opt.detail
                textFormat: Text.PlainText
                color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                wrapMode: Text.WordWrap
            }
        }
        MouseArea {
            id: optMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: dlg.choice = opt.value
            onDoubleClicked: { dlg.choice = opt.value; dlg.confirm() }
        }
    }
}
