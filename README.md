<p align="center">
  <b>English</b> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="120">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  A torrent client that shows your downloads as cover art, plays them while they download,<br>
  and keeps your movies, shows and games in one library.
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="#install"><b>Download</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com/assets/trailer.mp4"><b>Watch the trailer</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com"><b>Website</b></a>
</p>

<p align="center">
  <img src="docs/images/grid.jpg" alt="The Downloads tab in grid view, with a series selected and its episodes on the right" width="900">
</p>

BATorrent is a desktop torrent client built on [libtorrent](https://www.libtorrent.org/), the engine qBittorrent and Deluge use. It reads each torrent's name, finds the matching poster (TMDB for movies and shows, IGDB for games) and lays your downloads out as covers instead of filenames. Episodes of the same show are grouped together, video plays in the app before the download finishes, and games can be installed and launched from the same window.

It's free and open source. No ads, no account, no "Pro" tier. The only request it makes on its own is the update check against GitHub, and you can turn that off ([`updater.cpp`](src/services/integrations/updater.cpp)).

## Three tabs

The app has three tabs at the top: **Downloads**, **Find** and **HUB**.

### Downloads

Your torrents as a grid of covers or a dense list, with the details of whatever you select on the side. Series collapse into one row with their episodes inside, so twelve episodes of a show take one line instead of twelve.

<p align="center">
  <img src="docs/images/list.jpg" alt="The Downloads tab in list view, with a series expanded into its episodes" width="900">
</p>

### Find and HUB

**Find** is where you look for something new: a front page of trending movies, shows and games, and a search that goes across your sources and puts the best match first. **HUB** is the library of what you already have: continue watching, continue playing, and everything grouped into games, series and movies.

<table>
  <tr>
    <td width="50%"><img src="docs/images/find.jpg" alt="Find: a featured title and trending movies"></td>
    <td width="50%"><img src="docs/images/hub.jpg" alt="HUB: continue watching, continue playing and the library"></td>
  </tr>
  <tr>
    <td align="center"><sub>Find</sub></td>
    <td align="center"><sub>HUB</sub></td>
  </tr>
</table>

## The player

The built-in player uses FFmpeg, so MKV, AVI and WebM play directly. It downloads the start of the file first, so you can watch while the rest comes in. It remembers where you stopped, skips intros, offers the next episode at the end, and finds subtitles for you (via SubDL) or loads the `.srt` sitting next to the file.

<p align="center">
  <img src="docs/images/player.jpg" alt="The built-in player with subtitles and the next episode lined up" width="900">
</p>

If you have a [Real-Debrid](https://real-debrid.com) or [TorBox](https://torbox.app) account and the torrent is already cached there, it streams from them instead, and nothing is downloaded or seeded on your machine.

## What else it does

- **Games.** Covers from IGDB, install and launch from the HUB, play time and achievements. Your game downloads end up looking like a launcher instead of a folder of setup files.
- **Privacy.** Bind to your VPN's network interface with a kill switch that pauses everything if the tunnel drops, import WireGuard profiles, and check for leaks with the built-in IP test. There's also a private-tracker mode, a Tor preset and blocking of known leecher clients.
- **Adding things.** Paste a magnet, a link or an info-hash anywhere with Ctrl+V. RSS with regex filters, a watch folder, drag and drop, and plain HTTP/HTTPS downloads.
- **Remote control.** A WebUI you open on your phone by scanning a QR code. The code is generated on your machine.
- **Organising.** Categories and tags, auto-extract of archives, ratio and time limits, a bandwidth schedule by hour and day, and an alternate speed (the turtle) one click away.
- **Looks.** Dark, Light, Midnight, Sakura, Dark Star, or your own colours. A command palette on Ctrl/⌘+K. Nine languages.

<details>
<summary><b>The full list</b></summary>

Per-file priority, sequential download, tracker injection, content-layout control, excluded-file regex, a separate temp folder, seeding windows, auto-pause when files go missing, Plex/Jellyfin/Emby library refresh, Telegram and Discord notifications, import from qBittorrent, `.torrent` creation, IP blocklists, protocol encryption, a Gitee update mirror, stable and beta update channels, shutdown when downloads finish, a Windows Defender exclusion helper, backup and restore, recently removed history, force start, a log viewer with diagnostics, and keyboard shortcuts for most things.

</details>

## The engine

BATorrent ships a small patched fork of libtorrent, applied as versioned patches under [`third_party/patches/`](third_party/patches):

- **Faster ramp-up.** On a fast link with high latency, stock libtorrent grows its request pipeline one step at a time. The fork grows it geometrically, so it fills the link in far fewer round-trips. In the project's A/B runs that was about 27% faster on a fast connection, and never slower than stock.
- **Same-country peers.** An offline GeoIP database (db-ip Lite) tags each peer by country, and when there's a choice the fork prefers peers in your own country, which usually means lower latency.

Both are compile-time switches and are off in a stock build.

## Install

| Platform | Download | Needs |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Installer](https://batorrent.com/win) · [Portable](https://batorrent.com/portable) | Windows 10 or later |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` or the [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Once it's open, drop a `.torrent` or a magnet link on the window.

<sub><b>macOS:</b> the app isn't notarised yet (Apple's developer program is a paid subscription). Homebrew is the easiest route because it clears the quarantine flag. With the <code>.dmg</code>, right-click the app and choose <b>Open</b> the first time.</sub>

<details>
<summary><b>Build from source</b></summary>

**Needs:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`), libtorrent-rasterbar 2.0+, Boost, zlib, and optionally Qt6Keychain.

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

- A Catch2 suite (unit, security, memory) and the QML tests run on every CI build.
- The build passes under AddressSanitizer and UndefinedBehaviorSanitizer.
- Secrets live in the OS keychain, and the WebUI only opens to the network after you set a password.

</details>

## Why I made it

I'm one developer in Brazil. I wanted a torrent client that took privacy seriously, ran natively on Windows, macOS and Linux, and didn't look like it was made in 2009. I couldn't find one I liked, so I wrote it. It's MIT-licensed, so if it ever picked up ads or tracking, anyone could fork it and ship it without them.

## Contributing

Issues and pull requests are welcome. For anything bigger than a small fix, open an issue first so we can agree on the approach. Bug reports help most with your platform, the version (from `Help → About`) and the steps to reproduce. Translations are especially welcome. If you use AI tools, read [how I use AI](USING_AI.md) first.

## License and trademark

The **code** is [MIT](LICENSE), © 2024-2026 Mateus Cruz. You're free to fork it and ship your own build.

The **name "BATorrent" and the logo** aren't covered by the code license. If you ship a fork, please give it its own name so people can tell which build is the official one. Details in [TRADEMARK.md](TRADEMARK.md).

<p align="center"><sub>Made in Brazil.</sub></p>
