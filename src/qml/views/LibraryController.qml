// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

// Selection + filter state for the downloads library. Instantiated as a child
// of Main's Window (id: library). Leaf chrome takes it as `controller` —
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

    // Ask Main to clear the filter search focus / release the accent ring.
    signal clearFilterFocusRequested()
    // Ask Main to scroll the visible grid/list to this proxy row.
    signal scrollToRowRequested(int proxyRow)
    // Ask Main to drop search-field focus after a row click.
    signal releaseSearchFocusRequested()

    function commitSel() {
        if (typeof session === "undefined" || typeof torrentFilter === "undefined") return
        var src = []
        for (var i = 0; i < root.selectedRows.length; ++i) {
            var s = torrentFilter.mapToSource(root.selectedRows[i])
            if (s >= 0) src.push(s)
        }
        session.setSelectedRows(src)
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

    function isRowSelected(proxyRow) { return root.selectedRows.indexOf(proxyRow) >= 0 }

    function selectAll() {
        if (typeof session === "undefined" || typeof torrentFilter === "undefined") return
        var rows = []
        for (var s = 0; s < session.torrentCount; ++s) {
            var p = torrentFilter.mapFromSource(s)
            if (p >= 0) rows.push(p)
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
            var p = torrentFilter.mapFromSource(src[i])
            if (p >= 0) proxy.push(p)
        }
        root.selectedRows = proxy
        root.selected = proxy.length > 0 ? proxy[0] : -1
        root.anchorRow = root.selected
        if (root.selected >= 0)
            root.scrollToRowRequested(root.selected)
    }

    function remapRow(r, from, to) {
        if (r === from) return to
        if (from < to) return (r > from && r <= to) ? r - 1 : r
        return (r >= to && r < from) ? r + 1 : r
    }

    function onQueueMoved(from, to) {
        var rows = []
        for (var i = 0; i < root.selectedRows.length; ++i)
            rows.push(root.remapRow(root.selectedRows[i], from, to))
        root.selectedRows = rows
        if (root.selected >= 0) root.selected = root.remapRow(root.selected, from, to)
        if (root.anchorRow >= 0) root.anchorRow = root.remapRow(root.anchorRow, from, to)
    }
}
