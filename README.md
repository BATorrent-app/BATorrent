<p align="center">
  <b>English</b> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>The BitTorrent client with a face — movie covers, six themes, zero ads.</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

<p align="center">
  <img src="src/images/with_startup.gif" alt="BATorrent — open and go, covers resolve automatically" width="860">
</p>

Most torrent clients look like a tax form. This one shows your downloads as a **wall of movie, show, and game covers** — the same thing you'd recognise on Netflix or Steam — and lets you dress it in six themes (or your own wallpaper). Under the hood it's the battle-tested **libtorrent** engine, so it's not a pretty toy: it's a real client that just happens to have taste.

> **No ads. No telemetry. No "Pro" tier. No account.** The only request it makes on its own is the GitHub update check, and you can turn that off. The source is right here — read [`updater.cpp`](src/app/updater.cpp) and see for yourself.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Why this exists

I'm one developer in Brazil. I wanted a torrent client that took privacy seriously, ran natively on every desktop, and didn't look like it was built in 2009 — and I couldn't find one, so I built it. It's free and **MIT-licensed**: no strings, no telemetry sneaking in later, and it can't be quietly sold to a company that bolts on ads. Eight languages, because "useful" shouldn't mean "English only."

## The look

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

- **Automatic cover art** — it reads the torrent name and pulls the real poster (movies & shows via TMDB, games via IGDB) into a grid view. One click flips to a tight list.
- **Six themes** — Dark, Light, Midnight, Sakura, Dark Star, and a fully **Custom** one (your own background + accent colours), each with optional anime accent art.
- **Command palette** — Ctrl/⌘+K jumps to any torrent or action: pause all, toggle alt speed, open any page, no mouse needed.
- Real-time speed graph, state-coloured progress, a rich tray popup with live speeds and ETA — the details that make it *feel* finished.

## What it actually does

| | |
|---|---|
|  **Privacy first** | VPN interface binding + **kill switch** (drops all traffic if the tunnel dies), PT mode for private trackers, Tor preset, anonymous handshake, anti-leecher blocking |
|  **Find & add** | Built-in search (incl. open CIS/RuTor sources, no login), Smart Paste (magnet / `.torrent` / `thunder://` / hash on Ctrl+V), RSS auto-download with regex filters, drag-and-drop |
|  **Control it anywhere** | Browser WebUI with **QR pairing** — scan from your phone, no typing IPs. The QR is generated locally; your address never leaves the machine |
|  **Watch & organise** | Stream while downloading, auto-extract archives, categories + tags, Plex/Jellyfin/Emby library refresh on completion |
|  **Stay in the loop** | Native desktop notifications, Telegram alerts, Discord Rich Presence ("Downloading X · 67%") |

<details>
<summary><b>…and the long tail</b> (click to expand)</summary>

Per-file priority · sequential download · auto-tracker injection · content-layout control · excluded-file regex · temp download path · completed-state with seeding windows · auto-pause on file errors · global + per-torrent ratio/time limits · bandwidth scheduler (hour + day) · import from qBittorrent · create `.torrent` files · torrent inspector · IP blocklists · protocol encryption · Gitee update mirror · auto-shutdown when done · Windows Defender exclusion · full backup/restore · recently-removed history · force start · built-in log viewer + diagnostics + IP-leak test · locale-aware formatting · keyboard shortcuts.

</details>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Get it

| Platform | | |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Installer](https://batorrent.com/win) · [Portable](https://batorrent.com/portable) | Windows 10+ |
| **macOS** | **`brew install --cask Mateuscruz19/batorrent/batorrent`** · [`.dmg`](https://batorrent.com/mac) | macOS 12+ · Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Then just drop a `.torrent` or magnet onto the window. That's it.

<sub>**macOS:** not notarised yet (Apple's dev program is paid). Homebrew is the smoothest path — `brew` strips the quarantine flag, so it opens with no Gatekeeper prompt. With the `.dmg`, right-click → **Open** the first time.</sub>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

<details>
<summary><b>Build from source & engineering notes</b></summary>

### Requirements
C++17 · CMake 3.16+ · Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`) · libtorrent-rasterbar 2.0+ · Boost · optional Qt6Keychain.

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```
(macOS: `brew install qt libtorrent-rasterbar boost openssl`. Windows: Qt installer + `vcpkg install libtorrent:x64-windows`.)

### Quality & security

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- **Tests** — Catch2 suite (unit, security, memory) on every CI build; new backend behaviour gets a test.
- **Sanitizers** — passes clean under AddressSanitizer + UndefinedBehaviorSanitizer (0 leaks / UAF / UB).
- **Reviewed** before each release for memory/thread safety, WebUI auth, injection, path traversal, input validation, and secret handling. Secrets live in the OS keychain, never plaintext; the WebUI only binds to the network once you set a password.

</details>

## Contributing

Issues and PRs welcome — for anything non-trivial, open an issue first. Bug reports: include your platform + version (`Help → About`) and steps to reproduce. Translations especially appreciated.

## License & trademark

The **code** is [MIT](LICENSE) © 2024–2026 Mateus Cruz — fork it, build on it, ship it.

The **name "BATorrent" and the logo** are the project's identity, not part of the code license — please give redistributed forks their own name so users always know which build is official and malware-free. Details: [TRADEMARK.md](TRADEMARK.md). Good-faith forks and contributions are very welcome.

Made in Brazil 🦇
