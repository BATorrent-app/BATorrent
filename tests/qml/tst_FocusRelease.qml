// SPDX-License-Identifier: MIT
// After a visit to Find, Ctrl+A and Ctrl+V did nothing on Downloads: the
// search field kept the keyboard focus with its page hidden, and took those
// keys before the shortcuts. FocusRelease.dropHidden takes it off.

import QtQuick
import QtQuick.Controls.Basic
import QtTest
import "qrc:/src/qml/widgets/FocusRelease.js" as FocusRelease

Item {
    id: root
    width: 300
    height: 120

    FocusScope {
        id: page
        width: 200; height: 40
        TextField { id: field; anchors.fill: parent; focus: true }
    }
    TextField { id: shown; y: 60; width: 200; height: 40 }

    TestCase {
        name: "FocusRelease"
        when: windowShown

        function init() {
            page.visible = true
            field.forceActiveFocus()
            compare(root.Window.window.activeFocusItem, field)
        }

        function test_aHiddenFieldKeepsTheFocusOnItsOwn() {
            page.visible = false
            // what the app ran into: Qt leaves it there
            compare(root.Window.window.activeFocusItem, field)
        }

        function test_dropHiddenTakesItOff() {
            page.visible = false
            verify(FocusRelease.dropHidden(root.Window.window))
            verify(root.Window.window.activeFocusItem !== field)
            verify(!field.activeFocus)
        }

        function test_aVisibleFieldIsLeftAlone() {
            verify(!FocusRelease.dropHidden(root.Window.window))
            compare(root.Window.window.activeFocusItem, field)
            shown.forceActiveFocus()
            verify(!FocusRelease.dropHidden(root.Window.window))
            compare(root.Window.window.activeFocusItem, shown)
        }
    }
}
