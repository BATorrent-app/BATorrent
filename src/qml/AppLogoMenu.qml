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

    AppFileMenu {
        sub: true
        host: root.host
        openFileDialog: root.openFileDialog
        addDialog: root.addDialog
        createDialog: root.createDialog
        inspectFileDialog: root.inspectFileDialog
        importQbtDialog: root.importQbtDialog
    }
    AppTorrentMenu {
        sub: true
        host: root.host
        removeDialog: root.removeDialog
    }
    AppSettingsMenu {
        sub: true
        host: root.host
        addAddonDialog: root.addAddonDialog
        pairingDialog: root.pairingDialog
    }
    AppHelpMenu {
        sub: true
        host: root.host
        tourOverlay: root.tourOverlay
        welcomeDialog: root.welcomeDialog
        releaseNotesDialog: root.releaseNotesDialog
    }
    DMenuSep {}
    DMenuItem { text: (i18n.language, i18n.t("menu_add_torrent_short")); shortcutText: "Ctrl+O"; onTriggered: root.openFileDialog.open() }
    DMenuItem { text: (i18n.language, i18n.t("menu_paste_magnet")); shortcutText: "Ctrl+V"; onTriggered: root.addDialog.open() }
    DMenuSep {}
    DMenuItem { text: (i18n.language, i18n.t("menu_donate")); tone: "accent"; onTriggered: Qt.openUrlExternally("https://github.com/sponsors/Mateuscruz19") }
    DMenuItem { text: (i18n.language, i18n.t("menu_about")); onTriggered: root.aboutDialog.open() }
    DMenuItem { text: (i18n.language, i18n.t("menu_quit")); shortcutText: "Ctrl+Q"; onTriggered: Qt.quit() }
}
