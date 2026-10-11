// SPDX-License-Identifier: MIT
// Clicking an episode you already have plays it; clicking one you don't
// opens the torrents for it.

import QtQuick
import QtTest
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 700
    height: 200

    Component { id: rowComp; StageEpisodeRow { width: 600; number: 1; title: "Pilot" } }

    TestCase {
        name: "StageEpisodeRow"
        when: windowShown

        SignalSpy { id: plays; signalName: "playRequested" }
        SignalSpy { id: opens; signalName: "openRequested" }

        function make(playable, offered) {
            var r = createTemporaryObject(rowComp, root, { playable: playable, offered: offered })
            plays.target = r; opens.target = r
            plays.clear(); opens.clear()
            return r
        }

        function test_anEpisodeOnDiskPlaysOnClick() {
            var r = make(true, true)
            mouseClick(r, r.width - 40, r.height / 2)
            compare(plays.count, 1)
            compare(opens.count, 0, "not the torrent list for something already here")
        }

        function test_anEpisodeNotHereOpensItsTorrents() {
            var r = make(false, true)
            mouseClick(r, r.width - 40, r.height / 2)
            compare(opens.count, 1)
            compare(plays.count, 0)
        }

        function test_anEpisodeWithNothingDoesNothing() {
            var r = make(false, false)
            mouseClick(r, r.width - 40, r.height / 2)
            compare(opens.count + plays.count, 0)
        }
    }
}
