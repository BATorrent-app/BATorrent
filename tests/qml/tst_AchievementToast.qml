// SPDX-License-Identifier: MIT
// One unlock at a time, in order; a rare one says so.

import QtQuick
import QtTest
import "qrc:/src/qml/windows"

Item {
    id: root

    Component { id: toastComp; AchievementToast {} }

    TestCase {
        name: "AchievementToast"
        when: windowShown

        function test_unlocksShowOneAtATimeInOrder() {
            var t = createTemporaryObject(toastComp, root)
            t.push("Elden Ring", "First", "", "41", false)
            t.push("Elden Ring", "Second", "", "4", true)
            compare(t.current.name, "First")
            compare(t.queue.length, 1)
            verify(t.visible)
            verify(!t.rare, "the service said it wasn't rare")
            t.next()
            compare(t.current.name, "Second")
            verify(t.rare, "and that this one is")
            t.next()
            compare(t.current, null)
        }

        function test_anUnknownShareIsNeverRare() {
            var t = createTemporaryObject(toastComp, root)
            t.push("X", "No data", "", "")
            verify(!t.rare)
        }

        function test_itNeverTakesFocusOrClicks() {
            var t = createTemporaryObject(toastComp, root)
            verify((t.flags & Qt.WindowDoesNotAcceptFocus) !== 0)
            verify((t.flags & Qt.WindowTransparentForInput) !== 0)
            verify((t.flags & Qt.WindowStaysOnTopHint) !== 0)
        }
    }
}
