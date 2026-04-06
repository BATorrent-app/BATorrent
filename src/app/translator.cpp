// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "translator.h"

Translator &Translator::instance()
{
    static Translator t;
    return t;
}

Translator::Translator()
{
    loadEnglish();
}

void Translator::setLanguage(Language lang)
{
    m_lang = lang;
    switch (lang) {
    case Portuguese: loadPortuguese(); break;
    case Chinese:    loadChinese();    break;
    case Japanese:   loadJapanese();   break;
    case Russian:    loadRussian();    break;
    case Spanish:    loadSpanish();    break;
    case German:     loadGerman();     break;
    default:         loadEnglish();    break;
    }
}

QString Translator::tr(const QString &key) const
{
    return m_strings.value(key, key);
}

void Translator::loadEnglish()
{
    m_strings = {
        // Menu
        {"menu_file", "&File"},
        {"menu_torrent", "&Torrent"},
        {"menu_settings", "&Settings"},
        {"menu_help", "&Help"},
        {"action_open", "&Open Torrent..."},
        {"action_magnet", "Open &Magnet Link..."},
        {"action_quit", "&Quit"},
        {"action_pause", "&Pause"},
        {"action_resume", "&Resume"},
        {"action_remove", "&Remove"},
        {"action_remove_files", "Remove with &Files..."},
        {"action_settings", "&Preferences..."},
        {"action_language", "&Language"},
        {"action_about", "&About BATorrent"},
        {"action_release_notes", "&Release Notes"},
        {"release_notes_title", "Release Notes"},
        {"release_notes_subtitle", "Here's what's new in this version:"},
        {"release_notes_close", "Close"},
        {"action_welcome", "&Welcome Guide"},
        {"action_create", "&Create Torrent..."},
        {"action_pause_all", "Pause &All"},
        {"action_resume_all", "Resume A&ll"},
        {"action_import_qbt", "&Import from qBittorrent..."},
        {"action_check_update", "Check for &Updates..."},

        // Update
        {"update_title", "Update"},
        {"update_available", "BATorrent %1 is available. Download and install now?"},
        {"update_downloading", "Downloading update..."},
        {"update_none", "You are running the latest version."},

        // Import
        {"import_qbt_success", "Imported %1 torrent(s) from qBittorrent."},
        {"import_qbt_none", "No new torrents found in qBittorrent data."},

        // Context menu
        {"ctx_sequential", "Sequential Download"},
        {"ctx_open_folder", "Open Folder"},
        {"ctx_category", "Category"},
        {"category_none", "None"},

        // Filter
        {"filter_all_categories", "All Categories"},

        // Create torrent
        {"create_title", "Create Torrent"},
        {"create_source", "Source file/folder:"},
        {"create_output", "Output .torrent:"},
        {"create_trackers", "Trackers (one per line):"},
        {"create_comment", "Comment:"},
        {"create_piece_size", "Piece size:"},
        {"create_auto", "Auto"},
        {"create_btn", "Create Torrent"},
        {"create_select_source", "Select Source"},
        {"create_success", "Torrent created successfully!"},
        {"create_err_empty", "Please fill in source and output paths."},
        {"create_err_no_files", "No files found in the selected source."},

        // Toolbar
        {"tb_open", "Open"},
        {"tb_magnet", "Magnet"},
        {"tb_pause", "Pause"},
        {"tb_resume", "Resume"},
        {"tb_remove", "Remove"},
        {"tb_settings", "Settings"},

        // Table headers
        {"col_name", "Name"},
        {"col_size", "Size"},
        {"col_progress", "Progress"},
        {"col_down", "Down"},
        {"col_up", "Up"},
        {"col_state", "State"},
        {"col_category", "Category"},
        {"col_peers", "Peers"},

        // Details
        {"detail_general", "General"},
        {"detail_peers", "Peers"},
        {"detail_files", "Files"},
        {"detail_trackers", "Trackers"},
        {"detail_pieces", "Pieces"},
        {"detail_name", "Name:"},
        {"detail_save_path", "Save Path:"},
        {"detail_size", "Size:"},
        {"detail_downloaded", "Downloaded:"},
        {"detail_progress", "Progress:"},
        {"detail_down_speed", "Down Speed:"},
        {"detail_up_speed", "Up Speed:"},
        {"detail_state", "State:"},
        {"detail_peers_count", "Peers:"},
        {"detail_ratio", "Ratio:"},

        // Peer table
        {"peer_ip", "IP"},
        {"peer_port", "Port"},
        {"peer_client", "Client"},
        {"peer_down", "Down"},
        {"peer_up", "Up"},
        {"peer_progress", "Progress"},

        // File table
        {"file_name", "File"},
        {"file_size", "Size"},
        {"file_progress", "Progress"},
        {"file_priority", "Priority"},

        // File priorities
        {"priority_skip", "Skip"},
        {"priority_low", "Low"},
        {"priority_normal", "Normal"},
        {"priority_high", "High"},

        // Tracker table
        {"tracker_url_col", "URL"},
        {"tracker_tier", "Tier"},
        {"tracker_status", "Status"},
        {"tracker_add", "Add Tracker"},
        {"tracker_url", "Tracker URL:"},

        // Status
        {"status_no_torrents", "No torrents"},
        {"status_format", "%1 torrent(s)  |  Down: %2 KB/s  |  Up: %3 KB/s"},

        // States
        {"state_checking", "Checking"},
        {"state_metadata", "Metadata"},
        {"state_downloading", "Downloading"},
        {"state_finished", "Finished"},
        {"state_seeding", "Seeding"},
        {"state_paused", "Paused"},
        {"state_unknown", "Unknown"},

        // Dialogs
        {"dlg_open_torrent", "Open Torrent"},
        {"dlg_save_to", "Save To"},
        {"dlg_choose_folder", "Choose Save Folder"},
        {"dlg_add_magnet", "Add Magnet Link"},
        {"dlg_paste_magnet", "Paste magnet link:"},
        {"dlg_torrent_filter", "Torrent Files (*.torrent)"},
        {"dlg_download_complete", "Download Complete"},
        {"dlg_finished_msg", "%1 has finished downloading."},
        {"dlg_error", "Error"},
        {"dlg_confirm_delete", "Confirm Deletion"},
        {"dlg_confirm_delete_msg", "Delete selected torrent(s) and their downloaded files?"},

        // About
        {"about_description", "A lightweight, open-source BitTorrent client."},
        {"about_libraries", "Libraries"},
        {"about_license", "License:"},

        // Settings
        {"settings_title", "Preferences"},
        {"settings_general", "General"},
        {"settings_speed", "Speed Limits"},
        {"settings_network", "Network"},
        {"settings_default_save", "Default save path:"},
        {"settings_browse", "Browse..."},
        {"settings_language", "Language:"},
        {"settings_max_down", "Max download (0 = unlimited):"},
        {"settings_max_up", "Max upload (0 = unlimited):"},
        {"settings_start_tray", "Start minimized to tray"},
        {"settings_close_to_tray", "Close to tray instead of quitting"},
        {"settings_use_default_path", "Always use default save path (skip folder dialog)"},
        {"settings_theme", "Theme:"},
        {"settings_unlimited", "Unlimited"},
        {"settings_seed_ratio", "Stop seeding at ratio (0 = no limit):"},
        {"settings_max_conn", "Max connections:"},
        {"settings_enable_dht", "Enable DHT (trackerless peer discovery)"},
        {"settings_encryption", "Protocol encryption:"},
        {"settings_enc_enabled", "Enabled (prefer encrypted)"},
        {"settings_enc_forced", "Forced (encrypted only)"},
        {"settings_enc_disabled", "Disabled"},

        // Buttons
        {"btn_ok", "OK"},
        {"btn_cancel", "Cancel"},

        // Tray
        {"tray_show", "Show"},
        {"tray_quit", "Quit"},

        // Welcome
        {"welcome_title", "Welcome to BATorrent!"},
        {"welcome_subtitle", "Your lightweight BitTorrent client"},
        {"welcome_step1_title", "Add a Torrent"},
        {"welcome_step1_desc", "Click 'Open' in the toolbar to add a .torrent file, or use 'Magnet' for magnet links. You can also drag & drop files directly into the window."},
        {"welcome_step2_title", "Manage Downloads"},
        {"welcome_step2_desc", "Use Pause, Resume, and Remove buttons to control your downloads. Select a torrent to see details, peers, and files in the panel below."},
        {"welcome_step3_title", "Settings & Tray"},
        {"welcome_step3_desc", "Go to Settings > Preferences to set speed limits and default save path. Closing the window minimizes to system tray — right-click the tray icon to quit."},
        {"welcome_got_it", "Got it!"},
        {"welcome_dont_show", "Don't show again"},

        // Filter bar
        {"filter_search", "Search torrents..."},
        {"filter_all_active", "Active"},
        {"filter_downloading", "Downloading"},
        {"filter_seeding", "Seeding"},
        {"filter_paused", "Paused"},
        {"filter_finished", "Finished"},

        // VPN / Interface binding
        {"settings_vpn_group", "VPN / Interface Binding"},
        {"settings_interface", "Network interface:"},
        {"settings_iface_any", "Any (default)"},
        {"settings_iface_any_desc", "Traffic will use any available interface"},
        {"settings_iface_no_ip", "No IPv4 address found"},
        {"settings_refresh", "Refresh"},
        {"settings_kill_switch", "Pause torrents if interface drops (Kill Switch)"},
        {"settings_auto_resume", "Auto-resume when interface returns"},
        {"killswitch_title", "Kill Switch"},
        {"killswitch_triggered", "VPN interface went down — all torrents paused."},
        {"killswitch_restored", "VPN interface restored — torrents resumed."},

        // Auto-shutdown
        {"settings_auto_shutdown", "Shut down PC when all downloads finish"},
        {"action_auto_shutdown", "Auto Shutdown When Done"},
        {"shutdown_title", "Auto Shutdown"},
        {"shutdown_msg", "All downloads complete. Shutting down in %1 seconds..."},

        // Streaming
        {"ctx_stream", "Stream"},
        {"stream_started", "Streaming started for %1"},
        {"stream_no_video", "No video file found in this torrent."},
        {"stream_no_player", "No video player found. Install VLC or IINA to stream."},

        // Notifications
        {"notif_torrent_added", "Torrent Added"},
        {"settings_notif_sound", "Play sound on notifications"},
        {"settings_splash_sound", "Play bat sound on startup"},
        {"settings_set_default", "Set BATorrent as default torrent app"},
        {"settings_default_success", "BATorrent is now the default torrent application."},
        {"settings_default_failed", "Could not set default. Try running as administrator."},
        {"dlg_set_default_title", "Default Torrent Application"},
        {"dlg_set_default_msg", "Would you like to set BATorrent as the default application for .torrent files and magnet links?"},

        // Global stats
        {"status_global", "Total: %1 down  |  %2 up  |  Ratio: %3"},

        // Addons
        {"addon_title", "Addons"},
        {"addon_trackers_group", "Auto Tracker List"},
        {"addon_auto_trackers", "Automatically add public trackers to new torrents"},
        {"addon_tracker_count", "%1 trackers loaded"},
        {"addon_installed", "Installed Addons"},
        {"addon_remove", "Remove"},
        {"addon_install", "Install Addon (Stremio-compatible)"},
        {"addon_url_hint", "Paste addon URL (e.g. https://addon.example.com)"},
        {"addon_install_btn", "Install"},
        {"action_addons", "&Addons..."},
        {"action_search_addons", "&Search Addons..."},

        // Search
        {"search_title", "Search"},
        {"search_placeholder", "Search movies, series..."},
        {"search_btn", "Search"},
        {"search_col_name", "Name"},
        {"search_col_type", "Type"},
        {"search_col_year", "Year"},
        {"search_col_quality", "Quality / Title"},
        {"search_col_size", "Size"},
        {"search_col_addon", "Source"},
        {"search_back", "Back"},
        {"search_searching", "Searching..."},
        {"search_done", "%1 result(s) found"},
        {"search_loading_streams", "Loading streams for %1..."},
        {"search_streams_done", "%1 stream(s) available"},
        {"search_added", "Added: %1"},
        {"search_no_addons", "No addons installed. Go to Settings > Addons to add one."},
        {"search_no_catalog", "No catalog addon enabled. Enable Cinemeta in Settings > Addons."},
        {"search_no_stream", "No stream addon enabled. Enable Torrentio in Settings > Addons."},
        {"addon_suggested", "Suggested Addons"},
        {"addon_suggest_hint", "Click to install recommended addons:"},

        // Torrent Search
        {"addon_torrent_search_group", "Torrent Search"},
        {"addon_torrent_search_enable", "Enable torrent search"},
        {"addon_torrent_search_url", "API URL:"},
        {"addon_torrent_search_url_hint", "URL of a compatible torrent search API"},
        {"addon_torrent_search_hint", "Enter the URL of a torrent search API that returns JSON arrays with name, info_hash, size, seeders, leechers fields."},
        {"search_source_stremio", "Movies / Series"},
        {"search_source_torrents", "Torrents"},
        {"search_placeholder_torrent", "Search torrents..."},
        {"search_source_games", "Games"},
        {"search_placeholder_games", "Search game repacks..."},
        {"search_col_repacker", "Repacker"},
        {"search_cat_all", "All"},
        {"search_cat_audio", "Audio"},
        {"search_cat_video", "Video"},
        {"search_cat_apps", "Applications"},
        {"search_cat_games", "Games"},
        {"search_cat_other", "Other"},
        {"search_col_seeds", "Seeds"},
        {"search_col_leechers", "Leechers"},

        // RSS
        {"action_rss", "&RSS Manager..."},
        {"rss_title", "RSS Auto-Download"},
        {"rss_url_hint", "Paste RSS feed URL..."},
        {"rss_add", "Add Feed"},
        {"rss_remove", "Remove"},
        {"rss_refresh_all", "Refresh All"},
        {"rss_feeds", "Feeds"},
        {"rss_items", "Feed Items (double-click to download)"},
        {"rss_feed_settings", "Feed Settings"},
        {"rss_enabled", "Enabled"},
        {"rss_auto_download", "Auto-download matching items"},
        {"rss_filter", "Filter (regex):"},
        {"rss_filter_hint", "e.g. 1080p|720p or S01E\\d+"},
        {"rss_save_path", "Save to:"},
        {"rss_save_path_hint", "Leave empty for default path"},
        {"rss_interval", "Check every:"},
        {"rss_save_settings", "Save Settings"},
        {"rss_last_checked", "Last checked: %1"},
        {"rss_never_checked", "Never checked"},
        {"rss_col_title", "Title"},
        {"rss_col_size", "Size"},
        {"rss_col_date", "Date"},
        {"rss_adding", "Adding feed..."},
        {"rss_removed", "Feed removed."},
        {"rss_settings_saved", "Feed settings saved."},
        {"rss_items_count", "%1 item(s)"},
        {"rss_downloading", "Download started."},
        {"rss_refreshing", "Refreshing all feeds..."},
        {"rss_disabled", "disabled"},
        {"rss_auto", "auto"},
        {"rss_auto_downloaded", "RSS Auto-Download"},

        // WebUI
        {"settings_webui_enable", "Enable WebUI"},
        {"settings_webui_port", "Port:"},
        {"settings_webui_user", "Username:"},
        {"settings_webui_pass", "Password:"},
        {"settings_webui_pass_hint", "Leave empty to keep current"},
        {"settings_webui_remote", "Allow remote access (bind 0.0.0.0)"},
        {"settings_webui_warning_title", "Security Warning"},
        {"settings_webui_warning_msg", "Enabling remote access exposes the WebUI to your network. Use a VPN or reverse proxy with HTTPS for secure remote access."},

        // Bandwidth Scheduler
        {"settings_scheduler_group", "Bandwidth Scheduler"},
        {"settings_scheduler_enable", "Enable speed scheduler"},
        {"settings_alt_down", "Alt download limit:"},
        {"settings_alt_up", "Alt upload limit:"},
        {"settings_sched_from", "Active from:"},
        {"settings_sched_to", "to"},
        {"settings_sched_days", "Days:"},

        // Proxy
        {"settings_proxy_group", "Proxy"},
        {"settings_proxy_type", "Proxy type:"},
        {"settings_proxy_none", "None"},
        {"settings_proxy_host", "Host:"},
        {"settings_proxy_port", "Port:"},
        {"settings_proxy_user", "Username:"},
        {"settings_proxy_pass", "Password:"},
        {"settings_proxy_user_hint", "Optional"},

        // IP Filter
        {"settings_ip_filter_group", "IP Filtering"},
        {"settings_ip_filter_file", "Blocklist file:"},
        {"settings_ip_filter_hint", "P2P blocklist (.txt, .p2p, .dat)"},

        // Media Server
        {"settings_media_server", "Media Server"},
        {"settings_media_enable_plex", "Notify Plex on download complete"},
        {"settings_media_enable_jellyfin", "Notify Jellyfin/Emby on download complete"},
        {"settings_media_api_key", "API Key:"},

        // Speed Test
        {"action_speedtest", "&Speed Test"},
        {"speedtest_title", "Speed Test"},
        {"speedtest_start", "Start Test"},
        {"speedtest_ping", "Ping"},
        {"speedtest_download", "Download"},
        {"speedtest_upload", "Upload"},
        {"speedtest_testing", "Testing"},
        {"speedtest_complete", "Complete!"},
        {"speedtest_result", "Result"},

        // Statistics
        {"action_statistics", "S&tatistics"},
        {"stats_title", "Statistics"},
        {"stats_alltime", "All-Time"},
        {"stats_session", "This Session"},
        {"stats_downloaded", "Downloaded"},
        {"stats_uploaded", "Uploaded"},
        {"stats_ratio", "Ratio"},
        {"stats_torrents_added", "Torrents Added"},
        {"stats_uptime", "Uptime"},

        // Shortcuts & Settings Export/Import
        {"action_shortcuts", "&Keyboard Shortcuts"},
        {"shortcuts_title", "Keyboard Shortcuts"},
        {"shortcuts_key", "Shortcut"},
        {"shortcuts_action", "Action"},
        {"action_export_settings", "&Export Settings..."},
        {"action_import_settings", "&Import Settings..."},
        {"export_success", "Settings exported successfully."},
        {"import_success", "Settings imported successfully."},
        {"import_restart", "Please restart BATorrent for changes to take effect."},

        // GeoIP / Peers
        {"peer_country", "Country"},

        // Auto-move
        {"settings_automove", "Auto-move completed downloads to folder"},
        {"settings_automove_path", "Move to:"},

        // Download queue
        {"settings_max_active", "Max active downloads (0 = unlimited):"},
        {"ctx_queue_up", "Move Up in Queue"},
        {"ctx_queue_down", "Move Down in Queue"},
    };
}

void Translator::loadPortuguese()
{
    m_strings = {
        // Menu
        {"menu_file", "&Arquivo"},
        {"menu_torrent", "&Torrent"},
        {"menu_settings", "&Configurações"},
        {"menu_help", "A&juda"},
        {"action_open", "&Abrir Torrent..."},
        {"action_magnet", "Abrir Link &Magnet..."},
        {"action_quit", "&Sair"},
        {"action_pause", "&Pausar"},
        {"action_resume", "&Continuar"},
        {"action_remove", "&Remover"},
        {"action_remove_files", "Remover com &Arquivos..."},
        {"action_settings", "&Preferências..."},
        {"action_language", "&Idioma"},
        {"action_about", "&Sobre o BATorrent"},
        {"action_release_notes", "&Notas de Versão"},
        {"release_notes_title", "Notas de Versão"},
        {"release_notes_subtitle", "Confira as novidades desta versão:"},
        {"release_notes_close", "Fechar"},
        {"action_welcome", "&Guia de Boas-vindas"},
        {"action_create", "&Criar Torrent..."},
        {"action_pause_all", "Pausar &Todos"},
        {"action_resume_all", "Continuar T&odos"},
        {"action_import_qbt", "&Importar do qBittorrent..."},
        {"action_check_update", "Verificar &Atualizações..."},

        // Update
        {"update_title", "Atualização"},
        {"update_available", "BATorrent %1 está disponível. Baixar e instalar agora?"},
        {"update_downloading", "Baixando atualização..."},
        {"update_none", "Você já está usando a versão mais recente."},

        // Import
        {"import_qbt_success", "Importados %1 torrent(s) do qBittorrent."},
        {"import_qbt_none", "Nenhum torrent novo encontrado nos dados do qBittorrent."},

        // Context menu
        {"ctx_sequential", "Download Sequencial"},
        {"ctx_open_folder", "Abrir Pasta"},
        {"ctx_category", "Categoria"},
        {"category_none", "Nenhuma"},
        {"filter_all_categories", "Todas as Categorias"},

        // Create torrent
        {"create_title", "Criar Torrent"},
        {"create_source", "Arquivo/pasta de origem:"},
        {"create_output", "Arquivo .torrent de saída:"},
        {"create_trackers", "Trackers (um por linha):"},
        {"create_comment", "Comentário:"},
        {"create_piece_size", "Tamanho do pedaço:"},
        {"create_auto", "Automático"},
        {"create_btn", "Criar Torrent"},
        {"create_select_source", "Selecionar Origem"},
        {"create_success", "Torrent criado com sucesso!"},
        {"create_err_empty", "Preencha os caminhos de origem e saída."},
        {"create_err_no_files", "Nenhum arquivo encontrado na origem selecionada."},

        // Toolbar
        {"tb_open", "Abrir"},
        {"tb_magnet", "Magnet"},
        {"tb_pause", "Pausar"},
        {"tb_resume", "Continuar"},
        {"tb_remove", "Remover"},
        {"tb_settings", "Config"},

        // Table headers
        {"col_name", "Nome"},
        {"col_size", "Tamanho"},
        {"col_progress", "Progresso"},
        {"col_down", "Download"},
        {"col_up", "Upload"},
        {"col_state", "Estado"},
        {"col_category", "Categoria"},
        {"col_peers", "Peers"},

        // Details
        {"detail_general", "Geral"},
        {"detail_peers", "Peers"},
        {"detail_files", "Arquivos"},
        {"detail_trackers", "Trackers"},
        {"detail_pieces", "Pedaços"},
        {"detail_name", "Nome:"},
        {"detail_save_path", "Salvar em:"},
        {"detail_size", "Tamanho:"},
        {"detail_downloaded", "Baixado:"},
        {"detail_progress", "Progresso:"},
        {"detail_down_speed", "Vel. Download:"},
        {"detail_up_speed", "Vel. Upload:"},
        {"detail_state", "Estado:"},
        {"detail_peers_count", "Peers:"},
        {"detail_ratio", "Proporção:"},

        // Peer table
        {"peer_ip", "IP"},
        {"peer_port", "Porta"},
        {"peer_client", "Cliente"},
        {"peer_down", "Download"},
        {"peer_up", "Upload"},
        {"peer_progress", "Progresso"},

        // File table
        {"file_name", "Arquivo"},
        {"file_size", "Tamanho"},
        {"file_progress", "Progresso"},
        {"file_priority", "Prioridade"},

        // File priorities
        {"priority_skip", "Pular"},
        {"priority_low", "Baixa"},
        {"priority_normal", "Normal"},
        {"priority_high", "Alta"},

        // Tracker table
        {"tracker_url_col", "URL"},
        {"tracker_tier", "Nível"},
        {"tracker_status", "Status"},
        {"tracker_add", "Adicionar Tracker"},
        {"tracker_url", "URL do Tracker:"},

        // Status
        {"status_no_torrents", "Nenhum torrent"},
        {"status_format", "%1 torrent(s)  |  Down: %2 KB/s  |  Up: %3 KB/s"},

        // States
        {"state_checking", "Verificando"},
        {"state_metadata", "Metadados"},
        {"state_downloading", "Baixando"},
        {"state_finished", "Concluído"},
        {"state_seeding", "Semeando"},
        {"state_paused", "Pausado"},
        {"state_unknown", "Desconhecido"},

        // Dialogs
        {"dlg_open_torrent", "Abrir Torrent"},
        {"dlg_save_to", "Salvar em"},
        {"dlg_choose_folder", "Escolher Pasta de Destino"},
        {"dlg_add_magnet", "Adicionar Link Magnet"},
        {"dlg_paste_magnet", "Cole o link magnet:"},
        {"dlg_torrent_filter", "Arquivos Torrent (*.torrent)"},
        {"dlg_download_complete", "Download Concluído"},
        {"dlg_finished_msg", "%1 terminou de baixar."},
        {"dlg_error", "Erro"},
        {"dlg_confirm_delete", "Confirmar Exclusão"},
        {"dlg_confirm_delete_msg", "Excluir torrent(s) selecionado(s) e seus arquivos baixados?"},

        // About
        {"about_description", "Um cliente BitTorrent leve e de código aberto."},
        {"about_libraries", "Bibliotecas"},
        {"about_license", "Licença:"},

        // Settings
        {"settings_title", "Preferências"},
        {"settings_general", "Geral"},
        {"settings_speed", "Limites de Velocidade"},
        {"settings_network", "Rede"},
        {"settings_default_save", "Pasta padrão para salvar:"},
        {"settings_browse", "Procurar..."},
        {"settings_language", "Idioma:"},
        {"settings_max_down", "Max download (0 = ilimitado):"},
        {"settings_max_up", "Max upload (0 = ilimitado):"},
        {"settings_start_tray", "Iniciar minimizado na bandeja"},
        {"settings_close_to_tray", "Fechar para a bandeja ao invés de sair"},
        {"settings_use_default_path", "Sempre usar pasta padrão (pular diálogo de pasta)"},
        {"settings_theme", "Tema:"},
        {"settings_unlimited", "Ilimitado"},
        {"settings_seed_ratio", "Parar de semear na proporção (0 = sem limite):"},
        {"settings_max_conn", "Máximo de conexões:"},
        {"settings_enable_dht", "Habilitar DHT (descoberta de peers sem tracker)"},
        {"settings_encryption", "Criptografia do protocolo:"},
        {"settings_enc_enabled", "Habilitada (preferir criptografado)"},
        {"settings_enc_forced", "Forçada (somente criptografado)"},
        {"settings_enc_disabled", "Desabilitada"},

        // Buttons
        {"btn_ok", "OK"},
        {"btn_cancel", "Cancelar"},

        // Tray
        {"tray_show", "Mostrar"},
        {"tray_quit", "Sair"},

        // Welcome
        {"welcome_title", "Bem-vindo ao BATorrent!"},
        {"welcome_subtitle", "Seu cliente BitTorrent leve e rápido"},
        {"welcome_step1_title", "Adicionar um Torrent"},
        {"welcome_step1_desc", "Clique em 'Abrir' na barra de ferramentas para adicionar um arquivo .torrent, ou use 'Magnet' para links magnet. Você também pode arrastar e soltar arquivos diretamente na janela."},
        {"welcome_step2_title", "Gerenciar Downloads"},
        {"welcome_step2_desc", "Use os botões Pausar, Continuar e Remover para controlar seus downloads. Selecione um torrent para ver detalhes, peers e arquivos no painel abaixo."},
        {"welcome_step3_title", "Configurações e Bandeja"},
        {"welcome_step3_desc", "Vá em Configurações > Preferências para definir limites de velocidade e pasta padrão. Fechar a janela minimiza para a bandeja do sistema — clique com o botão direito no ícone da bandeja para sair."},
        {"welcome_got_it", "Entendi!"},
        {"welcome_dont_show", "Não mostrar novamente"},

        // Filter bar
        {"filter_search", "Buscar torrents..."},
        {"filter_all_active", "Ativos"},
        {"filter_downloading", "Baixando"},
        {"filter_seeding", "Semeando"},
        {"filter_paused", "Pausados"},
        {"filter_finished", "Concluídos"},

        // VPN / Interface binding
        {"settings_vpn_group", "VPN / Vínculo de Interface"},
        {"settings_interface", "Interface de rede:"},
        {"settings_iface_any", "Qualquer (padrão)"},
        {"settings_iface_any_desc", "O tráfego usará qualquer interface disponível"},
        {"settings_iface_no_ip", "Nenhum endereço IPv4 encontrado"},
        {"settings_refresh", "Atualizar"},
        {"settings_kill_switch", "Pausar torrents se a interface cair (Kill Switch)"},
        {"settings_auto_resume", "Retomar automaticamente quando a interface voltar"},
        {"killswitch_title", "Kill Switch"},
        {"killswitch_triggered", "Interface VPN caiu — todos os torrents pausados."},
        {"killswitch_restored", "Interface VPN restaurada — torrents retomados."},

        // Auto-shutdown
        {"settings_auto_shutdown", "Desligar PC quando todos os downloads terminarem"},
        {"action_auto_shutdown", "Desligar Automaticamente ao Concluir"},
        {"shutdown_title", "Desligamento Automático"},
        {"shutdown_msg", "Todos os downloads concluídos. Desligando em %1 segundos..."},

        // Streaming
        {"ctx_stream", "Transmitir"},
        {"stream_started", "Transmissão iniciada para %1"},
        {"stream_no_video", "Nenhum arquivo de vídeo encontrado neste torrent."},
        {"stream_no_player", "Nenhum player de vídeo encontrado. Instale o VLC ou IINA para transmitir."},

        // Notifications
        {"notif_torrent_added", "Torrent Adicionado"},
        {"settings_notif_sound", "Tocar som nas notificações"},
        {"settings_splash_sound", "Tocar som de morcego ao iniciar"},
        {"settings_set_default", "Definir BATorrent como app padrão de torrent"},
        {"settings_default_success", "BATorrent agora é o aplicativo padrão para torrents."},
        {"settings_default_failed", "Não foi possível definir como padrão. Tente executar como administrador."},
        {"dlg_set_default_title", "Aplicativo Padrão de Torrent"},
        {"dlg_set_default_msg", "Deseja definir o BATorrent como aplicativo padrão para arquivos .torrent e links magnet?"},

        // Global stats
        {"status_global", "Total: %1 baixado  |  %2 enviado  |  Proporção: %3"},

        // Addons
        {"addon_title", "Addons"},
        {"addon_trackers_group", "Lista de Trackers Automática"},
        {"addon_auto_trackers", "Adicionar trackers públicos automaticamente em novos torrents"},
        {"addon_tracker_count", "%1 trackers carregados"},
        {"addon_installed", "Addons Instalados"},
        {"addon_remove", "Remover"},
        {"addon_install", "Instalar Addon (compatível com Stremio)"},
        {"addon_url_hint", "Cole a URL do addon (ex: https://addon.exemplo.com)"},
        {"addon_install_btn", "Instalar"},
        {"action_addons", "&Addons..."},
        {"action_search_addons", "&Buscar nos Addons..."},

        // Search
        {"search_title", "Buscar"},
        {"search_placeholder", "Buscar filmes, séries..."},
        {"search_btn", "Buscar"},
        {"search_col_name", "Nome"},
        {"search_col_type", "Tipo"},
        {"search_col_year", "Ano"},
        {"search_col_quality", "Qualidade / Título"},
        {"search_col_size", "Tamanho"},
        {"search_col_addon", "Fonte"},
        {"search_back", "Voltar"},
        {"search_searching", "Buscando..."},
        {"search_done", "%1 resultado(s) encontrado(s)"},
        {"search_loading_streams", "Carregando streams para %1..."},
        {"search_streams_done", "%1 stream(s) disponível(eis)"},
        {"search_added", "Adicionado: %1"},
        {"search_no_addons", "Nenhum addon instalado. Vá em Configurações > Addons para adicionar."},
        {"search_no_catalog", "Nenhum addon de catálogo habilitado. Habilite o Cinemeta em Configurações > Addons."},
        {"search_no_stream", "Nenhum addon de streams habilitado. Habilite o Torrentio em Configurações > Addons."},
        {"addon_suggested", "Addons Sugeridos"},
        {"addon_suggest_hint", "Clique para instalar addons recomendados:"},

        // Torrent Search
        {"addon_torrent_search_group", "Busca de Torrents"},
        {"addon_torrent_search_enable", "Habilitar busca de torrents"},
        {"addon_torrent_search_url", "URL da API:"},
        {"addon_torrent_search_url_hint", "URL de uma API de busca de torrents compatível"},
        {"addon_torrent_search_hint", "Insira a URL de uma API de busca de torrents que retorne arrays JSON com os campos name, info_hash, size, seeders, leechers."},
        {"search_source_stremio", "Filmes / Séries"},
        {"search_source_torrents", "Torrents"},
        {"search_placeholder_torrent", "Buscar torrents..."},
        {"search_source_games", "Jogos"},
        {"search_placeholder_games", "Buscar repacks de jogos..."},
        {"search_col_repacker", "Repacker"},
        {"search_cat_all", "Todos"},
        {"search_cat_audio", "Áudio"},
        {"search_cat_video", "Vídeo"},
        {"search_cat_apps", "Aplicativos"},
        {"search_cat_games", "Jogos"},
        {"search_cat_other", "Outros"},
        {"search_col_seeds", "Seeds"},
        {"search_col_leechers", "Leechers"},

        // RSS
        {"action_rss", "&Gerenciador RSS..."},
        {"rss_title", "RSS Auto-Download"},
        {"rss_url_hint", "Cole a URL do feed RSS..."},
        {"rss_add", "Adicionar"},
        {"rss_remove", "Remover"},
        {"rss_refresh_all", "Atualizar Todos"},
        {"rss_feeds", "Feeds"},
        {"rss_items", "Itens do Feed (duplo-clique para baixar)"},
        {"rss_feed_settings", "Configurações do Feed"},
        {"rss_enabled", "Habilitado"},
        {"rss_auto_download", "Baixar automaticamente itens correspondentes"},
        {"rss_filter", "Filtro (regex):"},
        {"rss_filter_hint", "ex: 1080p|720p ou S01E\\d+"},
        {"rss_save_path", "Salvar em:"},
        {"rss_save_path_hint", "Deixe vazio para pasta padrão"},
        {"rss_interval", "Verificar a cada:"},
        {"rss_save_settings", "Salvar Config"},
        {"rss_last_checked", "Última verificação: %1"},
        {"rss_never_checked", "Nunca verificado"},
        {"rss_col_title", "Título"},
        {"rss_col_size", "Tamanho"},
        {"rss_col_date", "Data"},
        {"rss_adding", "Adicionando feed..."},
        {"rss_removed", "Feed removido."},
        {"rss_settings_saved", "Configurações do feed salvas."},
        {"rss_items_count", "%1 item(ns)"},
        {"rss_downloading", "Download iniciado."},
        {"rss_refreshing", "Atualizando todos os feeds..."},
        {"rss_disabled", "desabilitado"},
        {"rss_auto", "auto"},
        {"rss_auto_downloaded", "RSS Auto-Download"},

        // WebUI
        {"settings_webui_enable", "Habilitar WebUI"},
        {"settings_webui_port", "Porta:"},
        {"settings_webui_user", "Usuário:"},
        {"settings_webui_pass", "Senha:"},
        {"settings_webui_pass_hint", "Deixe vazio para manter a atual"},
        {"settings_webui_remote", "Permitir acesso remoto (bind 0.0.0.0)"},
        {"settings_webui_warning_title", "Aviso de Segurança"},
        {"settings_webui_warning_msg", "Habilitar acesso remoto expõe a WebUI para sua rede. Use uma VPN ou proxy reverso com HTTPS para acesso remoto seguro."},

        // Bandwidth Scheduler
        {"settings_scheduler_group", "Agendamento de Velocidade"},
        {"settings_scheduler_enable", "Habilitar agendamento de velocidade"},
        {"settings_alt_down", "Limite alt. download:"},
        {"settings_alt_up", "Limite alt. upload:"},
        {"settings_sched_from", "Ativo das:"},
        {"settings_sched_to", "até"},
        {"settings_sched_days", "Dias:"},

        // Proxy
        {"settings_proxy_group", "Proxy"},
        {"settings_proxy_type", "Tipo de proxy:"},
        {"settings_proxy_none", "Nenhum"},
        {"settings_proxy_host", "Host:"},
        {"settings_proxy_port", "Porta:"},
        {"settings_proxy_user", "Usuário:"},
        {"settings_proxy_pass", "Senha:"},
        {"settings_proxy_user_hint", "Opcional"},

        // IP Filter
        {"settings_ip_filter_group", "Filtragem de IP"},
        {"settings_ip_filter_file", "Arquivo de blocklist:"},
        {"settings_ip_filter_hint", "Blocklist P2P (.txt, .p2p, .dat)"},

        // Media Server
        {"settings_media_server", "Servidor de Mídia"},
        {"settings_media_enable_plex", "Notificar Plex ao concluir download"},
        {"settings_media_enable_jellyfin", "Notificar Jellyfin/Emby ao concluir download"},
        {"settings_media_api_key", "Chave API:"},

        // Speed Test
        {"action_speedtest", "&Teste de Velocidade"},
        {"speedtest_title", "Teste de Velocidade"},
        {"speedtest_start", "Iniciar Teste"},
        {"speedtest_ping", "Ping"},
        {"speedtest_download", "Download"},
        {"speedtest_upload", "Upload"},
        {"speedtest_testing", "Testando"},
        {"speedtest_complete", "Completo!"},
        {"speedtest_result", "Resultado"},

        // Statistics
        {"action_statistics", "E&statísticas"},
        {"stats_title", "Estatísticas"},
        {"stats_alltime", "Todo o Período"},
        {"stats_session", "Esta Sessão"},
        {"stats_downloaded", "Baixado"},
        {"stats_uploaded", "Enviado"},
        {"stats_ratio", "Proporção"},
        {"stats_torrents_added", "Torrents Adicionados"},
        {"stats_uptime", "Tempo Ativo"},

        // Shortcuts & Settings Export/Import
        {"action_shortcuts", "&Atalhos de Teclado"},
        {"shortcuts_title", "Atalhos de Teclado"},
        {"shortcuts_key", "Atalho"},
        {"shortcuts_action", "Ação"},
        {"action_export_settings", "&Exportar Configurações..."},
        {"action_import_settings", "&Importar Configurações..."},
        {"export_success", "Configurações exportadas com sucesso."},
        {"import_success", "Configurações importadas com sucesso."},
        {"import_restart", "Reinicie o BATorrent para aplicar as alterações."},

        // GeoIP / Peers
        {"peer_country", "País"},

        // Auto-move
        {"settings_automove", "Mover downloads concluídos automaticamente"},
        {"settings_automove_path", "Mover para:"},

        // Download queue
        {"settings_max_active", "Downloads ativos simultâneos (0 = ilimitado):"},
        {"ctx_queue_up", "Subir na Fila"},
        {"ctx_queue_down", "Descer na Fila"},
    };
}

void Translator::loadChinese()
{
    m_strings = {
        // Menu
        {"menu_file", "文件(&F)"},
        {"menu_torrent", "种子(&T)"},
        {"menu_settings", "设置(&S)"},
        {"menu_help", "帮助(&H)"},
        {"action_open", "打开种子(&O)..."},
        {"action_magnet", "打开磁力链接(&M)..."},
        {"action_quit", "退出(&Q)"},
        {"action_pause", "暂停(&P)"},
        {"action_resume", "恢复(&R)"},
        {"action_remove", "移除(&R)"},
        {"action_remove_files", "移除并删除文件(&F)..."},
        {"action_settings", "首选项(&P)..."},
        {"action_language", "语言(&L)"},
        {"action_about", "关于 BATorrent(&A)"},
        {"action_release_notes", "版本说明(&R)"},
        {"release_notes_title", "版本说明"},
        {"release_notes_subtitle", "此版本的新功能:"},
        {"release_notes_close", "关闭"},
        {"action_welcome", "欢迎指南(&W)"},
        {"action_create", "创建种子(&C)..."},
        {"action_pause_all", "全部暂停(&A)"},
        {"action_resume_all", "全部恢复(&L)"},
        {"action_import_qbt", "从 qBittorrent 导入(&I)..."},
        {"action_check_update", "检查更新(&U)..."},

        // Update
        {"update_title", "更新"},
        {"update_available", "BATorrent %1 已发布。是否立即下载并安装？"},
        {"update_downloading", "正在下载更新..."},
        {"update_none", "您正在使用最新版本。"},

        // Import
        {"import_qbt_success", "已从 qBittorrent 导入 %1 个种子。"},
        {"import_qbt_none", "在 qBittorrent 数据中未找到新种子。"},

        // Context menu
        {"ctx_sequential", "顺序下载"},
        {"ctx_open_folder", "打开文件夹"},
        {"ctx_category", "分类"},
        {"category_none", "无"},
        {"filter_all_categories", "所有分类"},

        // Create torrent
        {"create_title", "创建种子"},
        {"create_source", "源文件/文件夹："},
        {"create_output", "输出 .torrent 文件："},
        {"create_trackers", "Tracker（每行一个）："},
        {"create_comment", "备注："},
        {"create_piece_size", "分块大小："},
        {"create_auto", "自动"},
        {"create_btn", "创建种子"},
        {"create_select_source", "选择来源"},
        {"create_success", "种子创建成功！"},
        {"create_err_empty", "请填写源路径和输出路径。"},
        {"create_err_no_files", "在所选来源中未找到文件。"},

        // Toolbar
        {"tb_open", "打开"},
        {"tb_magnet", "磁力"},
        {"tb_pause", "暂停"},
        {"tb_resume", "恢复"},
        {"tb_remove", "移除"},
        {"tb_settings", "设置"},

        // Table headers
        {"col_name", "名称"},
        {"col_size", "大小"},
        {"col_progress", "进度"},
        {"col_down", "下载"},
        {"col_up", "上传"},
        {"col_state", "状态"},
        {"col_category", "分类"},
        {"col_peers", "节点"},

        // Details
        {"detail_general", "概要"},
        {"detail_peers", "节点"},
        {"detail_files", "文件"},
        {"detail_trackers", "Tracker"},
        {"detail_pieces", "片段"},
        {"detail_name", "名称："},
        {"detail_save_path", "保存路径："},
        {"detail_size", "大小："},
        {"detail_downloaded", "已下载："},
        {"detail_progress", "进度："},
        {"detail_down_speed", "下载速度："},
        {"detail_up_speed", "上传速度："},
        {"detail_state", "状态："},
        {"detail_peers_count", "节点："},
        {"detail_ratio", "分享率："},

        // Peer table
        {"peer_ip", "IP"},
        {"peer_port", "端口"},
        {"peer_client", "客户端"},
        {"peer_down", "下载"},
        {"peer_up", "上传"},
        {"peer_progress", "进度"},

        // File table
        {"file_name", "文件"},
        {"file_size", "大小"},
        {"file_progress", "进度"},
        {"file_priority", "优先级"},

        // File priorities
        {"priority_skip", "跳过"},
        {"priority_low", "低"},
        {"priority_normal", "普通"},
        {"priority_high", "高"},

        // Tracker table
        {"tracker_url_col", "URL"},
        {"tracker_tier", "层级"},
        {"tracker_status", "状态"},
        {"tracker_add", "添加 Tracker"},
        {"tracker_url", "Tracker URL："},

        // Status
        {"status_no_torrents", "暂无种子"},
        {"status_format", "%1 个种子  |  下载：%2 KB/s  |  上传：%3 KB/s"},

        // States
        {"state_checking", "校验中"},
        {"state_metadata", "获取元数据"},
        {"state_downloading", "下载中"},
        {"state_finished", "已完成"},
        {"state_seeding", "做种中"},
        {"state_paused", "已暂停"},
        {"state_unknown", "未知"},

        // Dialogs
        {"dlg_open_torrent", "打开种子"},
        {"dlg_save_to", "保存到"},
        {"dlg_choose_folder", "选择保存文件夹"},
        {"dlg_add_magnet", "添加磁力链接"},
        {"dlg_paste_magnet", "粘贴磁力链接："},
        {"dlg_torrent_filter", "种子文件 (*.torrent)"},
        {"dlg_download_complete", "下载完成"},
        {"dlg_finished_msg", "%1 已下载完成。"},
        {"dlg_error", "错误"},
        {"dlg_confirm_delete", "确认删除"},
        {"dlg_confirm_delete_msg", "删除所选种子及其已下载的文件？"},

        // About
        {"about_description", "一个轻量级的开源 BitTorrent 客户端。"},
        {"about_libraries", "依赖库"},
        {"about_license", "许可证："},

        // Settings
        {"settings_title", "首选项"},
        {"settings_general", "常规"},
        {"settings_speed", "速度限制"},
        {"settings_network", "网络"},
        {"settings_default_save", "默认保存路径："},
        {"settings_browse", "浏览..."},
        {"settings_language", "语言："},
        {"settings_max_down", "最大下载速度（0 = 不限）："},
        {"settings_max_up", "最大上传速度（0 = 不限）："},
        {"settings_start_tray", "启动时最小化到托盘"},
        {"settings_close_to_tray", "关闭窗口时最小化到托盘"},
        {"settings_use_default_path", "始终使用默认保存路径（跳过文件夹选择）"},
        {"settings_theme", "主题："},
        {"settings_unlimited", "不限"},
        {"settings_seed_ratio", "达到分享率时停止做种（0 = 不限）："},
        {"settings_max_conn", "最大连接数："},
        {"settings_enable_dht", "启用 DHT（无 Tracker 节点发现）"},
        {"settings_encryption", "协议加密："},
        {"settings_enc_enabled", "启用（优先加密）"},
        {"settings_enc_forced", "强制（仅加密）"},
        {"settings_enc_disabled", "禁用"},

        // Buttons
        {"btn_ok", "确定"},
        {"btn_cancel", "取消"},

        // Tray
        {"tray_show", "显示"},
        {"tray_quit", "退出"},

        // Welcome
        {"welcome_title", "欢迎使用 BATorrent！"},
        {"welcome_subtitle", "您的轻量级 BitTorrent 客户端"},
        {"welcome_step1_title", "添加种子"},
        {"welcome_step1_desc", "点击工具栏中的'打开'添加 .torrent 文件，或使用'磁力'添加磁力链接。您也可以将文件直接拖放到窗口中。"},
        {"welcome_step2_title", "管理下载"},
        {"welcome_step2_desc", "使用暂停、恢复和移除按钮控制下载。选中一个种子可在下方面板查看详情、节点和文件。"},
        {"welcome_step3_title", "设置与托盘"},
        {"welcome_step3_desc", "前往设置 > 首选项来配置速度限制和默认保存路径。关闭窗口将最小化到系统托盘——右键点击托盘图标可退出。"},
        {"welcome_got_it", "知道了！"},
        {"welcome_dont_show", "不再显示"},

        // Filter bar
        {"filter_search", "搜索种子..."},
        {"filter_all_active", "活动中"},
        {"filter_downloading", "下载中"},
        {"filter_seeding", "做种中"},
        {"filter_paused", "已暂停"},
        {"filter_finished", "已完成"},

        // VPN / Interface binding
        {"settings_vpn_group", "VPN / 网络接口绑定"},
        {"settings_interface", "网络接口："},
        {"settings_iface_any", "任意（默认）"},
        {"settings_iface_any_desc", "流量将使用任何可用的接口"},
        {"settings_iface_no_ip", "未找到 IPv4 地址"},
        {"settings_refresh", "刷新"},
        {"settings_kill_switch", "接口断开时暂停种子（Kill Switch）"},
        {"settings_auto_resume", "接口恢复时自动继续"},
        {"killswitch_title", "Kill Switch"},
        {"killswitch_triggered", "VPN 接口已断开——所有种子已暂停。"},
        {"killswitch_restored", "VPN 接口已恢复——种子已继续。"},

        // Auto-shutdown
        {"settings_auto_shutdown", "所有下载完成后关机"},
        {"action_auto_shutdown", "下载完成后自动关机"},
        {"shutdown_title", "自动关机"},
        {"shutdown_msg", "所有下载已完成。将在 %1 秒后关机..."},

        // Streaming
        {"ctx_stream", "串流"},
        {"stream_started", "已开始串流 %1"},
        {"stream_no_video", "此种子中未找到视频文件。"},
        {"stream_no_player", "未找到视频播放器。请安装 VLC 或 IINA 以进行串流。"},

        // Notifications
        {"notif_torrent_added", "已添加种子"},
        {"settings_notif_sound", "通知时播放声音"},
        {"settings_splash_sound", "启动时播放蝙蝠声音"},
        {"settings_set_default", "将 BATorrent 设为默认种子应用"},
        {"settings_default_success", "BATorrent 已成为默认种子应用程序。"},
        {"settings_default_failed", "无法设为默认。请尝试以管理员身份运行。"},
        {"dlg_set_default_title", "默认种子应用程序"},
        {"dlg_set_default_msg", "是否将 BATorrent 设为 .torrent 文件和磁力链接的默认应用程序？"},

        // Global stats
        {"status_global", "总计：%1 下载  |  %2 上传  |  分享率：%3"},

        // Addons
        {"addon_title", "插件"},
        {"addon_trackers_group", "自动 Tracker 列表"},
        {"addon_auto_trackers", "自动为新种子添加公共 Tracker"},
        {"addon_tracker_count", "已加载 %1 个 Tracker"},
        {"addon_installed", "已安装的插件"},
        {"addon_remove", "移除"},
        {"addon_install", "安装插件（Stremio 兼容）"},
        {"addon_url_hint", "粘贴插件 URL（例如 https://addon.example.com）"},
        {"addon_install_btn", "安装"},
        {"action_addons", "插件(&A)..."},
        {"action_search_addons", "搜索插件(&S)..."},

        // Search
        {"search_title", "搜索"},
        {"search_placeholder", "搜索电影、剧集..."},
        {"search_btn", "搜索"},
        {"search_col_name", "名称"},
        {"search_col_type", "类型"},
        {"search_col_year", "年份"},
        {"search_col_quality", "画质 / 标题"},
        {"search_col_size", "大小"},
        {"search_col_addon", "来源"},
        {"search_back", "返回"},
        {"search_searching", "搜索中..."},
        {"search_done", "找到 %1 个结果"},
        {"search_loading_streams", "正在加载 %1 的资源..."},
        {"search_streams_done", "有 %1 个可用资源"},
        {"search_added", "已添加：%1"},
        {"search_no_addons", "未安装任何插件。请前往设置 > 插件进行添加。"},
        {"search_no_catalog", "未启用目录插件。请在设置 > 插件中启用 Cinemeta。"},
        {"search_no_stream", "未启用资源插件。请在设置 > 插件中启用 Torrentio。"},
        {"addon_suggested", "推荐插件"},
        {"addon_suggest_hint", "点击安装推荐的插件："},

        // Torrent Search
        {"addon_torrent_search_group", "种子搜索"},
        {"addon_torrent_search_enable", "启用种子搜索"},
        {"addon_torrent_search_url", "API URL："},
        {"addon_torrent_search_url_hint", "兼容的种子搜索 API URL"},
        {"addon_torrent_search_hint", "输入种子搜索 API 的 URL，该 API 应返回包含 name、info_hash、size、seeders、leechers 字段的 JSON 数组。"},
        {"search_source_stremio", "电影 / 剧集"},
        {"search_source_torrents", "种子"},
        {"search_placeholder_torrent", "搜索种子..."},
        {"search_source_games", "游戏"},
        {"search_placeholder_games", "搜索游戏重打包..."},
        {"search_col_repacker", "重打包者"},
        {"search_cat_all", "全部"},
        {"search_cat_audio", "音频"},
        {"search_cat_video", "视频"},
        {"search_cat_apps", "应用"},
        {"search_cat_games", "游戏"},
        {"search_cat_other", "其他"},
        {"search_col_seeds", "做种"},
        {"search_col_leechers", "下载"},

        // RSS
        {"action_rss", "RSS 管理器(&R)..."},
        {"rss_title", "RSS 自动下载"},
        {"rss_url_hint", "粘贴 RSS 订阅 URL..."},
        {"rss_add", "添加订阅"},
        {"rss_remove", "移除"},
        {"rss_refresh_all", "全部刷新"},
        {"rss_feeds", "订阅源"},
        {"rss_items", "订阅项目（双击下载）"},
        {"rss_feed_settings", "订阅设置"},
        {"rss_enabled", "已启用"},
        {"rss_auto_download", "自动下载匹配的项目"},
        {"rss_filter", "过滤器（正则）："},
        {"rss_filter_hint", "例如 1080p|720p 或 S01E\\d+"},
        {"rss_save_path", "保存到："},
        {"rss_save_path_hint", "留空使用默认路径"},
        {"rss_interval", "检查间隔："},
        {"rss_save_settings", "保存设置"},
        {"rss_last_checked", "上次检查：%1"},
        {"rss_never_checked", "从未检查"},
        {"rss_col_title", "标题"},
        {"rss_col_size", "大小"},
        {"rss_col_date", "日期"},
        {"rss_adding", "正在添加订阅..."},
        {"rss_removed", "订阅已移除。"},
        {"rss_settings_saved", "订阅设置已保存。"},
        {"rss_items_count", "%1 个项目"},
        {"rss_downloading", "下载已开始。"},
        {"rss_refreshing", "正在刷新所有订阅..."},
        {"rss_disabled", "已禁用"},
        {"rss_auto", "自动"},
        {"rss_auto_downloaded", "RSS 自动下载"},

        // WebUI
        {"settings_webui_enable", "启用 WebUI"},
        {"settings_webui_port", "端口："},
        {"settings_webui_user", "用户名："},
        {"settings_webui_pass", "密码："},
        {"settings_webui_pass_hint", "留空保持当前密码"},
        {"settings_webui_remote", "允许远程访问（绑定 0.0.0.0）"},
        {"settings_webui_warning_title", "安全警告"},
        {"settings_webui_warning_msg", "启用远程访问会将 WebUI 暴露在网络中。请使用 VPN 或带 HTTPS 的反向代理来安全地远程访问。"},

        // Bandwidth Scheduler
        {"settings_scheduler_group", "带宽调度"},
        {"settings_scheduler_enable", "启用速度调度"},
        {"settings_alt_down", "备用下载限速："},
        {"settings_alt_up", "备用上传限速："},
        {"settings_sched_from", "生效时间从："},
        {"settings_sched_to", "到"},
        {"settings_sched_days", "生效日期："},

        // Proxy
        {"settings_proxy_group", "代理"},
        {"settings_proxy_type", "代理类型："},
        {"settings_proxy_none", "无"},
        {"settings_proxy_host", "主机："},
        {"settings_proxy_port", "端口："},
        {"settings_proxy_user", "用户名："},
        {"settings_proxy_pass", "密码："},
        {"settings_proxy_user_hint", "可选"},

        // IP Filter
        {"settings_ip_filter_group", "IP 过滤"},
        {"settings_ip_filter_file", "屏蔽列表文件："},
        {"settings_ip_filter_hint", "P2P 屏蔽列表（.txt、.p2p、.dat）"},

        // Media Server
        {"settings_media_server", "媒体服务器"},
        {"settings_media_enable_plex", "下载完成后通知 Plex"},
        {"settings_media_enable_jellyfin", "下载完成后通知 Jellyfin/Emby"},
        {"settings_media_api_key", "API 密钥："},

        // Speed Test
        {"action_speedtest", "网速测试(&S)"},
        {"speedtest_title", "网速测试"},
        {"speedtest_start", "开始测试"},
        {"speedtest_ping", "延迟"},
        {"speedtest_download", "下载"},
        {"speedtest_upload", "上传"},
        {"speedtest_testing", "测试中"},
        {"speedtest_complete", "完成！"},
        {"speedtest_result", "结果"},

        // Statistics
        {"action_statistics", "统计(&T)"},
        {"stats_title", "统计"},
        {"stats_alltime", "所有时间"},
        {"stats_session", "本次会话"},
        {"stats_downloaded", "已下载"},
        {"stats_uploaded", "已上传"},
        {"stats_ratio", "分享率"},
        {"stats_torrents_added", "已添加种子数"},
        {"stats_uptime", "运行时间"},

        // Shortcuts & Settings Export/Import
        {"action_shortcuts", "快捷键(&K)"},
        {"shortcuts_title", "快捷键"},
        {"shortcuts_key", "快捷键"},
        {"shortcuts_action", "操作"},
        {"action_export_settings", "导出设置(&E)..."},
        {"action_import_settings", "导入设置(&I)..."},
        {"export_success", "设置导出成功。"},
        {"import_success", "设置导入成功。"},
        {"import_restart", "请重启 BATorrent 以使更改生效。"},

        // GeoIP / Peers
        {"peer_country", "国家"},

        // Auto-move
        {"settings_automove", "自动移动已完成的下载"},
        {"settings_automove_path", "移动到："},

        // Download queue
        {"settings_max_active", "最大活动下载数（0 = 无限）："},
        {"ctx_queue_up", "上移队列"},
        {"ctx_queue_down", "下移队列"},
    };
}

void Translator::loadJapanese()
{
    m_strings = {
        // Menu
        {"menu_file", "ファイル(&F)"},
        {"menu_torrent", "トレント(&T)"},
        {"menu_settings", "設定(&S)"},
        {"menu_help", "ヘルプ(&H)"},
        {"action_open", "トレントを開く(&O)..."},
        {"action_magnet", "マグネットリンクを開く(&M)..."},
        {"action_quit", "終了(&Q)"},
        {"action_pause", "一時停止(&P)"},
        {"action_resume", "再開(&R)"},
        {"action_remove", "削除(&R)"},
        {"action_remove_files", "ファイルごと削除(&F)..."},
        {"action_settings", "環境設定(&P)..."},
        {"action_language", "言語(&L)"},
        {"action_about", "BATorrent について(&A)"},
        {"action_release_notes", "リリースノート(&R)"},
        {"release_notes_title", "リリースノート"},
        {"release_notes_subtitle", "このバージョンの新機能:"},
        {"release_notes_close", "閉じる"},
        {"action_welcome", "ようこそガイド(&W)"},
        {"action_create", "トレント作成(&C)..."},
        {"action_pause_all", "すべて一時停止(&A)"},
        {"action_resume_all", "すべて再開(&L)"},
        {"action_import_qbt", "qBittorrent からインポート(&I)..."},
        {"action_check_update", "アップデートを確認(&U)..."},

        // Update
        {"update_title", "アップデート"},
        {"update_available", "BATorrent %1 が利用可能です。今すぐダウンロードしてインストールしますか？"},
        {"update_downloading", "アップデートをダウンロード中..."},
        {"update_none", "最新バージョンを使用しています。"},

        // Import
        {"import_qbt_success", "qBittorrent から %1 個のトレントをインポートしました。"},
        {"import_qbt_none", "qBittorrent のデータに新しいトレントは見つかりませんでした。"},

        // Context menu
        {"ctx_sequential", "シーケンシャルダウンロード"},
        {"ctx_open_folder", "フォルダーを開く"},
        {"ctx_category", "カテゴリ"},
        {"category_none", "なし"},
        {"filter_all_categories", "すべてのカテゴリ"},

        // Create torrent
        {"create_title", "トレント作成"},
        {"create_source", "ソースファイル/フォルダー："},
        {"create_output", "出力先 .torrent："},
        {"create_trackers", "トラッカー（1行に1つ）："},
        {"create_comment", "コメント："},
        {"create_piece_size", "ピースサイズ："},
        {"create_auto", "自動"},
        {"create_btn", "トレント作成"},
        {"create_select_source", "ソースを選択"},
        {"create_success", "トレントが正常に作成されました！"},
        {"create_err_empty", "ソースと出力のパスを入力してください。"},
        {"create_err_no_files", "選択したソースにファイルが見つかりません。"},

        // Toolbar
        {"tb_open", "開く"},
        {"tb_magnet", "マグネット"},
        {"tb_pause", "一時停止"},
        {"tb_resume", "再開"},
        {"tb_remove", "削除"},
        {"tb_settings", "設定"},

        // Table headers
        {"col_name", "名前"},
        {"col_size", "サイズ"},
        {"col_progress", "進捗"},
        {"col_down", "ダウン"},
        {"col_up", "アップ"},
        {"col_state", "状態"},
        {"col_category", "カテゴリ"},
        {"col_peers", "ピア"},

        // Details
        {"detail_general", "概要"},
        {"detail_peers", "ピア"},
        {"detail_files", "ファイル"},
        {"detail_trackers", "トラッカー"},
        {"detail_pieces", "ピース"},
        {"detail_name", "名前："},
        {"detail_save_path", "保存先："},
        {"detail_size", "サイズ："},
        {"detail_downloaded", "ダウンロード済み："},
        {"detail_progress", "進捗："},
        {"detail_down_speed", "ダウンロード速度："},
        {"detail_up_speed", "アップロード速度："},
        {"detail_state", "状態："},
        {"detail_peers_count", "ピア："},
        {"detail_ratio", "共有比："},

        // Peer table
        {"peer_ip", "IP"},
        {"peer_port", "ポート"},
        {"peer_client", "クライアント"},
        {"peer_down", "ダウン"},
        {"peer_up", "アップ"},
        {"peer_progress", "進捗"},

        // File table
        {"file_name", "ファイル"},
        {"file_size", "サイズ"},
        {"file_progress", "進捗"},
        {"file_priority", "優先度"},

        // File priorities
        {"priority_skip", "スキップ"},
        {"priority_low", "低"},
        {"priority_normal", "通常"},
        {"priority_high", "高"},

        // Tracker table
        {"tracker_url_col", "URL"},
        {"tracker_tier", "ティア"},
        {"tracker_status", "ステータス"},
        {"tracker_add", "トラッカーを追加"},
        {"tracker_url", "トラッカー URL："},

        // Status
        {"status_no_torrents", "トレントなし"},
        {"status_format", "%1 個のトレント  |  ダウン：%2 KB/s  |  アップ：%3 KB/s"},

        // States
        {"state_checking", "チェック中"},
        {"state_metadata", "メタデータ取得中"},
        {"state_downloading", "ダウンロード中"},
        {"state_finished", "完了"},
        {"state_seeding", "シード中"},
        {"state_paused", "一時停止中"},
        {"state_unknown", "不明"},

        // Dialogs
        {"dlg_open_torrent", "トレントを開く"},
        {"dlg_save_to", "保存先"},
        {"dlg_choose_folder", "保存先フォルダーを選択"},
        {"dlg_add_magnet", "マグネットリンクを追加"},
        {"dlg_paste_magnet", "マグネットリンクを貼り付け："},
        {"dlg_torrent_filter", "トレントファイル (*.torrent)"},
        {"dlg_download_complete", "ダウンロード完了"},
        {"dlg_finished_msg", "%1 のダウンロードが完了しました。"},
        {"dlg_error", "エラー"},
        {"dlg_confirm_delete", "削除の確認"},
        {"dlg_confirm_delete_msg", "選択したトレントとダウンロード済みファイルを削除しますか？"},

        // About
        {"about_description", "軽量なオープンソース BitTorrent クライアント。"},
        {"about_libraries", "使用ライブラリ"},
        {"about_license", "ライセンス："},

        // Settings
        {"settings_title", "環境設定"},
        {"settings_general", "一般"},
        {"settings_speed", "速度制限"},
        {"settings_network", "ネットワーク"},
        {"settings_default_save", "既定の保存先："},
        {"settings_browse", "参照..."},
        {"settings_language", "言語："},
        {"settings_max_down", "最大ダウンロード速度（0 = 無制限）："},
        {"settings_max_up", "最大アップロード速度（0 = 無制限）："},
        {"settings_start_tray", "トレイに最小化して起動"},
        {"settings_close_to_tray", "閉じるときにトレイに最小化"},
        {"settings_use_default_path", "常に既定の保存先を使用（フォルダー選択をスキップ）"},
        {"settings_theme", "テーマ："},
        {"settings_unlimited", "無制限"},
        {"settings_seed_ratio", "指定共有比で停止（0 = 無制限）："},
        {"settings_max_conn", "最大接続数："},
        {"settings_enable_dht", "DHT を有効にする（トラッカーなしでピア発見）"},
        {"settings_encryption", "プロトコル暗号化："},
        {"settings_enc_enabled", "有効（暗号化を優先）"},
        {"settings_enc_forced", "強制（暗号化のみ）"},
        {"settings_enc_disabled", "無効"},

        // Buttons
        {"btn_ok", "OK"},
        {"btn_cancel", "キャンセル"},

        // Tray
        {"tray_show", "表示"},
        {"tray_quit", "終了"},

        // Welcome
        {"welcome_title", "BATorrent へようこそ！"},
        {"welcome_subtitle", "軽量な BitTorrent クライアント"},
        {"welcome_step1_title", "トレントを追加"},
        {"welcome_step1_desc", "ツールバーの「開く」をクリックして .torrent ファイルを追加するか、「マグネット」でマグネットリンクを追加できます。ファイルを直接ウィンドウにドラッグ＆ドロップすることもできます。"},
        {"welcome_step2_title", "ダウンロードの管理"},
        {"welcome_step2_desc", "一時停止・再開・削除ボタンでダウンロードを管理できます。トレントを選択すると、下部パネルに詳細、ピア、ファイルが表示されます。"},
        {"welcome_step3_title", "設定とトレイ"},
        {"welcome_step3_desc", "設定 > 環境設定から速度制限や既定の保存先を設定できます。ウィンドウを閉じるとシステムトレイに最小化されます。トレイアイコンを右クリックして終了できます。"},
        {"welcome_got_it", "了解！"},
        {"welcome_dont_show", "今後表示しない"},

        // Filter bar
        {"filter_search", "トレントを検索..."},
        {"filter_all_active", "アクティブ"},
        {"filter_downloading", "ダウンロード中"},
        {"filter_seeding", "シード中"},
        {"filter_paused", "一時停止中"},
        {"filter_finished", "完了"},

        // VPN / Interface binding
        {"settings_vpn_group", "VPN / インターフェース バインド"},
        {"settings_interface", "ネットワークインターフェース："},
        {"settings_iface_any", "すべて（既定）"},
        {"settings_iface_any_desc", "利用可能な任意のインターフェースを使用"},
        {"settings_iface_no_ip", "IPv4 アドレスが見つかりません"},
        {"settings_refresh", "更新"},
        {"settings_kill_switch", "インターフェース切断時にトレントを一時停止（Kill Switch）"},
        {"settings_auto_resume", "インターフェース復帰時に自動再開"},
        {"killswitch_title", "Kill Switch"},
        {"killswitch_triggered", "VPN インターフェースが切断されました — すべてのトレントが一時停止されました。"},
        {"killswitch_restored", "VPN インターフェースが復帰しました — トレントが再開されました。"},

        // Auto-shutdown
        {"settings_auto_shutdown", "すべてのダウンロード完了後に PC をシャットダウン"},
        {"action_auto_shutdown", "完了後に自動シャットダウン"},
        {"shutdown_title", "自動シャットダウン"},
        {"shutdown_msg", "すべてのダウンロードが完了しました。%1 秒後にシャットダウンします..."},

        // Streaming
        {"ctx_stream", "ストリーミング"},
        {"stream_started", "%1 のストリーミングを開始しました"},
        {"stream_no_video", "このトレントに動画ファイルが見つかりません。"},
        {"stream_no_player", "動画プレーヤーが見つかりません。VLC または IINA をインストールしてください。"},

        // Notifications
        {"notif_torrent_added", "トレントを追加しました"},
        {"settings_notif_sound", "通知時にサウンドを再生"},
        {"settings_splash_sound", "起動時にコウモリの音を再生"},
        {"settings_set_default", "BATorrent を既定のトレントアプリに設定"},
        {"settings_default_success", "BATorrent が既定のトレントアプリに設定されました。"},
        {"settings_default_failed", "既定に設定できませんでした。管理者として実行してみてください。"},
        {"dlg_set_default_title", "既定のトレントアプリ"},
        {"dlg_set_default_msg", "BATorrent を .torrent ファイルとマグネットリンクの既定のアプリに設定しますか？"},

        // Global stats
        {"status_global", "合計：%1 ダウン  |  %2 アップ  |  共有比：%3"},

        // Addons
        {"addon_title", "アドオン"},
        {"addon_trackers_group", "自動トラッカーリスト"},
        {"addon_auto_trackers", "新しいトレントに公開トラッカーを自動追加"},
        {"addon_tracker_count", "%1 個のトラッカーを読み込み済み"},
        {"addon_installed", "インストール済みアドオン"},
        {"addon_remove", "削除"},
        {"addon_install", "アドオンをインストール（Stremio 互換）"},
        {"addon_url_hint", "アドオン URL を貼り付け（例：https://addon.example.com）"},
        {"addon_install_btn", "インストール"},
        {"action_addons", "アドオン(&A)..."},
        {"action_search_addons", "アドオンで検索(&S)..."},

        // Search
        {"search_title", "検索"},
        {"search_placeholder", "映画・ドラマを検索..."},
        {"search_btn", "検索"},
        {"search_col_name", "名前"},
        {"search_col_type", "種類"},
        {"search_col_year", "年"},
        {"search_col_quality", "画質 / タイトル"},
        {"search_col_size", "サイズ"},
        {"search_col_addon", "ソース"},
        {"search_back", "戻る"},
        {"search_searching", "検索中..."},
        {"search_done", "%1 件の結果が見つかりました"},
        {"search_loading_streams", "%1 のストリームを読み込み中..."},
        {"search_streams_done", "%1 件のストリームが利用可能"},
        {"search_added", "追加しました：%1"},
        {"search_no_addons", "アドオンがインストールされていません。設定 > アドオンから追加してください。"},
        {"search_no_catalog", "カタログアドオンが有効になっていません。設定 > アドオンで Cinemeta を有効にしてください。"},
        {"search_no_stream", "ストリームアドオンが有効になっていません。設定 > アドオンで Torrentio を有効にしてください。"},
        {"addon_suggested", "おすすめアドオン"},
        {"addon_suggest_hint", "クリックしておすすめアドオンをインストール："},

        // Torrent Search
        {"addon_torrent_search_group", "トレント検索"},
        {"addon_torrent_search_enable", "トレント検索を有効にする"},
        {"addon_torrent_search_url", "API URL："},
        {"addon_torrent_search_url_hint", "互換性のあるトレント検索 API の URL"},
        {"addon_torrent_search_hint", "name、info_hash、size、seeders、leechers フィールドを含む JSON 配列を返すトレント検索 API の URL を入力してください。"},
        {"search_source_stremio", "映画 / ドラマ"},
        {"search_source_torrents", "トレント"},
        {"search_placeholder_torrent", "トレントを検索..."},
        {"search_source_games", "ゲーム"},
        {"search_placeholder_games", "ゲームリパックを検索..."},
        {"search_col_repacker", "リパッカー"},
        {"search_cat_all", "すべて"},
        {"search_cat_audio", "オーディオ"},
        {"search_cat_video", "ビデオ"},
        {"search_cat_apps", "アプリ"},
        {"search_cat_games", "ゲーム"},
        {"search_cat_other", "その他"},
        {"search_col_seeds", "シード"},
        {"search_col_leechers", "リーチャー"},

        // RSS
        {"action_rss", "RSS マネージャー(&R)..."},
        {"rss_title", "RSS 自動ダウンロード"},
        {"rss_url_hint", "RSS フィード URL を貼り付け..."},
        {"rss_add", "フィードを追加"},
        {"rss_remove", "削除"},
        {"rss_refresh_all", "すべて更新"},
        {"rss_feeds", "フィード"},
        {"rss_items", "フィード項目（ダブルクリックでダウンロード）"},
        {"rss_feed_settings", "フィード設定"},
        {"rss_enabled", "有効"},
        {"rss_auto_download", "一致する項目を自動ダウンロード"},
        {"rss_filter", "フィルター（正規表現）："},
        {"rss_filter_hint", "例：1080p|720p または S01E\\d+"},
        {"rss_save_path", "保存先："},
        {"rss_save_path_hint", "空欄で既定のパスを使用"},
        {"rss_interval", "チェック間隔："},
        {"rss_save_settings", "設定を保存"},
        {"rss_last_checked", "最終チェック：%1"},
        {"rss_never_checked", "未チェック"},
        {"rss_col_title", "タイトル"},
        {"rss_col_size", "サイズ"},
        {"rss_col_date", "日付"},
        {"rss_adding", "フィードを追加中..."},
        {"rss_removed", "フィードを削除しました。"},
        {"rss_settings_saved", "フィード設定を保存しました。"},
        {"rss_items_count", "%1 件"},
        {"rss_downloading", "ダウンロードを開始しました。"},
        {"rss_refreshing", "すべてのフィードを更新中..."},
        {"rss_disabled", "無効"},
        {"rss_auto", "自動"},
        {"rss_auto_downloaded", "RSS 自動ダウンロード"},

        // WebUI
        {"settings_webui_enable", "WebUI を有効にする"},
        {"settings_webui_port", "ポート："},
        {"settings_webui_user", "ユーザー名："},
        {"settings_webui_pass", "パスワード："},
        {"settings_webui_pass_hint", "空欄で現在のパスワードを維持"},
        {"settings_webui_remote", "リモートアクセスを許可（0.0.0.0 にバインド）"},
        {"settings_webui_warning_title", "セキュリティ警告"},
        {"settings_webui_warning_msg", "リモートアクセスを有効にすると、WebUI がネットワークに公開されます。安全なリモートアクセスには VPN または HTTPS 対応のリバースプロキシを使用してください。"},

        // Bandwidth Scheduler
        {"settings_scheduler_group", "帯域スケジューラー"},
        {"settings_scheduler_enable", "速度スケジューラーを有効にする"},
        {"settings_alt_down", "代替ダウンロード制限："},
        {"settings_alt_up", "代替アップロード制限："},
        {"settings_sched_from", "開始時刻："},
        {"settings_sched_to", "まで"},
        {"settings_sched_days", "曜日："},

        // Proxy
        {"settings_proxy_group", "プロキシ"},
        {"settings_proxy_type", "プロキシの種類："},
        {"settings_proxy_none", "なし"},
        {"settings_proxy_host", "ホスト："},
        {"settings_proxy_port", "ポート："},
        {"settings_proxy_user", "ユーザー名："},
        {"settings_proxy_pass", "パスワード："},
        {"settings_proxy_user_hint", "任意"},

        // IP Filter
        {"settings_ip_filter_group", "IP フィルタリング"},
        {"settings_ip_filter_file", "ブロックリストファイル："},
        {"settings_ip_filter_hint", "P2P ブロックリスト（.txt、.p2p、.dat）"},

        // Media Server
        {"settings_media_server", "メディアサーバー"},
        {"settings_media_enable_plex", "ダウンロード完了時に Plex に通知"},
        {"settings_media_enable_jellyfin", "ダウンロード完了時に Jellyfin/Emby に通知"},
        {"settings_media_api_key", "API キー："},

        // Speed Test
        {"action_speedtest", "速度テスト(&S)"},
        {"speedtest_title", "速度テスト"},
        {"speedtest_start", "テスト開始"},
        {"speedtest_ping", "Ping"},
        {"speedtest_download", "ダウンロード"},
        {"speedtest_upload", "アップロード"},
        {"speedtest_testing", "テスト中"},
        {"speedtest_complete", "完了！"},
        {"speedtest_result", "結果"},

        // Statistics
        {"action_statistics", "統計(&T)"},
        {"stats_title", "統計"},
        {"stats_alltime", "全期間"},
        {"stats_session", "今回のセッション"},
        {"stats_downloaded", "ダウンロード済み"},
        {"stats_uploaded", "アップロード済み"},
        {"stats_ratio", "共有比"},
        {"stats_torrents_added", "追加されたトレント"},
        {"stats_uptime", "稼働時間"},

        // Shortcuts & Settings Export/Import
        {"action_shortcuts", "キーボードショートカット(&K)"},
        {"shortcuts_title", "キーボードショートカット"},
        {"shortcuts_key", "ショートカット"},
        {"shortcuts_action", "アクション"},
        {"action_export_settings", "設定をエクスポート(&E)..."},
        {"action_import_settings", "設定をインポート(&I)..."},
        {"export_success", "設定のエクスポートに成功しました。"},
        {"import_success", "設定のインポートに成功しました。"},
        {"import_restart", "変更を適用するには BATorrent を再起動してください。"},

        // GeoIP / Peers
        {"peer_country", "国"},

        // Auto-move
        {"settings_automove", "完了したダウンロードを自動移動"},
        {"settings_automove_path", "移動先："},

        // Download queue
        {"settings_max_active", "最大同時ダウンロード数（0 = 無制限）："},
        {"ctx_queue_up", "キューを上に移動"},
        {"ctx_queue_down", "キューを下に移動"},
    };
}

void Translator::loadRussian()
{
    m_strings = {
        // Menu
        {"menu_file", "&Файл"},
        {"menu_torrent", "&Торрент"},
        {"menu_settings", "&Настройки"},
        {"menu_help", "&Справка"},
        {"action_open", "&Открыть торрент..."},
        {"action_magnet", "Открыть &магнет-ссылку..."},
        {"action_quit", "&Выход"},
        {"action_pause", "&Пауза"},
        {"action_resume", "&Возобновить"},
        {"action_remove", "&Удалить"},
        {"action_remove_files", "Удалить с &файлами..."},
        {"action_settings", "&Настройки..."},
        {"action_language", "&Язык"},
        {"action_about", "&О BATorrent"},
        {"action_release_notes", "&Примечания к выпуску"},
        {"release_notes_title", "Примечания к выпуску"},
        {"release_notes_subtitle", "Что нового в этой версии:"},
        {"release_notes_close", "Закрыть"},
        {"action_welcome", "&Приветственное руководство"},
        {"action_create", "&Создать торрент..."},
        {"action_pause_all", "Приостановить &все"},
        {"action_resume_all", "Возобновить в&се"},
        {"action_import_qbt", "&Импорт из qBittorrent..."},
        {"action_check_update", "Проверить &обновления..."},

        // Update
        {"update_title", "Обновление"},
        {"update_available", "Доступна версия BATorrent %1. Скачать и установить сейчас?"},
        {"update_downloading", "Загрузка обновления..."},
        {"update_none", "Вы используете последнюю версию."},

        // Import
        {"import_qbt_success", "Импортировано торрентов из qBittorrent: %1."},
        {"import_qbt_none", "Новые торренты в данных qBittorrent не найдены."},

        // Context menu
        {"ctx_sequential", "Последовательная загрузка"},
        {"ctx_open_folder", "Открыть папку"},
        {"ctx_category", "Категория"},
        {"category_none", "Нет"},
        {"filter_all_categories", "Все категории"},

        // Create torrent
        {"create_title", "Создать торрент"},
        {"create_source", "Исходный файл/папка:"},
        {"create_output", "Выходной .torrent:"},
        {"create_trackers", "Трекеры (по одному на строку):"},
        {"create_comment", "Комментарий:"},
        {"create_piece_size", "Размер части:"},
        {"create_auto", "Авто"},
        {"create_btn", "Создать торрент"},
        {"create_select_source", "Выбрать источник"},
        {"create_success", "Торрент успешно создан!"},
        {"create_err_empty", "Укажите пути источника и назначения."},
        {"create_err_no_files", "В выбранном источнике файлы не найдены."},

        // Toolbar
        {"tb_open", "Открыть"},
        {"tb_magnet", "Магнет"},
        {"tb_pause", "Пауза"},
        {"tb_resume", "Продолжить"},
        {"tb_remove", "Удалить"},
        {"tb_settings", "Настройки"},

        // Table headers
        {"col_name", "Имя"},
        {"col_size", "Размер"},
        {"col_progress", "Прогресс"},
        {"col_down", "Загрузка"},
        {"col_up", "Отдача"},
        {"col_state", "Состояние"},
        {"col_category", "Категория"},
        {"col_peers", "Пиры"},

        // Details
        {"detail_general", "Общее"},
        {"detail_peers", "Пиры"},
        {"detail_files", "Файлы"},
        {"detail_trackers", "Трекеры"},
        {"detail_pieces", "Части"},
        {"detail_name", "Имя:"},
        {"detail_save_path", "Путь сохранения:"},
        {"detail_size", "Размер:"},
        {"detail_downloaded", "Загружено:"},
        {"detail_progress", "Прогресс:"},
        {"detail_down_speed", "Скорость загрузки:"},
        {"detail_up_speed", "Скорость отдачи:"},
        {"detail_state", "Состояние:"},
        {"detail_peers_count", "Пиры:"},
        {"detail_ratio", "Рейтинг:"},

        // Peer table
        {"peer_ip", "IP"},
        {"peer_port", "Порт"},
        {"peer_client", "Клиент"},
        {"peer_down", "Загрузка"},
        {"peer_up", "Отдача"},
        {"peer_progress", "Прогресс"},

        // File table
        {"file_name", "Файл"},
        {"file_size", "Размер"},
        {"file_progress", "Прогресс"},
        {"file_priority", "Приоритет"},

        // File priorities
        {"priority_skip", "Пропустить"},
        {"priority_low", "Низкий"},
        {"priority_normal", "Обычный"},
        {"priority_high", "Высокий"},

        // Tracker table
        {"tracker_url_col", "URL"},
        {"tracker_tier", "Уровень"},
        {"tracker_status", "Статус"},
        {"tracker_add", "Добавить трекер"},
        {"tracker_url", "URL трекера:"},

        // Status
        {"status_no_torrents", "Нет торрентов"},
        {"status_format", "%1 торрент(ов)  |  Загрузка: %2 КБ/с  |  Отдача: %3 КБ/с"},

        // States
        {"state_checking", "Проверка"},
        {"state_metadata", "Метаданные"},
        {"state_downloading", "Загрузка"},
        {"state_finished", "Завершено"},
        {"state_seeding", "Раздача"},
        {"state_paused", "На паузе"},
        {"state_unknown", "Неизвестно"},

        // Dialogs
        {"dlg_open_torrent", "Открыть торрент"},
        {"dlg_save_to", "Сохранить в"},
        {"dlg_choose_folder", "Выберите папку для сохранения"},
        {"dlg_add_magnet", "Добавить магнет-ссылку"},
        {"dlg_paste_magnet", "Вставьте магнет-ссылку:"},
        {"dlg_torrent_filter", "Торрент-файлы (*.torrent)"},
        {"dlg_download_complete", "Загрузка завершена"},
        {"dlg_finished_msg", "Загрузка %1 завершена."},
        {"dlg_error", "Ошибка"},
        {"dlg_confirm_delete", "Подтверждение удаления"},
        {"dlg_confirm_delete_msg", "Удалить выбранные торренты и загруженные файлы?"},

        // About
        {"about_description", "Лёгкий BitTorrent-клиент с открытым исходным кодом."},
        {"about_libraries", "Библиотеки"},
        {"about_license", "Лицензия:"},

        // Settings
        {"settings_title", "Настройки"},
        {"settings_general", "Общие"},
        {"settings_speed", "Ограничения скорости"},
        {"settings_network", "Сеть"},
        {"settings_default_save", "Папка сохранения по умолчанию:"},
        {"settings_browse", "Обзор..."},
        {"settings_language", "Язык:"},
        {"settings_max_down", "Макс. загрузка (0 = без ограничений):"},
        {"settings_max_up", "Макс. отдача (0 = без ограничений):"},
        {"settings_start_tray", "Запуск свёрнутым в трей"},
        {"settings_close_to_tray", "Сворачивать в трей при закрытии"},
        {"settings_use_default_path", "Всегда использовать папку по умолчанию (не спрашивать)"},
        {"settings_theme", "Тема:"},
        {"settings_unlimited", "Без ограничений"},
        {"settings_seed_ratio", "Прекратить раздачу при рейтинге (0 = без ограничений):"},
        {"settings_max_conn", "Макс. соединений:"},
        {"settings_enable_dht", "Включить DHT (поиск пиров без трекера)"},
        {"settings_encryption", "Шифрование протокола:"},
        {"settings_enc_enabled", "Включено (предпочтительно)"},
        {"settings_enc_forced", "Принудительно (только шифрование)"},
        {"settings_enc_disabled", "Отключено"},

        // Buttons
        {"btn_ok", "OK"},
        {"btn_cancel", "Отмена"},

        // Tray
        {"tray_show", "Показать"},
        {"tray_quit", "Выход"},

        // Welcome
        {"welcome_title", "Добро пожаловать в BATorrent!"},
        {"welcome_subtitle", "Ваш лёгкий BitTorrent-клиент"},
        {"welcome_step1_title", "Добавить торрент"},
        {"welcome_step1_desc", "Нажмите «Открыть» на панели инструментов, чтобы добавить .torrent-файл, или «Магнет» для магнет-ссылки. Вы также можете перетащить файлы прямо в окно."},
        {"welcome_step2_title", "Управление загрузками"},
        {"welcome_step2_desc", "Используйте кнопки «Пауза», «Продолжить» и «Удалить» для управления загрузками. Выберите торрент, чтобы увидеть подробности, пиры и файлы в нижней панели."},
        {"welcome_step3_title", "Настройки и трей"},
        {"welcome_step3_desc", "Перейдите в Настройки > Настройки для установки ограничений скорости и папки сохранения. Закрытие окна сворачивает в системный трей — щёлкните правой кнопкой мыши по значку в трее для выхода."},
        {"welcome_got_it", "Понятно!"},
        {"welcome_dont_show", "Больше не показывать"},

        // Filter bar
        {"filter_search", "Поиск торрентов..."},
        {"filter_all_active", "Активные"},
        {"filter_downloading", "Загрузка"},
        {"filter_seeding", "Раздача"},
        {"filter_paused", "На паузе"},
        {"filter_finished", "Завершённые"},

        // VPN / Interface binding
        {"settings_vpn_group", "VPN / Привязка интерфейса"},
        {"settings_interface", "Сетевой интерфейс:"},
        {"settings_iface_any", "Любой (по умолчанию)"},
        {"settings_iface_any_desc", "Трафик будет использовать любой доступный интерфейс"},
        {"settings_iface_no_ip", "IPv4-адрес не найден"},
        {"settings_refresh", "Обновить"},
        {"settings_kill_switch", "Приостановить торренты при разрыве интерфейса (Kill Switch)"},
        {"settings_auto_resume", "Автовозобновление при восстановлении интерфейса"},
        {"killswitch_title", "Kill Switch"},
        {"killswitch_triggered", "VPN-интерфейс отключён — все торренты приостановлены."},
        {"killswitch_restored", "VPN-интерфейс восстановлен — торренты возобновлены."},

        // Auto-shutdown
        {"settings_auto_shutdown", "Выключить ПК после завершения всех загрузок"},
        {"action_auto_shutdown", "Автовыключение по завершении"},
        {"shutdown_title", "Автовыключение"},
        {"shutdown_msg", "Все загрузки завершены. Выключение через %1 сек..."},

        // Streaming
        {"ctx_stream", "Стриминг"},
        {"stream_started", "Стриминг запущен для %1"},
        {"stream_no_video", "В этом торренте не найден видеофайл."},
        {"stream_no_player", "Видеоплеер не найден. Установите VLC или IINA для стриминга."},

        // Notifications
        {"notif_torrent_added", "Торрент добавлен"},
        {"settings_notif_sound", "Звук при уведомлениях"},
        {"settings_splash_sound", "Звук летучей мыши при запуске"},
        {"settings_set_default", "Назначить BATorrent приложением по умолчанию"},
        {"settings_default_success", "BATorrent назначен приложением для торрентов по умолчанию."},
        {"settings_default_failed", "Не удалось назначить по умолчанию. Попробуйте запустить от имени администратора."},
        {"dlg_set_default_title", "Приложение для торрентов по умолчанию"},
        {"dlg_set_default_msg", "Назначить BATorrent приложением по умолчанию для .torrent-файлов и магнет-ссылок?"},

        // Global stats
        {"status_global", "Всего: %1 загружено  |  %2 отдано  |  Рейтинг: %3"},

        // Addons
        {"addon_title", "Дополнения"},
        {"addon_trackers_group", "Автоматический список трекеров"},
        {"addon_auto_trackers", "Автоматически добавлять публичные трекеры к новым торрентам"},
        {"addon_tracker_count", "Загружено трекеров: %1"},
        {"addon_installed", "Установленные дополнения"},
        {"addon_remove", "Удалить"},
        {"addon_install", "Установить дополнение (совместимо со Stremio)"},
        {"addon_url_hint", "Вставьте URL дополнения (напр. https://addon.example.com)"},
        {"addon_install_btn", "Установить"},
        {"action_addons", "&Дополнения..."},
        {"action_search_addons", "&Поиск в дополнениях..."},

        // Search
        {"search_title", "Поиск"},
        {"search_placeholder", "Поиск фильмов, сериалов..."},
        {"search_btn", "Искать"},
        {"search_col_name", "Название"},
        {"search_col_type", "Тип"},
        {"search_col_year", "Год"},
        {"search_col_quality", "Качество / Название"},
        {"search_col_size", "Размер"},
        {"search_col_addon", "Источник"},
        {"search_back", "Назад"},
        {"search_searching", "Поиск..."},
        {"search_done", "Найдено результатов: %1"},
        {"search_loading_streams", "Загрузка потоков для %1..."},
        {"search_streams_done", "Доступно потоков: %1"},
        {"search_added", "Добавлено: %1"},
        {"search_no_addons", "Дополнения не установлены. Перейдите в Настройки > Дополнения."},
        {"search_no_catalog", "Каталожное дополнение не включено. Включите Cinemeta в Настройки > Дополнения."},
        {"search_no_stream", "Потоковое дополнение не включено. Включите Torrentio в Настройки > Дополнения."},
        {"addon_suggested", "Рекомендуемые дополнения"},
        {"addon_suggest_hint", "Нажмите для установки рекомендуемых дополнений:"},

        // Torrent Search
        {"addon_torrent_search_group", "Поиск торрентов"},
        {"addon_torrent_search_enable", "Включить поиск торрентов"},
        {"addon_torrent_search_url", "URL API:"},
        {"addon_torrent_search_url_hint", "URL совместимого API поиска торрентов"},
        {"addon_torrent_search_hint", "Укажите URL API поиска торрентов, возвращающего JSON-массив с полями name, info_hash, size, seeders, leechers."},
        {"search_source_stremio", "Фильмы / Сериалы"},
        {"search_source_torrents", "Торренты"},
        {"search_placeholder_torrent", "Поиск торрентов..."},
        {"search_source_games", "Игры"},
        {"search_placeholder_games", "Поиск репаков игр..."},
        {"search_col_repacker", "Репакер"},
        {"search_cat_all", "Все"},
        {"search_cat_audio", "Аудио"},
        {"search_cat_video", "Видео"},
        {"search_cat_apps", "Приложения"},
        {"search_cat_games", "Игры"},
        {"search_cat_other", "Прочее"},
        {"search_col_seeds", "Сиды"},
        {"search_col_leechers", "Личеры"},

        // RSS
        {"action_rss", "&Менеджер RSS..."},
        {"rss_title", "RSS автозагрузка"},
        {"rss_url_hint", "Вставьте URL RSS-ленты..."},
        {"rss_add", "Добавить ленту"},
        {"rss_remove", "Удалить"},
        {"rss_refresh_all", "Обновить все"},
        {"rss_feeds", "Ленты"},
        {"rss_items", "Элементы ленты (дважды щёлкните для загрузки)"},
        {"rss_feed_settings", "Настройки ленты"},
        {"rss_enabled", "Включено"},
        {"rss_auto_download", "Автоматически загружать совпадения"},
        {"rss_filter", "Фильтр (regex):"},
        {"rss_filter_hint", "напр. 1080p|720p или S01E\\d+"},
        {"rss_save_path", "Сохранить в:"},
        {"rss_save_path_hint", "Оставьте пустым для пути по умолчанию"},
        {"rss_interval", "Проверять каждые:"},
        {"rss_save_settings", "Сохранить настройки"},
        {"rss_last_checked", "Последняя проверка: %1"},
        {"rss_never_checked", "Не проверялось"},
        {"rss_col_title", "Заголовок"},
        {"rss_col_size", "Размер"},
        {"rss_col_date", "Дата"},
        {"rss_adding", "Добавление ленты..."},
        {"rss_removed", "Лента удалена."},
        {"rss_settings_saved", "Настройки ленты сохранены."},
        {"rss_items_count", "Элементов: %1"},
        {"rss_downloading", "Загрузка начата."},
        {"rss_refreshing", "Обновление всех лент..."},
        {"rss_disabled", "отключено"},
        {"rss_auto", "авто"},
        {"rss_auto_downloaded", "RSS автозагрузка"},

        // WebUI
        {"settings_webui_enable", "Включить WebUI"},
        {"settings_webui_port", "Порт:"},
        {"settings_webui_user", "Имя пользователя:"},
        {"settings_webui_pass", "Пароль:"},
        {"settings_webui_pass_hint", "Оставьте пустым для сохранения текущего"},
        {"settings_webui_remote", "Разрешить удалённый доступ (bind 0.0.0.0)"},
        {"settings_webui_warning_title", "Предупреждение безопасности"},
        {"settings_webui_warning_msg", "Включение удалённого доступа открывает WebUI для вашей сети. Используйте VPN или обратный прокси с HTTPS для безопасного доступа."},

        // Bandwidth Scheduler
        {"settings_scheduler_group", "Планировщик скорости"},
        {"settings_scheduler_enable", "Включить планировщик скорости"},
        {"settings_alt_down", "Альт. лимит загрузки:"},
        {"settings_alt_up", "Альт. лимит отдачи:"},
        {"settings_sched_from", "Активен с:"},
        {"settings_sched_to", "до"},
        {"settings_sched_days", "Дни:"},

        // Proxy
        {"settings_proxy_group", "Прокси"},
        {"settings_proxy_type", "Тип прокси:"},
        {"settings_proxy_none", "Нет"},
        {"settings_proxy_host", "Хост:"},
        {"settings_proxy_port", "Порт:"},
        {"settings_proxy_user", "Имя пользователя:"},
        {"settings_proxy_pass", "Пароль:"},
        {"settings_proxy_user_hint", "Необязательно"},

        // IP Filter
        {"settings_ip_filter_group", "Фильтрация IP"},
        {"settings_ip_filter_file", "Файл блокировки:"},
        {"settings_ip_filter_hint", "Список блокировки P2P (.txt, .p2p, .dat)"},

        // Media Server
        {"settings_media_server", "Медиасервер"},
        {"settings_media_enable_plex", "Уведомить Plex по завершении загрузки"},
        {"settings_media_enable_jellyfin", "Уведомить Jellyfin/Emby по завершении загрузки"},
        {"settings_media_api_key", "API-ключ:"},
        // Speed Test
        {"action_speedtest", "&Тест скорости"},
        {"speedtest_title", "Тест скорости"},
        {"speedtest_start", "Начать тест"},
        {"speedtest_ping", "Пинг"},
        {"speedtest_download", "Загрузка"},
        {"speedtest_upload", "Отдача"},
        {"speedtest_testing", "Тестирование"},
        {"speedtest_complete", "Завершено!"},
        {"speedtest_result", "Результат"},

        // Statistics
        {"action_statistics", "С&татистика"},
        {"stats_title", "Статистика"},
        {"stats_alltime", "За всё время"},
        {"stats_session", "Текущая сессия"},
        {"stats_downloaded", "Загружено"},
        {"stats_uploaded", "Отдано"},
        {"stats_ratio", "Рейтинг"},
        {"stats_torrents_added", "Добавлено торрентов"},
        {"stats_uptime", "Время работы"},

        // Shortcuts & Settings Export/Import
        {"action_shortcuts", "&Горячие клавиши"},
        {"shortcuts_title", "Горячие клавиши"},
        {"shortcuts_key", "Клавиша"},
        {"shortcuts_action", "Действие"},
        {"action_export_settings", "&Экспорт настроек..."},
        {"action_import_settings", "&Импорт настроек..."},
        {"export_success", "Настройки успешно экспортированы."},
        {"import_success", "Настройки успешно импортированы."},
        {"import_restart", "Перезапустите BATorrent для применения изменений."},

        // GeoIP / Peers
        {"peer_country", "Страна"},

        // Auto-move
        {"settings_automove", "Автоперемещение завершённых загрузок"},
        {"settings_automove_path", "Переместить в:"},

        // Download queue
        {"settings_max_active", "Макс. активных загрузок (0 = без ограничений):"},
        {"ctx_queue_up", "Вверх в очереди"},
        {"ctx_queue_down", "Вниз в очереди"},
    };
}

void Translator::loadSpanish()
{
    m_strings = {
        // Menu
        {"menu_file", "&Archivo"},
        {"menu_torrent", "&Torrent"},
        {"menu_settings", "&Configuración"},
        {"menu_help", "A&yuda"},
        {"action_open", "&Abrir Torrent..."},
        {"action_magnet", "Abrir enlace &Magnet..."},
        {"action_quit", "&Salir"},
        {"action_pause", "&Pausar"},
        {"action_resume", "&Reanudar"},
        {"action_remove", "&Eliminar"},
        {"action_remove_files", "Eliminar con &archivos..."},
        {"action_settings", "&Preferencias..."},
        {"action_language", "&Idioma"},
        {"action_about", "&Acerca de BATorrent"},
        {"action_release_notes", "&Notas de la versión"},
        {"release_notes_title", "Notas de la versión"},
        {"release_notes_subtitle", "Novedades de esta versión:"},
        {"release_notes_close", "Cerrar"},
        {"action_welcome", "&Guía de bienvenida"},
        {"action_create", "&Crear Torrent..."},
        {"action_pause_all", "Pausar &todos"},
        {"action_resume_all", "Reanudar t&odos"},
        {"action_import_qbt", "&Importar desde qBittorrent..."},
        {"action_check_update", "Buscar &actualizaciones..."},

        // Update
        {"update_title", "Actualización"},
        {"update_available", "BATorrent %1 está disponible. ¿Descargar e instalar ahora?"},
        {"update_downloading", "Descargando actualización..."},
        {"update_none", "Estás usando la versión más reciente."},

        // Import
        {"import_qbt_success", "Se importaron %1 torrent(s) desde qBittorrent."},
        {"import_qbt_none", "No se encontraron torrents nuevos en los datos de qBittorrent."},

        // Context menu
        {"ctx_sequential", "Descarga secuencial"},
        {"ctx_open_folder", "Abrir carpeta"},
        {"ctx_category", "Categoría"},
        {"category_none", "Ninguna"},
        {"filter_all_categories", "Todas las categorías"},

        // Create torrent
        {"create_title", "Crear Torrent"},
        {"create_source", "Archivo/carpeta de origen:"},
        {"create_output", "Archivo .torrent de salida:"},
        {"create_trackers", "Trackers (uno por línea):"},
        {"create_comment", "Comentario:"},
        {"create_piece_size", "Tamaño de pieza:"},
        {"create_auto", "Automático"},
        {"create_btn", "Crear Torrent"},
        {"create_select_source", "Seleccionar origen"},
        {"create_success", "¡Torrent creado exitosamente!"},
        {"create_err_empty", "Completa las rutas de origen y salida."},
        {"create_err_no_files", "No se encontraron archivos en el origen seleccionado."},

        // Toolbar
        {"tb_open", "Abrir"},
        {"tb_magnet", "Magnet"},
        {"tb_pause", "Pausar"},
        {"tb_resume", "Reanudar"},
        {"tb_remove", "Eliminar"},
        {"tb_settings", "Ajustes"},

        // Table headers
        {"col_name", "Nombre"},
        {"col_size", "Tamaño"},
        {"col_progress", "Progreso"},
        {"col_down", "Descarga"},
        {"col_up", "Subida"},
        {"col_state", "Estado"},
        {"col_category", "Categoría"},
        {"col_peers", "Pares"},

        // Details
        {"detail_general", "General"},
        {"detail_peers", "Pares"},
        {"detail_files", "Archivos"},
        {"detail_trackers", "Trackers"},
        {"detail_pieces", "Piezas"},
        {"detail_name", "Nombre:"},
        {"detail_save_path", "Ruta de guardado:"},
        {"detail_size", "Tamaño:"},
        {"detail_downloaded", "Descargado:"},
        {"detail_progress", "Progreso:"},
        {"detail_down_speed", "Vel. descarga:"},
        {"detail_up_speed", "Vel. subida:"},
        {"detail_state", "Estado:"},
        {"detail_peers_count", "Pares:"},
        {"detail_ratio", "Ratio:"},

        // Peer table
        {"peer_ip", "IP"},
        {"peer_port", "Puerto"},
        {"peer_client", "Cliente"},
        {"peer_down", "Descarga"},
        {"peer_up", "Subida"},
        {"peer_progress", "Progreso"},

        // File table
        {"file_name", "Archivo"},
        {"file_size", "Tamaño"},
        {"file_progress", "Progreso"},
        {"file_priority", "Prioridad"},

        // File priorities
        {"priority_skip", "Omitir"},
        {"priority_low", "Baja"},
        {"priority_normal", "Normal"},
        {"priority_high", "Alta"},

        // Tracker table
        {"tracker_url_col", "URL"},
        {"tracker_tier", "Nivel"},
        {"tracker_status", "Estado"},
        {"tracker_add", "Agregar Tracker"},
        {"tracker_url", "URL del Tracker:"},

        // Status
        {"status_no_torrents", "Sin torrents"},
        {"status_format", "%1 torrent(s)  |  Descarga: %2 KB/s  |  Subida: %3 KB/s"},

        // States
        {"state_checking", "Verificando"},
        {"state_metadata", "Metadatos"},
        {"state_downloading", "Descargando"},
        {"state_finished", "Finalizado"},
        {"state_seeding", "Sembrando"},
        {"state_paused", "Pausado"},
        {"state_unknown", "Desconocido"},

        // Dialogs
        {"dlg_open_torrent", "Abrir Torrent"},
        {"dlg_save_to", "Guardar en"},
        {"dlg_choose_folder", "Elegir carpeta de destino"},
        {"dlg_add_magnet", "Agregar enlace Magnet"},
        {"dlg_paste_magnet", "Pega el enlace magnet:"},
        {"dlg_torrent_filter", "Archivos Torrent (*.torrent)"},
        {"dlg_download_complete", "Descarga completa"},
        {"dlg_finished_msg", "%1 terminó de descargarse."},
        {"dlg_error", "Error"},
        {"dlg_confirm_delete", "Confirmar eliminación"},
        {"dlg_confirm_delete_msg", "¿Eliminar los torrents seleccionados y sus archivos descargados?"},

        // About
        {"about_description", "Un cliente BitTorrent ligero y de código abierto."},
        {"about_libraries", "Bibliotecas"},
        {"about_license", "Licencia:"},

        // Settings
        {"settings_title", "Preferencias"},
        {"settings_general", "General"},
        {"settings_speed", "Límites de velocidad"},
        {"settings_network", "Red"},
        {"settings_default_save", "Ruta de guardado predeterminada:"},
        {"settings_browse", "Examinar..."},
        {"settings_language", "Idioma:"},
        {"settings_max_down", "Descarga máx. (0 = ilimitado):"},
        {"settings_max_up", "Subida máx. (0 = ilimitado):"},
        {"settings_start_tray", "Iniciar minimizado en la bandeja"},
        {"settings_close_to_tray", "Cerrar a la bandeja en lugar de salir"},
        {"settings_use_default_path", "Usar siempre la ruta predeterminada (omitir diálogo de carpeta)"},
        {"settings_theme", "Tema:"},
        {"settings_unlimited", "Ilimitado"},
        {"settings_seed_ratio", "Dejar de sembrar en ratio (0 = sin límite):"},
        {"settings_max_conn", "Máx. conexiones:"},
        {"settings_enable_dht", "Habilitar DHT (descubrimiento de pares sin tracker)"},
        {"settings_encryption", "Cifrado de protocolo:"},
        {"settings_enc_enabled", "Habilitado (preferir cifrado)"},
        {"settings_enc_forced", "Forzado (solo cifrado)"},
        {"settings_enc_disabled", "Deshabilitado"},

        // Buttons
        {"btn_ok", "Aceptar"},
        {"btn_cancel", "Cancelar"},

        // Tray
        {"tray_show", "Mostrar"},
        {"tray_quit", "Salir"},

        // Welcome
        {"welcome_title", "¡Bienvenido a BATorrent!"},
        {"welcome_subtitle", "Tu cliente BitTorrent ligero"},
        {"welcome_step1_title", "Agregar un Torrent"},
        {"welcome_step1_desc", "Haz clic en 'Abrir' en la barra de herramientas para agregar un archivo .torrent, o usa 'Magnet' para enlaces magnet. También puedes arrastrar y soltar archivos directamente en la ventana."},
        {"welcome_step2_title", "Administrar descargas"},
        {"welcome_step2_desc", "Usa los botones Pausar, Reanudar y Eliminar para controlar tus descargas. Selecciona un torrent para ver detalles, pares y archivos en el panel inferior."},
        {"welcome_step3_title", "Ajustes y bandeja"},
        {"welcome_step3_desc", "Ve a Configuración > Preferencias para establecer límites de velocidad y ruta de guardado. Cerrar la ventana minimiza a la bandeja del sistema — haz clic derecho en el ícono de la bandeja para salir."},
        {"welcome_got_it", "¡Entendido!"},
        {"welcome_dont_show", "No mostrar de nuevo"},

        // Filter bar
        {"filter_search", "Buscar torrents..."},
        {"filter_all_active", "Activos"},
        {"filter_downloading", "Descargando"},
        {"filter_seeding", "Sembrando"},
        {"filter_paused", "Pausados"},
        {"filter_finished", "Finalizados"},

        // VPN / Interface binding
        {"settings_vpn_group", "VPN / Enlace de interfaz"},
        {"settings_interface", "Interfaz de red:"},
        {"settings_iface_any", "Cualquiera (predeterminado)"},
        {"settings_iface_any_desc", "El tráfico usará cualquier interfaz disponible"},
        {"settings_iface_no_ip", "No se encontró dirección IPv4"},
        {"settings_refresh", "Actualizar"},
        {"settings_kill_switch", "Pausar torrents si la interfaz se desconecta (Kill Switch)"},
        {"settings_auto_resume", "Reanudar automáticamente al restaurarse la interfaz"},
        {"killswitch_title", "Kill Switch"},
        {"killswitch_triggered", "La interfaz VPN se desconectó — todos los torrents pausados."},
        {"killswitch_restored", "La interfaz VPN se restauró — torrents reanudados."},

        // Auto-shutdown
        {"settings_auto_shutdown", "Apagar PC al terminar todas las descargas"},
        {"action_auto_shutdown", "Apagado automático al finalizar"},
        {"shutdown_title", "Apagado automático"},
        {"shutdown_msg", "Todas las descargas completadas. Apagando en %1 segundos..."},

        // Streaming
        {"ctx_stream", "Transmitir"},
        {"stream_started", "Transmisión iniciada para %1"},
        {"stream_no_video", "No se encontró archivo de video en este torrent."},
        {"stream_no_player", "No se encontró reproductor de video. Instala VLC o IINA para transmitir."},

        // Notifications
        {"notif_torrent_added", "Torrent agregado"},
        {"settings_notif_sound", "Reproducir sonido en notificaciones"},
        {"settings_splash_sound", "Reproducir sonido de murciélago al iniciar"},
        {"settings_set_default", "Establecer BATorrent como app predeterminada"},
        {"settings_default_success", "BATorrent es ahora la aplicación predeterminada para torrents."},
        {"settings_default_failed", "No se pudo establecer como predeterminada. Intenta ejecutar como administrador."},
        {"dlg_set_default_title", "Aplicación de torrent predeterminada"},
        {"dlg_set_default_msg", "¿Deseas establecer BATorrent como la aplicación predeterminada para archivos .torrent y enlaces magnet?"},

        // Global stats
        {"status_global", "Total: %1 descargado  |  %2 subido  |  Ratio: %3"},

        // Addons
        {"addon_title", "Complementos"},
        {"addon_trackers_group", "Lista automática de trackers"},
        {"addon_auto_trackers", "Agregar trackers públicos automáticamente a nuevos torrents"},
        {"addon_tracker_count", "%1 trackers cargados"},
        {"addon_installed", "Complementos instalados"},
        {"addon_remove", "Eliminar"},
        {"addon_install", "Instalar complemento (compatible con Stremio)"},
        {"addon_url_hint", "Pega la URL del complemento (ej. https://addon.example.com)"},
        {"addon_install_btn", "Instalar"},
        {"action_addons", "&Complementos..."},
        {"action_search_addons", "&Buscar en complementos..."},

        // Search
        {"search_title", "Buscar"},
        {"search_placeholder", "Buscar películas, series..."},
        {"search_btn", "Buscar"},
        {"search_col_name", "Nombre"},
        {"search_col_type", "Tipo"},
        {"search_col_year", "Año"},
        {"search_col_quality", "Calidad / Título"},
        {"search_col_size", "Tamaño"},
        {"search_col_addon", "Fuente"},
        {"search_back", "Volver"},
        {"search_searching", "Buscando..."},
        {"search_done", "%1 resultado(s) encontrado(s)"},
        {"search_loading_streams", "Cargando streams para %1..."},
        {"search_streams_done", "%1 stream(s) disponible(s)"},
        {"search_added", "Agregado: %1"},
        {"search_no_addons", "No hay complementos instalados. Ve a Configuración > Complementos para agregar uno."},
        {"search_no_catalog", "No hay complemento de catálogo habilitado. Habilita Cinemeta en Configuración > Complementos."},
        {"search_no_stream", "No hay complemento de streams habilitado. Habilita Torrentio en Configuración > Complementos."},
        {"addon_suggested", "Complementos sugeridos"},
        {"addon_suggest_hint", "Haz clic para instalar complementos recomendados:"},

        // Torrent Search
        {"addon_torrent_search_group", "Búsqueda de torrents"},
        {"addon_torrent_search_enable", "Habilitar búsqueda de torrents"},
        {"addon_torrent_search_url", "URL de la API:"},
        {"addon_torrent_search_url_hint", "URL de una API de búsqueda de torrents compatible"},
        {"addon_torrent_search_hint", "Ingresa la URL de una API de búsqueda de torrents que devuelva arrays JSON con los campos name, info_hash, size, seeders, leechers."},
        {"search_source_stremio", "Películas / Series"},
        {"search_source_torrents", "Torrents"},
        {"search_placeholder_torrent", "Buscar torrents..."},
        {"search_source_games", "Juegos"},
        {"search_placeholder_games", "Buscar repacks de juegos..."},
        {"search_col_repacker", "Repacker"},
        {"search_cat_all", "Todos"},
        {"search_cat_audio", "Audio"},
        {"search_cat_video", "Video"},
        {"search_cat_apps", "Aplicaciones"},
        {"search_cat_games", "Juegos"},
        {"search_cat_other", "Otros"},
        {"search_col_seeds", "Seeds"},
        {"search_col_leechers", "Leechers"},

        // RSS
        {"action_rss", "&Administrador RSS..."},
        {"rss_title", "RSS auto-descarga"},
        {"rss_url_hint", "Pega la URL del feed RSS..."},
        {"rss_add", "Agregar feed"},
        {"rss_remove", "Eliminar"},
        {"rss_refresh_all", "Actualizar todos"},
        {"rss_feeds", "Feeds"},
        {"rss_items", "Elementos del feed (doble clic para descargar)"},
        {"rss_feed_settings", "Configuración del feed"},
        {"rss_enabled", "Habilitado"},
        {"rss_auto_download", "Descargar automáticamente elementos coincidentes"},
        {"rss_filter", "Filtro (regex):"},
        {"rss_filter_hint", "ej. 1080p|720p o S01E\\d+"},
        {"rss_save_path", "Guardar en:"},
        {"rss_save_path_hint", "Dejar vacío para ruta predeterminada"},
        {"rss_interval", "Verificar cada:"},
        {"rss_save_settings", "Guardar configuración"},
        {"rss_last_checked", "Última verificación: %1"},
        {"rss_never_checked", "Nunca verificado"},
        {"rss_col_title", "Título"},
        {"rss_col_size", "Tamaño"},
        {"rss_col_date", "Fecha"},
        {"rss_adding", "Agregando feed..."},
        {"rss_removed", "Feed eliminado."},
        {"rss_settings_saved", "Configuración del feed guardada."},
        {"rss_items_count", "%1 elemento(s)"},
        {"rss_downloading", "Descarga iniciada."},
        {"rss_refreshing", "Actualizando todos los feeds..."},
        {"rss_disabled", "deshabilitado"},
        {"rss_auto", "auto"},
        {"rss_auto_downloaded", "RSS auto-descarga"},

        // WebUI
        {"settings_webui_enable", "Habilitar WebUI"},
        {"settings_webui_port", "Puerto:"},
        {"settings_webui_user", "Usuario:"},
        {"settings_webui_pass", "Contraseña:"},
        {"settings_webui_pass_hint", "Dejar vacío para mantener la actual"},
        {"settings_webui_remote", "Permitir acceso remoto (bind 0.0.0.0)"},
        {"settings_webui_warning_title", "Advertencia de seguridad"},
        {"settings_webui_warning_msg", "Habilitar el acceso remoto expone la WebUI a tu red. Usa una VPN o proxy inverso con HTTPS para acceso remoto seguro."},

        // Bandwidth Scheduler
        {"settings_scheduler_group", "Programador de ancho de banda"},
        {"settings_scheduler_enable", "Habilitar programador de velocidad"},
        {"settings_alt_down", "Límite alt. descarga:"},
        {"settings_alt_up", "Límite alt. subida:"},
        {"settings_sched_from", "Activo desde:"},
        {"settings_sched_to", "hasta"},
        {"settings_sched_days", "Días:"},

        // Proxy
        {"settings_proxy_group", "Proxy"},
        {"settings_proxy_type", "Tipo de proxy:"},
        {"settings_proxy_none", "Ninguno"},
        {"settings_proxy_host", "Host:"},
        {"settings_proxy_port", "Puerto:"},
        {"settings_proxy_user", "Usuario:"},
        {"settings_proxy_pass", "Contraseña:"},
        {"settings_proxy_user_hint", "Opcional"},

        // IP Filter
        {"settings_ip_filter_group", "Filtrado de IP"},
        {"settings_ip_filter_file", "Archivo de lista de bloqueo:"},
        {"settings_ip_filter_hint", "Lista de bloqueo P2P (.txt, .p2p, .dat)"},

        // Media Server
        {"settings_media_server", "Servidor de medios"},
        {"settings_media_enable_plex", "Notificar a Plex al completar descarga"},
        {"settings_media_enable_jellyfin", "Notificar a Jellyfin/Emby al completar descarga"},
        {"settings_media_api_key", "Clave API:"},
        // Speed Test
        {"action_speedtest", "&Test de Velocidad"},
        {"speedtest_title", "Test de Velocidad"},
        {"speedtest_start", "Iniciar Test"},
        {"speedtest_ping", "Ping"},
        {"speedtest_download", "Descarga"},
        {"speedtest_upload", "Subida"},
        {"speedtest_testing", "Probando"},
        {"speedtest_complete", "¡Completado!"},
        {"speedtest_result", "Resultado"},

        // Statistics
        {"action_statistics", "E&stadísticas"},
        {"stats_title", "Estadísticas"},
        {"stats_alltime", "Todo el Tiempo"},
        {"stats_session", "Esta Sesión"},
        {"stats_downloaded", "Descargado"},
        {"stats_uploaded", "Subido"},
        {"stats_ratio", "Proporción"},
        {"stats_torrents_added", "Torrents Añadidos"},
        {"stats_uptime", "Tiempo Activo"},

        // Shortcuts & Settings Export/Import
        {"action_shortcuts", "&Atajos de Teclado"},
        {"shortcuts_title", "Atajos de Teclado"},
        {"shortcuts_key", "Atajo"},
        {"shortcuts_action", "Acción"},
        {"action_export_settings", "&Exportar Configuración..."},
        {"action_import_settings", "&Importar Configuración..."},
        {"export_success", "Configuración exportada correctamente."},
        {"import_success", "Configuración importada correctamente."},
        {"import_restart", "Reinicie BATorrent para aplicar los cambios."},

        // GeoIP / Peers
        {"peer_country", "País"},

        // Auto-move
        {"settings_automove", "Mover descargas completadas automáticamente"},
        {"settings_automove_path", "Mover a:"},

        // Download queue
        {"settings_max_active", "Descargas activas máximas (0 = ilimitado):"},
        {"ctx_queue_up", "Subir en la Cola"},
        {"ctx_queue_down", "Bajar en la Cola"},
    };
}

void Translator::loadGerman()
{
    m_strings = {
        // Menu
        {"menu_file", "&Datei"},
        {"menu_torrent", "&Torrent"},
        {"menu_settings", "&Einstellungen"},
        {"menu_help", "&Hilfe"},
        {"action_open", "Torrent &öffnen..."},
        {"action_magnet", "&Magnet-Link öffnen..."},
        {"action_quit", "&Beenden"},
        {"action_pause", "&Pausieren"},
        {"action_resume", "&Fortsetzen"},
        {"action_remove", "&Entfernen"},
        {"action_remove_files", "Mit &Dateien entfernen..."},
        {"action_settings", "&Einstellungen..."},
        {"action_language", "&Sprache"},
        {"action_about", "&Über BATorrent"},
        {"action_release_notes", "&Versionshinweise"},
        {"release_notes_title", "Versionshinweise"},
        {"release_notes_subtitle", "Neuerungen in dieser Version:"},
        {"release_notes_close", "Schließen"},
        {"action_welcome", "&Willkommensanleitung"},
        {"action_create", "Torrent &erstellen..."},
        {"action_pause_all", "&Alle pausieren"},
        {"action_resume_all", "A&lle fortsetzen"},
        {"action_import_qbt", "Aus qBittorrent &importieren..."},
        {"action_check_update", "Auf &Updates prüfen..."},

        // Update
        {"update_title", "Update"},
        {"update_available", "BATorrent %1 ist verfügbar. Jetzt herunterladen und installieren?"},
        {"update_downloading", "Update wird heruntergeladen..."},
        {"update_none", "Sie verwenden bereits die neueste Version."},

        // Import
        {"import_qbt_success", "%1 Torrent(s) aus qBittorrent importiert."},
        {"import_qbt_none", "Keine neuen Torrents in den qBittorrent-Daten gefunden."},

        // Context menu
        {"ctx_sequential", "Sequenzieller Download"},
        {"ctx_open_folder", "Ordner öffnen"},
        {"ctx_category", "Kategorie"},
        {"category_none", "Keine"},
        {"filter_all_categories", "Alle Kategorien"},

        // Create torrent
        {"create_title", "Torrent erstellen"},
        {"create_source", "Quelldatei/-ordner:"},
        {"create_output", "Ausgabe-.torrent:"},
        {"create_trackers", "Tracker (einer pro Zeile):"},
        {"create_comment", "Kommentar:"},
        {"create_piece_size", "Stückgröße:"},
        {"create_auto", "Automatisch"},
        {"create_btn", "Torrent erstellen"},
        {"create_select_source", "Quelle auswählen"},
        {"create_success", "Torrent erfolgreich erstellt!"},
        {"create_err_empty", "Bitte Quell- und Ausgabepfade ausfüllen."},
        {"create_err_no_files", "Keine Dateien in der ausgewählten Quelle gefunden."},

        // Toolbar
        {"tb_open", "Öffnen"},
        {"tb_magnet", "Magnet"},
        {"tb_pause", "Pause"},
        {"tb_resume", "Fortsetzen"},
        {"tb_remove", "Entfernen"},
        {"tb_settings", "Einstellungen"},

        // Table headers
        {"col_name", "Name"},
        {"col_size", "Größe"},
        {"col_progress", "Fortschritt"},
        {"col_down", "Download"},
        {"col_up", "Upload"},
        {"col_state", "Status"},
        {"col_category", "Kategorie"},
        {"col_peers", "Peers"},

        // Details
        {"detail_general", "Allgemein"},
        {"detail_peers", "Peers"},
        {"detail_files", "Dateien"},
        {"detail_trackers", "Tracker"},
        {"detail_pieces", "Stücke"},
        {"detail_name", "Name:"},
        {"detail_save_path", "Speicherpfad:"},
        {"detail_size", "Größe:"},
        {"detail_downloaded", "Heruntergeladen:"},
        {"detail_progress", "Fortschritt:"},
        {"detail_down_speed", "Download-Geschw.:"},
        {"detail_up_speed", "Upload-Geschw.:"},
        {"detail_state", "Status:"},
        {"detail_peers_count", "Peers:"},
        {"detail_ratio", "Verhältnis:"},

        // Peer table
        {"peer_ip", "IP"},
        {"peer_port", "Port"},
        {"peer_client", "Client"},
        {"peer_down", "Download"},
        {"peer_up", "Upload"},
        {"peer_progress", "Fortschritt"},

        // File table
        {"file_name", "Datei"},
        {"file_size", "Größe"},
        {"file_progress", "Fortschritt"},
        {"file_priority", "Priorität"},

        // File priorities
        {"priority_skip", "Überspringen"},
        {"priority_low", "Niedrig"},
        {"priority_normal", "Normal"},
        {"priority_high", "Hoch"},

        // Tracker table
        {"tracker_url_col", "URL"},
        {"tracker_tier", "Ebene"},
        {"tracker_status", "Status"},
        {"tracker_add", "Tracker hinzufügen"},
        {"tracker_url", "Tracker-URL:"},

        // Status
        {"status_no_torrents", "Keine Torrents"},
        {"status_format", "%1 Torrent(s)  |  Download: %2 KB/s  |  Upload: %3 KB/s"},

        // States
        {"state_checking", "Prüfung"},
        {"state_metadata", "Metadaten"},
        {"state_downloading", "Herunterladen"},
        {"state_finished", "Abgeschlossen"},
        {"state_seeding", "Seeden"},
        {"state_paused", "Pausiert"},
        {"state_unknown", "Unbekannt"},

        // Dialogs
        {"dlg_open_torrent", "Torrent öffnen"},
        {"dlg_save_to", "Speichern unter"},
        {"dlg_choose_folder", "Speicherordner wählen"},
        {"dlg_add_magnet", "Magnet-Link hinzufügen"},
        {"dlg_paste_magnet", "Magnet-Link einfügen:"},
        {"dlg_torrent_filter", "Torrent-Dateien (*.torrent)"},
        {"dlg_download_complete", "Download abgeschlossen"},
        {"dlg_finished_msg", "%1 wurde fertig heruntergeladen."},
        {"dlg_error", "Fehler"},
        {"dlg_confirm_delete", "Löschen bestätigen"},
        {"dlg_confirm_delete_msg", "Ausgewählte Torrents und heruntergeladene Dateien löschen?"},

        // About
        {"about_description", "Ein leichtgewichtiger, quelloffener BitTorrent-Client."},
        {"about_libraries", "Bibliotheken"},
        {"about_license", "Lizenz:"},

        // Settings
        {"settings_title", "Einstellungen"},
        {"settings_general", "Allgemein"},
        {"settings_speed", "Geschwindigkeitslimits"},
        {"settings_network", "Netzwerk"},
        {"settings_default_save", "Standard-Speicherpfad:"},
        {"settings_browse", "Durchsuchen..."},
        {"settings_language", "Sprache:"},
        {"settings_max_down", "Max. Download (0 = unbegrenzt):"},
        {"settings_max_up", "Max. Upload (0 = unbegrenzt):"},
        {"settings_start_tray", "Minimiert im Tray starten"},
        {"settings_close_to_tray", "Beim Schließen in den Tray minimieren"},
        {"settings_use_default_path", "Immer Standard-Speicherpfad verwenden (Ordnerdialog überspringen)"},
        {"settings_theme", "Design:"},
        {"settings_unlimited", "Unbegrenzt"},
        {"settings_seed_ratio", "Seeden bei Verhältnis stoppen (0 = kein Limit):"},
        {"settings_max_conn", "Max. Verbindungen:"},
        {"settings_enable_dht", "DHT aktivieren (Peer-Erkennung ohne Tracker)"},
        {"settings_encryption", "Protokollverschlüsselung:"},
        {"settings_enc_enabled", "Aktiviert (verschlüsselt bevorzugt)"},
        {"settings_enc_forced", "Erzwungen (nur verschlüsselt)"},
        {"settings_enc_disabled", "Deaktiviert"},

        // Buttons
        {"btn_ok", "OK"},
        {"btn_cancel", "Abbrechen"},

        // Tray
        {"tray_show", "Anzeigen"},
        {"tray_quit", "Beenden"},

        // Welcome
        {"welcome_title", "Willkommen bei BATorrent!"},
        {"welcome_subtitle", "Ihr leichtgewichtiger BitTorrent-Client"},
        {"welcome_step1_title", "Torrent hinzufügen"},
        {"welcome_step1_desc", "Klicken Sie auf 'Öffnen' in der Symbolleiste, um eine .torrent-Datei hinzuzufügen, oder verwenden Sie 'Magnet' für Magnet-Links. Sie können Dateien auch direkt in das Fenster ziehen."},
        {"welcome_step2_title", "Downloads verwalten"},
        {"welcome_step2_desc", "Verwenden Sie die Schaltflächen Pause, Fortsetzen und Entfernen zur Steuerung Ihrer Downloads. Wählen Sie einen Torrent aus, um Details, Peers und Dateien im unteren Bereich zu sehen."},
        {"welcome_step3_title", "Einstellungen & Tray"},
        {"welcome_step3_desc", "Gehen Sie zu Einstellungen > Einstellungen, um Geschwindigkeitslimits und Standard-Speicherpfad festzulegen. Das Schließen des Fensters minimiert in den System-Tray — klicken Sie mit der rechten Maustaste auf das Tray-Symbol zum Beenden."},
        {"welcome_got_it", "Verstanden!"},
        {"welcome_dont_show", "Nicht mehr anzeigen"},

        // Filter bar
        {"filter_search", "Torrents suchen..."},
        {"filter_all_active", "Aktiv"},
        {"filter_downloading", "Herunterladen"},
        {"filter_seeding", "Seeden"},
        {"filter_paused", "Pausiert"},
        {"filter_finished", "Abgeschlossen"},

        // VPN / Interface binding
        {"settings_vpn_group", "VPN / Schnittstellenbindung"},
        {"settings_interface", "Netzwerkschnittstelle:"},
        {"settings_iface_any", "Beliebig (Standard)"},
        {"settings_iface_any_desc", "Datenverkehr nutzt jede verfügbare Schnittstelle"},
        {"settings_iface_no_ip", "Keine IPv4-Adresse gefunden"},
        {"settings_refresh", "Aktualisieren"},
        {"settings_kill_switch", "Torrents bei Schnittstellenausfall pausieren (Kill Switch)"},
        {"settings_auto_resume", "Automatisch fortsetzen bei Schnittstellenrückkehr"},
        {"killswitch_title", "Kill Switch"},
        {"killswitch_triggered", "VPN-Schnittstelle getrennt — alle Torrents pausiert."},
        {"killswitch_restored", "VPN-Schnittstelle wiederhergestellt — Torrents fortgesetzt."},

        // Auto-shutdown
        {"settings_auto_shutdown", "PC herunterfahren wenn alle Downloads fertig sind"},
        {"action_auto_shutdown", "Automatisches Herunterfahren nach Abschluss"},
        {"shutdown_title", "Automatisches Herunterfahren"},
        {"shutdown_msg", "Alle Downloads abgeschlossen. Herunterfahren in %1 Sekunden..."},

        // Streaming
        {"ctx_stream", "Streamen"},
        {"stream_started", "Streaming gestartet für %1"},
        {"stream_no_video", "Keine Videodatei in diesem Torrent gefunden."},
        {"stream_no_player", "Kein Videoplayer gefunden. Installieren Sie VLC oder IINA zum Streamen."},

        // Notifications
        {"notif_torrent_added", "Torrent hinzugefügt"},
        {"settings_notif_sound", "Ton bei Benachrichtigungen abspielen"},
        {"settings_splash_sound", "Fledermauston beim Start abspielen"},
        {"settings_set_default", "BATorrent als Standard-Torrent-App festlegen"},
        {"settings_default_success", "BATorrent ist jetzt die Standard-Torrent-Anwendung."},
        {"settings_default_failed", "Konnte nicht als Standard festgelegt werden. Versuchen Sie es als Administrator."},
        {"dlg_set_default_title", "Standard-Torrent-Anwendung"},
        {"dlg_set_default_msg", "Möchten Sie BATorrent als Standardanwendung für .torrent-Dateien und Magnet-Links festlegen?"},

        // Global stats
        {"status_global", "Gesamt: %1 heruntergeladen  |  %2 hochgeladen  |  Verhältnis: %3"},

        // Addons
        {"addon_title", "Erweiterungen"},
        {"addon_trackers_group", "Automatische Tracker-Liste"},
        {"addon_auto_trackers", "Öffentliche Tracker automatisch zu neuen Torrents hinzufügen"},
        {"addon_tracker_count", "%1 Tracker geladen"},
        {"addon_installed", "Installierte Erweiterungen"},
        {"addon_remove", "Entfernen"},
        {"addon_install", "Erweiterung installieren (Stremio-kompatibel)"},
        {"addon_url_hint", "Erweiterungs-URL einfügen (z.B. https://addon.example.com)"},
        {"addon_install_btn", "Installieren"},
        {"action_addons", "&Erweiterungen..."},
        {"action_search_addons", "In Erweiterungen &suchen..."},

        // Search
        {"search_title", "Suche"},
        {"search_placeholder", "Filme, Serien suchen..."},
        {"search_btn", "Suchen"},
        {"search_col_name", "Name"},
        {"search_col_type", "Typ"},
        {"search_col_year", "Jahr"},
        {"search_col_quality", "Qualität / Titel"},
        {"search_col_size", "Größe"},
        {"search_col_addon", "Quelle"},
        {"search_back", "Zurück"},
        {"search_searching", "Suche läuft..."},
        {"search_done", "%1 Ergebnis(se) gefunden"},
        {"search_loading_streams", "Streams für %1 werden geladen..."},
        {"search_streams_done", "%1 Stream(s) verfügbar"},
        {"search_added", "Hinzugefügt: %1"},
        {"search_no_addons", "Keine Erweiterungen installiert. Gehen Sie zu Einstellungen > Erweiterungen."},
        {"search_no_catalog", "Keine Katalog-Erweiterung aktiviert. Aktivieren Sie Cinemeta unter Einstellungen > Erweiterungen."},
        {"search_no_stream", "Keine Stream-Erweiterung aktiviert. Aktivieren Sie Torrentio unter Einstellungen > Erweiterungen."},
        {"addon_suggested", "Vorgeschlagene Erweiterungen"},
        {"addon_suggest_hint", "Klicken Sie, um empfohlene Erweiterungen zu installieren:"},

        // Torrent Search
        {"addon_torrent_search_group", "Torrent-Suche"},
        {"addon_torrent_search_enable", "Torrent-Suche aktivieren"},
        {"addon_torrent_search_url", "API-URL:"},
        {"addon_torrent_search_url_hint", "URL einer kompatiblen Torrent-Such-API"},
        {"addon_torrent_search_hint", "Geben Sie die URL einer Torrent-Such-API ein, die JSON-Arrays mit den Feldern name, info_hash, size, seeders, leechers zurückgibt."},
        {"search_source_stremio", "Filme / Serien"},
        {"search_source_torrents", "Torrents"},
        {"search_placeholder_torrent", "Torrents suchen..."},
        {"search_source_games", "Spiele"},
        {"search_placeholder_games", "Spiele-Repacks suchen..."},
        {"search_col_repacker", "Repacker"},
        {"search_cat_all", "Alle"},
        {"search_cat_audio", "Audio"},
        {"search_cat_video", "Video"},
        {"search_cat_apps", "Anwendungen"},
        {"search_cat_games", "Spiele"},
        {"search_cat_other", "Sonstige"},
        {"search_col_seeds", "Seeds"},
        {"search_col_leechers", "Leechers"},

        // RSS
        {"action_rss", "&RSS-Manager..."},
        {"rss_title", "RSS-Auto-Download"},
        {"rss_url_hint", "RSS-Feed-URL einfügen..."},
        {"rss_add", "Feed hinzufügen"},
        {"rss_remove", "Entfernen"},
        {"rss_refresh_all", "Alle aktualisieren"},
        {"rss_feeds", "Feeds"},
        {"rss_items", "Feed-Einträge (Doppelklick zum Herunterladen)"},
        {"rss_feed_settings", "Feed-Einstellungen"},
        {"rss_enabled", "Aktiviert"},
        {"rss_auto_download", "Passende Einträge automatisch herunterladen"},
        {"rss_filter", "Filter (Regex):"},
        {"rss_filter_hint", "z.B. 1080p|720p oder S01E\\d+"},
        {"rss_save_path", "Speichern unter:"},
        {"rss_save_path_hint", "Leer lassen für Standardpfad"},
        {"rss_interval", "Prüfen alle:"},
        {"rss_save_settings", "Einstellungen speichern"},
        {"rss_last_checked", "Letzte Prüfung: %1"},
        {"rss_never_checked", "Nie geprüft"},
        {"rss_col_title", "Titel"},
        {"rss_col_size", "Größe"},
        {"rss_col_date", "Datum"},
        {"rss_adding", "Feed wird hinzugefügt..."},
        {"rss_removed", "Feed entfernt."},
        {"rss_settings_saved", "Feed-Einstellungen gespeichert."},
        {"rss_items_count", "%1 Eintrag/Einträge"},
        {"rss_downloading", "Download gestartet."},
        {"rss_refreshing", "Alle Feeds werden aktualisiert..."},
        {"rss_disabled", "deaktiviert"},
        {"rss_auto", "auto"},
        {"rss_auto_downloaded", "RSS-Auto-Download"},

        // WebUI
        {"settings_webui_enable", "WebUI aktivieren"},
        {"settings_webui_port", "Port:"},
        {"settings_webui_user", "Benutzername:"},
        {"settings_webui_pass", "Passwort:"},
        {"settings_webui_pass_hint", "Leer lassen um aktuelles beizubehalten"},
        {"settings_webui_remote", "Fernzugriff erlauben (bind 0.0.0.0)"},
        {"settings_webui_warning_title", "Sicherheitswarnung"},
        {"settings_webui_warning_msg", "Fernzugriff aktivieren macht die WebUI in Ihrem Netzwerk zugänglich. Verwenden Sie ein VPN oder einen Reverse-Proxy mit HTTPS für sicheren Fernzugriff."},

        // Bandwidth Scheduler
        {"settings_scheduler_group", "Bandbreitenplaner"},
        {"settings_scheduler_enable", "Geschwindigkeitsplaner aktivieren"},
        {"settings_alt_down", "Alt. Download-Limit:"},
        {"settings_alt_up", "Alt. Upload-Limit:"},
        {"settings_sched_from", "Aktiv von:"},
        {"settings_sched_to", "bis"},
        {"settings_sched_days", "Tage:"},

        // Proxy
        {"settings_proxy_group", "Proxy"},
        {"settings_proxy_type", "Proxy-Typ:"},
        {"settings_proxy_none", "Keiner"},
        {"settings_proxy_host", "Host:"},
        {"settings_proxy_port", "Port:"},
        {"settings_proxy_user", "Benutzername:"},
        {"settings_proxy_pass", "Passwort:"},
        {"settings_proxy_user_hint", "Optional"},

        // IP Filter
        {"settings_ip_filter_group", "IP-Filterung"},
        {"settings_ip_filter_file", "Sperrlisten-Datei:"},
        {"settings_ip_filter_hint", "P2P-Sperrliste (.txt, .p2p, .dat)"},

        // Media Server
        {"settings_media_server", "Medienserver"},
        {"settings_media_enable_plex", "Plex bei Download-Abschluss benachrichtigen"},
        {"settings_media_enable_jellyfin", "Jellyfin/Emby bei Download-Abschluss benachrichtigen"},
        {"settings_media_api_key", "API-Schlüssel:"},
        // Speed Test
        {"action_speedtest", "&Geschwindigkeitstest"},
        {"speedtest_title", "Geschwindigkeitstest"},
        {"speedtest_start", "Test starten"},
        {"speedtest_ping", "Ping"},
        {"speedtest_download", "Download"},
        {"speedtest_upload", "Upload"},
        {"speedtest_testing", "Teste"},
        {"speedtest_complete", "Abgeschlossen!"},
        {"speedtest_result", "Ergebnis"},

        // Statistics
        {"action_statistics", "S&tatistiken"},
        {"stats_title", "Statistiken"},
        {"stats_alltime", "Gesamtzeitraum"},
        {"stats_session", "Diese Sitzung"},
        {"stats_downloaded", "Heruntergeladen"},
        {"stats_uploaded", "Hochgeladen"},
        {"stats_ratio", "Verhältnis"},
        {"stats_torrents_added", "Hinzugefügte Torrents"},
        {"stats_uptime", "Laufzeit"},

        // Shortcuts & Settings Export/Import
        {"action_shortcuts", "&Tastaturkürzel"},
        {"shortcuts_title", "Tastaturkürzel"},
        {"shortcuts_key", "Kürzel"},
        {"shortcuts_action", "Aktion"},
        {"action_export_settings", "Einstellungen &exportieren..."},
        {"action_import_settings", "Einstellungen &importieren..."},
        {"export_success", "Einstellungen erfolgreich exportiert."},
        {"import_success", "Einstellungen erfolgreich importiert."},
        {"import_restart", "Bitte starten Sie BATorrent neu, damit die Änderungen wirksam werden."},

        // GeoIP / Peers
        {"peer_country", "Land"},

        // Auto-move
        {"settings_automove", "Abgeschlossene Downloads automatisch verschieben"},
        {"settings_automove_path", "Verschieben nach:"},

        // Download queue
        {"settings_max_active", "Max. aktive Downloads (0 = unbegrenzt):"},
        {"ctx_queue_up", "In Warteschlange nach oben"},
        {"ctx_queue_down", "In Warteschlange nach unten"},
    };
}
