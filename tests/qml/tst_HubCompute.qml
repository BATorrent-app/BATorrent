// SPDX-License-Identifier: MIT
// Watch path characterization: Hub continue-watching shelf + playMovie →
// playByHash. Fails if HUB library → player wiring regresses.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

Item {
    id: root
    width: 400
    height: 200

    QtObject {
        id: mockApi
        property var playCalls: []
        function playByHash(hash) { playCalls.push(hash) }
        function launchGame(hash) { playCalls.push("game:" + hash) }
        function movieLibrary() { return [] }
        function gameLibrary() { return [] }
    }

    QtObject {
        id: mockEpisodeMenu
        property var openCalls: []
        function openFor(item) { openCalls.push(item) }
    }

    QtObject {
        id: mockPage
        property var api: mockApi
        property var disco: null
        property var episodeMenu: mockEpisodeMenu
        property var library: [
            { title: "Old", infoHash: "aaa", resumeMs: 1000, resumeAt: 10,
              videos: [{ idx: 0 }], addedTime: 1 },
            { title: "Fresh", infoHash: "bbb", resumeMs: 5000, resumeAt: 99,
              videos: [{ idx: 0 }], addedTime: 2 },
            { title: "Unplayed", infoHash: "ccc", resumeMs: 0, resumeAt: 0,
              videos: [{ idx: 0 }], addedTime: 3 }
        ]
        property var gameItems: []
        property string librarySearch: ""
        property string librarySort: "added"
        property var detailItem: null
        property bool detailIsGame: false
        property bool detailOpen: false
        property var exePicker: null
        function fileUrl(p) { return "file://" + p }
    }

    Component {
        id: computeComp
        HubCompute { page: mockPage }
    }

    TestCase {
        name: "HubCompute"
        when: windowShown
        width: 400
        height: 200

        function init() {
            mockApi.playCalls = []
            mockEpisodeMenu.openCalls = []
        }

        function test_continueItemsOrdersByResumeAt() {
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            compare(c.continueItems.length, 2)
            compare(c.continueItems[0].infoHash, "bbb")
            compare(c.continueItems[1].infoHash, "aaa")
            compare(c.empty, false)
        }

        function test_playMovieCallsPlayByHash() {
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            c.playMovie({ infoHash: "bbb", videos: [{ idx: 0 }] })
            compare(mockApi.playCalls.length, 1)
            compare(mockApi.playCalls[0], "bbb")
            compare(mockEpisodeMenu.openCalls.length, 0)
        }

        function test_playMovieOpensEpisodeMenuWhenMultiVideo() {
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            c.playMovie({
                infoHash: "ser",
                videos: [{ idx: 0 }, { idx: 1 }]
            })
            compare(mockEpisodeMenu.openCalls.length, 1)
            compare(mockApi.playCalls.length, 0)
        }

        function test_playMovieNoopsWithoutApi() {
            mockPage.api = null
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            c.playMovie({ infoHash: "x", videos: [{ idx: 0 }] })
            compare(mockApi.playCalls.length, 0)
            mockPage.api = mockApi
        }
    }
}
