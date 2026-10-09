// SPDX-License-Identifier: MIT
// Qt Quick Test for DiskGauge — that the figures on screen never belong to a
// volume other than the one being drawn.
//
// The gauge rotates through every save volume. It used to swap the name, the
// free figure and the bar the instant the index advanced, so a machine with a
// 21 GB disk and an 8 GB disk looked like one disk losing 13 GB. What is drawn
// therefore lags the index until the outgoing volume has left the screen.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

Item {
    id: root
    width: 300
    height: 60

    Component {
        id: gaugeComp
        DiskGauge {}
    }

    TestCase {
        name: "DiskGauge"
        when: windowShown

        function mkGauge() { return createTemporaryObject(gaugeComp, root, {}) }

        function test_shownLagsIndexUntilTransitionEnds() {
            var g = mkGauge()
            compare(g.shownIdx, 0, "starts on the first volume")

            g.idx = 1
            compare(g.shownIdx, 0, "the figures must not change the instant the index does")

            tryCompare(g, "shownIdx", 1, 2000, "the new volume arrives once the old one has left")
        }

        function test_shownIndexFollowsEveryStep() {
            var g = mkGauge()
            g.idx = 2
            tryCompare(g, "shownIdx", 2, 2000)
            g.idx = 0
            tryCompare(g, "shownIdx", 0, 2000, "wrapping back to the first volume works the same way")
        }
    }
}
