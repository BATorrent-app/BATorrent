// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import "theme"
import "widgets"

// The app menu behind the logo, on Windows and Linux (macOS keeps the system
// menu bar). The four menus that used to be a bar of their own become
// submenus, with the few things people reach for most right below them.
DMenu {
    id: root
    required property var host
    required property var openFileDialog
    required property var addDialog
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
    required property var settingsPage

    width: 248

    DMenu {
        sub: true
        title: (i18n.language, i18n.t("menu_file_title"))
        DMenuItem { iconSrc: "qrc:/icons/file.svg"; text: (i18n.language, i18n.t("menu_open_torrent")); onTriggered: root.openFileDialog.open() }
        DMenuItem { iconSrc: "qrc:/icons/magnet.svg"; text: (i18n.language, i18n.t("menu_add_magnet")); onTriggered: root.addDialog.open() }
        DMenuItem { iconSrc: "qrc:/icons/link.svg"; text: (i18n.language, i18n.t("menu_add_http")); onTriggered: root.host.promptHttpDownload() }
        DMenuItem { iconSrc: "qrc:/icons/plus.svg"; text: (i18n.language, i18n.t("menu_create_torrent")); onTriggered: root.createDialog.open() }
        DMenuItem { iconSrc: "qrc:/icons/search.svg"; text: (i18n.language, i18n.t("menu_inspect_torrent")); onTriggered: root.inspectFileDialog.open() }
        DMenuItem { iconSrc: "qrc:/icons/download.svg"; text: (i18n.language, i18n.t("menu_import_qbt")); onTriggered: root.importQbtDialog.open() }
        DMenuSep {}
        DMenuItem { iconSrc: "qrc:/icons/clock.svg"; text: (i18n.language, i18n.t("menu_recently_removed")); onTriggered: root.host.showWin(root.host.removedWinLoader) }
    }
    DMenu {
        sub: true
        title: (i18n.language, i18n.t("menu_torrent_title"))
        DMenuItem { iconSrc: "qrc:/icons/pause.svg"; text: (i18n.language, i18n.t("menu_pause")); enabled: root.host.hasSel; onTriggered: session.pauseSelected() }
        DMenuItem { iconSrc: "qrc:/icons/play.svg"; text: (i18n.language, i18n.t("menu_resume")); enabled: root.host.hasSel; onTriggered: session.resumeSelected() }
        DMenuSep {}
        DMenuItem { iconSrc: "qrc:/icons/pause.svg"; text: (i18n.language, i18n.t("menu_pause_all")); onTriggered: if (typeof session !== "undefined") session.pauseAll() }
        DMenuItem { iconSrc: "qrc:/icons/play.svg"; text: (i18n.language, i18n.t("menu_resume_all")); onTriggered: if (typeof session !== "undefined") session.resumeAll() }
        DMenuSep {}
        DMenuItem { iconSrc: "qrc:/icons/refresh.svg"; text: (i18n.language, i18n.t("tb_refresh")); shortcutText: "F5"; onTriggered: root.host.refreshView() }
        DMenuItem { iconSrc: "qrc:/icons/trash.svg"; text: (i18n.language, i18n.t("menu_remove")); enabled: root.host.hasSel; onTriggered: root.removeDialog.open() }
    }
    DMenu {
        sub: true
        title: (i18n.language, i18n.t("menu_settings_title"))
        DMenuItem { iconSrc: "qrc:/icons/settings.svg"; text: (i18n.language, i18n.t("menu_preferences")); onTriggered: root.host.currentPage = 3 }
        DMenuItem { iconSrc: "qrc:/icons/set-addons.svg"; text: (i18n.language, i18n.t("menu_addons")); onTriggered: root.addAddonDialog.open() }
        DMenuItem { iconSrc: "qrc:/icons/rss.svg"; text: (i18n.language, i18n.t("menu_rss")); onTriggered: root.host.showWin(root.host.rssWinLoader) }
        DMenuItem { iconSrc: "qrc:/icons/share.svg"; text: (i18n.language, i18n.t("menu_pair")); onTriggered: { root.pairingDialog.reload(); root.pairingDialog.open() } }
        DMenuSep {}
        DMenuItem { iconSrc: "qrc:/icons/gauge.svg"; text: (i18n.language, i18n.t("menu_statistics")); onTriggered: root.host.showWin(root.host.statsWinLoader) }
        DMenuItem { iconSrc: "qrc:/icons/bolt.svg"; text: (i18n.language, i18n.t("menu_speedtest")); onTriggered: Qt.openUrlExternally("https://fast.com") }
    }
    DMenu {
        sub: true
        title: (i18n.language, i18n.t("menu_help_title"))
        DMenuItem { iconSrc: "qrc:/icons/sliders.svg"; text: (i18n.language, i18n.t("menu_setup_wizard")); onTriggered: { root.welcomeDialog.mode = "welcome"; root.welcomeDialog.open() } }
        DMenuItem { iconSrc: "qrc:/icons/discover.svg"; text: (i18n.language, i18n.t("menu_tour")); onTriggered: root.tourOverlay.start() }
        DMenuItem { iconSrc: "qrc:/icons/info.svg"; text: (i18n.language, i18n.t("menu_whatsnew")); onTriggered: { root.welcomeDialog.mode = "update"; root.welcomeDialog.open() } }
        DMenuItem { iconSrc: "qrc:/icons/file.svg"; text: (i18n.language, i18n.t("menu_release_notes")); onTriggered: root.releaseNotesDialog.open() }
        DMenuItem { iconSrc: "qrc:/icons/keyboard.svg"; text: (i18n.language, i18n.t("menu_shortcuts")); onTriggered: root.host.showWin(root.host.shortcutsWinLoader) }
        DMenuItem { iconSrc: "qrc:/icons/list.svg"; text: (i18n.language, i18n.t("menu_logs")); onTriggered: root.host.showWin(root.host.logWinLoader) }
        DMenuItem { iconSrc: "qrc:/icons/shield-check.svg"; text: (i18n.language, i18n.t("menu_diagnostics")); onTriggered: root.host.showWin(root.host.diagWinLoader) }
        DMenuItem {
            iconSrc: "qrc:/icons/refresh.svg"
            text: (i18n.language, i18n.t("menu_check_updates"))
            enabled: typeof updater !== "undefined" && updater !== null
            onTriggered: if (typeof updater !== "undefined" && updater) updater.check(false)
        }
        DMenuItem { iconSrc: "qrc:/icons/external.svg"; text: (i18n.language, i18n.t("menu_feedback")); onTriggered: Qt.openUrlExternally("https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform") }
    }
    DMenuSep {}
    DMenuItem { text: (i18n.language, i18n.t("menu_add_torrent_short")); shortcutText: "Ctrl+O"; onTriggered: root.openFileDialog.open() }
    DMenuItem { text: (i18n.language, i18n.t("menu_paste_magnet")); shortcutText: "Ctrl+V"; onTriggered: root.addDialog.open() }
    DMenuSep {}
    DMenuItem { text: (i18n.language, i18n.t("menu_donate")); tone: "accent"; onTriggered: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19") }
    DMenuItem { text: (i18n.language, i18n.t("menu_about")); onTriggered: root.aboutDialog.open() }
    DMenuItem { text: (i18n.language, i18n.t("menu_quit")); shortcutText: "Ctrl+Q"; onTriggered: Qt.quit() }
}
