// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Above a stage's release list: quality as one segmented control with counts,
// then the yes/no filters that matter before downloading. A toggle with
// nothing to act on (no CAM in the list, nothing in your language) is not shown.
import QtQuick
import "../theme"

Flow {
    id: bar
    property var qualities: []      // [{ value, count }], value "" = all
    property string quality: ""
    property bool showCam: false
    property bool hideCam: true
    property bool showLang: false
    property bool myLang: false
    property bool showFits: true
    property bool fits: false
    signal qualityPicked(string value)
    signal camToggled()
    signal langToggled()
    signal fitsToggled()

    spacing: 6

    Rectangle {
        visible: bar.qualities.length > 2
        width: seg.width + 4
        height: 30
        radius: 15
        color: Theme.stageBg
        border.width: 1
        border.color: Theme.stageHair
        Row {
            id: seg
            anchors.centerIn: parent
            spacing: 2
            Repeater {
                model: bar.qualities
                Rectangle {
                    id: chip
                    required property var modelData
                    readonly property bool on: bar.quality === modelData.value
                    width: chipRow.implicitWidth + 20
                    height: 26
                    radius: 13
                    color: on ? Theme.accentTint : (chipMa.containsMouse ? Theme.stageHover : "transparent")
                    Behavior on color { ColorAnimation { duration: Theme.durFast } }
                    Row {
                        id: chipRow
                        anchors.centerIn: parent
                        spacing: 5
                        Text {
                            id: chipLabel
                            text: chip.modelData.value === "" ? (i18n.language, i18n.t("find_all")) : chip.modelData.value
                            color: chip.on ? Theme.accentText : Theme.stageT2
                            font.pixelSize: 12
                            font.weight: chip.on ? Font.Medium : Font.Normal
                            font.family: Theme.fontSans
                        }
                        Text {
                            anchors.baseline: chipLabel.baseline
                            text: chip.modelData.count
                            color: chip.on ? Theme.accentText : Theme.stageT4
                            font.pixelSize: 10; font.family: Theme.fontSans; font.features: Theme.tnum
                        }
                    }
                    MouseArea {
                        id: chipMa
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: bar.qualityPicked(chip.modelData.value)
                    }
                }
            }
        }
    }

    component Toggle: Rectangle {
        id: tog
        property string label
        property bool on: false
        signal toggled()
        width: togRow.implicitWidth + 22
        height: 30
        radius: 15
        color: on ? Theme.accentTint : (togMa.containsMouse ? Theme.stageHover : "transparent")
        border.width: 1
        border.color: on ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.35) : Theme.stageHair
        Behavior on color { ColorAnimation { duration: Theme.durFast } }
        Row {
            id: togRow
            anchors.centerIn: parent
            spacing: 7
            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                width: 12; height: 12; radius: 4
                color: tog.on ? Theme.accent : "transparent"
                border.width: 1
                border.color: tog.on ? Theme.accent : Qt.rgba(1, 1, 1, 0.2)
                IconImg {
                    anchors.centerIn: parent
                    visible: tog.on
                    src: "qrc:/icons/check.svg"; tint: Theme.inkOn(Theme.accent); s: 9
                }
            }
            Text {
                text: tog.label
                color: tog.on ? Theme.stageT1 : Theme.stageT2
                font.pixelSize: 12; font.family: Theme.fontSans
            }
        }
        MouseArea {
            id: togMa
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: tog.toggled()
        }
    }

    Toggle {
        visible: bar.showCam
        label: (i18n.language, i18n.t("find_hide_cam"))
        on: bar.hideCam
        onToggled: bar.camToggled()
    }
    Toggle {
        visible: bar.showLang
        label: (i18n.language, i18n.t("find_my_language"))
        on: bar.myLang
        onToggled: bar.langToggled()
    }
    Toggle {
        visible: bar.showFits
        label: (i18n.language, i18n.t("find_fits_disk"))
        on: bar.fits
        onToggled: bar.fitsToggled()
    }
}
