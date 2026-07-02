// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Settings schema: the nav metadata + the declarative field list per section.
// Pure data (only i18n) — separated from SettingsView's rendering. The
// (i18n.language, …) idiom keeps labels reactive to language changes.
import QtQuick

QtObject {
    // ---- nav metadata ----
    readonly property var navs: [
        { nav: (i18n.language, i18n.t("detail_general")),               icon: "qrc:/icons/set-general.svg" },
        { nav: (i18n.language, i18n.t("detail_kv_speed")),          icon: "qrc:/icons/set-speed.svg" },
        { nav: (i18n.language, i18n.t("settings_network")),                icon: "qrc:/icons/set-network.svg" },
        { nav: (i18n.language, i18n.t("set_nav_vpn")),   icon: "qrc:/icons/set-vpn.svg" },
        { nav: (i18n.language, i18n.t("set_nav_proxy")),icon: "qrc:/icons/set-proxy.svg" },
        { nav: (i18n.language, i18n.t("set_nav_webui")),  icon: "qrc:/icons/set-webui.svg" },
        { nav: (i18n.language, i18n.t("set_nav_notif")),        icon: "qrc:/icons/set-notif.svg" },
        { nav: (i18n.language, i18n.t("set_nav_addons")),      icon: "qrc:/icons/set-addons.svg" },
        { nav: (i18n.language, i18n.t("settings_advanced")),            icon: "qrc:/icons/set-advanced.svg" }
    ]

    readonly property var heads: [
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("detail_general")), sub: (i18n.language, i18n.t("set_sub_general")) },
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("settings_speed")), sub: (i18n.language, i18n.t("set_sub_speed")) },
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("settings_network")), sub: (i18n.language, i18n.t("set_sub_network")) },
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("set_nav_vpn")), sub: (i18n.language, i18n.t("set_sub_vpn")) },
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("set_nav_proxy")), sub: (i18n.language, i18n.t("set_sub_proxy")) },
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("set_nav_webui")), sub: (i18n.language, i18n.t("set_sub_webui")) },
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("set_nav_notif")), sub: (i18n.language, i18n.t("set_sub_notif")) },
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("set_nav_addons")), sub: (i18n.language, i18n.t("set_sub_addons")) },
        { eyebrow: (i18n.language, i18n.t("set_eyebrow")), h: (i18n.language, i18n.t("settings_advanced")), sub: (i18n.language, i18n.t("set_sub_advanced")) }
    ]

    // ---- fields per section (settings-data.js, verbatim) ----
    readonly property var sections: [
        // 0 Geral
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_appearance")) },
            { type: "select", isLang: true, label: (i18n.language, i18n.t("set_language2")), options: ["English", "Português", "中文", "日本語", "Русский", "Español", "Deutsch", "Українська", "Türkçe"], icons: ["qrc:/icons/flags/en.svg", "qrc:/icons/flags/pt.svg", "qrc:/icons/flags/zh.svg", "qrc:/icons/flags/ja.svg", "qrc:/icons/flags/ru.svg", "qrc:/icons/flags/es.svg", "qrc:/icons/flags/de.svg", "qrc:/icons/flags/uk.svg", "qrc:/icons/flags/tr.svg"], value: 0 },
            { type: "theme", label: (i18n.language, i18n.t("set_theme2")), options: [(i18n.language, i18n.t("set_theme_dark")), (i18n.language, i18n.t("set_theme_light")), "Midnight", "Sakura", "Dark Star", "Matrix", (i18n.language, i18n.t("set_theme_custom"))], value: 0 },
            { type: "toggle", key: "followSystem", label: (i18n.language, i18n.t("set_follow_system")), note: (i18n.language, i18n.t("set_follow_system_note")) },
            { type: "appicon", label: (i18n.language, i18n.t("set_app_icon")), note: (i18n.language, i18n.t("set_app_icon_note")) },
            { type: "anime", label: (i18n.language, i18n.t("set_anime2")) },
            { type: "profiles", label: (i18n.language, i18n.t("set_custom_profile")),   customOnly: true },
            { type: "color",  role: "bg",        label: (i18n.language, i18n.t("set_custom_bg")),        customOnly: true },
            { type: "color",  role: "panel",     label: (i18n.language, i18n.t("set_custom_panel")),     customOnly: true },
            { type: "color",  role: "text",      label: (i18n.language, i18n.t("set_custom_text")),      customOnly: true },
            { type: "color",  role: "primary",   label: (i18n.language, i18n.t("set_custom_primary")),   customOnly: true },
            { type: "color",  role: "secondary", label: (i18n.language, i18n.t("set_custom_secondary")), customOnly: true },
            { type: "color",  role: "tertiary",  label: (i18n.language, i18n.t("set_custom_tertiary")),  customOnly: true },
            { type: "bgimage", label: (i18n.language, i18n.t("set_custom_bgimage")),  customOnly: true },
            { type: "slider",  label: (i18n.language, i18n.t("set_custom_opacity")),  customOnly: true },
            { type: "group", label: (i18n.language, i18n.t("set_grp_downloads")) },
            { type: "path", key: "lastSavePath", label: (i18n.language, i18n.t("set_default_save2")) },
            { type: "toggle", key: "useDefaultPath", label: (i18n.language, i18n.t("settings_use_default_path")), on: true },
            { type: "path", key: "tempPath", label: (i18n.language, i18n.t("set_temp_path2")), placeholder: (i18n.language, i18n.t("set_temp_path_ph")), note: (i18n.language, i18n.t("set_temp_path_note")) },
            { type: "toggle", key: "autoMoveEnabled", label: (i18n.language, i18n.t("settings_automove")) },
            { type: "path", key: "autoMovePath", label: (i18n.language, i18n.t("set_move_to2")), placeholder: (i18n.language, i18n.t("set_move_to_ph")) },
            { type: "toggle", key: "preallocate", label: (i18n.language, i18n.t("set_preallocate")), note: (i18n.language, i18n.t("set_preallocate_note")) },
            { type: "toggle", key: "autoRecheck", label: (i18n.language, i18n.t("set_auto_recheck")), note: (i18n.language, i18n.t("set_auto_recheck_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_extraction")) },
            { type: "toggle", key: "autoExtract", label: (i18n.language, i18n.t("set_auto_extract2")), note: (i18n.language, i18n.t("set_auto_extract_note")) },
            { type: "toggle", key: "autoExtractDelete", label: (i18n.language, i18n.t("settings_auto_extract_delete")) },
            { type: "text", key: "extractPasswords", label: (i18n.language, i18n.t("set_extract_passwords2")), placeholder: "senha1; senha2; online-fix.me", note: (i18n.language, i18n.t("set_extract_passwords_note")), w: "grow" },
            { type: "toggle", key: "gameAutoInstall", label: (i18n.language, i18n.t("set_game_autoinstall")), note: (i18n.language, i18n.t("set_game_autoinstall_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_playback")) },
            { type: "select", key: "preferredQuality", label: (i18n.language, i18n.t("set_preferred_quality")), options: ["Auto", "1080p", "720p", "2160p"], value: 1, note: (i18n.language, i18n.t("set_pref_quality_note")) },
            { type: "number", key: "preferMaxSize", label: (i18n.language, i18n.t("set_max_size")), value: "0", suffix: "MB", note: (i18n.language, i18n.t("note_unlimited")) },
            { type: "toggle", key: "autoplayNext", on: true, label: (i18n.language, i18n.t("set_autoplay_next")), note: (i18n.language, i18n.t("set_autoplay_next_note")) },
            { type: "toggle", key: "preferNativeLang", on: true, label: (i18n.language, i18n.t("set_prefer_native")), note: (i18n.language, i18n.t("set_prefer_native_note")) },
            { type: "number", key: "subFontScale", label: (i18n.language, i18n.t("set_sub_size")), value: "100", suffix: "%" },
            { type: "select", key: "subColor", label: (i18n.language, i18n.t("set_sub_color")), options: [(i18n.language, i18n.t("color_white")), (i18n.language, i18n.t("color_yellow")), (i18n.language, i18n.t("color_cyan")), (i18n.language, i18n.t("color_green"))], value: 0 },
            { type: "number", key: "subBgOpacity", label: (i18n.language, i18n.t("set_sub_bg")), value: "0", suffix: "%", note: (i18n.language, i18n.t("set_sub_bg_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_system")) },
            { type: "toggle", key: "startTray", label: (i18n.language, i18n.t("settings_start_tray")) },
            { type: "toggle", key: "closeToTray", label: (i18n.language, i18n.t("settings_close_to_tray")), on: true },
            { type: "toggle", key: "notifSound", label: (i18n.language, i18n.t("settings_notif_sound")), on: true },
            { type: "toggle", key: "showSplash", label: (i18n.language, i18n.t("settings_show_splash")), on: true },
            { type: "number", key: "memGuardMB", label: (i18n.language, i18n.t("set_mem_guard")), value: "8192", suffix: "MB", note: (i18n.language, i18n.t("set_mem_guard_hint")) },
            { type: "button", action: "default", label: (i18n.language, i18n.t("set_default_app")), btn: (i18n.language, i18n.t("settings_set_default")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_security")) },
            { type: "toggle", key: "warnSuspiciousFiles", on: true, label: (i18n.language, i18n.t("set_warn_suspicious")), note: (i18n.language, i18n.t("set_warn_suspicious_note")) },
            { type: "toggle", key: "autoDefenderExclude", winOnly: true, label: (i18n.language, i18n.t("set_auto_defender")), note: (i18n.language, i18n.t("set_auto_defender_note")) }
        ],
        // 1 Velocidade
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_global_limits")) },
            { type: "number", key: "downloadLimit", label: (i18n.language, i18n.t("set_max_down2")), value: "0", suffix: "KB/s", note: (i18n.language, i18n.t("note_unlimited")) },
            { type: "number", key: "uploadLimit", label: (i18n.language, i18n.t("set_max_up2")), value: "200", suffix: "KB/s", note: (i18n.language, i18n.t("note_unlimited")) },
            { type: "number", key: "maxActiveDownloads", label: (i18n.language, i18n.t("set_max_active2")), value: "5", note: (i18n.language, i18n.t("note_unlimited")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_seeding")) },
            { type: "number", key: "seedRatioLimit", label: (i18n.language, i18n.t("set_seed_ratio2")), value: "2.0", note: (i18n.language, i18n.t("set_seed_ratio_note")) },
            { type: "number", key: "maxSeedDays", label: (i18n.language, i18n.t("set_max_seed_days2")), value: "0", suffix: (i18n.language, i18n.t("settings_days")) },
            { type: "toggle", key: "stopAfterDownload", label: (i18n.language, i18n.t("settings_stop_after_download")) },
            { type: "select", key: "autoComplete", label: (i18n.language, i18n.t("settings_auto_complete")), options: [(i18n.language, i18n.t("auto_complete_never")), (i18n.language, i18n.t("auto_complete_1d")), (i18n.language, i18n.t("auto_complete_3d")), (i18n.language, i18n.t("auto_complete_7d")), (i18n.language, i18n.t("auto_complete_14d")), (i18n.language, i18n.t("auto_complete_30d"))], value: 0, note: (i18n.language, i18n.t("set_auto_complete_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_scheduler")) },
            { type: "toggle", key: "schedulerEnabled", label: (i18n.language, i18n.t("settings_scheduler_enable")), note: (i18n.language, i18n.t("set_scheduler_note")) },
            { type: "number", key: "altDownloadLimit", label: (i18n.language, i18n.t("set_alt_down2")), value: "50", suffix: "KB/s" },
            { type: "number", key: "altUploadLimit", label: (i18n.language, i18n.t("set_alt_up2")), value: "20", suffix: "KB/s" },
            { type: "timerange", label: (i18n.language, i18n.t("set_sched_from2")), from: "08:00", to: "18:00" },
            { type: "days", label: (i18n.language, i18n.t("set_sched_days2")), value: [1,2,3,4,5] }
        ],
        // 2 Rede
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_connection")) },
            { type: "number", key: "listenPort", label: (i18n.language, i18n.t("set_listen_port2")), value: "6881", note: (i18n.language, i18n.t("set_listen_port_note")) },
            { type: "toggle", key: "randomPort", label: (i18n.language, i18n.t("settings_random_port")) },
            { type: "number", key: "maxConnections", label: (i18n.language, i18n.t("set_max_conn2")), value: "200" },
            { type: "group", label: (i18n.language, i18n.t("set_grp_protocol")) },
            { type: "toggle", key: "dhtEnabled", label: (i18n.language, i18n.t("settings_enable_dht")), on: true, note: (i18n.language, i18n.t("set_dht_note")) },
            { type: "toggle", key: "utpEnabled", label: (i18n.language, i18n.t("settings_enable_utp")), on: true },
            { type: "segmented", key: "encryptionMode", label: (i18n.language, i18n.t("set_encryption2")), options: [(i18n.language, i18n.t("set_enc_enabled_opt")), (i18n.language, i18n.t("set_enc_forced_opt")), (i18n.language, i18n.t("settings_enc_disabled"))], value: 0, note: (i18n.language, i18n.t("set_encryption_note")) },
            { type: "segmented", key: "speedUnit", label: (i18n.language, i18n.t("settings_speed_unit")), options: [(i18n.language, i18n.t("set_speed_bytes_opt")), (i18n.language, i18n.t("set_speed_bits_opt"))], value: 0 },
            { type: "group", label: (i18n.language, i18n.t("set_grp_privacy")) },
            { type: "toggle", key: "anonymousMode", label: (i18n.language, i18n.t("settings_anonymous_mode")), note: (i18n.language, i18n.t("set_anon_note")) },
            { type: "toggle", key: "forceIpv4", label: (i18n.language, i18n.t("settings_force_ipv4")) },
            { type: "toggle", key: "ptMode", label: (i18n.language, i18n.t("settings_pt_mode")), note: (i18n.language, i18n.t("set_pt_note")) },
            { type: "toggle", key: "blockLeechers", label: (i18n.language, i18n.t("settings_block_leechers")), on: true }
        ],
        // 3 VPN
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_iface_bind")) },
            { type: "iface", label: (i18n.language, i18n.t("set_iface2")) },
            { type: "toggle", key: "killSwitchEnabled", label: (i18n.language, i18n.t("settings_kill_switch")), on: true, note: (i18n.language, i18n.t("set_killswitch_note")) },
            { type: "toggle", key: "autoResumeOnReconnect", label: (i18n.language, i18n.t("settings_auto_resume")), on: true, note: (i18n.language, i18n.t("set_autoresume_note")) },
            { type: "toggle", key: "useTor", label: (i18n.language, i18n.t("settings_use_tor")), note: (i18n.language, i18n.t("set_use_tor_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_power")) },
            { type: "toggle", key: "autoShutdown", label: (i18n.language, i18n.t("settings_auto_shutdown")), note: (i18n.language, i18n.t("set_auto_shutdown_note")) }
        ],
        // 4 Proxy
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_proxy")) },
            { type: "select", key: "proxyType", label: (i18n.language, i18n.t("set_proxy_type2")), options: [(i18n.language, i18n.t("settings_proxy_none")), "SOCKS5", "HTTP"], value: 0, note: (i18n.language, i18n.t("set_proxy_type_note")) },
            { type: "text", key: "proxyHost", label: (i18n.language, i18n.t("set_proxy_host2")), mono: true, placeholder: "127.0.0.1", w: "w-md" },
            { type: "number", key: "proxyPort", label: (i18n.language, i18n.t("set_port2")), value: "1080" },
            { type: "text", key: "proxyUser", label: (i18n.language, i18n.t("set_user2")), placeholder: (i18n.language, i18n.t("settings_proxy_user_hint")), w: "w-md" },
            { type: "password", key: "proxyPass", label: (i18n.language, i18n.t("set_pass2")), w: "w-md" },
            { type: "toggle", key: "proxyLeakProof", on: true, label: (i18n.language, i18n.t("set_proxy_leakproof")), note: (i18n.language, i18n.t("set_proxy_leakproof_note")) },
            { type: "button", action: "proxyMullvad", label: (i18n.language, i18n.t("set_proxy_presets")), btn: "Mullvad SOCKS5", note: (i18n.language, i18n.t("set_proxy_presets_note")) },
            { type: "button", action: "proxyLeakTest", label: (i18n.language, i18n.t("set_proxy_leaktest")), btn: (i18n.language, i18n.t("set_proxy_leaktest_btn")), note: (i18n.language, i18n.t("set_proxy_leaktest_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_ip_filter")) },
            { type: "path", key: "ipFilterPath", file: true, label: (i18n.language, i18n.t("set_blocklist_file")), placeholder: (i18n.language, i18n.t("settings_ip_filter_hint")), note: (i18n.language, i18n.t("set_blocklist_note")) }
        ],
        // 5 WebUI
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_webserver")) },
            { type: "toggle", key: "webUiEnabled", label: (i18n.language, i18n.t("settings_webui_enable")) },
            { type: "number", key: "webUiPort", label: (i18n.language, i18n.t("set_port2")), value: "8080" },
            { type: "text", key: "webUiUser", label: (i18n.language, i18n.t("set_user2")), value: "admin", w: "w-md" },
            { type: "password", key: "webUiPassword", label: (i18n.language, i18n.t("set_pass2")), placeholder: (i18n.language, i18n.t("settings_webui_pass_hint")), w: "w-md" },
            { type: "toggle", key: "webUiRemoteAccess", label: (i18n.language, i18n.t("settings_webui_remote")) },
            { type: "warning", text: (i18n.language, i18n.t("settings_webui_warning_msg")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_mobile")) },
            { type: "button", action: "pair", label: (i18n.language, i18n.t("pairing_title")), btn: (i18n.language, i18n.t("set_pair_phone")), note: (i18n.language, i18n.t("set_pair_note")) }
        ],
        // 6 Notificações
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_telegram")) },
            { type: "text", key: "telegramToken", label: (i18n.language, i18n.t("settings_telegram_token")), mono: true, placeholder: "123456:ABC-DEF…", w: "grow", note: (i18n.language, i18n.t("set_telegram_token_note")) },
            { type: "text", key: "telegramChatId", label: (i18n.language, i18n.t("settings_telegram_chat")), mono: true, placeholder: "@seucanal ou ID numérico", w: "w-md" },
            { type: "toggle", key: "telegramEvtFinished", label: (i18n.language, i18n.t("settings_telegram_finished")), on: true },
            { type: "toggle", key: "telegramEvtKill", label: (i18n.language, i18n.t("settings_telegram_killswitch")) },
            { type: "toggle", key: "telegramEvtRss", label: (i18n.language, i18n.t("settings_telegram_rss")) },
            { type: "toggle", key: "telegramEvtError", label: (i18n.language, i18n.t("settings_telegram_error")) },
            { type: "button", action: "telegram", label: (i18n.language, i18n.t("set_test_btn")), btn: (i18n.language, i18n.t("settings_telegram_test")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_discord")) },
            { type: "toggle", key: "discordEnabled", label: (i18n.language, i18n.t("set_discord_show")), on: true, note: (i18n.language, i18n.t("set_discord_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_system")) },
            { type: "toggle", key: "notifSound", label: (i18n.language, i18n.t("settings_notif_sound")), on: true }
        ],
        // 7 Addons
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_trackers")) },
            { type: "toggle", key: "autoTrackers", label: (i18n.language, i18n.t("set_auto_trackers2")), on: true, note: (i18n.language, i18n.t("set_auto_trackers_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_torrent_search")) },
            { type: "toggle", key: "torrentSearchEnabled", label: (i18n.language, i18n.t("set_torrent_search_enable")) },
            { type: "text", key: "torrentSearchUrl", label: (i18n.language, i18n.t("set_api_url2")), mono: true, placeholder: (i18n.language, i18n.t("set_api_url_ph")), w: "grow" },
            { type: "group", label: (i18n.language, i18n.t("set_grp_media_server")) },
            { type: "toggle", key: "plexEnabled", label: (i18n.language, i18n.t("set_media_plex")) },
            { type: "text", key: "plexUrl", label: (i18n.language, i18n.t("set_plex_url")), mono: true, placeholder: "http://127.0.0.1:32400", w: "grow" },
            { type: "password", key: "plexToken", label: (i18n.language, i18n.t("set_plex_token")), mono: true, w: "w-md" },
            { type: "toggle", key: "jellyfinEnabled", label: (i18n.language, i18n.t("set_media_jellyfin")) },
            { type: "text", key: "jellyfinUrl", label: (i18n.language, i18n.t("set_jellyfin_url")), mono: true, placeholder: "http://127.0.0.1:8096", w: "grow" },
            { type: "password", key: "jellyfinApiKey", label: (i18n.language, i18n.t("set_media_apikey")), mono: true, w: "w-md" }
        ],
        // 8 Avançado
        [
            { type: "group", label: (i18n.language, i18n.t("set_grp_engine")) },
            { type: "toggle", key: "engineSplit", label: (i18n.language, i18n.t("set_engine_split")), note: (i18n.language, i18n.t("set_engine_split_note")) },
            { type: "group", label: (i18n.language, i18n.t("adv_disk_io")) },
            { type: "number", key: "advAioThreads", label: (i18n.language, i18n.t("adv_aio_threads")), value: "4" },
            { type: "number", key: "advHashingThreads", label: (i18n.language, i18n.t("adv_hashing_threads")), value: "2" },
            { type: "number", key: "advFilePool", label: (i18n.language, i18n.t("adv_file_pool")), value: "40" },
            { type: "number", key: "advCheckingMem", label: (i18n.language, i18n.t("adv_checking_mem")), value: "512", suffix: "×16 KiB" },
            { type: "number", key: "advSendBuffer", label: (i18n.language, i18n.t("adv_send_buffer")), value: "500", suffix: "KiB" },
            { type: "group", label: (i18n.language, i18n.t("adv_connections")) },
            { type: "number", key: "advConnLimit", label: (i18n.language, i18n.t("adv_conn_limit")), value: "200" },
            { type: "number", key: "advConnSpeed", label: (i18n.language, i18n.t("adv_conn_speed")), value: "30", suffix: "/s" },
            { type: "number", key: "advUnchokeSlots", label: (i18n.language, i18n.t("adv_unchoke_slots")), value: "8" },
            { type: "number", key: "advMaxUploadsTor", label: (i18n.language, i18n.t("set_max_uploads_tor")), value: "4" },
            { type: "number", key: "advMaxConnsTor", label: (i18n.language, i18n.t("set_max_conns_tor")), value: "60" },
            { type: "group", label: (i18n.language, i18n.t("adv_algorithms")) },
            { type: "segmented", key: "advChokingAlgo", label: (i18n.language, i18n.t("adv_choking_algo")), options: [(i18n.language, i18n.t("adv_choke_fixed")), (i18n.language, i18n.t("adv_choke_rate"))], value: 1 },
            { type: "select", key: "advSeedChoking", label: (i18n.language, i18n.t("adv_seed_choking")), options: [(i18n.language, i18n.t("adv_seedchoke_robin")), (i18n.language, i18n.t("adv_seedchoke_fastest")), (i18n.language, i18n.t("adv_seedchoke_antileech"))], value: 2 },
            { type: "toggle", key: "advRateOverhead", label: (i18n.language, i18n.t("adv_rate_overhead")) },
            { type: "toggle", key: "advIgnoreLan", label: (i18n.language, i18n.t("adv_ignore_lan")), on: true },
            { type: "group", label: (i18n.language, i18n.t("set_grp_backup")) },
            { type: "button", action: "exportSettings", label: (i18n.language, i18n.t("action_export_settings")), btn: (i18n.language, i18n.t("btn_export")) },
            { type: "button", action: "importSettings", label: (i18n.language, i18n.t("action_import_settings")), btn: (i18n.language, i18n.t("btn_import")) },
            { type: "button", action: "fullBackup", label: (i18n.language, i18n.t("action_full_backup")), btn: (i18n.language, i18n.t("btn_export")) },
            { type: "button", action: "fullRestore", label: (i18n.language, i18n.t("action_full_restore")), btn: (i18n.language, i18n.t("btn_import")) },
            { type: "group", label: (i18n.language, i18n.t("adv_metadata_api")) },
            { type: "text", key: "tmdbApiKey", label: (i18n.language, i18n.t("set_tmdb2")), mono: true, w: "grow", note: (i18n.language, i18n.t("set_tmdb_note")) },
            { type: "text", key: "igdbClientId", label: (i18n.language, i18n.t("set_igdb_id2")), mono: true, w: "w-md" },
            { type: "text", key: "igdbClientSecret", label: (i18n.language, i18n.t("set_igdb_secret2")), mono: true, w: "w-md" },
            { type: "text", key: "subdlApiKey", label: (i18n.language, i18n.t("set_subdl_key")), mono: true, w: "grow", note: (i18n.language, i18n.t("set_subdl_key_note")) },
            { type: "text", key: "osApiKey", label: (i18n.language, i18n.t("set_os_key")), mono: true, w: "grow", note: (i18n.language, i18n.t("set_os_key_note")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_debrid")) },
            { type: "debrid", label: (i18n.language, i18n.t("set_debrid_provider")) },
            { type: "debridtoken", label: (i18n.language, i18n.t("set_rd_account")), note: (i18n.language, i18n.t("set_debrid_note")) },
            { type: "group", label: (i18n.language, i18n.t("diag_title")) },
            { type: "button", action: "defender", winOnly: true, label: (i18n.language, i18n.t("settings_defender_exclude")), btn: (i18n.language, i18n.t("settings_defender_exclude")), note: (i18n.language, i18n.t("tip_defender_exclude")) },
            { type: "toggle", key: "verboseLogging", label: (i18n.language, i18n.t("settings_verbose_log")), note: (i18n.language, i18n.t("set_verbose_note")) },
            { type: "text", key: "runOnComplete", label: (i18n.language, i18n.t("set_run_on_complete2")), mono: true, placeholder: "notify-send \"%N done\"", w: "grow" },
            { type: "path", key: "watchedFolder", label: (i18n.language, i18n.t("set_watched_folder2")), placeholder: (i18n.language, i18n.t("settings_watched_hint")) }
        ]
    ]
}
