// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// One release beside a title's art, in two short lines: what it is and where
// it is from on the left, how healthy and how big on the right. Dense on
// purpose: the choice is between a dozen of these, not two.
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import "../theme"

Rectangle {
    id: row
    property string pill            // 4K, 1080p, CAM, v1.0.3
    property string pillTone        // "hi" (4K), "cam", ""
    property string raw
    property string meta
    property string warnBadge       // a trust warning, already translated
    property bool best: false
    property string sizeText
    property bool sizeWarn: false
    property int seedsN: 0
    property color seedColor: Theme.stageT4
    property real seedFill: 0
    property bool added: false
    property bool canWatch: false
    signal addRequested()
    signal watchRequested()
    signal openRequested()

    implicitHeight: 58
    radius: 6
    color: ma.containsMouse ? Theme.stageHover : "transparent"
    Behavior on color { ColorAnimation { duration: Theme.durFast } }
    opacity: pillTone === "cam" ? 0.7 : 1

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: row.openRequested()
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 12

        Rectangle {
            Layout.preferredWidth: 56
            Layout.minimumWidth: 56
            Layout.preferredHeight: 20
            radius: 5
            color: row.pillTone === "cam" ? Theme.sel
                 : row.pillTone === "hi" ? Qt.rgba(1, 1, 1, 0.08) : "transparent"
            border.width: 1
            border.color: row.pillTone === "cam" ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.3)
                                                 : Theme.stageHair
            Text {
                anchors.centerIn: parent
                width: parent.width - 6
                horizontalAlignment: Text.AlignHCenter
                text: row.pill
                color: row.pillTone === "cam" ? Theme.accentText : Theme.stageT1
                font.pixelSize: 11; font.weight: Font.DemiBold; font.family: Theme.fontSans
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            spacing: 3
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                component Badge: Rectangle {
                    property alias text: badgeTxt.text
                    property color ink: Theme.stageT2
                    property color fill: "transparent"
                    Layout.preferredHeight: 16
                    Layout.preferredWidth: badgeTxt.implicitWidth + 12
                    radius: 8
                    color: fill
                    Text {
                        id: badgeTxt
                        anchors.centerIn: parent
                        color: parent.ink
                        font.pixelSize: 10; font.weight: Font.DemiBold; font.family: Theme.fontSans
                    }
                }
                Badge {
                    visible: row.best
                    text: "★ " + (i18n.language, i18n.t("find_best_pick"))
                    ink: Theme.accentText
                    fill: Theme.accentTint
                }
                Badge {
                    visible: row.warnBadge.length > 0
                    text: "! " + row.warnBadge
                    ink: Theme.accentText
                    fill: Theme.sel
                }
                Text {
                    Layout.fillWidth: true
                    text: row.raw
                    color: Theme.stageRaw
                    font.pixelSize: 13
                    font.family: Theme.fontSans
                    elide: Text.ElideRight
                }
            }
            Text {
                Layout.fillWidth: true
                text: row.meta
                color: Theme.stageT4
                font.pixelSize: 11
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            Layout.preferredWidth: 76
            Layout.minimumWidth: 76
            spacing: 4
            RowLayout {
                Layout.alignment: Qt.AlignRight
                spacing: 7
                Rectangle {
                    Layout.preferredWidth: 36
                    Layout.preferredHeight: 3
                    radius: 2
                    color: Qt.rgba(1, 1, 1, 0.06)
                    Rectangle {
                        height: parent.height
                        width: parent.width * row.seedFill
                        radius: 2
                        color: row.seedColor
                    }
                }
                Text {
                    text: row.seedsN
                    color: row.seedColor
                    font.pixelSize: 12; font.weight: Font.DemiBold
                    font.family: Theme.fontSans; font.features: Theme.tnum
                }
            }
            Text {
                Layout.alignment: Qt.AlignRight
                text: row.sizeText
                color: row.sizeWarn ? Theme.warn : Theme.stageT2
                font.pixelSize: 11; font.family: Theme.fontSans; font.features: Theme.tnum
            }
        }

        Rectangle {
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            visible: row.canWatch
            radius: 7
            color: watchMa.containsMouse ? Qt.rgba(1, 1, 1, 0.1) : "transparent"
            border.width: 1
            border.color: watchMa.containsMouse ? Theme.stageT2 : Theme.stageHair
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            IconImg {
                anchors.centerIn: parent
                anchors.horizontalCenterOffset: 1
                src: "qrc:/icons/play.svg"
                tint: watchMa.containsMouse ? Theme.stageT1 : Theme.stageT2
                s: 12
            }
            MouseArea {
                id: watchMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: row.watchRequested()
            }
            ToolTip.visible: watchMa.containsMouse
            ToolTip.delay: 400
            ToolTip.text: (i18n.language, i18n.t("find_add_watch"))
        }

        Rectangle {
            id: btn
            Layout.preferredWidth: 28
            Layout.preferredHeight: 28
            radius: 7
            readonly property bool hot: btnMa.containsMouse || (ma.containsMouse && !row.added)
            color: row.added ? "transparent" : (hot ? Theme.accent : "transparent")
            border.width: 1
            border.color: row.added ? Qt.rgba(Theme.grn.r, Theme.grn.g, Theme.grn.b, 0.4)
                                    : (hot ? Theme.accent : Theme.stageHair)
            Behavior on color { ColorAnimation { duration: Theme.durFast } }
            IconImg {
                anchors.centerIn: parent
                src: row.added ? "qrc:/icons/check.svg" : "qrc:/icons/download.svg"
                tint: row.added ? Theme.grn : (btn.hot ? Theme.stageT1 : Theme.stageT2)
                s: 14
            }
            MouseArea {
                id: btnMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: row.addRequested()
            }
            ToolTip.visible: btnMa.containsMouse && row.canWatch && !row.added
            ToolTip.delay: 400
            ToolTip.text: (i18n.language, i18n.t("find_only_add"))
        }
    }
}
