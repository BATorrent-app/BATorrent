// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "theme"
import "widgets"

// In-window MenuBar for Windows/Linux (platform MenuBar is empty there).
MenuBar {
    id: root
    required property var host
    required property var openFileDialog
    required property var magnetDialog
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
    required property var makeRoomPanel
    required property var settingsPage


    visible: Qt.platform.os !== "osx"
    Layout.fillWidth: true
    Layout.preferredHeight: visible ? implicitHeight : 0

    background: Rectangle {
        color: Theme.panel
        Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hair }
    }
    delegate: MenuBarItem {
        id: mbarItem
        padding: 6; leftPadding: 12; rightPadding: 12
        contentItem: Text {
            text: mbarItem.text
            color: Theme.t1
            font.pixelSize: 13
            font.family: Theme.fontSans
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: (mbarItem.highlighted || mbarItem.down) ? Theme.hover : "transparent"
            radius: 5
        }
    }

    // Same icon column as the right-click menu's CtxItem (icon at x:14, text at
    // 38): the two menus are the same vocabulary reached two ways, and one of
    // them having icons made the other look unfinished rather than different.
    component BarItem: MenuItem {
        id: bi
        property string iconSrc: ""
        implicitHeight: 30; padding: 0
        contentItem: Item {
            IconImg {
                visible: bi.iconSrc !== ""
                src: bi.iconSrc; s: 14
                tint: !bi.enabled ? Theme.t4 : (bi.highlighted ? Theme.t1 : Theme.t3)
                x: 14
                anchors.verticalCenter: parent.verticalCenter
            }
            Text {
                anchors.fill: parent
                leftPadding: 38; rightPadding: 28
                text: bi.text
                color: !bi.enabled ? Theme.t4 : (bi.highlighted ? Theme.t1 : Theme.t2)
                font.pixelSize: 12
                font.family: Theme.fontSans
                verticalAlignment: Text.AlignVCenter
            }
        }
        background: Rectangle { color: bi.highlighted ? Theme.hover : "transparent"; radius: 5 }
    }
    component BarSep: MenuSeparator { contentItem: Rectangle { implicitHeight: 1; color: Theme.hairSoft } }
    component BarMenu: Menu {
        implicitWidth: 240
        // Breathing room so the first/last item don't get clipped by the
        // rounded (radius 8) background corners: the "cut tail" on the
        // last entry.
        topPadding: 6; bottomPadding: 6
        delegate: BarItem {}
        background: Rectangle { color: Theme.panel; border.color: Theme.hair; border.width: 1; radius: 8 }
    }

    BarMenu {
        title: (i18n.language, i18n.t("menu_file_title"))
        BarItem { iconSrc: "qrc:/icons/file.svg"; text: (i18n.language, i18n.t("menu_open_torrent")); onTriggered: openFileDialog.open() }
        BarItem { iconSrc: "qrc:/icons/magnet.svg"; text: (i18n.language, i18n.t("menu_add_magnet")); onTriggered: magnetDialog.open() }
        BarItem { iconSrc: "qrc:/icons/link.svg"; text: (i18n.language, i18n.t("menu_add_http")); onTriggered: host.promptHttpDownload() }
        BarItem { iconSrc: "qrc:/icons/plus.svg"; text: (i18n.language, i18n.t("menu_create_torrent")); onTriggered: createDialog.open() }
        BarItem { iconSrc: "qrc:/icons/search.svg"; text: (i18n.language, i18n.t("menu_inspect_torrent")); onTriggered: inspectFileDialog.open() }
        BarItem { iconSrc: "qrc:/icons/download.svg"; text: (i18n.language, i18n.t("menu_import_qbt")); onTriggered: importQbtDialog.open() }
        BarSep {}
        BarItem { iconSrc: "qrc:/icons/clock.svg"; text: (i18n.language, i18n.t("menu_recently_removed")); onTriggered: host.showWin(host.removedWinLoader) }
        BarSep {}
        BarItem { iconSrc: "qrc:/icons/close.svg"; text: (i18n.language, i18n.t("menu_quit")); onTriggered: Qt.quit() }
    }
    BarMenu {
        title: (i18n.language, i18n.t("menu_torrent_title"))
        BarItem { iconSrc: "qrc:/icons/pause.svg"; text: (i18n.language, i18n.t("menu_pause")); enabled: host.hasSel; onTriggered: session.pauseSelected() }
        BarItem { iconSrc: "qrc:/icons/play.svg"; text: (i18n.language, i18n.t("menu_resume")); enabled: host.hasSel; onTriggered: session.resumeSelected() }
        BarSep {}
        BarItem { iconSrc: "qrc:/icons/pause.svg"; text: (i18n.language, i18n.t("menu_pause_all")); onTriggered: if (typeof session !== "undefined") session.pauseAll() }
        BarItem { iconSrc: "qrc:/icons/play.svg"; text: (i18n.language, i18n.t("menu_resume_all")); onTriggered: if (typeof session !== "undefined") session.resumeAll() }
        BarSep {}
        BarItem { iconSrc: "qrc:/icons/trash.svg"; text: (i18n.language, i18n.t("menu_remove")); enabled: host.hasSel; onTriggered: removeDialog.open() }
    }
    BarMenu {
        title: (i18n.language, i18n.t("menu_settings_title"))
        BarItem { iconSrc: "qrc:/icons/settings.svg"; text: (i18n.language, i18n.t("menu_preferences")); onTriggered: host.currentPage = 3 }
        BarItem { iconSrc: "qrc:/icons/set-addons.svg"; text: (i18n.language, i18n.t("menu_addons")); onTriggered: addAddonDialog.open() }
        BarItem { iconSrc: "qrc:/icons/rss.svg"; text: (i18n.language, i18n.t("menu_rss")); onTriggered: host.showWin(host.rssWinLoader) }
        BarItem { iconSrc: "qrc:/icons/share.svg"; text: (i18n.language, i18n.t("menu_pair")); onTriggered: { pairingDialog.reload(); pairingDialog.open() } }
        BarSep {}
        BarItem { iconSrc: "qrc:/icons/search.svg"; text: (i18n.language, i18n.t("menu_search_torrents")); onTriggered: host.currentPage = 1 }
        BarSep {}
        BarItem { iconSrc: "qrc:/icons/gauge.svg"; text: (i18n.language, i18n.t("menu_statistics")); onTriggered: host.showWin(host.statsWinLoader) }
        BarItem { iconSrc: "qrc:/icons/bolt.svg"; text: (i18n.language, i18n.t("menu_speedtest")); onTriggered: Qt.openUrlExternally("https://fast.com") }
    }
    BarMenu {
        title: (i18n.language, i18n.t("menu_help_title"))
        BarItem { iconSrc: "qrc:/icons/sliders.svg"; text: (i18n.language, i18n.t("menu_setup_wizard")); onTriggered: { welcomeDialog.mode = "welcome"; welcomeDialog.open() } }
        BarItem { iconSrc: "qrc:/icons/discover.svg"; text: (i18n.language, i18n.t("menu_tour")); onTriggered: tourOverlay.start() }
        BarItem { iconSrc: "qrc:/icons/info.svg"; text: (i18n.language, i18n.t("menu_whatsnew")); onTriggered: { welcomeDialog.mode = "update"; welcomeDialog.open() } }
        BarItem { iconSrc: "qrc:/icons/file.svg"; text: (i18n.language, i18n.t("menu_release_notes")); onTriggered: releaseNotesDialog.open() }
        BarItem { iconSrc: "qrc:/icons/abc.svg"; text: (i18n.language, i18n.t("menu_shortcuts")); onTriggered: host.showWin(host.shortcutsWinLoader) }
        BarItem { iconSrc: "qrc:/icons/list.svg"; text: (i18n.language, i18n.t("menu_logs")); onTriggered: host.showWin(host.logWinLoader) }
        BarItem { iconSrc: "qrc:/icons/shield-check.svg"; text: (i18n.language, i18n.t("menu_diagnostics")); onTriggered: host.showWin(host.diagWinLoader) }
        BarItem {
            iconSrc: "qrc:/icons/refresh.svg"
            text: (i18n.language, i18n.t("menu_check_updates"))
            enabled: typeof updater !== "undefined" && updater !== null
            onTriggered: if (typeof updater !== "undefined" && updater) updater.check(false)
        }
        BarItem { iconSrc: "qrc:/icons/external.svg"; text: (i18n.language, i18n.t("menu_feedback")); onTriggered: Qt.openUrlExternally("https://docs.google.com/forms/d/e/1FAIpQLScdwLxWC-LB4wLuMI6_D3-QNPLNJPpzbob5LU0Y2yMnhaBFrg/viewform") }
        BarSep {}
        BarItem { iconSrc: "qrc:/icons/heart.svg"; text: (i18n.language, i18n.t("menu_donate")); onTriggered: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19") }
        BarItem { iconSrc: "qrc:/icons/info.svg"; text: (i18n.language, i18n.t("menu_about")); onTriggered: aboutDialog.open() }
    }
}
