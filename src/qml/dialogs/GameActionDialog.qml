// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Two things from a game's Play menu that need a word from the person first:
// the arguments it starts with, and running its own uninstaller (which frees
// the space, and leaves the saves where they are).
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ModalShell {
    id: dlg
    cardWidth: 560
    cardHeight: col.implicitHeight

    property string mode: ""          // "args" | "uninstall"
    property string hash: ""
    property string title: ""
    property string frees: ""
    readonly property var api: typeof session !== "undefined" ? session : null

    function openFor(kind, infoHash, gameTitle) {
        mode = kind
        hash = infoHash
        title = gameTitle
        frees = ""
        if (kind === "args") argsFld.text = api ? api.gameLaunchArgs(infoHash) : ""
        if (kind === "uninstall") frees = api ? (api.gameUninstaller(infoHash).size || "") : ""
        opened = true
        if (kind === "args") argsFld.field.forceActiveFocus()
    }
    function confirm() {
        if (!api) { close(); return }
        if (mode === "args") api.setGameLaunchArgs(hash, argsFld.text)
        else if (mode === "uninstall") api.runUninstaller(hash)
        close()
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

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 12; Layout.topMargin: 16
            Text {
                Layout.fillWidth: true
                textFormat: Text.PlainText
                elide: Text.ElideRight
                text: (i18n.language, dlg.mode === "args" ? i18n.t("game_args_title")
                                                          : i18n.t("game_uninstall_title").arg(dlg.title))
                color: Theme.t1; font.pixelSize: 17; font.weight: Font.DemiBold; font.family: Theme.fontSans
            }
            CloseButton { onClicked: dlg.dismiss() }
        }
        Text {
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.topMargin: 10
            wrapMode: Text.WordWrap
            textFormat: Text.PlainText
            text: (i18n.language, dlg.mode === "args" ? i18n.t("game_args_body").arg(dlg.title)
                                                      : (dlg.frees ? i18n.t("game_uninstall_body").arg(dlg.frees)
                                                                   : i18n.t("game_uninstall_body_nosize")))
            color: Theme.t2; font.pixelSize: 14; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        TFld {
            id: argsFld
            visible: dlg.mode === "args"
            Layout.fillWidth: true
            Layout.leftMargin: 20; Layout.rightMargin: 20; Layout.topMargin: 16
            Layout.preferredHeight: 42
            mono: true
            placeholder: "-windowed -skipintro"
        }

        Item {
            Layout.fillWidth: true
            Layout.topMargin: 20
            Layout.preferredHeight: 64
            Rectangle { width: parent.width; height: 1; color: Theme.hairSoft }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20; anchors.rightMargin: 16
                spacing: 8
                Item { Layout.fillWidth: true }
                BtnFlat { text: (i18n.language, i18n.t("btn_cancel")); onClicked: dlg.dismiss() }
                BtnFlat {
                    primary: true
                    text: (i18n.language, i18n.t(dlg.mode === "args" ? "game_args_save" : "game_uninstall_btn"))
                    onClicked: dlg.confirm()
                }
            }
        }
    }
}
