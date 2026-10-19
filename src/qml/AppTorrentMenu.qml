// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import "theme"
import "widgets"

// Torrent: pausing, resuming, refreshing and removing. A submenu of the logo's
// menu, and its own menu in the bar that Alt shows.
DMenu {
    id: root
    required property var host
    required property var removeDialog
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
