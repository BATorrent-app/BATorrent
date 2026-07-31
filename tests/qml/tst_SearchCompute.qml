// SPDX-License-Identifier: MIT
// Search results model bind: fixture bridge results through SearchCompute
// filter/sort must produce a stable viewModel. Fails if Find-page wiring
// to results / quality filter / seeders sort regresses.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

Item {
    id: root
    width: 400
    height: 200

    QtObject {
        id: mockApi
        property var results: [
            { name: "A 720p", quality: "720p", source: "WEB", seedsN: 5,
              sizeBytes: 100, releaseGroup: "G1", provider: "p1", langs: ["EN"],
              audioMode: "original", season: 0, episode: 0, pack: false, version: "" },
            { name: "B 1080p", quality: "1080p", source: "BluRay", seedsN: 80,
              sizeBytes: 200, releaseGroup: "G2", provider: "p1", langs: ["EN"],
              audioMode: "original", season: 0, episode: 0, pack: false, version: "" },
            { name: "C 1080p", quality: "1080p", source: "WEB", seedsN: 20,
              sizeBytes: 150, releaseGroup: "G1", provider: "p2", langs: ["PT"],
              audioMode: "dub", season: 0, episode: 0, pack: false, version: "" }
        ]
        property string workType: "movie"
        property string mode: "torrent"
        property bool singleTitleView: false
        function queryWordSets() { return [] }
        function relevanceMulti(name, qsets) { return 0 }
        function compareBuildVersions(a, b) { return 0 }
        function compareGameReleases(a, b) { return 0 }
    }

    QtObject {
        id: mockHost
        property var api: mockApi
        property string qualityFilter: ""
        property string sourceFilter: ""
        property string groupFilter: ""
        property string providerFilter: ""
        property string langFilter: ""
        property string audioModeFilter: ""
        property int minSeeds: 0
        property string sortKey: ""
        property int seasonFilter: -2
        property int episodeFilter: -1
        property bool isEpisodes: false
        property bool isSeriesDrill: false
        property bool showCatalogBrowse: false
    }

    Component {
        id: computeComp
        SearchCompute { sv: mockHost }
    }

    TestCase {
        name: "SearchCompute"
        when: windowShown
        width: 400
        height: 200

        function test_viewModelBindsAllResults() {
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            compare(c.viewModel.length, 3)
            compare(c.qualityOptions.length, 2)
            compare(c.qualityOptions[0], "1080p")
            compare(c.qualityOptions[1], "720p")
        }

        function test_qualityFilterShrinksViewModel() {
            mockHost.qualityFilter = "1080p"
            mockHost.sortKey = ""
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            compare(c.viewModel.length, 2)
            compare(c.viewModel[0].quality, "1080p")
            compare(c.viewModel[1].quality, "1080p")
            mockHost.qualityFilter = ""
        }

        function test_sortBySeedersOrdersDescending() {
            mockHost.qualityFilter = ""
            mockHost.sortKey = "seeders"
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            compare(c.viewModel.length, 3)
            compare(c.viewModel[0].seedsN, 80)
            compare(c.viewModel[1].seedsN, 20)
            compare(c.viewModel[2].seedsN, 5)
            mockHost.sortKey = ""
        }

        function test_minSeedsDropsLowPeers() {
            mockHost.minSeeds = 30
            mockHost.sortKey = ""
            var c = createTemporaryObject(computeComp, root)
            verify(!!c, "Object exists")
            compare(c.viewModel.length, 1)
            compare(c.viewModel[0].name, "B 1080p")
            mockHost.minSeeds = 0
        }
    }
}
