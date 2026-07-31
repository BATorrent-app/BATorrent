// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import "theme"

// Library keyboard shortcuts. Instantiated as a Window child so Application/
// Window shortcut context behaves like the inlined Main originals.
Item {
    required property var host
    required property var library
    required property var filterBar
    required property var cmdPalette

    Shortcut { sequences: [StandardKey.HelpContents]; onActivated: host.showWin(host.shortcutsWinLoader) }
    Shortcut { sequence: "Ctrl+F"; onActivated: filterBar.searchInput.forceActiveFocus() }
    Shortcut { sequence: "Ctrl+R"; onActivated: if (typeof session !== "undefined") session.forceRecheckSelected() }
    // reorder queue: vertical in list, horizontal in grid (tiles sit side by side)
    Shortcut { sequence: "Ctrl+Up";    enabled: !library.gridView; onActivated: if (typeof session !== "undefined") session.queueUpSelected() }
    Shortcut { sequence: "Ctrl+Down";  enabled: !library.gridView; onActivated: if (typeof session !== "undefined") session.queueDownSelected() }
    Shortcut { sequence: "Ctrl+Left";  enabled: library.gridView;  onActivated: if (typeof session !== "undefined") session.queueUpSelected() }
    Shortcut { sequence: "Ctrl+Right"; enabled: library.gridView;  onActivated: if (typeof session !== "undefined") session.queueDownSelected() }
    // navigate selection — suppressed while the command palette owns the keyboard
    // (otherwise the arrows scroll the list behind it instead of its results)
    Shortcut { sequence: "Up";    enabled: !cmdPalette.opened; onActivated: host.moveSel(library.gridView ? -host.gridCols() : -1) }
    Shortcut { sequence: "Down";  enabled: !cmdPalette.opened; onActivated: host.moveSel(library.gridView ?  host.gridCols() :  1) }
    Shortcut { sequence: "Left";  enabled: library.gridView && !cmdPalette.opened; onActivated: host.moveSel(-1) }
    Shortcut { sequence: "Right"; enabled: library.gridView && !cmdPalette.opened; onActivated: host.moveSel(1) }
    Shortcut { sequence: "Ctrl+1"; onActivated: library.setFilter("all") }
    Shortcut { sequence: "Ctrl+2"; onActivated: library.setFilter("downloading") }
    Shortcut { sequence: "Ctrl+3"; onActivated: library.setFilter("seeding") }
    Shortcut { sequence: "Ctrl+4"; onActivated: library.setFilter("completed") }
    Shortcut { sequence: "Ctrl+5"; onActivated: library.setFilter("active") }
    Shortcut { sequence: "Ctrl+6"; onActivated: library.setFilter("paused") }
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: Theme.cycle() }
}
