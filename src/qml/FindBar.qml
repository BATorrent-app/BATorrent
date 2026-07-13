// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The Find page's search bar — one input, two skins: a large hero while
// browsing that shrinks and docks once the user types or scrolls the browse
// content. Category pills ride along in browse; the source/category selects
// and manager buttons appear with results. All search state lives in sv.
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "theme"
import "widgets"

Item {
    id: bar
    property var sv

    readonly property alias text: queryFld.text
    readonly property alias srcIndex: srcSel.currentIndex
    readonly property alias catIndex: catSel.currentIndex
    function setText(t) { queryFld.text = t }
    function resetSource() { srcSel.currentIndex = 0 }
    function focusInput() { queryFld.field.forceActiveFocus() }

    // large hero skin unless docked (typed or scrolled); the store fallback
    // (no catalog) keeps the old centered-680px landing look
    readonly property bool hero: !sv.docked
    readonly property bool centered: hero && !sv.catalogAvailable

    Layout.fillWidth: true
    Layout.preferredHeight: hero ? 74 : (36 + 2 * Theme.sp4)
    Behavior on Layout.preferredHeight { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }

    // bottom hairline only in the docked state
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair; visible: !bar.hero }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: bar.centered ? Math.max(Theme.sp5, (parent.width - 680) / 2) : Theme.sp5
        anchors.rightMargin: bar.centered ? Math.max(Theme.sp5, (parent.width - 680) / 2) : Theme.sp5
        anchors.topMargin: bar.hero ? 10 : Theme.sp4
        anchors.bottomMargin: bar.hero ? 10 : Theme.sp4
        Behavior on anchors.leftMargin { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        Behavior on anchors.rightMargin { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }
        spacing: Theme.sp3

        // field (+ its hero shadow) — grows tall and round in the hero skin
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: bar.hero ? 54 : 36
            Behavior on Layout.preferredHeight { NumberAnimation { duration: 240; easing.type: Easing.OutCubic } }

            MultiEffect {
                source: queryFld
                anchors.fill: queryFld
                shadowEnabled: true
                shadowBlur: 1.0
                blurMax: 36
                shadowColor: "#73000000"
                opacity: bar.hero ? 1 : 0
                Behavior on opacity { NumberAnimation { duration: 220 } }
            }
            TFld {
                id: queryFld
                anchors.fill: parent
                icon: "qrc:/icons/search.svg"
                clearable: true
                radius: bar.hero ? 14 : 8
                fontSize: bar.hero ? 17 : 13
                iconSize: bar.hero ? 20 : 14
                placeholder: (i18n.language, i18n.t("search_input"))
                Behavior on radius { NumberAnimation { duration: 240 } }
                onTextChanged: bar.sv.queryEdited(text)
                // commit on Enter only — committing on focus-out re-ran the search
                // when you clicked a filter dropdown, eating the first click
                Connections {
                    target: queryFld.field
                    function onAccepted() { bar.sv.commitSearch() }
                }
            }
        }

        // category pills — browse only (both skins), filtering the catalog
        Row {
            visible: bar.sv.browse && bar.sv.catalogAvailable
            Layout.alignment: Qt.AlignVCenter
            spacing: 4
            Repeater {
                model: [
                    { k: "all",    lbl: i18n.t("filter_all") },
                    { k: "movie",  lbl: i18n.t("cat_movies") },
                    { k: "series", lbl: i18n.t("cat_series") },
                    { k: "game",   lbl: i18n.t("cat_games") }
                ]
                delegate: Rectangle {
                    required property var modelData
                    readonly property bool on: bar.sv.typeFilter === modelData.k
                    radius: 999
                    implicitHeight: 30
                    implicitWidth: pillTxt.implicitWidth + 26
                    color: on ? Theme.accentTint : (pillMa.containsMouse ? Theme.hover : "transparent")
                    Behavior on color { ColorAnimation { duration: 120 } }
                    Text {
                        id: pillTxt
                        anchors.centerIn: parent
                        text: (i18n.language, parent.modelData.lbl)
                        color: parent.on ? Theme.accentText : (pillMa.containsMouse ? Theme.t2 : Theme.t3)
                        font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                    MouseArea {
                        id: pillMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: bar.sv.typeFilter = modelData.k
                    }
                }
            }
        }

        TSelect {
            id: srcSel
            visible: !bar.sv.browse
            Layout.preferredHeight: 36
            Layout.preferredWidth: 180
            model: bar.sv.api ? bar.sv.api.sources : []
            textRole: "label"
        }
        TSelect {
            id: catSel
            visible: bar.sv.isLegacy && !bar.sv.browse
            Layout.preferredHeight: 36
            Layout.preferredWidth: 130
            model: bar.sv.api ? bar.sv.api.categories : []
            textRole: "label"
        }
        // catalog manager belongs next to the source it configures, not
        // floating in the footer
        BtnFlat {
            visible: bar.sv.isGames && !bar.sv.browse
            text: (i18n.language, i18n.t("game_sources_btn"))
            onClicked: bar.sv.showGameMgr = true
        }
        // manage the torrent sources (toggle + add localized ones)
        BtnFlat {
            visible: !bar.sv.browse
            text: (i18n.language, i18n.t("src_manage_btn"))
            onClicked: bar.sv.showSourcesMgr = true
        }
        BtnFlat {
            visible: !bar.sv.browse
            primary: true
            text: (i18n.language, i18n.t("empty_search_btn"))
            onClicked: bar.sv.commitSearch()
        }
    }
}
