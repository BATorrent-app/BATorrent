// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

Item {
    id: bar
    property var hub
    property var counts: ({ all: 0, game: 0, series: 0, movie: 0 })
    implicitHeight: 70

    RowLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.leftMargin: 24
        anchors.rightMargin: 24
        anchors.bottomMargin: 4
        spacing: 16

        Text {
            text: (i18n.language, i18n.t("hub_your_library"))
            color: Theme.t1
            font.pixelSize: 22
            font.weight: Font.DemiBold
            font.letterSpacing: -0.3
            font.family: Theme.fontSans
        }

        Row {
            Layout.leftMargin: 8
            spacing: 4
            Repeater {
                model: [ { key: "all", label: "hub_filter_all" }, { key: "game", label: "hub_kind_games" },
                         { key: "series", label: "hub_kind_series" }, { key: "movie", label: "hub_kind_movies" } ]
                delegate: Rectangle {
                    id: pill
                    required property var modelData
                    readonly property bool on: bar.hub.libraryFilter === modelData.key
                    height: 30
                    width: pillRow.implicitWidth + 26
                    radius: 15
                    color: on ? Qt.rgba(1, 1, 1, 0.08) : (pillMa.containsMouse ? Qt.rgba(1, 1, 1, 0.04) : "transparent")
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                    Row {
                        id: pillRow
                        anchors.centerIn: parent
                        spacing: 6
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: (i18n.language, i18n.t(modelData.label))
                            color: pill.on ? Theme.t1 : Theme.t3
                            font.pixelSize: 13
                            font.family: Theme.fontSans
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: bar.counts[modelData.key] || 0
                            color: Theme.stageDim
                            font.pixelSize: 11
                            font.family: Theme.fontSans
                            font.features: Theme.tnum
                        }
                    }
                    MouseArea {
                        id: pillMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bar.hub.libraryFilter = modelData.key
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        TFld {
            Layout.preferredWidth: 240
            Layout.preferredHeight: 34
            icon: "qrc:/icons/search.svg"
            clearable: true
            placeholder: (i18n.language, i18n.t("hub_search_lib"))
            onTextChanged: bar.hub.librarySearch = text
        }
        TSelect {
            Layout.preferredWidth: 130
            Layout.preferredHeight: 34
            property var keys: ["recent", "name", "size"]
            model: [i18n.t("hub_sort_recent"), i18n.t("hub_sort_name"), i18n.t("hub_sort_size")]
            onActivated: bar.hub.librarySort = keys[currentIndex]
        }
    }
}
