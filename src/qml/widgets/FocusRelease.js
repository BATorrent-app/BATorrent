// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// A text field keeps the keyboard focus when its page is hidden (Qt keeps
// focus on invisible items), and a field with the focus takes Ctrl+A and
// Ctrl+V before the app's shortcuts see them: after a visit to Find, both did
// nothing on Downloads until something else took the focus.
.pragma library

// Takes the focus off a hidden text field in `win`. The field itself gives it
// up: focusing the window's root alone hands the focus straight back to the
// field its scope remembers.
function dropHidden(win) {
    var f = win.activeFocusItem
    if (!f || f.cursorPosition === undefined || f.visible) return false
    f.focus = false
    if (win.activeFocusItem === f) win.contentItem.forceActiveFocus()
    return true
}
