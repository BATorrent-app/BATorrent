// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// The Find page's search bar: one input, two skins: a large hero while
// browsing that shrinks and docks once the user types or scrolls the browse
// content. Category pills ride along in browse; the source/category selects
// and manager buttons appear with results. All search state lives in sv.
import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: bar
    property var sv

    readonly property alias text: queryFld.text
    // where a search goes: the sources list from the bridge, picked in the ··· menu
    property int srcIndex: 0
    readonly property alias catIndex: catSel.currentIndex
    function setText(t) { queryFld.text = t }
    function resetSource() { srcIndex = 0 }
    function focusInput() { queryFld.field.forceActiveFocus() }

    // large hero skin unless docked (typed or scrolled); the store fallback
    // (no catalog) keeps the old centered-680px landing look
    readonly property bool hero: !sv.docked
    readonly property bool centered: hero && !sv.catalogAvailable

    Layout.fillWidth: true
    Layout.preferredHeight: 42 + 2 * Theme.sp3
    Behavior on Layout.preferredHeight { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }

    // bottom hairline only in the docked state
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair; visible: !bar.hero }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: bar.centered ? Math.max(Theme.sp5, (parent.width - 680) / 2) : Theme.sp5
        anchors.rightMargin: bar.centered ? Math.max(Theme.sp5, (parent.width - 680) / 2) : Theme.sp5
        anchors.topMargin: Theme.sp3
        anchors.bottomMargin: Theme.sp3
        Behavior on anchors.leftMargin { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        Behavior on anchors.rightMargin { NumberAnimation { duration: Theme.durSlow; easing.type: Theme.easeOut } }
        spacing: Theme.sp3

        // field: grows tall and round in the hero skin
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 42

            TFld {
                id: queryFld
                anchors.fill: parent
                icon: "qrc:/icons/search.svg"
                clearable: true
                radius: Theme.rad(8)
                fontSize: 14
                iconSize: 15
                placeholder: (i18n.language, i18n.t("find_placeholder"))
                // Enter searches: said in the field, not by a red button
                hint: queryFld.text.length > 0 && queryFld.field.activeFocus ? "Enter" : ""
                onTextChanged: bar.sv.queryEdited(text)
                // commit on Enter only: committing on focus-out re-ran the search
                // when you clicked a filter dropdown, eating the first click
                Connections {
                    target: queryFld.field
                    function onAccepted() { bar.sv.commitSearch() }
                }
            }
        }

        // category pills: browse and the titles a search found, filtering both
        Row {
            visible: (bar.sv.browse || bar.sv.isTitles || bar.sv.titlePicked) && bar.sv.catalogAvailable
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
                    // on a title's page the chip says what the title is
                    readonly property bool on: bar.sv.titlePicked && bar.sv.api
                                               ? bar.sv.api.workType === modelData.k
                                               : bar.sv.typeFilter === modelData.k
                    radius: Theme.rad(8)
                    implicitHeight: 32
                    implicitWidth: pillTxt.implicitWidth + 24
                    // selected is neutral: red is for acting, not for where you are
                    color: on ? Theme.selectedFill : (pillMa.containsMouse ? Theme.hover : "transparent")
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                    Text {
                        id: pillTxt
                        anchors.centerIn: parent
                        text: (i18n.language, parent.modelData.lbl)
                        color: parent.on ? Theme.t1 : (pillMa.containsMouse ? Theme.t2 : Theme.t3)
                        font.pixelSize: 14; font.weight: parent.on ? Font.DemiBold : Font.Normal; font.family: Theme.fontSans
                    }
                    MouseArea {
                        id: pillMa; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (bar.sv.titlePicked) bar.sv.closeSub()
                            bar.sv.typeFilter = modelData.k
                        }
                    }
                }
            }
        }

        TSelect {
            id: catSel
            visible: bar.sv.isLegacy && !bar.sv.browse
            Layout.preferredHeight: 36
            Layout.preferredWidth: 130
            model: bar.sv.api ? bar.sv.api.categories : []
            textRole: "label"
        }
        // where the search goes, and the catalogs and sources behind it: one menu
        Rectangle {
            id: moreBtn
            Layout.preferredWidth: 36; Layout.preferredHeight: 36
            radius: Theme.rad(8)
            color: moreMa.containsMouse || moreMenu.opened ? Theme.hover : "transparent"
            border.color: Theme.hair; border.width: 1
            IconImg { anchors.centerIn: parent; src: "qrc:/icons/ellipsis.svg"; s: 16; tint: Theme.t2 }
            MouseArea { id: moreMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: moreMenu.popup(moreBtn, moreBtn.width - moreMenu.width, moreBtn.height + 6) }
        }
        DMenu {
            id: moreMenu
            Instantiator {
                model: bar.sv.api ? bar.sv.api.sources : []
                delegate: DMenuItem {
                    required property var modelData
                    required property int index
                    text: modelData.label
                    iconSrc: index === bar.srcIndex ? "qrc:/icons/check.svg" : ""
                    onTriggered: {
                        bar.srcIndex = index
                        if (queryFld.text.trim().length > 0) bar.sv.commitSearch()
                    }
                }
                onObjectAdded: function (i, o) { moreMenu.insertItem(i, o) }
                onObjectRemoved: function (i, o) { moreMenu.removeItem(o) }
            }
            DMenuSep {}
            DMenuItem {
                text: (i18n.language, i18n.t("game_sources_btn"))
                onTriggered: bar.sv.showGameMgr = true
            }
            DMenuItem {
                text: (i18n.language, i18n.t("src_manage_btn"))
                onTriggered: bar.sv.showSourcesMgr = true
            }
        }
    }
}
