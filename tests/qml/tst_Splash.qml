// SPDX-License-Identifier: MIT
// Qt Quick Test for the startup splash. Drives the real Splash component
// headless (offscreen) and asserts the skip-to-finish contract: a click or a
// key press dismisses it and emits finished(), and exit() is idempotent.

import QtQuick
import QtTest
import "qrc:/src/qml"

Item {
    id: root
    width: 800
    height: 600

    SignalSpy { id: finishedSpy; signalName: "finished" }

    Component {
        id: splashComp
        Splash { anchors.fill: parent }
    }

    TestCase {
        id: tc
        name: "Splash"
        when: windowShown
        width: 800
        height: 600

        function test_clickDismissesAndEmitsFinished() {
            var splash = createTemporaryObject(splashComp, root)
            verify(!!splash, "Object exists")
            finishedSpy.target = splash
            finishedSpy.clear()
            mouseClick(splash, 400, 300)
            tryCompare(finishedSpy, "count", 1)
        }

        function test_keyPressDismissesAndEmitsFinished() {
            var splash = createTemporaryObject(splashComp, root)
            verify(!!splash, "Object exists")
            splash.forceActiveFocus()
            finishedSpy.target = splash
            finishedSpy.clear()
            keyClick(Qt.Key_Space)
            tryCompare(finishedSpy, "count", 1)
        }

        function test_exitIsIdempotent() {
            var splash = createTemporaryObject(splashComp, root)
            verify(!!splash, "Object exists")
            finishedSpy.target = splash
            finishedSpy.clear()
            splash.exit(true)
            splash.exit(true)   // the morphing guard must swallow the second call
            tryCompare(finishedSpy, "count", 1)
            wait(300)
            compare(finishedSpy.count, 1)
        }
    }
}
