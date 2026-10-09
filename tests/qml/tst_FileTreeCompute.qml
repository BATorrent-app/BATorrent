// SPDX-License-Identifier: MIT
// Qt Quick Test for FileTreeCompute — which rows a collapsed folder hides in
// the add-torrent file list.
//
// The rows are one flat run, so "everything under this folder" is a depth
// comparison rather than a parent pointer. The case worth pinning is a
// collapsed folder inside an already-collapsed one: when the inner subtree
// ends, nothing may become visible again while the outer folder is still shut.

import QtQuick
import QtTest
import "qrc:/src/qml/dialogs"

Item {
    id: root
    width: 200
    height: 100

    FileTreeCompute { id: compute }

    function dir(depth, collapsed) { return { depth: depth, dir: true, collapsed: collapsed === true } }
    function file(depth) { return { depth: depth, dir: false, collapsed: false } }

    TestCase {
        name: "FileTreeCompute"

        function test_nothingHiddenWhenAllOpen() {
            var rows = [dir(0), file(1), file(1), file(0)]
            compare(compute.hiddenFlags(rows), [false, false, false, false])
        }

        function test_collapsedFolderHidesItsContentsOnly() {
            //  Season 1 (collapsed) / ep1 / ep2 / readme.txt at the root
            var rows = [dir(0, true), file(1), file(1), file(0)]
            compare(compute.hiddenFlags(rows), [false, true, true, false],
                    "the folder stays, its files go, the sibling at root is untouched")
        }

        function test_siblingFolderIsUnaffected() {
            var rows = [dir(0, true), file(1), dir(0), file(1)]
            compare(compute.hiddenFlags(rows), [false, true, false, false])
        }

        function test_nestedCollapseDoesNotReopenTheOuterFolder() {
            //  Show (collapsed) / Season 1 (collapsed) / ep1 / extra.nfo
            //  extra.nfo is depth 1: it ends Season 1's subtree but is still
            //  inside Show, so it must stay hidden.
            var rows = [dir(0, true), dir(1, true), file(2), file(1)]
            compare(compute.hiddenFlags(rows), [false, true, true, true])
        }

        function test_innerCollapseAloneHidesOnlyTheInnerFiles() {
            var rows = [dir(0), dir(1, true), file(2), file(1)]
            compare(compute.hiddenFlags(rows), [false, false, true, false])
        }

        function test_emptyListIsFine() {
            compare(compute.hiddenFlags([]), [])
        }
    }
}
