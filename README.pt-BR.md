🌐 [English](README.md) | **[Português](README.pt-BR.md)** | [中文](README.zh-CN.md) | [日本語](README.ja.md) | [Русский](README.ru.md) | [Español](README.es.md) | [Deutsch](README.de.md)

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="160">
</p>

<h1 align="center">BATorrent</h1>

</p>

<p align="center">
  <a href="https://github.com/Mateuscruz19/BAT-Torrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/Mateuscruz19/BAT-Torrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/Mateuscruz19/BAT-Torrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/Mateuscruz19/BAT-Torrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/Mateuscruz19/BAT-Torrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-dc2626?style=flat-square">
  <img alt="C++" src="https://img.shields.io/badge/C%2B%2B-17-dc2626?style=flat-square&logo=c%2B%2B">
  <img alt="Qt" src="https://img.shields.io/badge/Qt-6-dc2626?style=flat-square&logo=qt">
</p>

<p align="center"><em>Um cliente BitTorrent moderno e multiplataforma focado em privacidade, performance e clareza</em></p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Visao geral

BATorrent e um cliente BitTorrent para desktop que prioriza clareza, performance e privacidade. Combina o robusto motor libtorrent-rasterbar com uma interface Qt 6 cuidadosamente ajustada, WebUI para controle remoto, download automatico via RSS, busca compativel com Stremio, isolamento de trafego com suporte a VPN e integracao nativa com servidores de midia.

> **Sem telemetria, sem analytics, sem comunicacao oculta.** A unica requisicao externa que o app faz sem sua acao e a verificacao de atualizacoes no GitHub, que pode ser desativada nas Configuracoes. Confira voce mesmo em [`src/app/updater.cpp`](src/app/updater.cpp).

![Janela principal — Tema escuro](src/images/image1.png)

![Janela principal — Tema claro](src/images/image2.png)

![Painel de detalhes](src/images/image3.png)

![Dialogo de configuracoes](src/images/image4.png)

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Download

Binarios pre-compilados para a versao mais recente:

| Plataforma | Formato | Observacoes |
|---|---|---|
| Windows | [Instalador (`.exe`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) · [Portatil (`.zip`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | Windows 10+ (x86_64) |
| macOS | [Imagem de disco (`.dmg`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | macOS 12+ (Apple Silicon) |
| Linux | [AppImage](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | Glibc 2.35+ (x86_64) |

Todos os artefatos sao gerados pelo workflow [Build & Release](.github/workflows/build.yml) em cada release com tag.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Funcionalidades

### Torrents
- Arquivos `.torrent` e magnet links com dados de retomada persistentes
- **Decodificacao de links Thunder://** — foruns chineses compartilham links no formato `thunder://` da Xunlei; o BATorrent decodifica automaticamente via Smart Paste (Ctrl+V)
- **Smart Paste** — Ctrl+V com um magnet link, info hash ou link thunder:// na area de transferencia abre o dialogo de adicao instantaneamente
- **Inspetor de torrent** — visualize um arquivo `.torrent` (nome, tamanho, arquivos, trackers, hash, criador, flag privado) antes de confirmar o download
- Prioridade por arquivo, download sequencial, recheck e reannounce manuais
- Injecao automatica de trackers do [ngosang/trackerslist](https://github.com/ngosang/trackerslist)
- Sistema de multi-tags (texto livre, multiplas tags por torrent junto com categoria unica)
- **Layout de conteudo** — Original, Criar subpasta ou Sem subpasta controla como torrents multi-arquivo sao organizados no disco
- **Padroes de exclusao de arquivos** — regras regex para pular arquivos automaticamente (ex: `.nfo`, `.txt`, `sample`) ao adicionar um torrent
- **Caminho temporario de download** — baixa para uma pasta intermediaria primeiro, move automaticamente para o destino ao concluir (evita escaneamento de parciais por servidores de midia)
- Categorias, reordenacao por arrastar-e-soltar e acoes de contexto com clique direito
- Importacao de estado existente do qBittorrent
- Criacao de novos arquivos `.torrent` a partir de qualquer arquivo ou pasta

### Gerenciamento de estado
- **Estado Concluido** — marcado manualmente ou promovido automaticamente apos um periodo configuravel de seed (1, 3, 7, 14 ou 30 dias). Distinto de Seeding/Finished, persistido entre reinicializacoes.
- **Botao Parar** que congela um torrent finalizado sem remove-lo; **Retomar** desmarca e reinsere no pool de seed.
- Regras de parada de seed: limite global de ratio e tempo maximo de seed, com sobrescrita por torrent.
- **Pausa automatica em erros de arquivo** — se o libtorrent nao conseguir ler os arquivos de um torrent finalizado (ex.: drive externo desconectado), ele pausa em vez de re-baixar silenciosamente.

### Descoberta
- **Download automatico via RSS** com filtros regex, caminhos de salvamento por feed, agendamento de intervalos e deteccao de duplicatas. Suporta magnet links, URLs `.torrent` e tags `<enclosure>`.
- **Busca compativel com Stremio** para filmes e series atraves dos addons Cinemeta e Torrentio incluidos.

### Streaming
- Assista enquanto baixa — `.mp4`, `.mkv`, `.avi`, `.mov`, `.wmv`, `.flv`, `.webm`, `.m4v`, `.ts`.
- Detecta automaticamente VLC e IINA, com fallback para o player padrao do sistema.

### VPN e privacidade
- **Vinculacao de interface** restringe todo o trafego de torrent a uma interface de rede escolhida (ex.: `tun0`, `utun4`).
- **Kill switch** pausa todos os torrents ativos no momento em que a interface vinculada cai, com opcao de retomada automatica quando ela retorna.
- **Modo PT** — conformidade com trackers privados em um unico toggle: desativa DHT/PEX/LSD, forca handshake anonimo, anuncia em todos os tiers. Exigido pelo M-Team, TorrentLeech e a maioria das comunidades com controle de ratio.
- **Bloqueio anti-leecher** — detecta e bane automaticamente Xunlei (Thunder), QQDownload, Baidu Netdisk P2P e outros clientes que baixam sem semear. Detectado pelo prefixo peer_id no handshake BitTorrent.
- **Modo anonimo** — oculta nome/versao do cliente nos handshakes, desativa anuncios UPnP/NAT-PMP.
- **Preset de proxy Tor** — um clique preenche SOCKS5 127.0.0.1:9050.
- **Forcar IPv4** — rejeita peers IPv6 para prevenir vazamentos v6 quando o tunel nao cobre IPv6.
- Deteccao de VPN para WireGuard, Mullvad, NordLynx, ProtonVPN, tun/tap generico.
- Proxy SOCKS5 e HTTP com autenticacao.
- Suporte a blocklist de IPs (formato P2P).
- Criptografia de protocolo: habilitada / forcada / desabilitada.

### WebUI
- Painel de controle pelo navegador em `http://localhost:8080` (porta e acesso remoto configuraveis).
- **Pareamento por QR code** — escaneie um QR com seu celular para abrir a WebUI instantaneamente, sem digitar IP. O QR e gerado localmente em C++ puro (seu endereco LAN nunca sai da maquina).
- API REST com respostas JSON; adicione por magnet ou upload de `.torrent`; pause / retome / remova; visualizacao de arquivos e peers por torrent.
- Basic Auth com hash SHA-256, UI escura seguindo o tema, layout mobile totalmente responsivo.

### Largura de banda e agendamento
- Limites independentes de download e upload.
- Perfil de velocidade alternativa com agenda por hora do dia + dia da semana (suporte a faixas noturnas).
- Limites de ratio e tempo de seed por torrent e globais.

### Integracao com servidores de midia
- Notifica **Plex**, **Jellyfin** ou **Emby** para escanear a biblioteca quando um download e concluido.
- URL e token / chave de API configuraveis por servidor.

### Notificacoes e integracoes
- **Webhook Telegram** — download concluido, kill switch ativado, download automatico via RSS e erros de torrent enviados para qualquer chat do Telegram via bot token. Checkboxes por evento + botao de teste.
- **Discord Rich Presence** — mostra "Downloading X · 67%" no seu perfil do Discord com botoes "Download BATorrent" e "View on GitHub". Funciona direto, sem configuracao.

### Interface
- Tres temas — Dark, Light (paleta creme quente "Comfortable") e Midnight — com override global de QPalette para que widgets simples sigam o tema ativo.
- Grafico de velocidade em tempo real, painel de detalhes (Geral · Peers · Arquivos · Trackers · Pecas), barras de progresso coloridas por estado, notificacoes na bandeja com foco ao clicar.
- Popup customizado na bandeja (multiplataforma) com velocidades ao vivo, preview de torrents ativos com ETA, status da VPN e opcao de sair.
- Pills de filtro com contagem ao vivo (Todos / Ativos / Baixando / Semeando / Concluidos / Pausados / Finalizados / Na fila), barra de busca e filtro por categoria.
- Arrastar e soltar para arquivos `.torrent` e magnet links.
- **Sete idiomas na interface** com deteccao automatica: English, Portugues (BR), Espanol, Deutsch, Russky, Nihongo, Zhongwen — 630+ strings traduzidas com fallback para ingles em chaves ausentes.
- Exibicao de velocidade em bytes (KB/s, MB/s) ou bits (Kbps, Mbps) — alternavel nas Configuracoes.
- Formatacao de numeros sensivel ao locale (ex.: `1 234,5` para o locale russo).

### Sistema
- Atualizacao automatica com fonte configuravel: **GitHub** (padrao), **Gitee** (mirror China) ou desativada.
- Desligamento automatico quando todos os downloads sao concluidos (contagem regressiva de 60 s cancelavel).
- **Backup/restauracao completos** de todas as configuracoes + dados de retomada em um unico arquivo para migracao entre maquinas.
- **Historico de removidos recentemente** (ultimos 50 torrents, restauracao com um clique).
- **Inicio forcado** — ignora o limite de downloads ativos na fila para um unico torrent.
- **Visualizador de logs** integrado com rotacao de arquivos (5 MB/arquivo, manter 3), filtro por nivel, exportacao para relatorios de bug e flag `--debug` na CLI.
- **Dialogo de diagnostico** — verificacao de saude (caminho de salvamento, porta, DHT, criptografia, VPN, bloqueio de leechers) + teste de vazamento de IP de saida (via api.ipify.org).
- Argumentos CLI: passe quantos caminhos `.torrent` ou URIs `magnet:` quiser na inicializacao; execucoes subsequentes encaminham para a instancia em execucao.
- Popup de changelog automatico na primeira execucao apos uma atualizacao de versao.
- Atalhos de teclado e dialogo de referencia rapida com `?`.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Primeiros passos

1. Baixe o build para sua plataforma na [pagina de releases](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest).
2. Na primeira execucao, o dialogo de boas-vindas guia voce pelo caminho de salvamento padrao, tema e idioma.
3. Arraste um arquivo `.torrent` ou magnet link para a janela — ou use **Arquivo → Abrir Torrent** / **Arquivo → Adicionar Magnet**.
4. Opcional: vincule a interface de saida em **Configuracoes → VPN** e ative o kill switch antes de adicionar torrents sensiveis.

> **Dica de VPN:** quando a **Vinculacao de interface** esta configurada, todo announce e conexao de peer sai exclusivamente por aquela interface. Se a interface cair, o kill switch pausa tudo ate ela voltar.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Compilar a partir do codigo-fonte

### Requisitos
- Toolchain C++17 (GCC 11+, Clang 14+ ou MSVC 19.30+)
- CMake 3.16+
- Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`)
- libtorrent-rasterbar 2.0+
- Boost (dependencia transitiva do libtorrent)
- Opcional: Qt6Keychain (armazena credenciais no keyring do SO em vez de QSettings em texto puro)

### Linux

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake \
    qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev

# Arch
sudo pacman -S cmake qt6-base qt6-svg qt6-multimedia \
    libtorrent-rasterbar boost openssl

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/BATorrent
```

### macOS

```bash
brew install qt libtorrent-rasterbar boost openssl
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="$(brew --prefix qt);$(brew --prefix libtorrent-rasterbar);$(brew --prefix openssl)"
cmake --build build -j
open build/BATorrent.app
```

### Windows

Instale o Qt 6 pelo instalador oficial e o libtorrent via vcpkg:

```powershell
vcpkg install libtorrent:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

### Testes

A suite de testes e opt-in:

```bash
cmake -B build -DBAT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Estrutura do projeto

```
src/
├── torrent/      wrapper do libtorrent, dados de retomada, fila, regras de seed
├── app/          tradutor, atualizador, RSS, addons, armazenamento de segredos, GeoIP
├── gui/          UI com Qt Widgets (janela principal, dialogos, detalhes, popup da bandeja)
├── webui/        servidor HTTP embutido + UI do navegador
└── main.cpp      bootstrap de instancia unica, parsing de CLI, temas
.github/
└── workflows/    Linux AppImage, macOS DMG, Windows instalador + zip
installer/        script Inno Setup para o instalador Windows
dist/             arquivo desktop + assets para empacotamento Linux
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Contribuindo

Issues e pull requests sao bem-vindos. Para mudancas significativas, abra uma issue primeiro para discutir a abordagem. Artefatos pre-compilados podem ser gerados para qualquer branch pelo workflow **Build & Release** (`workflow_dispatch`).

Ao reportar um bug, anexe:
- Plataforma + versao (`Ajuda → Sobre`)
- Passos para reproduzir
- A secao relevante de `~/.local/share/BATorrent/` (Linux), `~/Library/Application Support/BATorrent/` (macOS) ou `%APPDATA%\BATorrent\` (Windows) se envolver dados de retomada ou configuracoes.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Licenca

[MIT](LICENSE) © 2024–2026 Mateus Cruz
