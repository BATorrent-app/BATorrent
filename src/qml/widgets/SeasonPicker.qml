// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Prev · Season N · Next, with a menu for the jump. A row of chips reads fine
// for four seasons and falls apart at twenty, which is the length of show this
// has to survive.
//
// Each entry carries how many of its episodes are actually downloaded, so the
// season worth opening is obvious before it is opened.
import QtQuick
import QtQuick.Layouts
import "../theme"

Item {
    id: picker
    property var seasons: []
    property int season: -1
    // season -> { have, total }. What "have" means is the caller's business:
    // on disk in the library, offered by an indexer in a search.
    property var counts: ({})
    signal seasonPicked(int season)

    readonly property int pos: seasons.indexOf(season)
    implicitHeight: 54

    function label(s) {
        // Season 0 is where TMDB keeps specials; calling it "Season 0" is
        // technically right and reads like a bug.
        return s === 0 ? (i18n.language, i18n.t("hub_specials"))
                       : (i18n.language, i18n.t("hub_season_n")).arg(s)
    }
    function tally(s) {
        var c = picker.counts[s]
        return c ? (c.have + "/" + c.total) : ""
    }

    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp3
        anchors.rightMargin: Theme.sp3
        spacing: Theme.sp2

        component Arrow: Item {
            property string glyph
            property bool canGo: false
            signal go()
            Layout.preferredWidth: 30
            Layout.preferredHeight: 30
            opacity: canGo ? 1 : 0.3
            IconImg {
                anchors.centerIn: parent
                src: "qrc:/icons/chevron-bold.svg"
                // chevron-bold points up at rest, so left and right are 270
                // and 90 — not 180 and 0, which gave up and down.
                rotation: parent.glyph === "prev" ? 270 : 90
                tint: arrowMa.containsMouse && parent.canGo ? Theme.t1 : Theme.t3
                s: 16
            }
            MouseArea {
                id: arrowMa
                anchors.fill: parent
                hoverEnabled: parent.canGo
                enabled: parent.canGo
                cursorShape: Qt.PointingHandCursor
                onClicked: parent.go()
            }
        }

        Arrow {
            glyph: "prev"
            canGo: picker.pos > 0
            onGo: picker.seasonPicked(picker.seasons[picker.pos - 1])
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 32
            Rectangle {
                anchors.fill: parent
                radius: 8
                color: menuMa.containsMouse ? Theme.hover : "transparent"
                Behavior on color { ColorAnimation { duration: Theme.durFast } }
            }
            RowLayout {
                anchors.centerIn: parent
                spacing: 6
                Text {
                    text: picker.season >= 0 ? picker.label(picker.season) : ""
                    color: Theme.t1
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    font.family: Theme.fontSans
                }
                IconImg {
                    src: "qrc:/icons/chevron-bold.svg"
                    rotation: 180          // down: it opens a menu below
                    tint: Theme.t3
                    s: 13
                }
            }
            MouseArea {
                id: menuMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: seasonMenu.popup()
            }
        }

        Arrow {
            glyph: "next"
            canGo: picker.pos >= 0 && picker.pos < picker.seasons.length - 1
            onGo: picker.seasonPicked(picker.seasons[picker.pos + 1])
        }
    }

    BatMenu {
        id: seasonMenu
        implicitWidth: 240
        Repeater {
            model: picker.seasons
            BatMenuItem {
                required property var modelData
                text: picker.label(modelData) + "   " + picker.tally(modelData)
                onTriggered: picker.seasonPicked(modelData)
            }
        }
    }
}
