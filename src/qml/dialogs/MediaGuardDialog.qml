// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

// What MediaGuard found, in plain words, and the way out: another release of
// the same title. "quarantine" when a "video" turned out not to be one,
// "lure" when a real video carries a licence/codec page.
BatDialog {
    id: dlg
    cardW: 460
    cardH: 300

    property string mode: "quarantine"
    property string infoHash: ""
    property string fileName: ""
    property string kindKey: ""
    property string searchQuery: ""
    readonly property bool held: mode === "quarantine"

    signal searchRequested(string query)
    signal trashRequested(string infoHash)

    function kindPhrase(k) {
        switch (k) {
        case "program":  return i18n.t("guard_kind_program")
        case "archive":  return i18n.t("guard_kind_archive")
        case "document": return i18n.t("guard_kind_document")
        case "webpage":  return i18n.t("guard_kind_webpage")
        default:         return i18n.t("guard_kind_unknown")
        }
    }
    function show(m, hash, name, kind, query) {
        mode = m; infoHash = hash; fileName = name; kindKey = kind || ""; searchQuery = query || ""
        open()
    }

    title: (i18n.language, i18n.t(held ? "guard_q_title" : "guard_l_title"))
    showOk: searchQuery.length > 0
    okText: (i18n.language, i18n.t("guard_search"))
    cancelText: (i18n.language, i18n.t(held ? "guard_q_keep" : "guard_l_ok"))
    onAccepted: searchRequested(searchQuery)

    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: Theme.sp2
        spacing: Theme.sp3

        Rectangle {
            Layout.preferredWidth: 40
            Layout.preferredHeight: 40
            Layout.alignment: Qt.AlignTop
            radius: 10
            color: Theme.accentTint
            IconImg {
                anchors.centerIn: parent
                src: dlg.held ? "qrc:/icons/shield-check.svg" : "qrc:/icons/triangle-alert.svg"
                tint: Theme.accentText; s: 18
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                Layout.fillWidth: true
                text: (i18n.language, i18n.t(dlg.held ? "guard_q_head" : "guard_l_head"))
                wrapMode: Text.WordWrap
                color: Theme.t1
                font.pixelSize: 16
                font.weight: Font.DemiBold
                font.family: Theme.fontSans
            }
            Text {
                Layout.fillWidth: true
                text: (i18n.language, dlg.held
                       ? i18n.t("guard_q_body").arg(dlg.fileName).arg(dlg.kindPhrase(dlg.kindKey))
                       : i18n.t("guard_l_body").arg(dlg.fileName))
                wrapMode: Text.Wrap   // release names have no spaces to break at
                color: Theme.t2
                font.pixelSize: 13
                font.family: Theme.fontSans
                lineHeight: 1.35
            }
        }
    }

    ColumnLayout {
        visible: dlg.held
        Layout.fillWidth: true
        spacing: Theme.sp2
        Text {
            text: (i18n.language, i18n.t("guard_q_did"))
            color: Theme.t3
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        Repeater {
            model: ["guard_q_did_pause", "guard_q_did_hold", "guard_q_did_keep"]
            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sp2
                IconImg {
                    Layout.alignment: Qt.AlignTop
                    Layout.topMargin: 2
                    src: "qrc:/icons/check.svg"
                    tint: Theme.t3; s: 13
                }
                Text {
                    Layout.fillWidth: true
                    text: (i18n.language, i18n.t(modelData))
                    wrapMode: Text.WordWrap
                    color: Theme.t2
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                }
            }
        }
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 6
        Text {
            text: (i18n.language, i18n.t("guard_next"))
            color: Theme.t3
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.family: Theme.fontSans
        }
        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t(dlg.held ? "guard_q_next" : "guard_l_next"))
            wrapMode: Text.WordWrap
            color: Theme.t1
            font.pixelSize: 13
            font.family: Theme.fontSans
            lineHeight: 1.35
        }
    }

    BtnFlat {
        visible: dlg.held
        Layout.alignment: Qt.AlignLeft
        sm: true
        icon: "qrc:/icons/trash.svg"
        text: (i18n.language, i18n.t("guard_q_trash"))
        onClicked: {
            dlg.close()
            dlg.trashRequested(dlg.infoHash)
        }
    }
}
