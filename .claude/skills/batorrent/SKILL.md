---
name: batorrent
description: Guia passo a passo para construir o BATorrent. Use quando o usuario quiser continuar o desenvolvimento do projeto.
argument-hint: [feature]
---

# BATorrent — Desenvolvimento

Voce esta ajudando a construir um cliente BitTorrent em C++/Qt.

## Regras

1. **Escreva o codigo direto.** O usuario quer que voce implemente.
2. **Responda em portugues (BR).**
3. **Seja conciso.** Sem introducoes longas. Direto ao ponto.
4. **Teste sempre.** Apos implementar, rode `cmake --build build` e `./build/BATorrent` pra verificar.

## Estado atual do ambiente

- GCC 15.2.1, Qt 6.10.2, OpenSSL 3.6.1, libtorrent-rasterbar 2.0.11, CMake instalado
- OS: Arch Linux
- CI: GitHub Actions (Windows, Linux, macOS)

## Arquitetura do Projeto

```
src/
├── torrent/       # Engine BitTorrent (wrapper libtorrent)
│   ├── sessionmanager.h/.cpp   # Sessao libtorrent, VPN binding, kill switch
│   └── types.h                 # TorrentInfo, PeerInfo, FileInfo, TrackerInfo
├── app/           # Infraestrutura do app (Qt, NAO GUI)
│   ├── translator.h/.cpp       # i18n (EN + PT-BR)
│   ├── updater.h/.cpp          # Auto-update via GitHub Releases
│   └── utils.h                 # formatSize, formatSpeed
├── gui/           # Desktop GUI (Qt Widgets)
│   ├── mainwindow.h/.cpp       # Janela principal, menus, toolbar, tray
│   ├── settingsdialog.h/.cpp   # Preferencias (geral, speed, network, VPN)
│   ├── detailspanel.h/.cpp     # Tabs: Geral, Peers, Arquivos, Trackers
│   ├── torrentmodel.h/.cpp     # QAbstractTableModel
│   ├── torrentfilter.h/.cpp    # QSortFilterProxyModel
│   ├── progressdelegate.h/.cpp # Barra de progresso custom
│   ├── speedgraph.h/.cpp       # Grafico de velocidade em tempo real
│   ├── batwidget.h/.cpp        # Animacao do morcego (idle state)
│   ├── splashwidget.h/.cpp     # Splash screen
│   ├── welcomedialog.h/.cpp    # Guia de boas-vindas
│   ├── createtorrentdialog.h/.cpp # Criar .torrent
│   └── thememanager.h/.cpp     # 3 temas: Dark, Light, Midnight
├── fonts/ icons/ images/
└── main.cpp
```

**Separacao torrent/ vs app/ vs gui/**: Quando a WebUI vier, ela importa de `torrent/` e `app/` mas NAO de `gui/`.

## Features ja implementadas (v1.6.0)

- .torrent e magnet links
- Resume data, import do qBittorrent, criar .torrent
- Download sequencial, prioridade por arquivo
- Seed ratio limits
- DHT, PEX, UPnP, NAT-PMP, protocol encryption
- VPN interface binding + auto-deteccao + kill switch + auto-resume
- 3 temas (Dark, Light, Midnight)
- Speed graph, filter bar, drag & drop, tray, splash screen
- Auto-update, CLI args, atalhos de teclado
- Bilingue EN + PT-BR

## Proximas features planejadas

### RSS Feed Auto-Download
- Parser RSS/Atom com QNetworkAccessManager
- Regras de filtro por nome/regex
- Polling configuravel (intervalo em minutos)
- UI: nova tab ou dialog pra gerenciar feeds e regras
- Persistencia via QSettings
- Arquivo: `src/app/rssmanager.h/.cpp` + UI em `src/gui/rssdialog.h/.cpp`

### WebUI
- HTTP server embarcado (QTcpServer ou lib leve tipo cpp-httplib)
- REST API JSON: listar torrents, add/remove/pause/resume, settings
- Frontend SPA minimo servido como recurso Qt (HTML/CSS/JS)
- Importa de `torrent/` e `app/`, NAO de `gui/`
- Arquivo: `src/web/webserver.h/.cpp` + `src/web/api.h/.cpp`
- Porta configuravel no SettingsDialog
- Autenticacao basica (user/pass)

## Como usar argumentos

Se o usuario passar `$ARGUMENTS` (ex: `/batorrent rss`), interprete como a feature a implementar.
Se nenhum argumento, pergunte o que o usuario quer fazer ou sugira as proximas features.
