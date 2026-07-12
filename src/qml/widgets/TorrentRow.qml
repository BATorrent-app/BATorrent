// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    property var win
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

    readonly property string posterUrl: win.fileUrl(posterPath)
    // the "stalled why" tooltip is driven by listArea (its z:2
    // hover-exclusive MouseArea starves any in-delegate handler)
    readonly property Item stateCell: lrowStateText

    color: win.isRowSelected(index) ? Theme.sel : (listArea.hoveredRow === index ? Theme.hover : "transparent")

    // .sel inset 2px barra esquerda — t1 (not accent), so it doesn't blend
    // into a red downloading progress bar in the same row
    Rectangle {
        visible: win.isRowSelected(lrow.index)
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
        Text {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignVCenter
            text: lrow.torrentName
            color: Theme.t1
            font.pixelSize: 13
            font.family: Theme.fontSans
            elide: Text.ElideRight
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
        // progress: field track, state-tinted fill (a solid block screamed
        // over 16 rows), state-colored integer %
        Item {
            Layout.preferredWidth: 96
            Layout.preferredHeight: 18
            Rectangle {
                id: pbarTrack
                anchors.fill: parent
                radius: 9
                color: Theme.field
                clip: true
                Rectangle {
                    readonly property color fc: win.fillFor(lrow.stateKey)
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: Math.max(lrow.progress > 0.001 ? 2 : 0, parent.width * lrow.progress)
                    radius: 9
                    color: Qt.rgba(fc.r, fc.g, fc.b, 0.30)
                }
                Text {
                    id: pbarPct
                    anchors.centerIn: parent
                    text: Math.floor(lrow.progress * 100) + "%"
                    color: win.textFor(lrow.stateKey)
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                    font.features: Theme.tnum
                }
            }
        }
        Text {
            id: lrowStateText
            text: lrow.stateString
            Layout.preferredWidth: 104
            color: win.textFor(lrow.stateKey)
            font.pixelSize: 12
            font.weight: Theme.hasAnime ? Font.DemiBold : Font.Medium
            font.family: Theme.fontSans
            elide: Text.ElideRight
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
        // speeds stay neutral — the state column is the one colored cell per
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
            // ratio: green at/above 1.0 (or ∞) — the seed-health at a glance
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
