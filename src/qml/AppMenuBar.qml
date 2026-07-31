// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import Qt.labs.platform as Platform

// Native/global menu bar. Must be instantiated as a direct Window child on macOS.
Platform.MenuBar {
    required property var host
    required property var openFileDialog
    required property var magnetDialog
    required property var inputPrompt
    required property var createDialog
    required property var inspectFileDialog
    required property var importQbtDialog
    required property var removeDialog
    required property var addAddonDialog
    required property var pairingDialog
    required property var tourOverlay
    required property var welcomeDialog
    required property var releaseNotesDialog
    required property var aboutDialog


    Platform.Menu {
        title: (i18n.language, i18n.t("menu_file_title"))
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_open_torrent")); shortcut: StandardKey.Open; onTriggered: openFileDialog.open() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_add_magnet")); shortcut: "Ctrl+M"; onTriggered: magnetDialog.open() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_add_url")); shortcut: "Ctrl+U"; onTriggered: inputPrompt.openWith(i18n.t("menu_add_url"), i18n.t("prompt_torrent_url"), "", "https://…/file.torrent", function(t){ if (t.length > 0) session.addTorrentUrl(t) }) }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_add_http")); shortcut: "Ctrl+D"; onTriggered: host.promptHttpDownload() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_create_torrent")); onTriggered: createDialog.open() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_inspect_torrent")); onTriggered: inspectFileDialog.open() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_import_qbt")); onTriggered: importQbtDialog.open() }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_recently_removed")); onTriggered: host.showWin(host.removedWinLoader) }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_quit")); shortcut: StandardKey.Quit; onTriggered: Qt.quit() }
    }
    Platform.Menu {
        title: (i18n.language, i18n.t("menu_torrent_title"))
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_pause")); enabled: host.hasSel; onTriggered: session.pauseSelected() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_resume")); enabled: host.hasSel; onTriggered: session.resumeSelected() }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_pause_all")); onTriggered: if (typeof session !== "undefined") session.pauseAll() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_resume_all")); onTriggered: if (typeof session !== "undefined") session.resumeAll() }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_remove")); shortcut: StandardKey.Delete; enabled: host.hasSel; onTriggered: removeDialog.open() }
    }
    Platform.Menu {
        title: (i18n.language, i18n.t("menu_settings_title"))
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_preferences")); shortcut: StandardKey.Preferences; onTriggered: host.currentPage = 3 }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_addons")); onTriggered: addAddonDialog.open() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_rss")); onTriggered: host.showWin(host.rssWinLoader) }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_pair")); onTriggered: { pairingDialog.reload(); pairingDialog.open() } }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_search_torrents")); onTriggered: host.currentPage = 1 }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_statistics")); onTriggered: host.showWin(host.statsWinLoader) }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_speedtest")); onTriggered: Qt.openUrlExternally("https://fast.com") }
    }
    Platform.Menu {
        title: (i18n.language, i18n.t("menu_help_title"))
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_tour")); onTriggered: tourOverlay.start() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_whatsnew")); onTriggered: { welcomeDialog.mode = "update"; welcomeDialog.open() } }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_release_notes")); onTriggered: releaseNotesDialog.open() }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_shortcuts")); onTriggered: host.showWin(host.shortcutsWinLoader) }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_logs")); shortcut: "Ctrl+Shift+L"; onTriggered: host.showWin(host.logWinLoader) }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_diagnostics")); onTriggered: host.showWin(host.diagWinLoader) }
        Platform.MenuItem {
            text: (i18n.language, i18n.t("menu_check_updates"))
            enabled: typeof updater !== "undefined" && updater !== null
            onTriggered: if (typeof updater !== "undefined" && updater) updater.check(false)
        }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_feedback")); onTriggered: Qt.openUrlExternally("https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform") }
        Platform.MenuSeparator {}
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_donate")); onTriggered: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19") }
        Platform.MenuItem { text: (i18n.language, i18n.t("menu_about")); role: Platform.MenuItem.AboutRole; onTriggered: aboutDialog.open() }
    }
}
