// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick
import QtQuick.Controls.Basic
import "theme"
import "widgets"

// Help: the setup wizard, the tour, what's new, shortcuts, logs, updates and
// feedback. A submenu of the logo's menu, and its own menu behind the ? in
// the title strip.
DMenu {
    id: root
    required property var host
    required property var tourOverlay
    required property var welcomeDialog
    required property var releaseNotesDialog
    iconSrc: "qrc:/icons/help.svg"
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
