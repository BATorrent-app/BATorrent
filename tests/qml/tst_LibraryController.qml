// SPDX-License-Identifier: MIT
// Downloads library selection state without a live session/filter model.
// commitSel no-ops when session is undefined — still pins select/clear contracts.

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

        function test_setFilterUpdatesActiveFilter() {
            var c = createTemporaryObject(ctrlComp, root)
            verify(!!c, "Object exists")
            c.setFilter("downloading")
            compare(c.activeFilter, "downloading")
        }
    }
}
