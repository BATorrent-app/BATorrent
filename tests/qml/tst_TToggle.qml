// SPDX-License-Identifier: MIT
// Qt Quick Test for the TToggle design-system switch: a click flips `on` and
// emits toggled(bool) carrying the new state.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 200
    height: 100

    SignalSpy { id: toggledSpy; signalName: "toggled" }

    Component {
        id: toggleComp
        TToggle { width: 38; height: 21 }
    }

    TestCase {
        name: "TToggle"
        when: windowShown
        width: 200
        height: 100

        function test_clickFlipsOnAndEmits() {
            var tg = createTemporaryObject(toggleComp, root)
            verify(!!tg, "Object exists")
            compare(tg.on, false)
            toggledSpy.target = tg
            toggledSpy.clear()

            mouseClick(tg, 19, 10)
            tryCompare(tg, "on", true)
            tryCompare(toggledSpy, "count", 1)
            compare(toggledSpy.signalArguments[0][0], true)
        }

        function test_secondClickTogglesBack() {
            var tg = createTemporaryObject(toggleComp, root, { on: true })
            verify(!!tg, "Object exists")
            toggledSpy.target = tg
            toggledSpy.clear()

            mouseClick(tg, 19, 10)
            tryCompare(tg, "on", false)
            tryCompare(toggledSpy, "count", 1)
            compare(toggledSpy.signalArguments[0][0], false)
        }
    }
}
