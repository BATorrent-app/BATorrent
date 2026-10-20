// SPDX-License-Identifier: MIT
// Find's titles: the best match as a banner, the next ones as tiles. A click
// did nothing: the pane looked the clicked item up in the results by identity,
// and in the app the results are the engine's list turned into JS anew on every
// read, so the item clicked is never the same object. The stub's list here is a
// fixed array, so the cases below click with copies, as the app does.

import QtQuick
import QtTest
import "qrc:/src/qml/views"
import "qrc:/src/qml/widgets"

Item {
    id: root
    width: 1400
    height: 700

    QtObject {
        id: apiStub
        property bool searching: false
        property string activeQuery: "harry potter"
        property string statusText: ""
        property string failure: ""
        property bool online: true
        property int activated: -1
        signal sourceSummary(string title, int count, real bestSize, int maxSeeds)
        function activateResult(i) { activated = i }
        function summarizeSources(name) {}
    }
    QtObject {
        id: svStub
        property var api: apiStub
        property bool isTitles: true
        property bool browse: false
        property bool visible: true
        property string typeFilter: "all"
        property var owned: null
        property var results: [
            { name: "Harry Potter", type: "series", year: "2026" },
            { name: "Harry Potter and the Philosopher's Stone", type: "movie", year: "2001", rating: 7.9 },
            { name: "Harry Potter and the Chamber of Secrets", type: "movie", year: "2002", rating: 7.7 },
            { name: "Harry Potter and the Prisoner of Azkaban", type: "movie", year: "2004", rating: 8.0 }
        ]
        function typeLabel(t) { return t }
        function runSearch() {}
        function openTitle() {}
        function backTo() {}
        signal addMagnetRequested(string m)
    }

    Component {
        id: paneComp
        SearchTitlesPane { anchors.fill: parent; sv: svStub }
    }

    TestCase {
        name: "SearchTitlesPane"
        when: windowShown

        function tiles(item, out) {
            out = out || []
            for (var i = 0; i < item.children.length; i++) {
                var c = item.children[i]
                if (c.hasOwnProperty("artH") && c.hasOwnProperty("typeLabel") && !c.hasOwnProperty("primaryLabel")) out.push(c)
                tiles(c, out)
            }
            return out
        }

        function copy(o) { return JSON.parse(JSON.stringify(o)) }

        function test_aCopyOfAnItemStillOpensIt() {
            apiStub.activated = -1
            var pane = createTemporaryObject(paneComp, root)
            pane.activate(copy(svStub.results[2]))
            compare(apiStub.activated, 2)
            pane.activate(copy(svStub.results[0]))   // the banner's
            compare(apiStub.activated, 0)
            apiStub.activated = -1
            pane.activate({ name: "Not in the list", type: "movie", year: "1999" })
            compare(apiStub.activated, -1)
        }

        function test_aTileOpensItsTitle() {
            apiStub.activated = -1
            var pane = createTemporaryObject(paneComp, root)
            var t = []
            tryVerify(function () { t = tiles(pane); return t.length >= 2 })
            t[1].activated()   // the second tile: the Chamber of Secrets, results[2]
            compare(apiStub.activated, 2)
            t[0].activated()
            compare(apiStub.activated, 1)
        }

        function test_aFilteredTileOpensItsTitleToo() {
            apiStub.activated = -1
            svStub.typeFilter = "movie"
            var pane = createTemporaryObject(paneComp, root)
            var t = []
            // the best of the films is the banner; the tiles are the rest
            tryVerify(function () { t = tiles(pane); return t.length === 2 })
            t[1].activated()   // the Prisoner of Azkaban, results[3]
            compare(apiStub.activated, 3)
            svStub.typeFilter = "all"
        }
    }
}
