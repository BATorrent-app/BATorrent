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
    required property string posterPath

    readonly property string posterUrl: win.fileUrl(posterPath)
    // the "stalled why" tooltip is driven by listArea (its z:2
    // hover-exclusive MouseArea starves any in-delegate handler)
    readonly property Item stateCell: lrowStateText

    color: win.isRowSelected(index) ? Theme.sel : (listArea.hoveredRow === index ? Theme.hover : "transparent")

    // .sel inset 2px barra esquerda
    Rectangle {
        visible: win.isRowSelected(lrow.index)
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2
        color: Theme.accent
    }
    // border-bottom hairSoft
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.sp4
        anchors.rightMargin: Theme.sp4
        spacing: Theme.sp4

        // .name: poster thumb + nome
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.sp3
            PosterThumb {
                Layout.alignment: Qt.AlignVCenter
                posterUrl: lrow.posterUrl
                label: lrow.metaTitle || lrow.torrentName || ""
            }
            Text {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
                text: lrow.metaTitle || lrow.torrentName
                color: Theme.t1
                font.pixelSize: 13
                font.family: Theme.fontSans
                elide: Text.ElideRight
            }
        }
        Text {
            text: lrow.size
            Layout.preferredWidth: 78
            horizontalAlignment: Text.AlignRight
            color: Theme.t2
            font.pixelSize: 12
            font.family: Theme.fontMono
        }
        // progress: surfaceAlt track, state-colored fill, centered % (white over fill, t1 over track)
        Item {
            Layout.preferredWidth: 104
            Layout.preferredHeight: 18
            Rectangle {
                id: pbarTrack
                anchors.fill: parent
                radius: 4
                color: Theme.field
                clip: true
                Rectangle {
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: Math.max(lrow.progress > 0.001 ? 2 : 0, parent.width * lrow.progress)
                    radius: 4
                    color: win.fillFor(lrow.stateKey)
                }
                Text {
                    id: pbarPct
                    anchors.centerIn: parent
                    text: (lrow.progress * 100).toFixed(1) + "%"
                    color: (parent.width / 2) < (parent.width * lrow.progress - 4) ? "#ffffff" : Theme.t1
                    font.pixelSize: 9
                    font.weight: Font.DemiBold
                    font.family: Theme.fontSans
                }
            }
        }
        Text {
            text: lrow.downRate > 0 ? lrow.downSpeed : "—"
            Layout.preferredWidth: 78
            horizontalAlignment: Text.AlignRight
            color: lrow.downRate > 0 ? Theme.accentText : Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontMono
        }
        Text {
            text: lrow.upRate > 0 ? lrow.upSpeed : "—"
            Layout.preferredWidth: 78
            horizontalAlignment: Text.AlignRight
            color: lrow.upRate > 0 ? Theme.up : Theme.t4
            font.pixelSize: 12
            font.family: Theme.fontMono
        }
        Text {
            id: lrowStateText
            text: lrow.stateString
            Layout.preferredWidth: 110
            color: win.textFor(lrow.stateKey)
            font.pixelSize: 12
            font.weight: Theme.hasAnime ? Font.DemiBold : Font.Medium
            font.family: Theme.fontSans
        }
        Text {
            text: win.catLabel(lrow.category)
            Layout.preferredWidth: 90
            color: Theme.hasAnime ? Theme.t1 : Theme.t3
            style: Theme.hasAnime ? Text.Outline : Text.Normal
            styleColor: Theme.isLight ? "#ffffff" : "#000000"
            font.pixelSize: 12
            font.weight: Theme.hasAnime ? Font.Medium : Font.Normal
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
        Text {
            text: lrow.numPeers
            Layout.preferredWidth: 56
            horizontalAlignment: Text.AlignRight
            color: lrow.numPeers === 0 ? (Theme.hasAnime ? Theme.t2 : Theme.t4) : (Theme.hasAnime ? Theme.t1 : Theme.t2)
            style: Theme.hasAnime ? Text.Outline : Text.Normal
            styleColor: Theme.isLight ? "#ffffff" : "#000000"
            font.pixelSize: 12
            font.weight: Theme.hasAnime ? Font.Medium : Font.Normal
            font.family: Theme.fontMono
        }
    }
}
