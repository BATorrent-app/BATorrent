// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

// Source: BATorrent Settings.html + batorrent-settings.css (+ settings-data.js)
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import "theme"
import "widgets"

Window {
    id: win
    width: 884
    height: 624
    minimumWidth: 740
    minimumHeight: 480
    color: Theme.bg
    flags: Theme.unifiedChrome ? (Qt.Window | Qt.ExpandedClientAreaHint | Qt.NoTitleBarBackgroundHint) : Qt.Window
    title: Theme.unifiedChrome ? "" : (i18n.language, i18n.t("settings_heading"))

    property int sec: 0

    // a stored bool pref, falling back to the field's `on` default when unset —
    // so a default-on toggle reads ON on a fresh profile (matches runtime behavior).
    function boolPref(field) {
        if (typeof settings === "undefined" || field.key === undefined) return field.on === true
        var v = settings.get(field.key)
        return (v === undefined || v === null || v === "") ? (field.on === true) : (v === true)
    }

    // active custom-theme profile map; re-read on any profile data change
    readonly property var ap: (typeof themeBridge !== "undefined" && themeBridge.customProfiles.length > 0)
                              ? themeBridge.customProfiles[themeBridge.activeProfile] : ({})

    // Instant-apply settings: Esc / ⌘W just close (nothing to confirm).
    Shortcut { sequences: [StandardKey.Cancel]; onActivated: win.close() }
    Shortcut { sequences: [StandardKey.Close];  onActivated: win.close() }

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
            { type: "group", label: (i18n.language, i18n.t("set_grp_playback")) },
            { type: "select", key: "preferredQuality", label: (i18n.language, i18n.t("set_preferred_quality")), options: ["Auto", "1080p", "720p", "2160p"], value: 1, note: (i18n.language, i18n.t("set_pref_quality_note")) },
            { type: "number", key: "preferMaxSize", label: (i18n.language, i18n.t("set_max_size")), value: "0", suffix: "MB", note: (i18n.language, i18n.t("note_unlimited")) },
            { type: "group", label: (i18n.language, i18n.t("set_grp_appearance")) },
            { type: "select", isLang: true, label: (i18n.language, i18n.t("set_language2")), options: ["English", "Português", "中文", "日本語", "Русский", "Español", "Deutsch", "Українська"], icons: ["qrc:/icons/flags/en.svg", "qrc:/icons/flags/pt.svg", "qrc:/icons/flags/zh.svg", "qrc:/icons/flags/ja.svg", "qrc:/icons/flags/ru.svg", "qrc:/icons/flags/es.svg", "qrc:/icons/flags/de.svg", "qrc:/icons/flags/uk.svg"], value: 0 },
            { type: "theme", label: (i18n.language, i18n.t("set_theme2")), options: [(i18n.language, i18n.t("set_theme_dark")), (i18n.language, i18n.t("set_theme_light")), "Midnight", "Sakura", "Dark Star", (i18n.language, i18n.t("set_theme_custom"))], value: 0 },
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
            { type: "group", label: (i18n.language, i18n.t("set_grp_system")) },
            { type: "toggle", key: "startTray", label: (i18n.language, i18n.t("settings_start_tray")) },
            { type: "toggle", key: "closeToTray", label: (i18n.language, i18n.t("settings_close_to_tray")), on: true },
            { type: "toggle", key: "notifSound", label: (i18n.language, i18n.t("settings_notif_sound")), on: true },
            { type: "toggle", key: "showSplash", label: (i18n.language, i18n.t("settings_show_splash")), on: true },
            { type: "number", key: "memGuardMB", label: (i18n.language, i18n.t("set_mem_guard")), value: "4096", suffix: "MB", note: (i18n.language, i18n.t("set_mem_guard_hint")) },
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
            { type: "text", key: "osApiKey", label: (i18n.language, i18n.t("set_os_key")), mono: true, w: "grow", note: (i18n.language, i18n.t("set_os_key_note")) },
            { type: "group", label: (i18n.language, i18n.t("diag_title")) },
            { type: "button", action: "defender", winOnly: true, label: (i18n.language, i18n.t("settings_defender_exclude")), btn: (i18n.language, i18n.t("settings_defender_exclude")), note: (i18n.language, i18n.t("tip_defender_exclude")) },
            { type: "toggle", key: "verboseLogging", label: (i18n.language, i18n.t("settings_verbose_log")), note: (i18n.language, i18n.t("set_verbose_note")) },
            { type: "text", key: "runOnComplete", label: (i18n.language, i18n.t("set_run_on_complete2")), mono: true, placeholder: "notify-send \"%N done\"", w: "grow" },
            { type: "path", key: "watchedFolder", label: (i18n.language, i18n.t("set_watched_folder2")), placeholder: (i18n.language, i18n.t("settings_watched_hint")) }
        ]
    ]

    // group consecutive fields into { label, rows[] } blocks (a "group" field starts a new card).
    // Hidden rows are dropped, and a group with no remaining rows is omitted entirely
    // (so we never render an orphan header over an empty card).
    function buildBlocks(fields) {
        var blocks = []
        var cur = null
        for (var i = 0; i < fields.length; i++) {
            var f = fields[i]
            if (f.type === "group") {
                cur = { label: f.label, rows: [] }
            } else {
                if (f.hidden === true) continue
                if (!cur) { cur = { label: "", rows: [] } }
                if (cur.rows.length === 0 && blocks.indexOf(cur) < 0) blocks.push(cur)
                cur.rows.push(f)
            }
        }
        return blocks
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ===== titlebar =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            color: Theme.elev
            Text { anchors.centerIn: parent; text: (i18n.language, i18n.t("settings_heading")); color: Theme.t2; font.pixelSize: 13; font.weight: Font.DemiBold; font.family: Theme.fontSans }
            Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: 1; color: Theme.hairSoft }
        }

        // ===== sbody =====
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ----- sidebar (.sside 226) -----
            Rectangle {
                Layout.preferredWidth: 226
                Layout.fillHeight: true
                color: Theme.panel
                Rectangle { anchors.right: parent.right; width: 1; height: parent.height; color: Theme.hair }

                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: Theme.sp3
                    anchors.bottomMargin: Theme.sp3
                    anchors.leftMargin: Theme.sp2
                    anchors.rightMargin: Theme.sp2
                    spacing: 0

                    // .ssearch
                    TFld {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 32
                        Layout.bottomMargin: Theme.sp3
                        icon: "qrc:/icons/search.svg"
                        placeholder: (i18n.language, i18n.t("set_search_config_ph"))
                    }

                    // .snav
                    ListView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        spacing: 1
                        model: win.navs
                        boundsBehavior: Flickable.StopAtBounds

                        delegate: Rectangle {
                            width: ListView.view.width
                            height: 34
                            radius: 7
                            color: win.sec === index ? Theme.sel : (nrMa.containsMouse ? Theme.hover : "transparent")

                            // .nrow.on::before left bar
                            Rectangle {
                                visible: win.sec === index
                                anchors.left: parent.left
                                anchors.leftMargin: -2
                                anchors.verticalCenter: parent.verticalCenter
                                width: 2
                                height: 20
                                radius: 1
                                color: Theme.accent
                            }

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 10
                                anchors.rightMargin: 10
                                spacing: Theme.sp3
                                IconImg {
                                    Layout.alignment: Qt.AlignVCenter
                                    src: modelData.icon
                                    tint: win.sec === index ? Theme.accent : Theme.t3
                                    s: 16
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData.nav
                                    color: win.sec === index ? Theme.t1 : (nrMa.containsMouse ? Theme.t1 : Theme.t2)
                                    font.pixelSize: 13
                                    font.family: Theme.fontSans
                                    elide: Text.ElideRight
                                }
                            }
                            MouseArea { id: nrMa; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onClicked: win.sec = index }
                        }
                    }
                }
            }

            // ----- content (.scontent) -----
            Flickable {
                id: contentScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: width
                contentHeight: contentCol.implicitHeight + 2 * Theme.sp5
                boundsBehavior: Flickable.StopAtBounds

                ColumnLayout {
                    id: contentCol
                    x: Theme.sp5
                    y: Theme.sp5
                    width: contentScroll.width - 2 * Theme.sp5
                    spacing: 0

                    // .shead
                    Eyebrow { text: win.heads[win.sec].eyebrow }
                    Text {
                        Layout.topMargin: 6
                        text: win.heads[win.sec].h
                        color: Theme.t1
                        font.pixelSize: 19
                        font.weight: Font.DemiBold
                        font.letterSpacing: -0.3
                        font.family: Theme.fontSans
                    }
                    Text {
                        Layout.topMargin: 8
                        Layout.maximumWidth: 540
                        Layout.bottomMargin: Theme.sp5
                        wrapMode: Text.WordWrap
                        text: win.heads[win.sec].sub
                        color: Theme.t3
                        font.pixelSize: 12
                        font.family: Theme.fontSans
                        lineHeight: 1.5
                    }

                    // blocks (glabel + card)
                    Repeater {
                        model: win.buildBlocks(win.sections[win.sec])
                        delegate: ColumnLayout {
                            required property var modelData
                            required property int index
                            Layout.fillWidth: true
                            spacing: 0

                            Text {
                                visible: modelData.label !== ""
                                Layout.topMargin: index === 0 ? 0 : Theme.sp5
                                Layout.bottomMargin: Theme.sp2
                                Layout.leftMargin: 2
                                text: modelData.label
                                color: Theme.t4
                                font.pixelSize: 10
                                font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                font.family: Theme.fontSans
                                font.capitalization: Font.AllUppercase
                            }

                            // .card
                            Rectangle {
                                Layout.fillWidth: true
                                radius: 11
                                color: Theme.panel
                                border.color: Theme.hair
                                border.width: 1
                                implicitHeight: rowsCol.implicitHeight

                                ColumnLayout {
                                    id: rowsCol
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.leftMargin: Theme.sp4
                                    anchors.rightMargin: Theme.sp4
                                    spacing: 0

                                    Repeater {
                                        model: modelData.rows
                                        delegate: SettingsRow {
                                            required property var modelData
                                            required property int index
                                            Layout.fillWidth: true
                                            field: modelData
                                            isLast: index === (rowsCol.parent.parent.modelData ? 0 : 0)  // border handled inside
                                            showDivider: true
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Item { Layout.preferredHeight: Theme.sp5 }
                }
            }
        }

        // ===== footer (.sfoot) =====
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 56
            color: Theme.elev
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.hair }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.sp5
                anchors.rightMargin: 20
                Text { text: (i18n.language, i18n.t("set_changes_instant")); color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontSans }
                Item { Layout.fillWidth: true }
                BtnFlat { primary: true; text: (i18n.language, i18n.t("btn_close")); onClicked: win.close() }
            }
        }
    }

    // ---- single settings row (one field) ----
    component SettingsRow: ColumnLayout {
        property var field
        property bool showDivider: true
        property bool isLast: false
        spacing: 0

        // custom-only rows collapse unless the custom theme is active; win-only collapse off Windows
        readonly property bool rowVisible: (!field.customOnly || Theme.name === "custom")
                                           && (!field.winOnly || Qt.platform.os === "windows")
                                           && field.hidden !== true
        visible: rowVisible
        Layout.preferredHeight: rowVisible ? -1 : 0

        // .srow — label/note on the left, control on the right. The text column
        // fills the space left of the control, so a long label/note word-wraps
        // to a second line instead of running into the control.
        RowLayout {
            id: srow
            visible: field.type !== "warning"
            Layout.fillWidth: true
            Layout.topMargin: 13
            Layout.bottomMargin: 13
            spacing: Theme.sp4

            // .smeta
            ColumnLayout {
                Layout.fillWidth: true
                Layout.minimumWidth: 120
                spacing: 4
                RowLayout {
                    spacing: Theme.sp2
                    Layout.fillWidth: true
                    Text {
                        text: field.label
                        color: Theme.t1
                        font.pixelSize: 13
                        font.family: Theme.fontSans
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                    }
                    // badge
                    Rectangle {
                        visible: field.badge !== undefined
                        implicitWidth: bdg.implicitWidth + 14
                        implicitHeight: 18
                        radius: 999
                        color: Theme.field
                        border.color: Theme.hair
                        border.width: 1
                        Text { id: bdg; anchors.centerIn: parent; text: field.badge || ""; color: Theme.t3; font.pixelSize: 10; font.family: Theme.fontMono }
                    }
                }
                Text {
                    visible: field.note !== undefined
                    Layout.fillWidth: true          // wrap within the text column, never under the control
                    text: field.note || ""
                    color: Theme.t4
                    font.pixelSize: 11
                    font.family: Theme.fontSans
                    wrapMode: Text.WordWrap
                    lineHeight: 1.5
                }
            }

            // .sctrl — control by type. Wide controls (path, grow text) fill the
            // remaining width (capped) and shrink instead of overflowing the card.
            // Fixed-width control on the right; the text column (fillWidth) takes the
            // rest and wraps. Making the control fillWidth makes it fight a long note
            // for space and get pushed off-screen, so we keep it a fixed size.
            Loader {
                id: ctrl
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                visible: field.type !== "warning"
                sourceComponent: {
                    switch (field.type) {
                    case "toggle": return cToggle
                    case "path": return cPath
                    case "timerange": return cTimeRange
                    case "days": return cDays
                    case "anime": return cAnime
                    case "number": return cNumber
                    case "text": return cText
                    case "password": return cText
                    case "select": return cSelect
                    case "segmented": return cSegmented
                    case "theme": return cTheme
                    case "appicon": return cAppIcon
                    case "button": return cButton
                    case "iface": return cIface
                    case "profiles": return cProfiles
                    case "color": return cColor
                    case "bgimage": return cBgImage
                    case "slider": return cSlider
                    }
                    return null
                }
            }
        }

        // warning: full-width amber banner (the .srow above is hidden for it)
        RowLayout {
            visible: field.type === "warning"
            Layout.fillWidth: true
            Layout.topMargin: 12
            Layout.bottomMargin: 12
            spacing: Theme.sp3
            Text { text: "⚠"; color: Theme.amber; font.pixelSize: 13; Layout.alignment: Qt.AlignTop }
            Text {
                Layout.fillWidth: true
                visible: field.type === "warning"
                text: field.text || ""
                color: Theme.t3
                font.pixelSize: 11
                font.family: Theme.fontSans
                wrapMode: Text.WordWrap
                lineHeight: 1.45
            }
        }

        Rectangle { visible: showDivider; Layout.fillWidth: true; Layout.preferredHeight: 1; color: Theme.hairSoft }

        // ---- control components ----
        Component { id: cToggle; TToggle {
            on: (field.key === "torrentSearchEnabled" && typeof addons !== "undefined") ? addons.torrentSearchEnabled
                : (field.key === "autoTrackers" && typeof addons !== "undefined") ? addons.autoTrackers
                : (field.key === "followSystem" && typeof themeBridge !== "undefined") ? themeBridge.followSystem
                : win.boolPref(field)
            onToggled: function(v) {
                if (field.key === "torrentSearchEnabled" && typeof addons !== "undefined") addons.torrentSearchEnabled = v
                else if (field.key === "autoTrackers" && typeof addons !== "undefined") addons.autoTrackers = v
                else if (field.key === "followSystem" && typeof themeBridge !== "undefined") themeBridge.followSystem = v
                else if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, v)
            }
        } }
        Component {
            id: cAnime
            TToggle {
                on: Theme.anime
                onToggled: function(v) { Theme.setAnime(v) }
            }
        }
        Component {
            id: cTheme
            TSelect {
                // options index ↔ Theme.name
                readonly property var names: ["dark", "light", "midnight", "sakura", "darkstar", "custom"]
                implicitWidth: 180
                model: field.options || []
                currentIndex: Math.max(0, names.indexOf(Theme.name))
                onActivated: function(i) { Theme.setName(names[i]) }
            }
        }
        // app-icon picker — clickable tiles, independent of the UI theme
        Component {
            id: cAppIcon
            Row {
                spacing: 7
                Repeater {
                    model: (typeof themeBridge !== "undefined") ? themeBridge.appIcons() : []
                    delegate: Rectangle {
                        id: tile
                        required property var modelData
                        readonly property bool sel: typeof themeBridge !== "undefined"
                            && (themeBridge.appIconChoice, themeBridge.appIconChoice === modelData.key)
                        width: 44; height: 44; radius: 11
                        color: tile.sel ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.12) : "transparent"
                        border.width: 2
                        border.color: tile.sel ? Theme.accent : Theme.hair
                        Image {
                            anchors.centerIn: parent
                            width: 34; height: 34
                            source: tile.modelData.preview
                            sourceSize: Qt.size(68, 68)
                            fillMode: Image.PreserveAspectFit
                            smooth: true
                        }
                        MouseArea {
                            id: tileMa
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: if (typeof themeBridge !== "undefined") themeBridge.setAppIcon(tile.modelData.key)
                        }
                        ToolTip.text: tile.modelData.key
                        ToolTip.visible: tileMa.containsMouse
                        ToolTip.delay: 400
                    }
                }
            }
        }
        // ---- custom-theme controls (operate on the active profile) ----
        Component {
            id: cProfiles
            RowLayout {
                spacing: Theme.sp2
                TSelect {
                    implicitWidth: 150
                    model: {
                        var names = []
                        if (typeof themeBridge !== "undefined")
                            for (var i = 0; i < themeBridge.customProfiles.length; i++)
                                names.push(themeBridge.customProfiles[i].name)
                        return names
                    }
                    currentIndex: themeBridge ? themeBridge.activeProfile : 0
                    onActivated: function(i) { if (themeBridge) themeBridge.activeProfile = i }
                }
                BtnFlat { text: "+"; sm: true; onClicked: if (themeBridge) themeBridge.activeProfile = themeBridge.addProfile() }
                BtnFlat {
                    text: (i18n.language, i18n.t("set_custom_rename")); sm: true
                    onClicked: { renameDlg.idx = themeBridge.activeProfile; renameField.text = themeBridge.customProfiles[themeBridge.activeProfile].name; renameDlg.open() }
                }
                BtnFlat {
                    text: (i18n.language, i18n.t("set_custom_clear")); sm: true
                    enabled: themeBridge && themeBridge.customProfiles.length > 1
                    onClicked: if (themeBridge) themeBridge.removeProfile(themeBridge.activeProfile)
                }
            }
        }
        Component {
            id: cColor
            RowLayout {
                spacing: Theme.sp2
                readonly property string cur: (win.ap && win.ap[field.role]) ? win.ap[field.role] : "#000000"
                Rectangle {
                    implicitWidth: 26; implicitHeight: 26; radius: 6
                    color: parent.cur
                    border.color: Theme.hair; border.width: 1
                }
                TFld {
                    implicitWidth: 110; implicitHeight: 30; mono: true
                    text: parent.cur
                    placeholder: "#rrggbb"
                    onEdited: function(t) {
                        var v = t.charAt(0) === "#" ? t : "#" + t
                        if (/^#[0-9a-fA-F]{6}$/.test(v) && themeBridge)
                            themeBridge.setProfileColor(themeBridge.activeProfile, field.role, v.toLowerCase())
                    }
                }
                BtnFlat {
                    text: "…"; sm: true
                    onClicked: { colorDlg.targetRole = field.role; colorDlg.selectedColor = parent.cur; colorDlg.open() }
                }
            }
        }
        Component {
            id: cBgImage
            RowLayout {
                spacing: Theme.sp2
                TFld {
                    implicitWidth: 210; implicitHeight: 30; mono: true
                    text: (win.ap && win.ap.image) ? win.ap.image : ""
                    placeholder: (i18n.language, i18n.t("set_custom_bgimage"))
                    readonly: true
                }
                BtnFlat { text: (i18n.language, i18n.t("settings_browse")); sm: true; onClicked: bgImageDlg.open() }
                BtnFlat { text: (i18n.language, i18n.t("set_custom_clear")); sm: true; onClicked: if (themeBridge) themeBridge.setProfileImage(themeBridge.activeProfile, "") }
            }
        }
        Component {
            id: cSlider
            RowLayout {
                spacing: Theme.sp2
                Slider {
                    id: opacitySlider
                    implicitWidth: 150
                    from: 0; to: 100; stepSize: 1
                    onMoved: if (themeBridge) themeBridge.setProfileOpacity(themeBridge.activeProfile, Math.round(value))
                    // Binding (not a plain `value:`) so dragging doesn't break the link.
                    Binding { target: opacitySlider; property: "value"; value: (win.ap && win.ap.opacity !== undefined) ? win.ap.opacity : 55 }
                    background: Rectangle {
                        x: parent.leftPadding; y: parent.topPadding + parent.availableHeight / 2 - height / 2
                        width: parent.availableWidth; height: 4; radius: 2; color: Theme.track
                        Rectangle { width: parent.width * parent.parent.visualPosition; height: parent.height; radius: 2; color: Theme.accent }
                    }
                    handle: Rectangle {
                        x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
                        y: parent.topPadding + parent.availableHeight / 2 - height / 2
                        implicitWidth: 16; implicitHeight: 16; radius: 8
                        color: Theme.accent; border.color: Theme.bg; border.width: 2
                    }
                }
                Text {
                    text: ((win.ap && win.ap.opacity !== undefined) ? win.ap.opacity : 55) + "%"
                    color: Theme.t3; font.pixelSize: 11; font.family: Theme.fontMono
                    Layout.preferredWidth: 36
                }
            }
        }
        Component {
            id: cNumber
            RowLayout {
                spacing: Theme.sp2
                Rectangle {
                    implicitWidth: 92; implicitHeight: 30; radius: 7
                    color: Theme.field; border.color: Theme.hair; border.width: 1
                    TextInput {
                        anchors.fill: parent; anchors.rightMargin: 10
                        text: (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : (field.value || "")
                        color: Theme.t1
                        font.pixelSize: 12; font.family: Theme.fontMono
                        horizontalAlignment: TextInput.AlignRight
                        verticalAlignment: TextInput.AlignVCenter
                        onEditingFinished: if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, text)
                    }
                }
                Text { visible: field.suffix !== undefined; text: field.suffix || ""; color: Theme.t4; font.pixelSize: 11; font.family: Theme.fontMono }
            }
        }
        Component {
            id: cText
            TFld {
                implicitWidth: field.w === "grow" ? 300 : field.w === "w-md" ? 210 : field.w === "w-sm" ? 120 : 180
                implicitHeight: 30
                mono: field.mono === true
                password: field.type === "password"
                text: (field.key === "torrentSearchUrl" && typeof addons !== "undefined") ? addons.torrentSearchUrl
                      : (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : (field.value || "")
                placeholder: field.placeholder || ""
                onEdited: function(t) {
                    if (field.key === "torrentSearchUrl" && typeof addons !== "undefined") addons.torrentSearchUrl = t
                    else if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, t)
                }
            }
        }
        Component {
            id: cIface
            TSelect {
                implicitWidth: 220
                model: (typeof settings !== "undefined") ? settings.networkInterfaces() : []
                currentIndex: {
                    var cur = (typeof settings !== "undefined") ? (settings.get("outgoingInterface") || "") : ""
                    if (cur === "") return 0
                    for (var i = 1; i < model.length; i++)
                        if (model[i].split(" — ")[0] === cur) return i
                    return 0
                }
                onActivated: function(i) {
                    if (typeof settings !== "undefined")
                        settings.set("outgoingInterface", i === 0 ? "" : model[i].split(" — ")[0])
                }
            }
        }
        Component {
            id: cSelect
            TSelect {
                implicitWidth: 180
                model: field.options || []
                icons: field.icons || []
                currentIndex: {
                    if (field.isLang) return i18n.language
                    var v = (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : field.value
                    return (v === undefined || v === null || v === "") ? 0 : v
                }
                onActivated: function(i) { if (field.isLang) i18n.setLanguage(i); else if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, i) }
            }
        }
        Component {
            id: cSegmented
            Rectangle {
                implicitWidth: segR.implicitWidth + 4
                implicitHeight: 29
                radius: 8
                color: Theme.field
                border.color: Theme.hair
                border.width: 1
                property int curIdx: {
                    var v = (typeof settings !== "undefined" && field.key !== undefined) ? settings.get(field.key) : field.value
                    return (v === undefined || v === null || v === "") ? 0 : v
                }
                Row {
                    id: segR
                    anchors.centerIn: parent
                    spacing: 2
                    Repeater {
                        model: field.options || []
                        delegate: Rectangle {
                            height: 25
                            implicitWidth: segLbl.implicitWidth + 22
                            radius: 6
                            color: index === parent.parent.parent.curIdx ? Theme.sel : "transparent"
                            Text {
                                id: segLbl
                                anchors.centerIn: parent
                                text: modelData
                                color: index === parent.parent.parent.curIdx ? Theme.accentText : Theme.t3
                                font.pixelSize: 11
                                font.weight: Font.Medium
                                font.family: Theme.fontSans
                            }
                            MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: { parent.parent.parent.curIdx = index; if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, index) } }
                        }
                    }
                }
            }
        }
        Component { id: cButton; BtnFlat { text: field.btn || ""; sm: false; onClicked: win.runButtonAction(field.action) } }
        Component {
            id: cPath
            RowLayout {
                id: pathRow
                spacing: Theme.sp2
                function curPath() {
                    return (typeof settings !== "undefined" && field.key !== undefined)
                        ? (settings.get(field.key) || "") : (field.value || "")
                }
                TFld {
                    id: pathFld
                    implicitWidth: 220; implicitHeight: 30; mono: true
                    placeholder: field.placeholder || ""
                    // Driven imperatively (not bound) so it survives both the user
                    // typing into it AND a Browse pick — settings.get() isn't a
                    // reactive property, so a plain binding never refreshed on Browse.
                    Component.onCompleted: text = pathRow.curPath()
                    onEdited: function(t) {
                        if (typeof settings !== "undefined" && field.key !== undefined)
                            settings.set(field.key, t)
                    }
                    Connections {
                        target: typeof settings !== "undefined" ? settings : null
                        ignoreUnknownSignals: true
                        function onChanged() {
                            var v = pathRow.curPath()
                            if (pathFld.text !== v) pathFld.text = v
                        }
                    }
                }
                BtnFlat {
                    text: (i18n.language, i18n.t("settings_browse")); sm: true
                    onClicked: win.openPathPicker(field.key, field.file === true)
                }
                BtnFlat {
                    visible: field.file !== true
                    text: (i18n.language, i18n.t("set_custom_clear")); sm: true
                    onClicked: if (typeof settings !== "undefined" && field.key !== undefined) settings.set(field.key, "")
                }
            }
        }
        // scheduler from/to hour (0–23)
        Component {
            id: cTimeRange
            RowLayout {
                spacing: Theme.sp2
                Repeater {
                    model: [{ k: "scheduleFromHour" }, { k: "scheduleToHour" }]
                    delegate: Row {
                        spacing: Theme.sp2
                        Text { visible: index === 1; text: "—"; color: Theme.t4; font.pixelSize: 12; anchors.verticalCenter: parent.verticalCenter }
                        Rectangle {
                            width: 64; height: 30; radius: 7
                            color: Theme.field; border.color: Theme.hair; border.width: 1
                            TextInput {
                                anchors.fill: parent; anchors.margins: 6
                                text: (typeof settings !== "undefined") ? String(settings.get(modelData.k)) : "0"
                                color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontMono
                                horizontalAlignment: TextInput.AlignHCenter; verticalAlignment: TextInput.AlignVCenter
                                validator: IntValidator { bottom: 0; top: 23 }
                                onEditingFinished: if (typeof settings !== "undefined") settings.set(modelData.k, Math.max(0, Math.min(23, parseInt(text) || 0)))
                            }
                        }
                        Text { text: "h"; color: Theme.t4; font.pixelSize: 11; anchors.verticalCenter: parent.verticalCenter }
                    }
                }
            }
        }
        // scheduler active days — bitmask, bit0=Mon … bit6=Sun
        Component {
            id: cDays
            Row {
                spacing: 4
                property int mask: (typeof settings !== "undefined") ? (settings.get("scheduleDays") || 0) : 0
                Repeater {
                    model: [(i18n.language, i18n.t("day_mon")), i18n.t("day_tue"), i18n.t("day_wed"), i18n.t("day_thu"), i18n.t("day_fri"), i18n.t("day_sat"), i18n.t("day_sun")]
                    delegate: Rectangle {
                        width: 30; height: 30; radius: 7
                        readonly property bool sel: (parent.mask & (1 << index)) !== 0
                        // tint + border like the filter pills — seven solid accent
                        // squares in a row read as a warning, not a selection
                        color: sel ? Theme.accentTint : Theme.field
                        border.color: sel ? Theme.accent : Theme.hair; border.width: 1
                        Text { anchors.centerIn: parent; text: modelData; color: parent.sel ? Theme.accentText : Theme.t3; font.pixelSize: 11; font.weight: parent.sel ? Font.DemiBold : Font.Medium; font.family: Theme.fontSans }
                        MouseArea {
                            anchors.fill: parent; cursorShape: Qt.PointingHandCursor
                            onClicked: if (typeof settings !== "undefined") {
                                parent.parent.mask ^= (1 << index)
                                settings.set("scheduleDays", parent.parent.mask)
                            }
                        }
                    }
                }
            }
        }
    }

    // custom-theme dialogs
    ColorDialog {
        id: colorDlg
        property string targetRole: ""
        // QML color.toString() is "#AARRGGBB" (alpha first); the last 6 hex
        // digits are RRGGBB — slice(0,7) would wrongly keep '#'+alpha+RR.
        onAccepted: if (typeof themeBridge !== "undefined" && targetRole !== "")
                        themeBridge.setProfileColor(themeBridge.activeProfile, targetRole,
                                                    "#" + selectedColor.toString().slice(-6))
    }
    FileDialog {
        id: bgImageDlg
        title: (i18n.language, i18n.t("set_custom_bgimage"))
        nameFilters: ["Images (*.png *.jpg *.jpeg *.webp *.bmp)"]
        // Store a decoded plain filesystem path (selectedFile is a percent-
        // encoded URL); Theme.bgImageSource re-encodes it for Image.source.
        onAccepted: if (typeof themeBridge !== "undefined") {
            var u = selectedFile.toString().replace(/^file:\/\/\/?/, Qt.platform.os === "windows" ? "" : "/")
            themeBridge.setProfileImage(themeBridge.activeProfile, decodeURIComponent(u))
        }
    }
    // action buttons (default-app / pair phone)
    function runButtonAction(a) {
        if (a === "default") {
            var ok = (typeof settings !== "undefined") && settings.setAsDefaultApp()
            infoDlg.title = (i18n.language, i18n.t("set_default_app"))
            infoDlg.message = ok ? (i18n.language, i18n.t("settings_default_success"))
                                 : (i18n.language, i18n.t("settings_default_failed"))
            infoDlg.open()
        } else if (a === "pair") {
            pairDlg.open()
        } else if (a === "telegram") {
            if (typeof settings !== "undefined") settings.testTelegram()
        } else if (a === "defender") {
            var dok = (typeof settings !== "undefined") && settings.excludeFromDefender()
            infoDlg.title = "Windows Defender"
            infoDlg.message = dok ? i18n.t("defender_exclude_ok").replace(/:?\s*%1\s*$/, "")
                                  : (i18n.language, i18n.t("defender_exclude_fail"))
            infoDlg.open()
        } else if (a === "exportSettings") { backupSaveDlg.mode = "export"; backupSaveDlg.currentFile = "batorrent_settings.json"; backupSaveDlg.open() }
        else if (a === "fullBackup")      { backupSaveDlg.mode = "backup"; backupSaveDlg.currentFile = "batorrent-backup.bat"; backupSaveDlg.open() }
        else if (a === "importSettings")  { backupOpenDlg.mode = "import"; backupOpenDlg.open() }
        else if (a === "fullRestore")     { backupOpenDlg.mode = "restore"; backupOpenDlg.open() }
    }
    function showInfo(title, msg) { infoDlg.title = title; infoDlg.message = msg; infoDlg.open() }
    FileDialog {
        id: backupSaveDlg
        property string mode: ""
        fileMode: FileDialog.SaveFile
        nameFilters: mode === "backup" ? ["BATorrent backup (*.bat)"] : ["JSON (*.json)"]
        onAccepted: if (typeof settings !== "undefined") {
            var p = selectedFile.toString()
            win.showInfo(i18n.t("settings_advanced"), mode === "backup" ? settings.fullBackup(p) : settings.exportSettings(p))
        }
    }
    FileDialog {
        id: backupOpenDlg
        property string mode: ""
        fileMode: FileDialog.OpenFile
        nameFilters: mode === "restore" ? ["BATorrent backup (*.bat)"] : ["JSON (*.json)"]
        onAccepted: if (typeof settings !== "undefined") {
            var p = selectedFile.toString()
            win.showInfo(i18n.t("settings_advanced"), mode === "restore" ? settings.fullRestore(p) : settings.importSettings(p))
        }
    }

    Connections {
        target: typeof settings !== "undefined" ? settings : null
        ignoreUnknownSignals: true
        function onTelegramTestResult(ok, message) {
            infoDlg.title = (i18n.language, i18n.t("settings_telegram_test"))
            infoDlg.message = message
            infoDlg.open()
        }
    }

    function openPathPicker(key, isFile) {
        if (isFile) { pathFileDlg.targetKey = key; pathFileDlg.open() }
        else { pathFolderDlg.targetKey = key; pathFolderDlg.open() }
    }
    function decodeLocalPath(u) {
        var s = u.toString()
        // file:///C:/x → C:/x (win) ; file:///Users/x → /Users/x (mac/linux) ;
        // file://server/share → //server/share (UNC)
        if (s.startsWith("file:///")) s = s.substring(Qt.platform.os === "windows" ? 8 : 7)
        else if (s.startsWith("file://")) s = "//" + s.substring(7)
        return decodeURIComponent(s)
    }
    FolderDialog {
        id: pathFolderDlg
        property string targetKey: ""
        onAccepted: if (typeof settings !== "undefined" && targetKey !== "")
                        settings.set(targetKey, win.decodeLocalPath(selectedFolder))
    }
    FileDialog {
        id: pathFileDlg
        property string targetKey: ""
        onAccepted: if (typeof settings !== "undefined" && targetKey !== "")
                        settings.set(targetKey, win.decodeLocalPath(selectedFile))
    }

    PairingDialog { id: pairDlg }

    Dialog {
        id: infoDlg
        property string message: ""
        anchors.centerIn: parent
        modal: true
        standardButtons: Dialog.Ok
        contentItem: Text {
            text: infoDlg.message
            color: Theme.t1; font.pixelSize: 12; font.family: Theme.fontSans
            wrapMode: Text.WordWrap
        }
    }

    // rename the active profile
    Dialog {
        id: renameDlg
        property int idx: 0
        anchors.centerIn: parent
        modal: true
        title: (i18n.language, i18n.t("set_custom_rename"))
        standardButtons: Dialog.Ok | Dialog.Cancel
        onAccepted: if (themeBridge && renameField.text.length > 0) themeBridge.renameProfile(idx, renameField.text)
        contentItem: TFld {
            id: renameField
            implicitWidth: 240; implicitHeight: 32
        }
    }
}
