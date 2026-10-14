// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A Downloads list row, 40px. Name (an episode reads "S01E01 · its TMDB
// name", the raw release name in the tooltip; a problem in grey beside it),
// size, an 18px pill with the percentage inside in the state's own solid
// colour, the state as one coloured symbol plus a grey uppercase word and the
// ETA, seeds / peers, down, up, ratio, category. Columns line up with the
// header in LibraryView through the shared widths below.
import QtQuick
import QtQuick.Layouts
import "../theme"

Rectangle {
    property var win
    property var controller
    id: lrow
    width: ListView.view.width
    height: 40

    // column widths (the header reads the same numbers)
    readonly property var cols: ({ chev: 20, size: 72, prog: 104, state: 140, peers: 76, down: 88, up: 88, ratio: 48, cat: 72 })

    required property int index
    required property string torrentName
    required property string metaTitle
    required property string stateKey
    required property real progress
    required property string stateString
    required property string stateDetail
    required property string stateDetailShort
    required property string size
    required property string downSpeed
    required property string upSpeed
    required property int downRate
    required property int upRate
    required property string category
    required property string autoCategory
    required property int numPeers
    required property int numSeeds
    required property real ratio
    required property real availability
    required property string eta
    required property string posterPath
    required property int season
    required property int episode
    required property int showTmdb
    // Episodes of one series fold into a group row (QmlLibraryModel). A group
    // adds its episodes up; an open one lists them indented under it.
    required property bool isGroup
    required property int groupCount
    required property bool groupExpanded
    required property bool groupChild

    // the "stalled why" tooltip is driven by listArea (its z:2
    // hover-exclusive MouseArea starves any in-delegate handler)
    readonly property Item stateCell: stateRow
    // Same reason: listArea checks a click against the chevron to open the group.
    readonly property Item toggleCell: lrowChevron

    readonly property bool selected: controller.isRowSelected(index)
    readonly property string k: stateKey
    readonly property bool stopped: k === "paused" || k === "queued" || k === "missing" || k === "error"
    readonly property bool trouble: k === "missing" || k === "error"
    readonly property bool slow: k === "downloading" && stateDetailShort.length > 0

    // An episode under its show reads by its code and the episode's name;
    // the release name stays in the tooltip.
    readonly property string epCode: season > 0 && episode > 0
        ? "S" + (season < 10 ? "0" : "") + season + "E" + (episode < 10 ? "0" : "") + episode
        : season > 0 ? "S" + (season < 10 ? "0" : "") + season : ""
    readonly property bool isPack: groupChild && season > 0 && episode <= 0
    readonly property string displayName: {
        if (isGroup) return metaTitle
        // the clean title when the catalogue knows it, the release name on hover
        if (!groupChild || epCode.length === 0) return metaTitle.length > 0 ? metaTitle : torrentName
        if (isPack) return epCode + " · " + (i18n.language, i18n.t("season_complete")).arg(season)
        var n = controller.episodeName(showTmdb, season, episode)
        return n.length > 0 ? epCode + " · " + n : epCode
    }
    readonly property string rawName: torrentName

    color: selected ? Theme.hover2 : (listArea.hoveredRow === index ? Theme.hover : "transparent")

    // selection: a 2px bar in the text colour, never the accent
    Rectangle {
        visible: lrow.selected
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 2
        color: Theme.t1
    }
    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }

    component Cell: Text {
        Layout.alignment: Qt.AlignVCenter
        color: Theme.t2
        font.pixelSize: 13
        font.family: Theme.fontSans
        font.features: Theme.tnum
        elide: Text.ElideRight
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 16
        anchors.rightMargin: 16
        spacing: 16

        Item {
            Layout.preferredWidth: lrow.cols.chev
            Layout.preferredHeight: 14
            IconImg {
                id: lrowChevron
                visible: lrow.isGroup
                anchors.centerIn: parent
                src: "qrc:/icons/chevron.svg"
                tint: Theme.t2
                s: 12
                rotation: lrow.groupExpanded ? 0 : -90
                Behavior on rotation { NumberAnimation { duration: Theme.durFast; easing.type: Theme.easeOut } }
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: lrow.groupChild ? 14 : 0
            spacing: 8
            Text {
                Layout.fillWidth: implicitWidth > width
                Layout.maximumWidth: implicitWidth
                Layout.preferredWidth: implicitWidth
                Layout.minimumWidth: 40
                text: lrow.displayName
                color: lrow.groupChild ? Theme.t2 : Theme.t1
                font.pixelSize: 13
                font.weight: lrow.isGroup ? Font.DemiBold : Font.Medium
                font.family: Theme.fontSans
                font.features: Theme.tnum
                elide: Text.ElideRight
            }
            Text {
                visible: text.length > 0
                text: (i18n.language, lrow.isGroup ? i18n.t("group_items_n").arg(lrow.groupCount)
                      : lrow.slow ? lrow.stateDetailShort.toLowerCase() : "")
                color: Theme.t3
                font.pixelSize: 12
                font.family: Theme.fontSans
            }
            Rectangle {
                visible: lrow.isPack
                implicitWidth: packTxt.implicitWidth + 12
                implicitHeight: 18
                radius: 5
                color: Theme.hover2
                Text {
                    id: packTxt
                    anchors.centerIn: parent
                    text: "PACK"
                    color: Theme.t2
                    font.pixelSize: 10
                    font.weight: Font.Bold
                    font.family: Theme.fontSans
                }
            }
            Item { Layout.fillWidth: true }
        }
        Cell {
            text: lrow.size
            Layout.preferredWidth: lrow.cols.size
            horizontalAlignment: Text.AlignRight
        }
        // The pill: the state's colour at full strength, the number inside.
        ProgressTrack {
            Layout.preferredWidth: lrow.cols.prog
            Layout.preferredHeight: 18
            Layout.alignment: Qt.AlignVCenter
            progress: lrow.progress
            stateKey: lrow.stateKey
            fill: lrow.k === "downloading" ? Theme.accent : Theme.fillFor(lrow.stateKey)
            sheen: lrow.downRate > 0 || lrow.upRate > 0
            showPercent: true
            percentColor: lrow.progress >= 0.6 ? Theme.inkOn(fill) : Theme.tSoft
        }
        // one coloured symbol, then the word in grey and the ETA
        Row {
            id: stateRow
            Layout.preferredWidth: lrow.cols.state
            Layout.alignment: Qt.AlignVCenter
            spacing: 5
            clip: true
            readonly property color symInk: lrow.k === "seeding" ? Theme.amber
                                          : lrow.k === "downloading" ? Theme.accent
                                          : lrow.k === "completed" ? Theme.grn : Theme.t3
            Text {
                visible: lrow.k === "seeding" || lrow.k === "downloading"
                anchors.verticalCenter: parent.verticalCenter
                width: 14
                horizontalAlignment: Text.AlignHCenter
                text: lrow.k === "seeding" ? "↑" : "↓"
                color: stateRow.symInk
                font.pixelSize: 15
                font.weight: Font.Bold
                font.family: Theme.fontSans
            }
            IconImg {
                visible: lrow.k === "paused" || lrow.k === "queued" || lrow.k === "completed" || lrow.trouble
                anchors.verticalCenter: parent.verticalCenter
                src: lrow.k === "completed" ? "qrc:/icons/check.svg"
                   : lrow.trouble ? "qrc:/icons/triangle-alert.svg" : "qrc:/icons/pause.svg"
                tint: lrow.trouble ? Theme.accentText : stateRow.symInk
                s: 14
            }
            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: lrow.stateString
                color: lrow.trouble ? Theme.accentText : (lrow.stopped || lrow.k === "fetching" ? Theme.t3 : Theme.t2)
                font.pixelSize: 11
                font.weight: Font.DemiBold
                font.letterSpacing: 0.7
                font.capitalization: Font.AllUppercase
                font.family: Theme.fontSans
            }
            Text {
                visible: lrow.k === "downloading" && lrow.eta.length > 0 && lrow.eta !== "∞"
                anchors.verticalCenter: parent.verticalCenter
                text: "· " + lrow.eta
                color: Theme.t3
                font.pixelSize: 12
                font.family: Theme.fontSans
                font.features: Theme.tnum
            }
        }
        Cell {
            text: lrow.stopped ? "" : lrow.numSeeds + " / " + lrow.numPeers
            Layout.preferredWidth: lrow.cols.peers
            horizontalAlignment: Text.AlignRight
            color: lrow.numSeeds + lrow.numPeers === 0 ? Theme.t4 : Theme.t2
        }
        // speeds stay neutral: the arrows in the header carry the direction
        Cell {
            text: lrow.downRate > 0 ? lrow.downSpeed : "—"
            Layout.preferredWidth: lrow.cols.down
            horizontalAlignment: Text.AlignRight
            color: lrow.downRate > 0 ? Theme.t2 : Theme.t4
        }
        // "0 B/s" while seeding with nobody asking; "—" when not sending at all
        Cell {
            text: lrow.upRate > 0 ? lrow.upSpeed : (lrow.k === "seeding" ? "0 B/s" : "—")
            Layout.preferredWidth: lrow.cols.up
            horizontalAlignment: Text.AlignRight
            color: lrow.upRate > 0 || lrow.k === "seeding" ? Theme.t2 : Theme.t4
        }
        Cell {
            text: lrow.ratio < 0 ? "∞" : lrow.ratio.toFixed(2)
            Layout.preferredWidth: lrow.cols.ratio
            horizontalAlignment: Text.AlignRight
            color: lrow.ratio === 0 ? Theme.t4 : Theme.t3
        }
        Cell {
            text: win.catLabel(lrow.category.length > 0 ? lrow.category : lrow.autoCategory)
            Layout.preferredWidth: lrow.cols.cat
            color: Theme.t3
        }
    }
}
