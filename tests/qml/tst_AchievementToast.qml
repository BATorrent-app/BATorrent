// SPDX-License-Identifier: MIT
// One card on screen; several unlocks at once become that card with a count.

import QtQuick
import QtTest
import "qrc:/src/qml/windows"

Item {
    id: root

    Component { id: toastComp; AchievementToast {} }

    TestCase {
        name: "AchievementToast"
        when: windowShown

        function test_oneUnlockShowsItself() {
            var t = createTemporaryObject(toastComp, root)
            t.push({ name: "First", icon: "", percentLabel: "41", rare: false, description: "", unlocked: 3, total: 10 })
            compare(t.current.name, "First")
            compare(t.extra, 0)
            verify(t.visible)
        }

        function test_aBurstKeepsTheFirstAndCounts() {
            var t = createTemporaryObject(toastComp, root)
            t.push({ name: "First", rare: false })
            t.push({ name: "Second", rare: true })
            t.push({ name: "Third", rare: false })
            compare(t.current.name, "First")
            compare(t.extra, 2)
        }

        function test_itNeverTakesFocusOrClicks() {
            var t = createTemporaryObject(toastComp, root)
            verify((t.flags & Qt.WindowDoesNotAcceptFocus) !== 0)
            verify((t.flags & Qt.WindowTransparentForInput) !== 0)
            verify((t.flags & Qt.WindowStaysOnTopHint) !== 0)
        }
    }
}
