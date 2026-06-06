// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Left navigation rail for the 4.0 hub. Switches the main content stack between
// Downloads / Discover / Search / HUB; Settings (bottom) opens its window.
// Animated: selection accent bar, hover/active color fades.
import QtQuick
import QtQuick.Layouts
import QtQuick.Effects
import "theme"
import "widgets"

Rectangle {
    id: rail
    implicitWidth: 188
    color: Theme.elev

    property int currentIndex: 0
    property bool discoverVisible: true     // gated off in store builds (step ⑦)
    signal settingsClicked()

    // right hairline
    Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }

    readonly property var items: [
        { icon: "qrc:/icons/download.svg", label: "Downloads", page: 0 },
        { icon: "qrc:/icons/grid.svg",     label: "Discover",  page: 1 },
        { icon: "qrc:/icons/search.svg",   label: "Search",    page: 2 },
        { icon: "qrc:/icons/play.svg",     label: "HUB",       page: 3 }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 2

        // ----- brand header -----
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            Image {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left; anchors.leftMargin: 18
                width: 28; height: 28
                source: "qrc:/images/logo.svg"
                sourceSize: Qt.size(56, 56)
                fillMode: Image.PreserveAspectFit
                layer.enabled: Theme.isLight
                layer.effect: MultiEffect { colorization: 1.0; colorizationColor: Theme.t1 }
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left; anchors.leftMargin: 54
                text: "BATorrent"
                color: Theme.t1
                font.pixelSize: 16; font.weight: Font.Bold; font.family: Theme.fontSans
            }
        }

        Item { Layout.preferredHeight: 6 }

        // ----- nav items -----
        Repeater {
            model: rail.items
            delegate: Item {
                id: navItem
                required property var modelData
                readonly property bool active: rail.currentIndex === modelData.page
                visible: !(modelData.page === 1 && !rail.discoverVisible)
                Layout.fillWidth: true
                Layout.preferredHeight: visible ? 46 : 0
                Layout.leftMargin: 10
                Layout.rightMargin: 10

                Rectangle {
                    anchors.fill: parent
                    radius: 10
                    color: navItem.active ? Theme.hover
                         : (itemMa.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent")
                    Behavior on color { ColorAnimation { duration: 140 } }

                    // animated active accent bar
                    Rectangle {
                        anchors.left: parent.left; anchors.leftMargin: 3
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: navItem.active ? 22 : 0
                        radius: 2
                        color: Theme.accent
                        Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.OutBack } }
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 17
                    anchors.rightMargin: 12
                    spacing: 13
                    IconImg {
                        Layout.alignment: Qt.AlignVCenter
                        src: navItem.modelData.icon
                        tint: navItem.active ? Theme.t1 : Theme.t3
                        s: 18
                        Behavior on tint { ColorAnimation { duration: 140 } }
                    }
                    Text {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignVCenter
                        text: navItem.modelData.label
                        color: navItem.active ? Theme.t1 : Theme.t2
                        font.pixelSize: 14
                        font.weight: navItem.active ? Font.DemiBold : Font.Medium
                        font.family: Theme.fontSans
                        Behavior on color { ColorAnimation { duration: 140 } }
                    }
                }
                MouseArea {
                    id: itemMa
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: rail.currentIndex = navItem.modelData.page
                }
            }
        }

        Item { Layout.fillHeight: true }   // push Settings to the bottom

        // ----- settings (bottom) -----
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            Layout.leftMargin: 10
            Layout.rightMargin: 10
            Layout.bottomMargin: 12
            Rectangle {
                anchors.fill: parent
                radius: 10
                color: setMa.containsMouse ? Qt.rgba(1, 1, 1, 0.05) : "transparent"
                Behavior on color { ColorAnimation { duration: 140 } }
            }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 17
                anchors.rightMargin: 12
                spacing: 13
                IconImg { Layout.alignment: Qt.AlignVCenter; src: "qrc:/icons/settings.svg"; tint: Theme.t3; s: 18 }
                Text {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    text: "Settings"
                    color: Theme.t2
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: Theme.fontSans
                }
            }
            MouseArea {
                id: setMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: rail.settingsClicked()
            }
        }
    }
}
