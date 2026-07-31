// SPDX-License-Identifier: MIT
// Get & Watch overlay phase contract: show/hide/fail and cancel emission.
// Complements HubCompute play wiring for the one-click Watch path.

import QtQuick
import QtTest
import "qrc:/src/qml/overlays"

Item {
    id: root
    width: 800
    height: 600

    SignalSpy { id: canceledSpy; signalName: "canceled" }

    Component {
        id: overlayComp
        GetWatchOverlay { width: 800; height: 600 }
    }

    TestCase {
        name: "GetWatchOverlay"
        when: windowShown
        width: 800
        height: 600

        function test_showSearchingMakesVisible() {
            var ov = createTemporaryObject(overlayComp, root)
            verify(!!ov, "Object exists")
            compare(ov.visible, false)
            ov.show("searching", "Title")
            compare(ov.phase, "searching")
            compare(ov.title, "Title")
            compare(ov.visible, true)
            compare(ov.showSpinner, true)
        }

        function test_bufferingShowsPercentBar() {
            var ov = createTemporaryObject(overlayComp, root)
            verify(!!ov, "Object exists")
            ov.show("buffering", "Film")
            ov.hash = "abcd"
            ov.percent = 0.42
            compare(ov.phase, "buffering")
            compare(ov.showPct, true)
            compare(ov.showBar, true)
            compare(ov.hash, "abcd")
        }

        function test_failThenHide() {
            var ov = createTemporaryObject(overlayComp, root)
            verify(!!ov, "Object exists")
            ov.show("searching", "X")
            ov.fail("gone")
            compare(ov.phase, "failed")
            compare(ov.failMessage, "gone")
            ov.hide()
            compare(ov.phase, "")
            compare(ov.visible, false)
        }

        function test_cancelEmitsAndHides() {
            var ov = createTemporaryObject(overlayComp, root)
            verify(!!ov, "Object exists")
            ov.show("searching", "Y")
            canceledSpy.target = ov
            canceledSpy.clear()
            ov.canceled()
            ov.hide()
            tryCompare(canceledSpy, "count", 1)
            compare(ov.phase, "")
        }
    }
}
