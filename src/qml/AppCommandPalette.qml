// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

import QtQuick

Item {
    id: root
    required property var host
    required property var settingsPage
    required property var library
    required property var magnetDlg
    required property var openFileDlg
    required property var createDlg

    property alias cmdPalette: cmdPalette

    // Own schema instance so the palette can index individual options — it's
    // available before SettingsView is built, avoiding a binding-order race.
    SettingsSchema { id: paletteSchema }

    // Ctrl/⌘+K command palette — actions + torrent jump
    CommandPalette {
        id: cmdPalette
        actions: {
            var l = i18n.language   // re-evaluate labels on language switch
            var acts = [
                { label: i18n.t("magnet_title"), run: function() { magnetDlg.open() } },
                { label: i18n.t("menu_open_torrent"), run: function() { openFileDlg.open() } },
                { label: i18n.t("menu_create_torrent"), run: function() { createDlg.open() } },
                { label: i18n.t("menu_pause_all"), run: function() { if (typeof session !== "undefined") session.pauseAll() } },
                { label: i18n.t("menu_resume_all"), run: function() { if (typeof session !== "undefined") session.resumeAll() } },
                { label: i18n.t("tb_alt_speed"), hint: i18n.t("palette_hint_toggle"), run: function() { if (typeof session !== "undefined") session.setAltSpeedsActive(!session.altSpeedsActive) } },
                { label: i18n.t("nav_downloads"), hint: i18n.t("palette_hint_page"), run: function() { host.currentPage = 0 } },
                { label: i18n.t("nav_find"), hint: i18n.t("palette_hint_page"), run: function() { host.currentPage = 1 } },
                { label: i18n.t("nav_hub"), hint: i18n.t("palette_hint_page"), run: function() { host.currentPage = 2 } },
                { label: i18n.t("tb_settings"), run: function() { host.currentPage = 3 } },
                { label: i18n.t("menu_rss"), run: function() { host.showWin(host.rssWinLoader) } },
                { label: i18n.t("menu_statistics"), run: function() { host.showWin(host.statsWinLoader) } },
                { label: i18n.t("wrapped_title"), run: function() { host.showWrapped() } },
                { label: i18n.t("menu_logs"), run: function() { host.showWin(host.logWinLoader) } },
                { label: i18n.t("menu_diagnostics"), run: function() { host.showWin(host.diagWinLoader) } },
                { label: i18n.t("shortcuts_title2"), run: function() { host.showWin(host.shortcutsWinLoader) } }
            ]
            // settings sections + a few high-value deep links — so "torrent search",
            // "proxy", "network", etc. are reachable straight from the palette
            var setNav = ["detail_general", "detail_kv_speed", "settings_network", "set_nav_vpn",
                          "set_nav_proxy", "set_nav_webui", "set_nav_notif", "set_nav_addons", "settings_advanced"]
            for (var si = 0; si < setNav.length; ++si) {
                (function(idx) {
                    acts.push({ label: i18n.t("tb_settings") + " · " + i18n.t(setNav[idx]),
                                hint: i18n.t("palette_hint_page"),
                                run: function() { settingsPage.sec = idx; host.currentPage = 3 } })
                })(si)
            }
            acts.push({ label: i18n.t("set_grp_torrent_search"), hint: i18n.t("tb_settings"),
                        run: function() { settingsPage.sec = 7; host.currentPage = 3 } })
            // individual settings options — so Ctrl+K finds "Memory guard",
            // "Preallocate", etc., not just the section. Jumps to the option via
            // the Settings search box.
            var secs = paletteSchema.sections
            for (var ps = 0; ps < secs.length; ++ps) {
                for (var pf = 0; pf < secs[ps].length; ++pf) {
                    var fld = secs[ps][pf]
                    if (!fld.label || fld.type === "group" || fld.type === "warning") continue
                    (function(sectionIdx, field) {
                        acts.push({ label: i18n.t("tb_settings") + " · " + field.label,
                                    hint: i18n.t("palette_hint_page"),
                                    run: function() { host.currentPage = 3; settingsPage.sec = sectionIdx; settingsPage.searchFor(field.label) } })
                    })(ps, fld)
                }
            }
            return acts
        }
        onTorrentPicked: function(src) {
            host.currentPage = 0
            if (typeof torrentFilter === "undefined") return
            var p = torrentFilter.mapFromSource(src)
            if (p < 0) { library.setFilter("all"); p = torrentFilter.mapFromSource(src) }
            if (p >= 0) host.selectRow(p)
        }
    }
    Shortcut { sequence: "Ctrl+K"; onActivated: cmdPalette.toggle() }

}
