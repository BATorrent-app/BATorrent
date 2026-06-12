// SPDX-License-Identifier: MIT
// Qt Quick Test for the BtnFlat button: a click emits clicked(), and the text
// property drives the rendered label.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 240
    height: 100

    SignalSpy { id: clickedSpy; signalName: "clicked" }

    Component {
        id: btnComp
        BtnFlat { width: 120; height: 32; text: "OK" }
    }

    TestCase {
        name: "BtnFlat"
        when: windowShown
        width: 240
        height: 100

        function test_clickEmitsClicked() {
            var btn = createTemporaryObject(btnComp, root)
            verify(!!btn, "Object exists")
            compare(btn.text, "OK")
            clickedSpy.target = btn
            clickedSpy.clear()

            mouseClick(btn, 60, 16)
            tryCompare(clickedSpy, "count", 1)
        }

        function test_textIsAssignable() {
            var btn = createTemporaryObject(btnComp, root, { text: "Cancel" })
            verify(!!btn, "Object exists")
            compare(btn.text, "Cancel")
        }
    }
}
