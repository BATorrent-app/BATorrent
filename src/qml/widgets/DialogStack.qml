// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Open BatDialogs register here so Esc/Enter only ever target the topmost one —
// two enabled Shortcuts on the same key are "ambiguous" to Qt and neither fires.
pragma Singleton
import QtQuick

QtObject {
    property var stack: []
    readonly property var topItem: stack.length > 0 ? stack[stack.length - 1] : null

    function push(d) {
        var s = stack.slice()
        s.push(d)
        stack = s
    }
    function pop(d) {
        var s = stack.slice()
        var i = s.indexOf(d)
        if (i >= 0) { s.splice(i, 1); stack = s }
    }
}
