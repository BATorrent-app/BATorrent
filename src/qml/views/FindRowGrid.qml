// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// "See all" for one browse shelf: the same posters, wrapped into a grid instead
// of a row you have to scroll sideways through.
//
// It used to set the page's type filter, which is what the chips at the top
// already do — so on a page whose shelves are all movies, clicking See all on a
// movie row changed a chip and nothing else, and read as a dead control.
import QtQuick
import QtQuick.Controls.Basic
import "../theme"
import "../widgets"

Item {
    id: rowGrid
    property var sv
    property string label: ""
    property var items: []
    signal backRequested()
    signal activated(var item)
    signal getWatch(var item)

    Item {
        id: header
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        height: 52

        Rectangle {
            id: backBtn
            anchors.left: parent.left
            anchors.leftMargin: Theme.sp5
            anchors.verticalCenter: parent.verticalCenter
            width: 30; height: 30; radius: 8
            color: backMa.containsMouse ? Theme.hover : "transparent"
            IconImg {
                anchors.centerIn: parent
                s: 17
                src: "qrc:/icons/chevron-bold.svg"
                rotation: 90                     // the chevron's base points down
                tint: backMa.containsMouse ? Theme.t1 : Theme.t3
            }
            MouseArea {
                id: backMa
                anchors.fill: parent
                hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                onClicked: rowGrid.backRequested()
            }
        }
        Text {
            anchors.left: backBtn.right
            anchors.leftMargin: Theme.sp3
            anchors.verticalCenter: parent.verticalCenter
            text: rowGrid.label
            color: Theme.t1
            font.pixelSize: 17; font.weight: Font.Bold; font.family: Theme.fontSans
        }
        Text {
            anchors.right: parent.right
            anchors.rightMargin: Theme.sp5
            anchors.verticalCenter: parent.verticalCenter
            text: rowGrid.items ? rowGrid.items.length : 0
            color: Theme.t4
            font.pixelSize: 13; font.family: Theme.fontSans; font.features: Theme.tnum
        }
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
    }

    GridView {
        id: grid
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.sp5
        clip: true
        cellWidth: 166
        cellHeight: 268
        model: rowGrid.items
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: PosterCard {
            id: pcard
            required property var modelData
            posterW: 150
            title: pcard.modelData.title || ""
            poster: pcard.modelData.poster || ""
            year: pcard.modelData.year || ""
            rating: pcard.modelData.rating || 0
            type: pcard.modelData.type || ""
            synopsis: pcard.modelData.overview || ""
            watchlistEnabled: typeof session !== "undefined"
            saved: typeof session !== "undefined"
                   && (session.watchlist, session.inWatchlist(pcard.modelData.title || "", pcard.modelData.type || ""))
            onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                title: pcard.modelData.title || "", type: pcard.modelData.type || "",
                poster: pcard.modelData.poster || "", year: pcard.modelData.year || "" })
            onActivated: rowGrid.activated(pcard.modelData)
            onGetWatch: rowGrid.getWatch(pcard.modelData)
        }
    }
}
