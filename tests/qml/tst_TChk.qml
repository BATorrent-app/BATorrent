// SPDX-License-Identifier: MIT
// Qt Quick Test for the TChk checkbox: a click flips `on`, emits toggled(bool),
// and clears the `partial` (indeterminate) state.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 100
    height: 100

    SignalSpy { id: toggledSpy; signalName: "toggled" }

    Component {
        id: chkComp
        TChk { width: 17; height: 17 }
    }

    TestCase {
        name: "TChk"
        when: windowShown
        width: 100
        height: 100

        function test_clickChecksAndEmits() {
            var cb = createTemporaryObject(chkComp, root)
            verify(!!cb, "Object exists")
            compare(cb.on, false)
            toggledSpy.target = cb
            toggledSpy.clear()

            mouseClick(cb, 8, 8)
            tryCompare(cb, "on", true)
            tryCompare(toggledSpy, "count", 1)
            compare(toggledSpy.signalArguments[0][0], true)
        }

        function test_clickClearsPartial() {
            var cb = createTemporaryObject(chkComp, root, { partial: true })
            verify(!!cb, "Object exists")
            compare(cb.partial, true)

            mouseClick(cb, 8, 8)
            tryCompare(cb, "partial", false)
            compare(cb.on, true)
        }
    }
}
