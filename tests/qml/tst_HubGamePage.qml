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
        signal achievementsReady(string hash, int appId, var list, int unlocked, int total)
        property var askedAch: []
        function requestAchievements(h, hint, title) { askedAch.push(h + ":" + hint + ":" + title) }
        function steamArt(id) { return { hero: "https://cdn/" + id + "/hero.jpg", logo: "https://cdn/" + id + "/logo.png" } }
        property var launched: []
        function gameFolder(h) { return "/games/" + h }
        function gameExe(h) { return h === "played" ? "/games/played/game.exe" : "" }
        function launchGame(h) { launched.push(h) }
        function gameActivity(h, days) {
            var out = []
            for (var i = 0; i < days; i++) out.push(h === "played" && i >= days - 2 ? 4500 : 0)
            return out
        }
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
              lastPlayed: Date.now() - 2 * 3600000, playSeconds: 9000, installState: 4,
              genres: ["RPG"], group: "FitGirl", addedTime: 1760000000 }
        ]
        function fileUrl(p) { return p ? "file://" + p : "" }
        function fmtPlaytime(s) { return s > 0 ? Math.floor(s / 3600) + "h " + Math.floor((s % 3600) / 60) + "m" : "" }
        function fmtAgo(ms) { return ms > 0 ? "2h" : "" }
        function hours(s) { return (s > 0 ? (s / 3600).toFixed(1) : "0") + " h" }
        function fmtSize(b) { return b > 0 ? (b / 1073741824).toFixed(1) + " GB" : "" }
        function gameStateLabel(it) { return it ? "hub_gs_play" : "" }
        function gameStateActionable(it) { return !!it && it.installState === 4 }
        function gamePrimary(it) { mockApi.launchGame(it.infoHash) }
        function gameMenuOpenFolder(h) {}
        function openExePicker(h, launch) {}
        property QtObject gameMenu: QtObject {
            property string opened: ""
            function openFor(h) { opened = h }
        }
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
            mockApi.askedAch = []
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
            verify(findByText(p, "game_playtime") !== null)
            verify(findByText(p, "—") !== null)
            verify(findByText(p, "game_activity") === null)
            verify(findByText(p, "game_build") === null)
            mockHub.gameHash = "played"
            verify(findByText(p, "2.5 h") !== null)
            verify(findByText(p, "2.0 GB") !== null)
            verify(findByText(p, "game_activity") !== null)
            verify(findByText(p, "game_activity_window") !== null)
            verify(findByText(p, "FitGirl") !== null)
            verify(findByText(p, "game.exe") !== null)
        }

        function test_settingsOpensTheGameMenu() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            var actions = null
            var stack = [p]
            while (stack.length && !actions) {
                var it = stack.pop()
                if (it.settingsRequested !== undefined) actions = it
                for (var i = 0; i < it.children.length; i++) stack.push(it.children[i])
                if (it.contentItem) stack.push(it.contentItem)
            }
            verify(actions !== null)
            actions.settingsRequested()
            compare(mockHub.gameMenu.opened, "played")
        }

        function findHero(p) {
            var stack = [p]
            while (stack.length) {
                var it = stack.pop()
                if (it.artChain !== undefined) return it
                for (var i = 0; i < it.children.length; i++) stack.push(it.children[i])
                if (it.contentItem) stack.push(it.contentItem)
            }
            return null
        }

        function achList(n) {
            var out = []
            for (var i = 0; i < n; i++)
                out.push({ apiName: "A" + i, name: "Ach " + i, description: "d" + i, icon: "", iconGray: "",
                           hidden: i === n - 1, percent: i === 0 ? 0.4 : 41, unlocked: i < 2,
                           unlockedAt: i === 0 ? 1700000000 : 0 })
            return out
        }

        function test_achievementsAreAskedForOnceTheDetailsKnowTheAppid() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            compare(mockApi.askedAch.length, 0, "not before IGDB has answered")
            mockDisco.gameDetailsReady("Elden Ring", { steamAppId: 1245620 })
            compare(mockApi.askedAch, ["played:1245620:Elden Ring"])
        }

        function test_achievementsFillTheBandAndTheList() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            mockDisco.gameDetailsReady("Elden Ring", { steamAppId: 1 })
            mockApi.achievementsReady("someone-else", 0, achList(3), 3, 3)
            verify(findByText(p, "game_achievements") === null, "another game's answer is ignored")
            mockApi.achievementsReady("played", 1, achList(7), 2, 7)
            verify(findByText(p, "2 / 7") !== null)
            verify(findByText(p, "Ach 0") !== null)
            verify(findByText(p, "Ach 5") === null, "five until asked for all")
            verify(findByText(p, "game_ach_all") !== null)
            verify(findByText(p, "game_ach_locked") !== null)
            verify(findByText(p, "game_ach_unlocked") !== null, "an unlock with no time says so")
        }

        function test_lockedSecretsAreOneLineNotARowEach() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            var list = achList(3)
            list.push({ apiName: "S2", name: "Secret two", description: "", icon: "", iconGray: "",
                        hidden: true, percent: 5, unlocked: false, unlockedAt: 0 })
            mockApi.achievementsReady("played", 1, list, 2, 4)
            verify(findByText(p, "Ach 2") === null, "a locked secret keeps its name")
            verify(findByText(p, "Secret two") === null)
            verify(findByText(p, "+2") !== null, "both secrets counted on one line")
            verify(findByText(p, "game_ach_secrets_left") !== null)
        }

        function test_heroWaitsForDetailsInsteadOfStretchingTheCover() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameItems[1].poster = "/covers/elden.jpg"
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            var hero = findHero(p)
            compare(hero.artChain, [])
            mockDisco.gameDetailsReady("Elden Ring", {})
            compare(hero.artChain, ["file:///covers/elden.jpg"])
            mockHub.gameItems[1].poster = ""
        }

        function test_heroPrefersSteamArtWhenTheAppIdIsKnown() {
            var p = createTemporaryObject(pageComp, root)
            mockHub.gameHash = "played"
            mockHub.gameOpen = true
            mockDisco.gameDetailsReady("Elden Ring", { steamHero: "https://cdn/1245620/library_hero.jpg",
                                                       backdrop: "https://x/art.jpg" })
            var hero = null
            var stack = [p]
            while (stack.length && !hero) {
                var it = stack.pop()
                if (it.artChain !== undefined) hero = it
                for (var i = 0; i < it.children.length; i++) stack.push(it.children[i])
                if (it.contentItem) stack.push(it.contentItem)
            }
            verify(hero !== null)
            compare(hero.artChain.length, 2)
            verify(hero.artChain[0].indexOf("/1245620/library_hero.jpg") > 0)
            compare(hero.artChain[1], "https://x/art.jpg")
            mockDisco.gameDetailsReady("Elden Ring", { backdrop: "https://x/art.jpg" })
            compare(hero.artChain, ["https://x/art.jpg"])
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
