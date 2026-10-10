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

        function test_aSlowSourceIsOfferedAnotherOnlyAfterAWhile() {
            var ov = createTemporaryObject(overlayComp, root)
            ov.show("buffering", "Film")
            ov.downBps = 50 * 1024; ov.peers = 12; ov.waited = 5
            compare(ov.slow, false)                 // too early to judge
            ov.waited = 20
            compare(ov.slow, true)                  // crawling
            ov.downBps = 3 * 1024 * 1024
            compare(ov.slow, false)                 // fast and well-peered
            ov.peers = 1
            compare(ov.slow, true)                  // fast now, but one peer away from stalling
            ov.hide()
            compare(ov.waited, 0)
            compare(ov.slow, false)
        }

        function test_rateReadsLikeTheRestOfTheApp() {
            var ov = createTemporaryObject(overlayComp, root)
            compare(ov.fmtRate(512 * 1024), "512 KB/s")
            compare(ov.fmtRate(2.1 * 1024 * 1024), "2.1 MB/s")
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
