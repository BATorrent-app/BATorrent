// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs
import "../theme"
import "../widgets"

BatDialog {
    id: dlg
    title: (i18n.language, i18n.t("add_any_title"))
    cardW: 480
    cardH: 470
    acceptOnReturn: false
    footHint: (i18n.language, i18n.t("magnet_multi_hint"))
    okText: (i18n.language, i18n.t("add_torrent_add_btn"))

    property alias magnetText: magnetArea.text
    property alias savePath: pathFld.text
    property bool skipNextClear: false
    signal browseRequested()

    // One label for one line, a count for several: a wall of per-line types
    // says less than the number of things about to be added.
    readonly property var lines: magnetArea.text.split("\n").filter(function (l) {
        return l.trim().length > 0
    })
    readonly property var kinds: (typeof session === "undefined") ? [] : lines.map(function (l) {
        return session.inputKind(l.trim())
    }).filter(function (k) { return k !== "" })
    readonly property string kindLabel: {
        if (kinds.length === 0) return ""
        if (kinds.length > 1) return (i18n.language, i18n.t("add_any_count")).arg(kinds.length)
        var k = kinds[0]
        return (i18n.language, i18n.t(k === "magnet" ? "add_any_kind_magnet"
                                    : k === "torrentFile" ? "add_any_kind_file"
                                    : k === "torrentUrl" ? "add_any_kind_url"
                                                         : "add_any_kind_web"))
    }
    onOpenedChanged: if (opened) {
        if (!skipNextClear) magnetArea.text = ""
        skipNextClear = false
        if (typeof session !== "undefined") pathFld.text = session.defaultSavePath()
        magFavRow.reload()
    }
    // clipboard-detected magnet (window regained focus): prefill instead of
    // the usual blank-on-open, so the user only has to confirm or cancel
    function openWithMagnet(uri) {
        magnetText = uri
        skipNextClear = true
        open()
    }

    FolderDialog {
        id: magFolderDlg
        onAccepted: if (typeof session !== "undefined")
            pathFld.text = session.urlToLocalPath(magFolderDlg.selectedFolder.toString())
    }

    // ----- col 1: eyebrow + title -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.sp1
        Eyebrow { text: (i18n.language, i18n.t("magnet_add")); red: true }
        Text {
            text: (i18n.language, i18n.t("magnet_link"))
            color: Theme.t1
            font.pixelSize: 19
            font.weight: Font.DemiBold
            font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }
    }

    // ----- col 2: label + textarea -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Text {
                text: (i18n.language, i18n.t("add_any_paste"))
                color: Theme.t3
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
            Item { Layout.fillWidth: true }
            // Says what it understood, as it is typed. The routing is the part
            // a pasted link cannot show you, so the field admits it instead of
            // asking for trust.
            Text {
                text: dlg.kindLabel
                color: dlg.kindLabel.length > 0 ? Theme.grn : Theme.t4
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
        }
        // Two ways in, the same size, because they are two gestures people
        // reach for equally: pasting text and picking a file. Magnet and link
        // are both the first one, which is why they share a field rather than
        // getting a button each.
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            TArea {
                id: magnetArea
                Layout.fillWidth: true
                Layout.preferredHeight: 92
                placeholder: "magnet:…\nhttps://…"
            }
            Rectangle {
                Layout.preferredWidth: 150
                Layout.preferredHeight: 92
                radius: 9
                color: fileMa.containsMouse ? Theme.hover : Theme.field
                border.color: fileMa.containsMouse ? Theme.accent : Theme.hair
                border.width: 1
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
                Behavior on border.color { ColorAnimation { duration: Theme.durFast } }
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    IconImg {
                        Layout.alignment: Qt.AlignHCenter
                        src: "qrc:/icons/file.svg"
                        tint: fileMa.containsMouse ? Theme.accent : Theme.t3
                        s: 20
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: (i18n.language, i18n.t("add_any_pick_file"))
                        color: fileMa.containsMouse ? Theme.t1 : Theme.t2
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                        font.family: Theme.fontSans
                    }
                    Text {
                        Layout.alignment: Qt.AlignHCenter
                        text: ".torrent"
                        color: Theme.t4
                        font.pixelSize: 11
                        font.family: Theme.fontSans
                    }
                }
                MouseArea {
                    id: fileMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dlg.browseRequested()
                }
            }
        }
    }

    // ----- col 3: meta-note card -----
    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 60
        radius: 9
        color: Theme.panel
        border.color: Theme.hair
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            anchors.topMargin: 10
            anchors.bottomMargin: 10
            spacing: 10

            // info icon (i circular): desenhado inline já que não temos info.svg
            Rectangle {
                Layout.alignment: Qt.AlignTop
                implicitWidth: 15; implicitHeight: 15
                radius: 7.5
                color: "transparent"
                border.color: Theme.t3
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "i"
                    color: Theme.t3
                    font.pixelSize: 9
                    font.weight: Font.Bold
                    font.family: Theme.fontSans
                }
            }
            Text {
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                text: (i18n.language, i18n.t("magnet_meta_hint"))
                color: Theme.t3
                font.pixelSize: 11
                font.family: Theme.fontSans
                lineHeight: 1.4
            }
        }
    }

    // ----- col 4: salvar em + path -----
    ColumnLayout {
        Layout.fillWidth: true
        spacing: 7
        Text {
            text: (i18n.language, i18n.t("detail_kv_save_to"))
            color: Theme.t3
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        PathFld {
            id: pathFld
            Layout.fillWidth: true
            onBrowseClicked: magFolderDlg.open()
        }
        FavFolders {
            id: magFavRow
            Layout.fillWidth: true
            current: pathFld.text
            onPicked: function(p) { pathFld.text = p }
        }
    }

    // ----- col 5: togrow Iniciar imediatamente -----
    RowLayout {
        Layout.fillWidth: true
        spacing: 12
        Text {
            text: (i18n.language, i18n.t("addt_start_now"))
            color: Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontSans
        }
        Item { Layout.fillWidth: true }
        TToggle { on: true }
    }
}
