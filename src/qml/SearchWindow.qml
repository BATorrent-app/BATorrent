// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent Search.html — wired to QmlSearchBridge (`search`).
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 780
    height: 560
    minimumWidth: 620
    minimumHeight: 420
    color: Theme.bg
    title: (i18n.language, i18n.t("search_heading"))

    readonly property var api: typeof search !== "undefined" ? search : null
    readonly property string sourceKey: {
        if (!api) return "stremio"
        var s = api.sources
        return (srcSel.currentIndex >= 0 && srcSel.currentIndex < s.length) ? s[srcSel.currentIndex].key : "stremio"
    }
    readonly property bool isLegacy: sourceKey === "legacy"

    onVisibleChanged: if (visible && api) api.refreshSources()

    function runSearch() {
        if (!api) return
        var cat = (isLegacy && catSel.currentIndex >= 0) ? api.categories[catSel.currentIndex].code : 0
        api.search(win.sourceKey, queryFld.text, cat)
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // titlebar
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
                    onEdited: win.runSearch()
                }
                TSelect {
                    id: srcSel
                    Layout.preferredHeight: 36
                    Layout.preferredWidth: 180
                    model: win.api ? win.api.sources : []
                    textRole: "label"
                }
                TSelect {
                    id: catSel
                    visible: win.isLegacy
                    Layout.preferredHeight: 36
                    Layout.preferredWidth: 130
                    model: win.api ? win.api.categories : []
                    textRole: "label"
                }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("empty_search_btn")); onClicked: win.runSearch() }
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
                property bool torrentish: win.api && (win.api.mode === "torrent" || win.api.mode === "games")
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
            model: win.api ? win.api.results : []
            boundsBehavior: Flickable.StopAtBounds

            // empty / loading state
            Text {
                anchors.centerIn: parent
                visible: resultsView.count === 0
                text: win.api && win.api.searching ? (i18n.language, i18n.t("search_searching2"))
                     : (win.api && win.api.statusText.length > 0 ? win.api.statusText : (i18n.language, i18n.t("search_prompt")))
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
                                text: (win.api && win.api.mode === "catalog") ? "›" : "+"
                                color: addMa.containsMouse ? Theme.accentText : Theme.t3
                                font.pixelSize: 15
                            }
                            MouseArea {
                                id: addMa
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: if (win.api) win.api.activateResult(index)
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
                    onDoubleClicked: if (win.api) win.api.activateResult(index)
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
                BtnFlat { visible: win.api && win.api.inStreams; text: (i18n.language, i18n.t("search_back2")); onClicked: if (win.api) win.api.back() }
                Text { text: win.api ? win.api.statusText : ""; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                BtnFlat { text: (i18n.language, i18n.t("release_notes_close")); onClicked: win.close() }
            }
        }
    }
}
