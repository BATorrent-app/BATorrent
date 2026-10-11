// SPDX-License-Identifier: MIT
// A game page opened from Downloads hands you back to Downloads when you
// close it; one opened inside the HUB just closes.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

Item {
    id: root
    width: 1000
    height: 700

    Component { id: hubComp; HubView { anchors.fill: parent } }

    TestCase {
        name: "HubViewRouting"
        when: windowShown

        SignalSpy { id: spy; signalName: "returnTo" }
        SignalSpy { id: titles; signalName: "openTitle" }

        function test_aShowOpensOnTheEpisodeYouAreIn() {
            var h = createTemporaryObject(hubComp, root)
            titles.target = h
            titles.clear()
            h.openItem({ title: "Breaking Bad", tmdbId: 1396, isSeries: true, isGroup: true,
                         videos: [ { season: 1, episode: 1, resumeAt: 10, watched: true },
                                   { season: 1, episode: 2, resumeAt: 0 } ] }, "series")
            compare(titles.count, 1)
            var work = titles.signalArguments[0][0]
            compare(work.type, "series")
            compare(work.season, 1)
            compare(work.episode, 2, "episode 1 was finished, so episode 2")
        }

        function test_aGameOpenedFromDownloadsReturnsThere() {
            var h = createTemporaryObject(hubComp, root)
            spy.target = h
            spy.clear()
            h.openGame("abc", 0)
            compare(h.gameOpen, true)
            compare(h.gameHash, "abc")
            h.closeGame()
            compare(h.gameOpen, false)
            compare(spy.count, 1)
            compare(spy.signalArguments[0][0], 0)
        }

        function test_aGameOpenedInTheHubJustCloses() {
            var h = createTemporaryObject(hubComp, root)
            spy.target = h
            spy.clear()
            h.openGame("abc", 0)
            h.closeGame()
            h.openGame("def")
            h.closeGame()
            compare(spy.count, 1, "the second open came from the HUB itself")
        }
    }
}
