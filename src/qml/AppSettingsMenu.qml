// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import "theme"
import "widgets"

// Settings: preferences, add-ons, RSS, pairing and statistics. A submenu of
// the logo's menu, and its own menu in the bar that Alt shows.
DMenu {
    id: root
    required property var host
    required property var addAddonDialog
    required property var pairingDialog
    title: (i18n.language, i18n.t("menu_settings_title"))

    DMenuItem { iconSrc: "qrc:/icons/settings.svg"; text: (i18n.language, i18n.t("menu_preferences")); onTriggered: root.host.currentPage = 3 }
    DMenuItem { iconSrc: "qrc:/icons/set-addons.svg"; text: (i18n.language, i18n.t("menu_addons")); onTriggered: root.addAddonDialog.open() }
    DMenuItem { iconSrc: "qrc:/icons/rss.svg"; text: (i18n.language, i18n.t("menu_rss")); onTriggered: root.host.showWin(root.host.rssWinLoader) }
    DMenuItem { iconSrc: "qrc:/icons/share.svg"; text: (i18n.language, i18n.t("menu_pair")); onTriggered: { root.pairingDialog.reload(); root.pairingDialog.open() } }
    DMenuSep {}
    DMenuItem { iconSrc: "qrc:/icons/gauge.svg"; text: (i18n.language, i18n.t("menu_statistics")); onTriggered: root.host.showWin(root.host.statsWinLoader) }
    DMenuItem { iconSrc: "qrc:/icons/bolt.svg"; text: (i18n.language, i18n.t("menu_speedtest")); onTriggered: Qt.openUrlExternally("https://fast.com") }
}
