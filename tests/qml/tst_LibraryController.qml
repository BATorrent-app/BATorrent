// SPDX-License-Identifier: MIT
// Downloads library selection state without a live session/filter model.
// commitSel no-ops when session is undefined: still pins select/clear contracts.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

Item {
    id: root
    width: 200
    height: 100

    Component {
        id: ctrlComp
        LibraryController {}
    }

    // Stands in for QmlLibraryModel: row 0 is a film, row 1 a series group of
    // two episodes that opens to rows 2 and 3.
    Component {
        id: modelComp
        QtObject {
            id: m
            property bool open: false
            property int toggles: 0
            signal structureChanged()
            function rowCount() { return open ? 4 : 2 }
            function isGroup(r) { return r === 1 }
            function toggleGroup(r) { open = !open; toggles++; structureChanged() }
            function rowKey(r) {
                var keys = open ? ["t:film", "g:show", "t:e1", "t:e2"] : ["t:film", "g:show"]
                return r >= 0 && r < keys.length ? keys[r] : ""
            }
            function rowForKey(k) {
                for (var r = 0; r < rowCount(); ++r) if (rowKey(r) === k) return r
                return -1
            }
            function sourceRowsAt(r) { return r === 0 ? [0] : r === 1 ? [1, 2] : r === 2 ? [1] : r === 3 ? [2] : [] }
            function mapToSource(r) { return r === 1 ? -1 : (sourceRowsAt(r)[0] || -1) }
            function revealSource(s) { if (s === 0) return 0; if (!open) toggleGroup(1); return s === 1 ? 2 : 3 }
        }
    }

    TestCase {
        name: "LibraryController"
        when: windowShown
        width: 200
        height: 100

        function test_selectRowSetsSingleSelection() {
            var c = createTemporaryObject(ctrlComp, root)
            verify(!!c, "Object exists")
            c.selectRow(3)
            compare(c.selected, 3)
            compare(c.selectedRows.length, 1)
            compare(c.selectedRows[0], 3)
            compare(c.anchorRow, 3)
            verify(c.isRowSelected(3))
            verify(!c.isRowSelected(0))
        }

        function test_clearSelectionResets() {
            var c = createTemporaryObject(ctrlComp, root)
            verify(!!c, "Object exists")
            c.selectRow(1)
            c.clearSelection()
            compare(c.selected, -1)
            compare(c.selectedRows.length, 0)
        }

        function test_toggleSortFlipsDirection() {
            var c = createTemporaryObject(ctrlComp, root)
            verify(!!c, "Object exists")
            c.toggleSort("name")
            compare(c.sortColumn, "name")
            compare(c.sortAsc, true)
            c.toggleSort("name")
            compare(c.sortAsc, false)
            c.toggleSort("size")
            compare(c.sortColumn, "size")
            compare(c.sortAsc, true)
        }

        function test_toggleGroupOnlyOnGroupRows() {
            var m = createTemporaryObject(modelComp, root)
            var c = createTemporaryObject(ctrlComp, root, { rowModel: m })
            verify(c.keyedRows)
            verify(!c.toggleGroup(0), "a film has nothing to open")
            compare(m.toggles, 0)
            verify(c.toggleGroup(1))
            verify(m.open)
        }

        function test_toggleGroupWithoutModelIsNoop() {
            var c = createTemporaryObject(ctrlComp, root, { rowModel: null })
            verify(!c.keyedRows)
            verify(!c.toggleGroup(1))
        }

        function test_selectionFollowsRowsWhenGroupOpens() {
            var m = createTemporaryObject(modelComp, root)
            var c = createTemporaryObject(ctrlComp, root, { rowModel: m })
            m.open = true
            c.selectRow(3)            // second episode
            compare(c.selectedKey, "t:e2")
            m.open = false            // closed: the episode is gone from the list
            m.structureChanged()
            compare(c.selectedRows.length, 0)
            c.selectRow(1)            // the group
            m.toggleGroup(1)          // opening keeps it selected, on its row
            compare(c.selectedRows.length, 1)
            compare(c.selectedRows[0], 1)
            compare(c.selected, 1)
        }

        function test_queueMoveLeavesKeyedSelectionAlone() {
            var m = createTemporaryObject(modelComp, root)
            var c = createTemporaryObject(ctrlComp, root, { rowModel: m })
            c.selectRow(1)
            c.onQueueMoved(1, 0)
            compare(c.selected, 1)
        }

        function test_setFilterUpdatesActiveFilter() {
            var c = createTemporaryObject(ctrlComp, root)
            verify(!!c, "Object exists")
            c.setFilter("downloading")
            compare(c.activeFilter, "downloading")
        }
    }
}
