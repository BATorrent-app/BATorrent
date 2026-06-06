// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Discover content page (4.0 step ③). Rotating hero + horizontal poster rows
// from DiscoveryService (TMDB/IGDB). Click a poster → openSearch(title), which
// Main.qml routes to the Search page.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "theme"
import "widgets"

Rectangle {
    id: page
    color: Theme.bg

    signal openSearch(string query)

    readonly property var api: typeof discovery !== "undefined" ? discovery : null
    readonly property var heroList: api ? api.hero : []
    readonly property var rowList: api ? api.rows : []

    property int heroIndex: 0
    property int shownIndex: 0
    readonly property var heroItem: (heroList.length > shownIndex) ? heroList[shownIndex] : null

    onVisibleChanged: if (visible && api) api.load()
    Component.onCompleted: if (visible && api) api.load()

    onHeroIndexChanged: heroFade.restart()
    SequentialAnimation {
        id: heroFade
        NumberAnimation { target: heroContent; property: "opacity"; to: 0; duration: 220; easing.type: Easing.InCubic }
        ScriptAction { script: page.shownIndex = page.heroIndex }
        NumberAnimation { target: heroContent; property: "opacity"; to: 1; duration: 420; easing.type: Easing.OutCubic }
    }
    Timer {
        interval: 7000
        running: page.visible && page.heroList.length > 1
        repeat: true
        onTriggered: page.heroIndex = (page.heroIndex + 1) % page.heroList.length
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentHeight: col.implicitHeight
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        ColumnLayout {
            id: col
            width: flick.width
            spacing: 26

            // ---------- hero ----------
            Item {
                Layout.fillWidth: true
                Layout.preferredHeight: 300
                visible: page.heroItem !== null

                Item {
                    id: heroContent
                    anchors.fill: parent

                    Image {
                        anchors.fill: parent
                        source: page.heroItem ? (page.heroItem.backdrop || "") : ""
                        fillMode: Image.PreserveAspectCrop
                        asynchronous: true
                        cache: true
                    }
                    // bottom + left scrim for text legibility
                    Rectangle {
                        anchors.fill: parent
                        gradient: Gradient {
                            GradientStop { position: 0.0; color: "#00000000" }
                            GradientStop { position: 0.45; color: "#33000000" }
                            GradientStop { position: 1.0; color: "#ee0b0b0d" }
                        }
                    }

                    ColumnLayout {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        anchors.leftMargin: Theme.sp5
                        anchors.rightMargin: Theme.sp5
                        anchors.bottomMargin: Theme.sp5
                        spacing: 8

                        Text {
                            text: page.heroItem ? page.heroItem.title : ""
                            color: "#ffffff"
                            font.pixelSize: 30; font.weight: Font.Bold; font.family: Theme.fontSans
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Text {
                            text: page.heroItem ? page.heroItem.overview : ""
                            color: "#d8d8dc"
                            font.pixelSize: 13; font.family: Theme.fontSans
                            Layout.fillWidth: true
                            Layout.maximumWidth: 620
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                        BtnFlat {
                            primary: true
                            text: (i18n.language, i18n.t("empty_search_btn"))
                            onClicked: if (page.heroItem) page.openSearch(page.heroItem.title)
                        }
                    }
                }
            }

            // ---------- poster rows ----------
            Repeater {
                model: page.rowList
                delegate: ColumnLayout {
                    id: rowItem
                    required property var modelData
                    Layout.fillWidth: true
                    spacing: 10

                    Text {
                        text: rowItem.modelData.label
                        color: Theme.t1
                        font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
                        Layout.leftMargin: Theme.sp5
                    }
                    ListView {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 252
                        orientation: ListView.Horizontal
                        clip: true
                        spacing: 16
                        boundsBehavior: Flickable.StopAtBounds
                        leftMargin: Theme.sp5
                        rightMargin: Theme.sp5
                        model: rowItem.modelData.items
                        delegate: PosterCard {
                            id: pcard
                            required property var modelData
                            posterW: 150
                            title: pcard.modelData.title
                            poster: pcard.modelData.poster
                            year: pcard.modelData.year
                            rating: pcard.modelData.rating
                            type: pcard.modelData.type
                            onActivated: page.openSearch(pcard.modelData.title)
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }
        }
    }

    // loading / empty state
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 14
        visible: page.rowList.length === 0
        BusyIndicator { Layout.alignment: Qt.AlignHCenter; running: page.api && page.api.loading; visible: running }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: page.api && page.api.loading ? (i18n.language, i18n.t("discover_loading"))
                 : (page.api && page.api.statusText.length > 0 ? page.api.statusText : (i18n.language, i18n.t("discover_empty")))
            color: Theme.t3; font.pixelSize: 13; font.family: Theme.fontSans
        }
    }
}
