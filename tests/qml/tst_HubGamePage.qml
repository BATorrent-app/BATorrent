// SPDX-License-Identifier: MIT
// Game page: the live item is found by hash, IGDB details are asked for by
// title and only accepted for that title, and the stats band tells a played
// game from an unplayed one.

import QtQuick
import QtTest
import "qrc:/src/qml/views"

Item {
    id: root
    width: 1000
    height: 700

    QtObject {
        id: mockDisco
        signal gameDetailsReady(string name, var details)
        property var asked: []
        function fetchGameDetails(name) { asked.push(name) }
    }

    QtObject {
        id: mockApi
        property var launched: []
        function gameFolder(h) { return "/games/" + h }
        function gameExe(h) { return h === "played" ? "/games/played/game.exe" : "" }
        function launchGame(h) { launched.push(h) }
    }

    QtObject {
        id: mockHub
        property var api: mockApi
        property var disco: mockDisco
        property string gameHash: ""
        property bool gameOpen: false
        property var gameItems: [
            { infoHash: "fresh", title: "Hades", poster: "", size: 1073741824,
              lastPlayed: 0, playSeconds: 0, installState: 4, genres: [] },
            { infoHash: "played", title: "Elden Ring", poster: "", size: 2147483648,
              lastPlayed: Date.now() - 2 * 3600000, playSeconds: 7380, installState: 4,
              genres: ["RPG"] }
        ]
        function fileUrl(p) { return p ? "file://" + p : "" }
        function fmtPlaytime(s) { return s > 0 ? Math.floor(s / 3600) + "h " + Math.floor((s % 3600) / 60) + "m" : "" }
        function fmtAgo(ms) { return ms > 0 ? "2h" : "" }
        function fmtSize(b) { return b > 0 ? (b / 1073741824).toFixed(1) + " GB" : "" }
        function gameStateLabel(it) { return it ? "hub_gs_play" : "" }
        function gameStateActionable(it) { return !!it && it.installState === 4 }
        function gamePrimary(it) { mockApi.launchGame(it.infoHash) }
        function gameMenuOpenFolder(h) {}
        function openExePicker(h, launch) {}
    }

    Component {
        id: pageComp
        HubGamePage { anchors.fill: parent; hub: mockHub }
    }

    TestCase {
        name: "HubGamePage"
        when: windowShown

        function init() {
            mockDisco.asked = []
            mockApi.launched = []
            mockHub.gameOpen = false
            mockHub.gameHash = ""
        }

        function findByText(item, text) {
            if (item.text !== undefined && item.text === text && item.visible) return item
            for (var i = 0; i < item.children.length; i++) {
                var hit = findByText(item.children[i], text)
                if (hit) return hit
            }
            if (item.contentItem) return findByText(item.contentItem, text)
            return null
        }

        function test_resolvesTheLiveItemAndAsksForItsDetails() {
            var p = createTemporaryObject(pageComp, root)
            verify(!!p)
            compare(p.visible, false)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            compare(p.game.title, "Elden Ring")
            compare(mockDisco.asked, ["Elden Ring"])
            tryCompare(p, "slide", 1)
        }

        function test_acceptsDetailsOnlyForTheOpenGame() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            mockDisco.gameDetailsReady("Hades", { maker: "Supergiant" })
            compare(p.details.maker, undefined)
            mockDisco.gameDetailsReady("Elden Ring",
                { maker: "FromSoftware", stills: ["https://x/s1.jpg", "https://x/s2.jpg"] })
            compare(p.details.maker, "FromSoftware")
            compare(p.details.stills.length, 2)
        }

        function test_switchingGamesDropsTheOldDetails() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            mockDisco.gameDetailsReady("Elden Ring", { maker: "FromSoftware" })
            mockHub.gameHash = "fresh"
            compare(p.details.maker, undefined)
            compare(mockDisco.asked, ["Elden Ring", "Hades"])
        }

        function test_statsBandTellsPlayedFromUnplayed() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "fresh"
            mockHub.gameOpen = true
            verify(findByText(p, "game_never_played") !== null)
            verify(findByText(p, "game_playtime") === null)
            mockHub.gameHash = "played"
            verify(findByText(p, "game_never_played") === null)
            verify(findByText(p, "2h 3m") !== null)
            verify(findByText(p, "/games/played/game.exe") !== null)
        }

        function test_unsavedExeReadsAsAutodetected() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "fresh"
            mockHub.gameOpen = true
            verify(findByText(p, "game_exe_auto") !== null)
            verify(findByText(p, "/games/fresh") !== null)
        }

        function test_pickedStillOpensAndClosesTheLightbox() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            p.stillOpen = "https://x/s1.jpg"
            compare(p.stillShown, "https://x/s1.jpg")
            p.stillOpen = ""
            // kept so the image fades out with the scrim
            compare(p.stillShown, "https://x/s1.jpg")
        }
    }
}
