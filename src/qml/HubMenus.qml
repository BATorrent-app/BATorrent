// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Dialogs
import "widgets"

// Hub context menus + exe picker. `page` is the HubView host.
Item {
    id: root
    required property var page

    property alias exePicker: exePicker
    property alias gameMenu: gameMenu
    property alias continueMenu: continueMenu
    property alias episodeMenu: episodeMenu

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

    BatMenu {
        id: gameMenu
        property string hash: ""
        function openFor(h) { hash = h; popup() }
        implicitWidth: 200
        BatMenuItem {
            text: (i18n.language, i18n.t("hub_gs_install"))
            onTriggered: if (page.api) page.api.installGame(gameMenu.hash)
        }
        BatMenuItem {
            text: (i18n.language, i18n.t("hub_set_exe"))
            onTriggered: page.openExePicker(gameMenu.hash, false)
        }
        BatMenuItem {
            text: (i18n.language, i18n.t("hub_open_folder"))
            onTriggered: if (page.api) Qt.openUrlExternally(page.fileUrl(page.api.gameFolder(gameMenu.hash)))
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
            if (tmdbId > 0 && page.api) {
                var seen = ({})
                for (var i = 0; i < videos.length; i++) {
                    var sn = videos[i].season
                    if (sn >= 0 && !seen[sn]) { seen[sn] = true; page.api.fetchEpisodes(tmdbId, sn) }
                }
            }
            popup()
        }
        implicitWidth: 380
        Connections {
            target: page.api
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
                onTriggered: if (page.api) page.api.playFile(episodeMenu.hash, epItem.modelData.idx)
            }
        }
    }
}
