// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "../theme"
import "../widgets"

// Hub context menus + exe picker. `page` is the HubView host.
Item {
    id: root
    required property var page

    property alias exePicker: exePicker
    property alias gameMenu: gameMenu
    property alias continueMenu: continueMenu
    property alias episodeMenu: episodeMenu

    // gameMenu's rows: a touch taller than a context menu's, squarer highlight.
    component GameItem: BatMenuItem {
        rowHeight: 34
        highlightRadius: 4
    }

    FileDialog {
        id: exePicker
        property string pendingHash: ""
        property bool launchAfter: true
        title: (i18n.language, i18n.t("hub_set_exe"))
        onAccepted: {
            if (!page.api || pendingHash.length === 0) return
            page.api.setGameExe(pendingHash, selectedFile.toString())
            page.refresh()
            if (launchAfter) page.api.launchGame(pendingHash)
        }
    }

    // The game's other actions. From a card's context or the gear it opens at
    // the cursor; from the split button's chevron it drops under the button,
    // at least as wide, with the button's squarer corners.
    BatMenu {
        id: gameMenu
        property string hash: ""
        // Its torrent is gone and it stays as an installed game: nothing to install.
        property bool torrentless: false
        // the game's own uninstaller and what it frees, when Windows lists one
        property var uninstaller: ({})
        property string gameTitle: ""
        function refresh(h) {
            hash = h
            torrentless = !!page.api && page.api.gameTorrentless(h)
            uninstaller = page.api ? page.api.gameUninstaller(h) : ({})
            var g = (page.gameItems || []).filter(function (x) { return x.infoHash === h })[0]
            gameTitle = g ? g.title : ""
        }
        function openFor(h) { refresh(h); width = 260; popup() }
        function dropFrom(h, anchorItem) {
            refresh(h)
            width = Math.max(260, anchorItem.width)
            popup(anchorItem, 0, anchorItem.height + 6)
        }
        implicitWidth: 260
        topPadding: 5
        bottomPadding: 5
        background: Rectangle {
            color: Theme.panel
            border.color: Theme.hair
            border.width: 1
            radius: Theme.rad(6)
        }
        GameItem {
            text: (i18n.language, i18n.t("hub_gs_install"))
            iconSrc: "qrc:/icons/download.svg"
            enabled: !gameMenu.torrentless
            hideWhenDisabled: true
            onTriggered: if (page.api) page.api.installGame(gameMenu.hash)
        }
        GameItem {
            text: (i18n.language, i18n.t("hub_install_defender"))
            iconSrc: "qrc:/icons/shield-check.svg"
            enabled: Qt.platform.os === "windows" && !gameMenu.torrentless
            hideWhenDisabled: true
            onTriggered: if (page.api) page.api.installGameExcluded(gameMenu.hash)
        }
        GameItem {
            text: (i18n.language, i18n.t("hub_set_exe"))
            iconSrc: "qrc:/icons/file.svg"
            onTriggered: page.openExePicker(gameMenu.hash, false)
        }
        GameItem {
            text: (i18n.language, i18n.t("game_args_item"))
            iconSrc: "qrc:/icons/settings.svg"
            onTriggered: page.gameActionRequested("args", gameMenu.hash, gameMenu.gameTitle)
        }
        // the download checked against itself: broken pieces fetched again
        GameItem {
            text: (i18n.language, i18n.t("game_verify_item"))
            iconSrc: "qrc:/icons/shield-check.svg"
            enabled: !gameMenu.torrentless
            hideWhenDisabled: true
            onTriggered: if (page.api) page.api.recheckGame(gameMenu.hash)
        }
        MenuSeparator {
            topPadding: 4
            bottomPadding: 4
            contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft }
        }
        GameItem {
            text: (i18n.language, i18n.t("hub_open_folder"))
            iconSrc: "qrc:/icons/folder-open.svg"
            onTriggered: if (page.api) Qt.openUrlExternally(page.fileUrl(page.api.gameFolder(gameMenu.hash)))
        }
        GameItem {
            text: (i18n.language, gameMenu.uninstaller.size
                   ? i18n.t("game_uninstall_item").arg(gameMenu.uninstaller.size) : i18n.t("game_uninstall_item_plain"))
            iconSrc: "qrc:/icons/trash.svg"
            enabled: !!gameMenu.uninstaller.exe
            hideWhenDisabled: true
            onTriggered: page.gameActionRequested("uninstall", gameMenu.hash, gameMenu.gameTitle)
        }
        GameItem {
            text: (i18n.language, i18n.t("hub_forget_game"))
            iconSrc: "qrc:/icons/trash.svg"
            enabled: gameMenu.torrentless
            hideWhenDisabled: true
            onTriggered: if (page.api) page.api.forgetGame(gameMenu.hash)
        }
    }

    BatMenu {
        id: continueMenu
        property string hash: ""
        property int fileIdx: 0
        function openFor(h, f) { hash = h; fileIdx = f || 0; popup() }
        implicitWidth: 210
        BatMenuItem {
            text: (i18n.language, i18n.t("hub_remove_continue"))
            onTriggered: {
                if (page.api) page.api.clearResume(continueMenu.hash, continueMenu.fileIdx)
                page.refresh()
            }
        }
    }

    BatMenu {
        id: episodeMenu
        property string hash: ""
        property var videos: []
        property int tmdbId: 0
        property var titles: ({})
        function openFor(item) {
            hash = item.infoHash
            videos = item.videos || []
            tmdbId = item.tmdbId || 0
            titles = ({})
            if (tmdbId > 0 && page.disco) {
                var seen = ({})
                for (var i = 0; i < videos.length; i++) {
                    var sn = videos[i].season
                    if (sn >= 0 && !seen[sn]) { seen[sn] = true; page.disco.fetchEpisodes(tmdbId, sn) }
                }
            }
            popup()
        }
        implicitWidth: 380
        Connections {
            // episodesReady comes from the discovery service, not the session:
            // this listened to the wrong object, so the TMDB titles never arrived.
            target: page.disco
            ignoreUnknownSignals: true
            function onEpisodesReady(tmdbId, season, episodes) {
                if (tmdbId !== episodeMenu.tmdbId) return
                var t = Object.assign({}, episodeMenu.titles)
                for (var i = 0; i < episodes.length; i++)
                    t[season + "_" + episodes[i].episode] = episodes[i].name
                episodeMenu.titles = t
            }
        }
        Repeater {
            model: episodeMenu.videos
            BatMenuItem {
                id: epItem
                required property var modelData
                text: {
                    var m = epItem.modelData
                    var check = m.watched ? "✓  " : ""
                    if (m.season >= 0 && m.episode >= 0) {
                        var title = episodeMenu.titles[m.season + "_" + m.episode] || m.name
                        return check + "S" + m.season + "·E" + (m.episode < 10 ? "0" + m.episode : m.episode) + "  —  " + title
                    }
                    return check + m.name
                }
                elideMode: Text.ElideMiddle
                // Its own torrent, not the menu's: a series card spans every
                // release that holds part of the show, so S01E01 and S02E01
                // can live in different ones.
                onTriggered: if (page.api) page.api.playFile(
                    epItem.modelData.hash || episodeMenu.hash, epItem.modelData.idx)
            }
        }
    }
}
