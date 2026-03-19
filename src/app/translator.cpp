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
    if (lang == Portuguese)
        loadPortuguese();
    else
        loadEnglish();
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
        {"col_peers", "Peers"},

        // Details
        {"detail_general", "General"},
        {"detail_peers", "Peers"},
        {"detail_files", "Files"},
        {"detail_trackers", "Trackers"},
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
        {"col_peers", "Peers"},

        // Details
        {"detail_general", "Geral"},
        {"detail_peers", "Peers"},
        {"detail_files", "Arquivos"},
        {"detail_trackers", "Trackers"},
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
    };
}
