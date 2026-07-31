// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Hub shelves below the continue rails: recent, disco recs, my list, games, movies.
import QtQuick
import QtQuick.Layouts
import "../theme"
import "../widgets"

ColumnLayout {
    id: root
    required property var page

    Layout.fillWidth: true
    spacing: 22

    // Recently added
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sp5
        Layout.rightMargin: Theme.sp5
        spacing: 12
        visible: page.recentlyAdded.length > 0
        Text {
            text: (i18n.language, i18n.t("hub_recent"))
            color: Theme.t1
            font.pixelSize: 17
            font.weight: Font.Bold
            font.family: Theme.fontSans
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 264
            orientation: ListView.Horizontal
            spacing: 18
            clip: true
            model: page.recentlyAdded
            boundsBehavior: Flickable.StopAtBounds
            delegate: HubCard {
                required property var modelData
                host: page
                item: modelData
                isGame: modelData.installState !== undefined
                requireDoubleClick: isGame
                onShowDetail: page.openDetail(modelData, isGame)
                onPlay: isGame ? page.gamePrimary(modelData) : page.playMovie(modelData)
                onContext: isGame ? page.gameMenu.openFor(modelData.infoHash)
                                  : page.continueMenu.openFor(modelData.infoHash, modelData.fileIndex)
            }
        }
    }

    // Recommended for you
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sp5
        Layout.rightMargin: Theme.sp5
        spacing: 12
        visible: page.recommendations.length > 0
        Text {
            text: (i18n.language, i18n.t("hub_recommended"))
            color: Theme.t1
            font.pixelSize: 17
            font.weight: Font.Bold
            font.family: Theme.fontSans
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 268
            orientation: ListView.Horizontal
            spacing: 16
            clip: true
            model: page.recommendations
            boundsBehavior: Flickable.StopAtBounds
            delegate: PosterCard {
                required property var modelData
                posterW: 150
                title: modelData.title || ""
                poster: modelData.poster || ""
                year: modelData.year || ""
                rating: modelData.rating || 0
                type: modelData.type || ""
                synopsis: modelData.overview || ""
                watchlistEnabled: typeof session !== "undefined"
                saved: typeof session !== "undefined"
                       && (session.watchlist, session.inWatchlist(modelData.title, modelData.type))
                onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                    title: modelData.title, type: modelData.type, poster: modelData.poster, year: modelData.year })
                onActivated: page.openSearch(modelData.title || "")
                onGetWatch: if (typeof search !== "undefined")
                                search.getAndWatch(modelData.title || "", modelData.year || "", modelData.type || "movie")
            }
        }
    }

    // Because you watched {X}
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sp5
        Layout.rightMargin: Theme.sp5
        spacing: 12
        visible: page.perTitleRecs.length > 0 && page.recSeed !== null
        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t("hub_because_watched")).arg(page.recSeed ? (page.recSeed.title || "") : "")
            color: Theme.t1
            font.pixelSize: 17
            font.weight: Font.Bold
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 268
            orientation: ListView.Horizontal
            spacing: 16
            clip: true
            model: page.perTitleRecs
            boundsBehavior: Flickable.StopAtBounds
            delegate: PosterCard {
                required property var modelData
                posterW: 150
                title: modelData.title || ""
                poster: modelData.poster || ""
                year: modelData.year || ""
                rating: modelData.rating || 0
                type: modelData.type || ""
                synopsis: modelData.overview || ""
                watchlistEnabled: typeof session !== "undefined"
                saved: typeof session !== "undefined"
                       && (session.watchlist, session.inWatchlist(modelData.title, modelData.type))
                onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                    title: modelData.title, type: modelData.type, poster: modelData.poster, year: modelData.year })
                onActivated: page.openSearch(modelData.title || "")
                onGetWatch: if (typeof search !== "undefined")
                                search.getAndWatch(modelData.title || "", modelData.year || "", modelData.type || "movie")
            }
        }
    }

    // Because you played {X}
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sp5
        Layout.rightMargin: Theme.sp5
        spacing: 12
        visible: page.gameRecs.length > 0 && page.gameSeed !== null
        Text {
            Layout.fillWidth: true
            text: (i18n.language, i18n.t("hub_because_played")).arg(page.gameSeed ? (page.gameSeed.title || "") : "")
            color: Theme.t1
            font.pixelSize: 17
            font.weight: Font.Bold
            font.family: Theme.fontSans
            elide: Text.ElideRight
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 268
            orientation: ListView.Horizontal
            spacing: 16
            clip: true
            model: page.gameRecs
            boundsBehavior: Flickable.StopAtBounds
            delegate: PosterCard {
                required property var modelData
                posterW: 150
                title: modelData.title || ""
                poster: modelData.poster || ""
                year: modelData.year || ""
                rating: modelData.rating || 0
                type: modelData.type || "game"
                synopsis: modelData.overview || ""
                watchlistEnabled: typeof session !== "undefined"
                saved: typeof session !== "undefined"
                       && (session.watchlist, session.inWatchlist(modelData.title, "game"))
                onWatchlistToggle: if (typeof session !== "undefined") session.toggleWatchlist({
                    title: modelData.title, type: "game", poster: modelData.poster, year: modelData.year })
                onActivated: page.openSearch(modelData.title || "")
            }
        }
    }

    // My List
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sp5
        Layout.rightMargin: Theme.sp5
        spacing: 12
        visible: page.api && page.api.watchlist.length > 0
        Text {
            text: (i18n.language, i18n.t("hub_mylist"))
            color: Theme.t1
            font.pixelSize: 17
            font.weight: Font.Bold
            font.family: Theme.fontSans
        }
        ListView {
            Layout.fillWidth: true
            Layout.preferredHeight: 268
            orientation: ListView.Horizontal
            spacing: 16
            clip: true
            model: page.api ? page.api.watchlist : []
            boundsBehavior: Flickable.StopAtBounds
            delegate: PosterCard {
                required property var modelData
                posterW: 150
                title: modelData.title || ""
                poster: modelData.poster || ""
                year: modelData.year || ""
                type: modelData.type || ""
                watchlistEnabled: true
                saved: true
                synopsis: modelData.overview || ""
                onWatchlistToggle: if (page.api) page.api.toggleWatchlist(modelData)
                onActivated: page.openSearch(modelData.title || "")
                onGetWatch: if (typeof search !== "undefined")
                                search.getAndWatch(modelData.title || "", modelData.year || "", modelData.type || "movie")
            }
        }
    }

    // Your games
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sp5
        Layout.rightMargin: Theme.sp5
        spacing: 12
        visible: page.gameItems.length > 0
        Text {
            text: (i18n.language, i18n.t("hub_games"))
            color: Theme.t1
            font.pixelSize: 17
            font.weight: Font.Bold
            font.family: Theme.fontSans
        }
        GridLayout {
            Layout.fillWidth: true
            columnSpacing: 18
            rowSpacing: 20
            columns: Math.max(1, Math.floor((page.width - 2 * Theme.sp5 + columnSpacing) / (150 + columnSpacing)))
            Repeater {
                model: page.applyView(page.gameItems)
                delegate: HubCard {
                    host: page
                    item: modelData
                    isGame: true
                    requireDoubleClick: true
                    onShowDetail: page.openDetail(modelData, true)
                    onPlay: page.gamePrimary(modelData)
                    onContext: page.gameMenu.openFor(modelData.infoHash)
                }
            }
        }
    }

    // Your movies
    ColumnLayout {
        Layout.fillWidth: true
        Layout.leftMargin: Theme.sp5
        Layout.rightMargin: Theme.sp5
        Layout.bottomMargin: Theme.sp5
        spacing: 12
        visible: page.library.length > 0
        Text {
            text: (i18n.language, i18n.t("hub_movies"))
            color: Theme.t1
            font.pixelSize: 17
            font.weight: Font.Bold
            font.family: Theme.fontSans
        }
        GridLayout {
            Layout.fillWidth: true
            columnSpacing: 18
            rowSpacing: 20
            columns: Math.max(1, Math.floor((page.width - 2 * Theme.sp5 + columnSpacing) / (150 + columnSpacing)))
            Repeater {
                model: page.applyView(page.library)
                delegate: HubCard {
                    host: page
                    item: modelData
                    onShowDetail: page.openDetail(modelData, false)
                    onPlay: page.playMovie(modelData)
                    onContext: page.continueMenu.openFor(modelData.infoHash, modelData.fileIndex)
                }
            }
        }
    }
}
