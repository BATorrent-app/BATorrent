// SPDX-License-Identifier: MIT
// The library's grid tile and list row with a series group in the model: a
// group row, one of its episodes and a plain torrent all build, the group
// shows its chevron, and the episode-count chip on a group tile opens it.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 900
    height: 700

    // TorrentRow reads the list's hover state from here, as it does from
    // LibraryView's overlay.
    Item { id: listArea; property int hoveredRow: -1 }

    QtObject {
        id: winStub
        function fileUrl(p) { return "" }
        function fmtEta(s) { return "" }
        function textFor(k) { return "#ffffff" }
        function catLabel(c) { return c }
        function openContext(r) {}
        function openTorrentPage(h) { return false }
    }
    QtObject {
        id: ctrlStub
        property var selectedRows: []
        property var toggled: []
        function isRowSelected(r) { return selectedRows.indexOf(r) >= 0 }
        function selectRow(r, m) { selectedRows = [r] }
        function toggleGroup(r) {
            if (r !== 0) return false
            toggled = toggled.concat([r])
            return true
        }

    }

    function row(name, group, count, open, child) {
        return {
            torrentName: name, metaTitle: name, stateKey: "downloading", progress: 0.5,
            posterPath: "", stateString: "Downloading", stateDetail: "", stateDetailShort: "",
            fileKind: "MKV", category: "", size: "1 GB", downSpeed: "0 B/s", upSpeed: "0 B/s",
            downRate: 0, upRate: 0, sizeBytes: 1000, infoHash: "h-" + name, playable: false,
            downloaded: "0 B", year: 0, genres: "", queuePos: 0, numPeers: 0, numSeeds: 0,
            ratio: 0, availability: 0, eta: "", isGroup: group, groupCount: count,
            groupExpanded: open, groupChild: child
        }
    }

    ListModel { id: rows }

    GridView {
        id: grid
        width: 900; height: 320
        cellWidth: 194; cellHeight: 286
        model: rows
        delegate: PosterTile { win: winStub; controller: ctrlStub }
    }
    ListView {
        id: list
        y: 340
        width: 900; height: 300
        model: rows
        delegate: TorrentRow { win: winStub; controller: ctrlStub }
    }

    TestCase {
        name: "LibraryGroupRows"
        when: windowShown

        function init() {
            rows.clear()
            rows.append(row("Bleach", true, 2, true, false))
            rows.append(row("Bleach S01E01", false, 0, false, true))
            rows.append(row("Dune", false, 0, false, false))
            ctrlStub.toggled = []
        }

        function test_rowsBuildForGroupEpisodeAndPlainTorrent() {
            tryVerify(function() { return grid.itemAtIndex(2) !== null && list.itemAtIndex(2) !== null })
            verify(grid.itemAtIndex(0).isGroup)
            verify(grid.itemAtIndex(1).groupChild)
            verify(!grid.itemAtIndex(2).isGroup)
            verify(list.itemAtIndex(0).toggleCell.visible, "a group row shows its chevron")
            verify(!list.itemAtIndex(2).toggleCell.visible)
        }

        function test_chipOnGroupTileSelectsTheGroup() {
            tryVerify(function() { return grid.itemAtIndex(0) !== null })
            var tile = grid.itemAtIndex(0)
            waitForRendering(grid)
            mouseClick(tile, 24, 18)
            // the episodes are listed in the inspector, so the chip selects
            // the group rather than expanding it into tiles
            compare(ctrlStub.toggled.length, 0)
            compare(ctrlStub.selectedRows, [0])
        }
    }
}
