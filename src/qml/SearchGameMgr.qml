// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Game-catalog manager overlay (neutral: the user adds their own source URLs).
// `sv` is the owning SearchView (showGameMgr, gameList, api).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: root
    property var sv

    anchors.fill: parent
    visible: root.sv.showGameMgr
    color: "#aa000000"
    MouseArea { anchors.fill: parent; onClicked: root.sv.showGameMgr = false }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48, 520)
        height: Math.min(parent.height - 48, 420)
        radius: 12
        color: Theme.elev
        border.color: Theme.hair
        border.width: 1
        MouseArea { anchors.fill: parent }   // swallow clicks so the dim layer doesn't close it

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: Theme.sp4

            Text { text: (i18n.language, i18n.t("game_sources_title")); color: Theme.t1; font.pixelSize: 15; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Text {
                Layout.fillWidth: true
                text: (i18n.language, i18n.t("game_sources_hint"))
                color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans
                wrapMode: Text.WordWrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: Theme.sp3
                TFld {
                    id: srcUrlFld
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                    placeholder: "https://…/catalog.json"
                    onEdited: addBtn.add()
                }
                BtnFlat {
                    id: addBtn
                    primary: true
                    text: (i18n.language, i18n.t("game_sources_add"))
                    function add() {
                        var u = srcUrlFld.text.trim()
                        if (u.length === 0 || !root.sv.api) return
                        root.sv.api.addGameSource("", u)
                        srcUrlFld.text = ""
                    }
                    onClicked: add()
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: root.sv.gameList
                boundsBehavior: Flickable.StopAtBounds
                Text {
                    anchors.centerIn: parent
                    visible: parent.count === 0
                    text: (i18n.language, i18n.t("game_sources_empty"))
                    color: Theme.t4; font.pixelSize: 12; font.family: Theme.fontSans
                }
                delegate: Rectangle {
                    required property var modelData
                    width: ListView.view.width
                    height: 40
                    color: "transparent"
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
                    RowLayout {
                        anchors.fill: parent
                        anchors.rightMargin: 4
                        spacing: Theme.sp3
                        Text {
                            Layout.fillWidth: true
                            text: modelData.url
                            color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono
                            elide: Text.ElideMiddle
                        }
                        BtnFlat { text: (i18n.language, i18n.t("game_sources_remove")); onClicked: if (root.sv.api) root.sv.api.removeGameSource(modelData.url) }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Item { Layout.fillWidth: true }
                BtnFlat { text: (i18n.language, i18n.t("game_sources_refresh")); onClicked: if (root.sv.api) root.sv.api.refreshGames() }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("release_notes_close")); onClicked: root.sv.showGameMgr = false }
            }
        }
    }
}
