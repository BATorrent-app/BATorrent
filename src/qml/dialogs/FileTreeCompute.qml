// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Which rows of the add-torrent file list a collapsed folder hides.
//
// Separate from the dialog so the nesting cases can be tested: the rows are a
// flat run and a folder owns everything after it until the depth comes back to
// its own, so a collapsed folder inside an already-collapsed one must not
// un-hide anything when it ends.
import QtQuick

QtObject {
    // rows: [{ depth, dir, collapsed }] in tree order. Returns one bool per row.
    function hiddenFlags(rows) {
        var out = []
        var hideFrom = -1
        for (var i = 0; i < rows.length; ++i) {
            var r = rows[i]
            if (hideFrom >= 0 && r.depth <= hideFrom) hideFrom = -1
            out.push(hideFrom >= 0)
            if (r.dir && r.collapsed && hideFrom < 0) hideFrom = r.depth
        }
        return out
    }
}
