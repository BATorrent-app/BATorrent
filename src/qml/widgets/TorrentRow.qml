// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    property var win
    property var controller
    id: lrow
    width: ListView.view.width
    height: 56

    required property int index
    required property string torrentName
    required property string metaTitle
    required property string stateKey
    required property real progress
    required property string stateString
    required property string stateDetail
    required property string size
    required property string downSpeed
    required property string upSpeed
    required property int downRate
    required property int upRate
    required property string category
    required property int numPeers
    required property int numSeeds
    required property real ratio
    required property real availability
    required property string eta
    required property string posterPath
    // Episodes of one series fold into a group row (QmlLibraryModel). A group
    // adds its episodes up; an open one lists them indented under it.
    required property bool isGroup
    required property int groupCount
    required property bool groupExpanded
    required property bool groupChild

    readonly property string posterUrl: win.fileUrl(posterPath)
    // the "stalled why" tooltip is driven by listArea (its z:2
    // hover-exclusive MouseArea starves any in-delegate handler)
    readonly property Item stateCell: lrowStateText
    // Same reason: listArea checks a click against the chevron to open the group.
    readonly property Item toggleCell: lrowChevron

    color: controller.isRowSelected(index) ? Theme.sel : (listArea.hoveredRow === index ? Theme.hover : "transparent")

    // .sel inset 2px barra esquerda: t1 (not accent), so it doesn't blend
    // into a red downloading progress bar in the same row
    Rectangle {
        visible: controller.isRowSelected(lrow.index)
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2
        color: Theme.t1
    }
    // border-bottom hairSoft
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp4
        spacing: Theme.sp4

        // classic = cover-less, dense qBittorrent-style row (this ListView is
        // only ever shown in classic mode now)
        RowLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            spacing: 6
            // An episode lines its name up past the group's chevron.
            Item {
                visible: lrow.groupChild
                Layout.preferredWidth: 14
                Layout.preferredHeight: 1
            }
            Item {
                id: lrowChevron
                visible: lrow.isGroup
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                Layout.alignment: Qt.AlignVCenter
                IconImg {
                    anchors.centerIn: parent
                    src: "qrc:/icons/chevron.svg"
                    tint: Theme.t2
                    s: 12
                    rotation: lrow.groupExpanded ? 0 : -90
                    Behavior on rotation { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
                }
            }
            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: lrow.isGroup ? lrow.metaTitle : lrow.torrentName
                color: lrow.groupChild ? Theme.t2 : Theme.t1
                font.pixelSize: 13
                font.weight: lrow.isGroup ? Font.DemiBold : Font.Normal
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }
            Text {
                visible: lrow.isGroup
                Layout.alignment: Qt.AlignVCenter
                text: (i18n.language, i18n.t("search_episodes_n")).arg(lrow.groupCount)
                color: Theme.t3
                font.pixelSize: 12
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
        }
        Text {
            text: lrow.size
            Layout.preferredWidth: 72
            horizontalAlignment: Text.AlignRight
            color: Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        // The shared bar, whole: same geometry, same fill, same travelling
        // indicator for the trouble states, and the same seeding sheen the grid
        // has always had — the one thing this row used to be missing. The fill
        // stays at 30% (a solid block screamed over 16 rows) but the percentage
        // on top of it does not, which is why the dimming is the bar's own.
        ProgressTrack {
            Layout.preferredWidth: 96
            Layout.preferredHeight: 18
            Layout.alignment: Qt.AlignVCenter
            progress: lrow.progress
            stateKey: lrow.stateKey
            sheen: (lrow.stateKey === "seeding" && lrow.upRate > 0)
                   || (lrow.stateKey === "downloading" && lrow.downRate > 0)
            fillOpacity: 0.30
            showPercent: true
            percentColor: win.textFor(lrow.stateKey)
        }
        StatusMark {
            id: lrowStateText
            Layout.preferredWidth: 104
            Layout.alignment: Qt.AlignVCenter
            stateKey: lrow.stateKey
            label: lrow.stateString
            stalled: lrow.stateKey === "downloading" && lrow.stateDetail.length > 0
            symbolSize: 14
            labelSize: 12
            maxLabelWidth: 104 - symbolSize - spacing
        }
        Text {
            text: lrow.numSeeds
            Layout.preferredWidth: 44
            horizontalAlignment: Text.AlignRight
            color: lrow.numSeeds === 0 ? Theme.t4 : Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            text: lrow.numPeers
            Layout.preferredWidth: 44
            horizontalAlignment: Text.AlignRight
            color: lrow.numPeers === 0 ? Theme.t4 : Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        // speeds stay neutral: the state column is the one colored cell per
        // row; a red/amber column per direction read as 16 rows of alarm
        Text {
            text: lrow.downRate > 0 ? lrow.downSpeed : "—"
            Layout.preferredWidth: 74
            horizontalAlignment: Text.AlignRight
            color: lrow.downRate > 0 ? Theme.t2 : Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            text: lrow.upRate > 0 ? lrow.upSpeed : "—"
            Layout.preferredWidth: 74
            horizontalAlignment: Text.AlignRight
            color: lrow.upRate > 0 ? Theme.t2 : Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            // ratio: green at/above 1.0 (or ∞); the seed-health at a glance
            text: lrow.ratio < 0 ? "∞" : lrow.ratio.toFixed(2)
            Layout.preferredWidth: 50
            horizontalAlignment: Text.AlignRight
            color: (lrow.ratio < 0 || lrow.ratio >= 1.0) ? Theme.grn : (lrow.ratio === 0 ? Theme.t4 : Theme.t3)
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            text: lrow.eta.length > 0 ? lrow.eta : "—"
            Layout.preferredWidth: 66
            horizontalAlignment: Text.AlignRight
            color: Theme.t3
            font.pixelSize: 12
            font.family: Theme.fontSans
            font.features: Theme.tnum
        }
        Text {
            text: win.catLabel(lrow.category)
            Layout.preferredWidth: 84
            color: Theme.hasAnime ? Theme.t1 : Theme.t3
            style: Theme.hasAnime ? Text.Outline : Text.Normal
            styleColor: Theme.isLight ? "#ffffff" : "#000000"
            font.pixelSize: 12
            font.weight: Theme.hasAnime ? Font.Medium : Font.Normal
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
    }
}
