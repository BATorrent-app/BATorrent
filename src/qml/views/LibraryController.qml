// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

// Selection + filter state for the downloads library. Instantiated as a child
// of Main's Window (id: library). Leaf chrome takes it as `controller`;
// no win.* hunting for selection. Scroll/focus side-effects go through signals.
QtObject {
    id: root

    property int selected: -1
    property var selectedRows: []
    property int anchorRow: -1
    property bool gridView: true
    property bool classicMode: false
    onClassicModeChanged: if (typeof settings !== "undefined") settings.set("classicMode", classicMode)
    property string activeFilter: "all"
    property string catFilter: ""
    property string sortColumn: ""
    property bool sortAsc: true

    // The same selection by row key (rowModel.rowKey). Row numbers go
    // stale when a series group opens or closes or the sort moves rows; the
    // keys don't, so the selection follows its rows.
    property var selectedKeys: []
    property string selectedKey: ""
    property string anchorKey: ""
    // The library model (QmlLibraryModel); a property so a test can hand in
    // its own.
    property var rowModel: typeof torrentFilter !== "undefined" ? torrentFilter : null
    readonly property bool keyedRows: !!root.rowModel && typeof root.rowModel.rowForKey === "function"

    property Connections rowFollower: Connections {
        target: root.keyedRows ? root.rowModel : null
        ignoreUnknownSignals: true
        function onStructureChanged() { root.followRows() }
    }

    // Ask Main to clear the filter search focus / release the accent ring.
    signal clearFilterFocusRequested()
    // Ask Main to scroll the visible grid/list to this proxy row.
    signal scrollToRowRequested(int proxyRow)
    // Ask Main to drop search-field focus after a row click.
    signal releaseSearchFocusRequested()

    function commitSel() {
        if (root.keyedRows) {
            var keys = []
            for (var k = 0; k < root.selectedRows.length; ++k)
                keys.push(root.rowModel.rowKey(root.selectedRows[k]))
            root.selectedKeys = keys
            root.selectedKey = root.rowModel.rowKey(root.selected)
            root.anchorKey = root.rowModel.rowKey(root.anchorRow)
        }
        if (typeof session === "undefined" || !root.rowModel) return
        var src = []
        for (var i = 0; i < root.selectedRows.length; ++i) {
            // A series group stands for all of its episodes: whatever is done
            // to the selection is done to each of them.
            var rows = typeof root.rowModel.sourceRowsAt === "function"
                       ? root.rowModel.sourceRowsAt(root.selectedRows[i])
                       : [root.rowModel.mapToSource(root.selectedRows[i])]
            for (var j = 0; j < rows.length; ++j)
                if (rows[j] >= 0 && src.indexOf(rows[j]) < 0) src.push(rows[j])
        }
        session.setSelectedRows(src)
    }

    // Rows moved under the selection: find them again by key. The session's
    // selection is by torrent, so while every row is still there it is right
    // and is left alone. A row that went (an episode hidden by closing its
    // group, a torrent filtered out) leaves the selection too, or actions
    // would reach something no longer on screen.
    function followRows() {
        if (!root.keyedRows || root.selectedKeys.length === 0) return
        var rows = []
        for (var i = 0; i < root.selectedKeys.length; ++i) {
            var r = root.rowModel.rowForKey(root.selectedKeys[i])
            if (r >= 0) rows.push(r)
        }
        var lost = rows.length !== root.selectedKeys.length
        root.selectedRows = rows
        root.selected = root.rowModel.rowForKey(root.selectedKey)
        if (root.selected < 0 && rows.length > 0) root.selected = rows[rows.length - 1]
        root.anchorRow = root.rowModel.rowForKey(root.anchorKey)
        if (lost) root.commitSel()
    }

    // Opens or closes the series group on this row. False when it is not one.
    function toggleGroup(row) {
        if (!root.keyedRows || !root.rowModel.isGroup(row)) return false
        root.rowModel.toggleGroup(row)
        return true
    }

    function selectRow(proxyRow, mods) {
        root.clearFilterFocusRequested()
        mods = mods || 0
        var ctrl = (mods & Qt.ControlModifier) || (mods & Qt.MetaModifier)
        var shift = (mods & Qt.ShiftModifier)
        var rows = root.selectedRows.slice()
        if (shift && root.anchorRow >= 0) {
            rows = []
            var a = Math.min(root.anchorRow, proxyRow)
            var b = Math.max(root.anchorRow, proxyRow)
            for (var i = a; i <= b; ++i) rows.push(i)
        } else if (ctrl) {
            var idx = rows.indexOf(proxyRow)
            if (idx >= 0) rows.splice(idx, 1); else rows.push(proxyRow)
            root.anchorRow = proxyRow
        } else {
            rows = [proxyRow]
            root.anchorRow = proxyRow
        }
        root.selectedRows = rows
        root.selected = proxyRow
        root.commitSel()
        root.releaseSearchFocusRequested()
    }

    // Launching into an inspector with nothing in it wastes the panel and
    // reads as broken. Not selectRow(): that also pulls focus out of the
    // search field, which at startup would be the app taking focus nobody
    // gave it.
    function selectFirstIfNone(rowCount) {
        if (root.selectedRows.length > 0 || rowCount <= 0) return
        root.selectedRows = [0]
        root.selected = 0
        root.anchorRow = 0
        root.commitSel()
    }

    function isRowSelected(proxyRow) { return root.selectedRows.indexOf(proxyRow) >= 0 }

    function selectAll() {
        if (typeof session === "undefined" || typeof torrentFilter === "undefined") return
        var rows = []
        if (root.keyedRows) {
            // every row on screen; a closed group brings its episodes along
            for (var r = 0; r < root.rowModel.rowCount(); ++r) rows.push(r)
        } else {
            for (var s = 0; s < session.torrentCount; ++s) {
                var p = torrentFilter.mapFromSource(s)
                if (p >= 0 && rows.indexOf(p) < 0) rows.push(p)
            }
        }
        rows.sort(function(x, y) { return x - y })
        root.selectedRows = rows
        root.anchorRow = rows.length > 0 ? rows[0] : -1
        root.selected = rows.length > 0 ? rows[rows.length - 1] : -1
        root.commitSel()
    }

    function clearSelection() {
        root.selectedRows = []
        root.selected = -1
        root.commitSel()
    }

    function toggleSort(col) {
        if (root.sortColumn === col) root.sortAsc = !root.sortAsc
        else { root.sortColumn = col; root.sortAsc = true }
        if (typeof torrentFilter !== "undefined") torrentFilter.setSortColumn(col, root.sortAsc)
    }

    function setFilter(f) {
        root.activeFilter = f
        if (typeof torrentFilter !== "undefined") torrentFilter.setFilterState(f)
    }

    function applyCatFilter(c) {
        root.catFilter = c
        if (typeof torrentFilter !== "undefined") torrentFilter.setCategoryFilter(c)
    }

    // Selection lives in two places: session (source rows) and selectedRows
    // (proxy rows). Jumping from the download chip must update both.
    function selectTorrentByHash(infoHash) {
        if (typeof session === "undefined") return
        if (!session.selectByInfoHash(infoHash)) {
            root.setFilter("all")
            session.selectByInfoHash(infoHash)
        }
        if (typeof torrentFilter === "undefined") return

        var proxy = []
        var src = session.selectedRows()
        for (var i = 0; i < src.length; ++i) {
            // open the series group it sits in, or the jump lands on nothing
            var p = root.keyedRows ? root.rowModel.revealSource(src[i])
                                   : root.rowModel.mapFromSource(src[i])
            if (p >= 0 && proxy.indexOf(p) < 0) proxy.push(p)
        }
        root.selectedRows = proxy
        root.selected = proxy.length > 0 ? proxy[0] : -1
        root.anchorRow = root.selected
        if (root.keyedRows) {
            root.selectedKeys = proxy.map(function(r) { return root.rowModel.rowKey(r) })
            root.selectedKey = root.rowModel.rowKey(root.selected)
            root.anchorKey = root.selectedKey
        }
        if (root.selected >= 0)
            root.scrollToRowRequested(root.selected)
    }

    function remapRow(r, from, to) {
        if (r === from) return to
        if (from < to) return (r > from && r <= to) ? r - 1 : r
        return (r >= to && r < from) ? r + 1 : r
    }

    function onQueueMoved(from, to) {
        // The keyed model already moved the selection along (followRows);
        // shifting it again here would move it twice.
        if (root.keyedRows) return
        var rows = []
        for (var i = 0; i < root.selectedRows.length; ++i)
            rows.push(root.remapRow(root.selectedRows[i], from, to))
        root.selectedRows = rows
        if (root.selected >= 0) root.selected = root.remapRow(root.selected, from, to)
        if (root.anchorRow >= 0) root.anchorRow = root.remapRow(root.anchorRow, from, to)
    }
}
