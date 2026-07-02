<p align="center">
  <b>English</b> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>A BitTorrent client that shows your downloads as cover art instead of spreadsheet rows.</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <img src="src/images/with_startup.gif" alt="BATorrent — open and go, covers resolve automatically" width="860">
</p>

BATorrent is a desktop torrent client built on the [libtorrent](https://www.libtorrent.org/) engine. The difference is the front end: it reads each torrent's name, looks up the matching poster (movies and shows from TMDB, games from IGDB), and lays your downloads out as a grid of covers instead of a list of filenames. The engine is the same one qBittorrent and Deluge use, so the covers sit on top of a client that actually holds up.

It's free and open source. No ads, no telemetry, no "Pro" tier, no account. The only network request it makes on its own is the update check against GitHub, and there's a switch to turn that off. If you want to confirm that, the code is [`updater.cpp`](src/services/integrations/updater.cpp).

## Why I built it

I'm one developer in Brazil. I wanted a torrent client that took privacy seriously, ran natively on Windows, macOS and Linux, and didn't look like it was designed in 2009. I couldn't find one I liked, so I wrote it. It's MIT-licensed, which means no telemetry can be slipped in later and nobody can buy the project and bolt ads onto it. The interface ships in nine languages, because "useful" shouldn't mean "English only."

## The interface

<p align="center">
  <img src="src/images/themes.gif" alt="Switching between built-in themes" width="860">
</p>

<p align="center">
  <img src="src/images/shot-grid.jpg" alt="Cover-art grid" width="860">
</p>

<p align="center">
  <img src="src/images/shot-list.jpg" alt="List view" width="860">
</p>

<p align="center">
  <img src="src/images/shot-theme.jpg" alt="Sakura theme" width="860">
</p>

<p align="center">
  <img src="src/images/palette-demo.gif" alt="Command palette (Ctrl/⌘+K): fuzzy-find any torrent or action" width="860">
</p>

- **Cover art.** It resolves posters from the torrent name and shows them in a grid. One click switches to a dense list when you want detail over decoration.
- **Six themes.** Dark, Light, Midnight, Sakura, Dark Star, and a Custom theme where you pick your own background and accent colour. Each supports optional anime accent art.
- **Command palette.** Ctrl/⌘+K opens a fuzzy finder for any torrent or action: pause all, toggle alternate speed, jump to any page, no mouse required.
- **Live status.** A real-time speed graph, state-coloured progress bars, and a tray popup with current speeds and ETA.

## What it does

**Privacy.** Bind to a specific VPN interface with a kill switch that drops all traffic if the tunnel goes down. Private-tracker mode, a Tor preset, anonymous handshake, and anti-leecher client blocking.

**Finding and adding.** Built-in search (including open CIS/RuTor sources that need no login), Smart Paste that recognises a magnet, `.torrent`, `thunder://` link or info hash on Ctrl+V, RSS auto-download with regex filters, and drag-and-drop.

**Remote control.** A browser WebUI with QR pairing: scan the code from your phone instead of typing IP addresses. The QR is generated on your machine and the address never leaves it.

**Watching and organising.** Stream a file while it's still downloading, auto-extract archives on completion, sort with categories and tags, and refresh a Plex, Jellyfin or Emby library when a download finishes.

**Notifications.** Native desktop alerts, Telegram messages, and Discord Rich Presence.

<details>
<summary><b>Full feature list</b></summary>

Per-file priority, sequential download, automatic tracker injection, content-layout control, excluded-file regex, separate temp download path, a completed state with seeding windows, auto-pause on file errors, global and per-torrent ratio and time limits, a bandwidth scheduler by hour and day, import from qBittorrent, `.torrent` creation, a torrent inspector, IP blocklists, protocol encryption, a Gitee update mirror, auto-shutdown when downloads finish, a Windows Defender exclusion helper, full backup and restore, recently-removed history, force start, a built-in log viewer with diagnostics and an IP-leak test, locale-aware formatting, and keyboard shortcuts.

</details>

## Install

| Platform | Download | Requirements |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6), [Installer](https://batorrent.com/win), or [Portable](https://batorrent.com/portable) | Windows 10 or later |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` or the [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Once it's running, drop a `.torrent` file or a magnet link onto the window.

<sub><b>macOS note:</b> the app isn't notarised yet (Apple's developer program is a paid subscription). Homebrew is the smoothest route because <code>brew</code> removes the quarantine flag, so it opens without a Gatekeeper prompt. If you use the <code>.dmg</code> instead, right-click the app and choose <b>Open</b> the first time.</sub>

<details>
<summary><b>Build from source</b></summary>

**Requirements:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`), libtorrent-rasterbar 2.0+, Boost, and optionally Qt6Keychain.

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```

On macOS: `brew install qt libtorrent-rasterbar boost openssl`.
On Windows: the Qt installer plus `vcpkg install libtorrent:x64-windows`.

</details>

<details>
<summary><b>Quality and security</b></summary>

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- A Catch2 test suite (unit, security, memory) runs on every CI build; new backend behaviour ships with a test.
- The build passes clean under AddressSanitizer and UndefinedBehaviorSanitizer.
- Before each release the code is reviewed for memory and thread safety, WebUI authentication, injection, path traversal, input validation, and secret handling. Secrets go in the OS keychain rather than plaintext, and the WebUI only opens to the network after you set a password.

</details>

## Contributing

Issues and pull requests are welcome. For anything non-trivial, open an issue first so we can agree on the approach. Bug reports are most useful with your platform and version (from `Help → About`) and the steps to reproduce. Translations are especially appreciated.

## License and trademark

The **code** is [MIT](LICENSE), © 2024–2026 Mateus Cruz. Fork it, build on it, ship it.

The **name "BATorrent" and the logo** are the project's identity and are not covered by the code license. If you redistribute a fork, please give it its own name so users can tell which build is the official one. The details are in [TRADEMARK.md](TRADEMARK.md). Good-faith forks and contributions are welcome.

Made in Brazil.
