// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One release, read the way a person picks one: the quality as the only chip,
// a name made from what it is ("BluRay · 5.1", the raw name in the tooltip),
// the language line with the person's own language strong, then size and
// seeds on the right. Seeds are a neutral bar; under 30 they turn orange and
// say "few seeds". A release that doesn't fit says how much is missing.
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Rectangle {
    id: row
    property string quality
    property string name
    property string raw
    property bool recommended: false
    property var langBits: []          // [{text, mine}]
    property string extra              // "8 episodes", after the language line
    property string sizeText
    property int seedsN: 0
    property real seedFill: 0
    property bool fewSeeds: false
    property string shortBy: ""        // "4,1 GB" when it doesn't fit
    property bool canWatch: false
    property bool added: false
    signal watchRequested()
    signal addRequested()
    signal openRequested()

    implicitHeight: 56
    radius: Theme.rad(8)
    color: ma.containsMouse ? Theme.stageHover : "transparent"
    // Recommended is said once, in grey after the name: no ring as well

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: row.openRequested()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12; anchors.rightMargin: 12
        spacing: 12

        // the only chip on the row
        Rectangle {
            Layout.preferredWidth: 52; Layout.preferredHeight: 26
            radius: Theme.rad(6)
            color: "transparent"
            border.width: 1; border.color: Theme.stageEdge
            Text {
                anchors.centerIn: parent
                text: row.quality
                color: Theme.stageT1
                font.pixelSize: 12; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            Text {
                Layout.fillWidth: true
                textFormat: Text.StyledText
                elide: Text.ElideRight
                font.pixelSize: 14; font.family: Theme.fontSans
                color: Theme.stageT1
                text: "<b>" + row.esc(row.name) + "</b>"
                      + (row.recommended ? "<font color='" + Theme.stageMuted + "'>  ·  " + (i18n.language, i18n.t("find_rel_recommended")) + "</font>" : "")
                ToolTip.visible: nameMa.containsMouse && row.raw.length > 0
                ToolTip.delay: 500
                ToolTip.text: row.raw
                MouseArea { id: nameMa; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
            }
            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                textFormat: Text.StyledText
                elide: Text.ElideRight
                font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
                color: Theme.stageMuted
                text: {
                    var parts = []
                    for (var i = 0; i < row.langBits.length; i++) {
                        var b = row.langBits[i]
                        parts.push(b.mine ? "<b><font color='" + Theme.stageT1 + "'>" + row.esc(b.text) + "</font></b>" : row.esc(b.text))
                    }
                    if (row.extra.length > 0) parts.push(row.esc(row.extra))
                    return parts.join("  ·  ")
                }
            }
        }

        // size, and how it is seeded (or how much it is short of the disk)
        ColumnLayout {
            Layout.alignment: Qt.AlignVCenter
            spacing: 4
            Text {
                Layout.alignment: Qt.AlignRight
                text: row.sizeText
                color: Theme.stageT1
                font.pixelSize: 14; font.weight: Font.DemiBold; font.family: Theme.fontSans; font.features: Theme.tnum
            }
            Row {
                Layout.alignment: Qt.AlignRight
                spacing: 7
                Rectangle {
                    visible: row.shortBy.length === 0
                    anchors.verticalCenter: parent.verticalCenter
                    width: 34; height: 2; radius: 1
                    color: Theme.stageHair
                    Rectangle {
                        width: Math.max(2, parent.width * row.seedFill); height: parent.height; radius: 1
                        color: row.fewSeeds ? Theme.warn : Theme.stageT1
                    }
                }
                Text {
                    text: {
                        i18n.language
                        if (row.shortBy.length > 0) return i18n.t("find_rel_short").arg(row.shortBy)
                        return row.fewSeeds ? i18n.t("find_rel_few_seeds").arg(row.seedsN)
                                            : i18n.t("find_rel_seeds").arg(row.seedsN)
                    }
                    color: row.shortBy.length > 0 || row.fewSeeds ? Theme.warn : Theme.stageMuted
                    font.pixelSize: 12; font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
        }

        RoundBtn {
            visible: row.canWatch
            icon: "qrc:/icons/play.svg"
            tip: (i18n.language, i18n.t("find_add_watch"))
            onClicked: row.watchRequested()
        }
        RoundBtn {
            icon: row.added ? "qrc:/icons/check.svg" : "qrc:/icons/download.svg"
            tint: row.added ? Theme.grn : Theme.stageT1
            tip: row.added ? "" : (i18n.language, i18n.t("find_only_add"))
            onClicked: row.addRequested()
        }
    }

    function esc(s) { return String(s || "").replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;") }

    // play and download: outlined, never red
    component RoundBtn: Rectangle {
        id: rb
        property string icon
        property color tint: Theme.stageT1
        property string tip
        signal clicked()
        Layout.preferredWidth: 32; Layout.preferredHeight: 32
        radius: Theme.rad(6)
        color: rbMa.containsMouse ? Theme.stageHover : "transparent"
        border.width: 1
        border.color: rbMa.containsMouse ? Theme.stageT2 : Theme.stageEdge
        IconImg { anchors.centerIn: parent; src: rb.icon; tint: rb.tint; s: 14 }
        MouseArea { id: rbMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: rb.clicked() }
        ToolTip.visible: rbMa.containsMouse && rb.tip.length > 0
        ToolTip.delay: 400
        ToolTip.text: rb.tip
    }
}
