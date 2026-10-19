// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import "theme"
import "widgets"

// File: adding, making and importing torrents. A submenu of the logo's menu,
// and its own menu in the bar that Alt shows.
DMenu {
    id: root
    required property var host
    required property var openFileDialog
    required property var addDialog
    required property var createDialog
    required property var inspectFileDialog
    required property var importQbtDialog
    title: (i18n.language, i18n.t("menu_file_title"))

    DMenuItem { iconSrc: "qrc:/icons/file.svg"; text: (i18n.language, i18n.t("menu_open_torrent")); shortcutText: "Ctrl+O"; onTriggered: root.openFileDialog.open() }
    DMenuItem { iconSrc: "qrc:/icons/magnet.svg"; text: (i18n.language, i18n.t("menu_add_magnet")); onTriggered: root.addDialog.open() }
    DMenuItem { iconSrc: "qrc:/icons/link.svg"; text: (i18n.language, i18n.t("menu_add_http")); onTriggered: root.host.promptHttpDownload() }
    DMenuItem { iconSrc: "qrc:/icons/plus.svg"; text: (i18n.language, i18n.t("menu_create_torrent")); onTriggered: root.createDialog.open() }
    DMenuItem { iconSrc: "qrc:/icons/search.svg"; text: (i18n.language, i18n.t("menu_inspect_torrent")); onTriggered: root.inspectFileDialog.open() }
    DMenuItem { iconSrc: "qrc:/icons/download.svg"; text: (i18n.language, i18n.t("menu_import_qbt")); onTriggered: root.importQbtDialog.open() }
    DMenuSep {}
    DMenuItem { iconSrc: "qrc:/icons/clock.svg"; text: (i18n.language, i18n.t("menu_recently_removed")); onTriggered: root.host.showWin(root.host.removedWinLoader) }
}
