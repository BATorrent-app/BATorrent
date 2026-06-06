// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Search content page (4.0 step ②). 1:1 port of the former SearchWindow into a
// nav-rail page — wired to QmlSearchBridge (`search`).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Rectangle {
    id: page
    color: Theme.bg

    readonly property var api: typeof search !== "undefined" ? search : null
    readonly property string sourceKey: {
        if (!api) return "stremio"
        var s = api.sources
        return (srcSel.currentIndex >= 0 && srcSel.currentIndex < s.length) ? s[srcSel.currentIndex].key : "stremio"
    }
    readonly property bool isLegacy: sourceKey === "legacy"
    readonly property bool isGames: sourceKey === "games" || sourceKey === "all"
    property bool showGameMgr: false
    property var gameList: []

    function reloadGames() { gameList = (api ? api.gameSources() : []) }

    Connections {
        target: page.api
        ignoreUnknownSignals: true
        function onGameSourcesChanged() { page.reloadGames() }
    }

    onShowGameMgrChanged: if (showGameMgr) reloadGames()
    onVisibleChanged: if (visible && api) api.refreshSources()

    function runSearch() {
        if (!api) return
        var cat = (isLegacy && catSel.currentIndex >= 0) ? api.categories[catSel.currentIndex].code : 0
        api.search(page.sourceKey, queryFld.text, cat)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // header
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("search_heading")); color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // search bar
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 36 + 2 * Theme.sp4
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                anchors.topMargin: Theme.sp4
                anchors.bottomMargin: Theme.sp4
                spacing: Theme.sp3

                TFld {
                    id: queryFld
                    Layout.fillWidth: true
                    Layout.preferredHeight: 36
                    icon: "qrc:/icons/search.svg"
                    placeholder: (i18n.language, i18n.t("search_input"))
                    onEdited: page.runSearch()
                }
                TSelect {
                    id: srcSel
                    Layout.preferredHeight: 36
                    Layout.preferredWidth: 180
                    model: page.api ? page.api.sources : []
                    textRole: "label"
                }
                TSelect {
                    id: catSel
                    visible: page.isLegacy
                    Layout.preferredHeight: 36
                    Layout.preferredWidth: 130
                    model: page.api ? page.api.categories : []
                    textRole: "label"
                }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("empty_search_btn")); onClicked: page.runSearch() }
            }
        }

        // results header (columns adapt to mode)
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            color: "transparent"
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: Theme.sp5
                spacing: Theme.sp4
                property bool torrentish: page.api && (page.api.mode === "torrent" || page.api.mode === "games" || page.api.mode === "all")
                Text { text: (i18n.language, i18n.t("search_col_name2")); Layout.fillWidth: true; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { text: (i18n.language, i18n.t("search_col_size2")); Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { visible: parent.torrentish; text: (i18n.language, i18n.t("search_col_seeds2")); Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Text { visible: parent.torrentish; text: (i18n.language, i18n.t("search_col_leech")); Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t4; font.pixelSize: 10; font.weight: Font.DemiBold; font.letterSpacing: 0.6; font.family: Theme.fontSans }
                Item { Layout.preferredWidth: 36 }
            }
        }

        // results
        ListView {
            id: resultsView
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: page.api ? page.api.results : []
            boundsBehavior: Flickable.StopAtBounds

            // empty / loading state
            Text {
                anchors.centerIn: parent
                visible: resultsView.count === 0
                text: page.api && page.api.searching ? (i18n.language, i18n.t("search_searching2"))
                     : (page.api && page.api.statusText.length > 0 ? page.api.statusText : (i18n.language, i18n.t("search_prompt")))
                color: Theme.t4
                font.pixelSize: 13
                font.family: Theme.fontSans
            }

            delegate: Rectangle {
                required property var modelData
                required property int index
                width: ListView.view.width
                height: 46
                color: resMa.containsMouse ? Theme.hover : "transparent"
                Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: Theme.sp5
                    anchors.rightMargin: Theme.sp5
                    spacing: Theme.sp4

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 1
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                Layout.fillWidth: true
                                text: modelData.name
                                color: Theme.t1
                                font.pixelSize: 13
                                font.family: Theme.fontSans
                                elide: Text.ElideRight
                            }
                            TChip { visible: (modelData.repacker || "").length > 0; text: modelData.repacker || "" }
                        }
                        Text {
                            visible: (modelData.sub || "").length > 0
                            text: modelData.sub || ""
                            color: Theme.t4
                            font.pixelSize: 10
                            font.family: Theme.fontSans
                        }
                    }
                    Text { text: modelData.sizeStr || ""; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignRight; color: Theme.t2; font.pixelSize: 12; font.family: Theme.fontMono }
                    Text { visible: (modelData.seeds || "").length > 0; text: modelData.seeds || ""; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: modelData.hasSeeds ? Theme.grn : Theme.t4; font.pixelSize: 12; font.family: Theme.fontMono }
                    Text { visible: (modelData.leech || "").length > 0; text: modelData.leech || ""; Layout.preferredWidth: 56; horizontalAlignment: Text.AlignRight; color: Theme.t3; font.pixelSize: 12; font.family: Theme.fontMono }
                    Item {
                        Layout.preferredWidth: 36
                        Rectangle {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: 28; height: 28; radius: 7
                            color: "transparent"
                            border.color: addMa.containsMouse ? Theme.accent : Theme.hair
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: (page.api && page.api.mode === "catalog") ? "›" : "+"
                                color: addMa.containsMouse ? Theme.accentText : Theme.t3
                                font.pixelSize: 15
                            }
                            MouseArea {
                                id: addMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (page.api) page.api.activateResult(index)
                            }
                        }
                    }
                }
                MouseArea {
                    id: resMa
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton
                    z: -1
                    onDoubleClicked: if (page.api) page.api.activateResult(index)
                }
            }
        }

        // footer
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                spacing: Theme.sp2
                BtnFlat { visible: page.api && page.api.inStreams; text: (i18n.language, i18n.t("search_back2")); onClicked: if (page.api) page.api.back() }
                Text { text: page.api ? page.api.statusText : ""; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                BtnFlat { visible: page.isGames; text: (i18n.language, i18n.t("game_sources_btn")); onClicked: page.showGameMgr = true }
            }
        }
    }

    // Game-catalog manager overlay (neutral: the user adds their own source URLs)
    Rectangle {
        anchors.fill: parent
        visible: page.showGameMgr
        color: "#aa000000"
        MouseArea { anchors.fill: parent; onClicked: page.showGameMgr = false }

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
                            if (u.length === 0 || !page.api) return
                            page.api.addGameSource("", u)
                            srcUrlFld.text = ""
                        }
                        onClicked: add()
                    }
                }

                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: page.gameList
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
                            BtnFlat { text: (i18n.language, i18n.t("game_sources_remove")); onClicked: if (page.api) page.api.removeGameSource(modelData.url) }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Item { Layout.fillWidth: true }
                    BtnFlat { text: (i18n.language, i18n.t("game_sources_refresh")); onClicked: if (page.api) page.api.refreshGames() }
                    BtnFlat { primary: true; text: (i18n.language, i18n.t("release_notes_close")); onClicked: page.showGameMgr = false }
                }
            }
        }
    }
}
