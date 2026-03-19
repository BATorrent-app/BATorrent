<p align="center">
  <img src="src/images/logo1.png" alt="BATorrent Logo" width="200">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  A lightweight, open-source BitTorrent client built with C++, Qt 6, and libtorrent-rasterbar.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-red?style=flat-square&logo=c%2B%2B"/>
  <img src="https://img.shields.io/badge/Qt-6-red?style=flat-square&logo=qt"/>
  <img src="https://img.shields.io/badge/Windows%20|%20Linux%20|%20macOS-red?style=flat-square"/>
  <img src="https://img.shields.io/badge/License-MIT-red?style=flat-square"/>
</p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## About

BATorrent is a cross-platform desktop BitTorrent client focusing on simplicity, privacy, and performance. Built from scratch using modern C++ and the Qt framework.

<img width="1176" height="790" alt="image" src="https://github.com/user-attachments/assets/776da3b7-5fe6-49eb-87cc-b985dcc8cfda" />
<img width="1172" height="782" alt="image" src="https://github.com/user-attachments/assets/bd1708c3-9987-4240-b37a-69c0aaa289e2" />
<img width="1177" height="795" alt="image" src="https://github.com/user-attachments/assets/2a0be1f2-b5b5-45b8-8e17-db67a0a35345" />
<img width="540" height="550" alt="image" src="https://github.com/user-attachments/assets/a572b3cd-ad96-4453-af1b-677e8c498907" />

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Features

### Core
- `.torrent` file and magnet link support
- Resume data — picks up where you left off after restart
- Import torrents from qBittorrent
- Create `.torrent` files from any file or folder
- Sequential download mode
- Per-file priority control (skip, low, normal, high)
- Seed ratio limits with auto-pause
- DHT, PEX, UPnP, NAT-PMP

### VPN & Privacy
- **Interface binding** — lock torrent traffic to a specific network interface (e.g. `tun0`)
- **Auto VPN detection** — identifies VPN interfaces (tun, tap, WireGuard, Mullvad, NordLynx, ProtonVPN)
- **Kill switch** — automatically pauses all torrents if the VPN interface drops
- **Auto-resume** — resumes only the torrents paused by the kill switch when VPN reconnects
- Protocol encryption (enabled / forced / disabled)

### WebUI
- **Remote management** — control torrents from any browser at `http://localhost:8080`
- REST API with JSON responses
- Add torrents via magnet link or `.torrent` upload
- Pause, resume, remove torrents remotely
- View peers and files per torrent
- Dark theme matching the desktop app
- HTTP Basic Auth with SHA-256 password hashing
- Configurable port and remote access (localhost vs 0.0.0.0)

### Interface
- 3 themes: Dark, Light, Midnight (bat/vampire aesthetic)
- Real-time speed graph
- Detailed panel with tabs: General, Peers, Files, Trackers
- Filter bar: search by name, filter by state (Active, Downloading, Seeding, Paused, Finished)
- Drag & drop `.torrent` files and magnet links
- Drag & drop reorder in torrent list
- System tray with notifications (download complete, kill switch events)
- Splash screen with bat animation
- Bilingual: English and Portuguese (BR), auto-detected from system locale

### System
- Cross-platform: Windows, Linux, macOS
- **Auto-shutdown** — automatically shut down PC when all downloads complete (60s cancellable countdown)
- Auto-update system (AppImage on Linux, installer on Windows, DMG on macOS)
- CLI arguments: pass `.torrent` files or `magnet:` URIs directly
- Keyboard shortcuts: Space to toggle pause, Ctrl+A to select all, Ctrl+O to open

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Tech Stack

| | Technology | Role |
|---|---|---|
| ⚙️ | **C++17** | Core language |
| 🖼️ | **Qt 6** | GUI framework + networking |
| 🔗 | **libtorrent-rasterbar** | BitTorrent protocol engine |
| 🔨 | **CMake** | Build system |

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Architecture

```
src/
├── torrent/       # BitTorrent engine (libtorrent wrapper)
│   ├── sessionmanager.h/.cpp
│   └── types.h
├── app/           # App infrastructure (non-GUI)
│   ├── translator.h/.cpp
│   ├── updater.h/.cpp
│   └── utils.h
├── gui/           # Desktop GUI (Qt Widgets)
│   ├── mainwindow, settingsdialog, detailspanel
│   ├── torrentmodel, torrentfilter, progressdelegate
│   ├── speedgraph, batwidget, splashwidget
│   ├── welcomedialog, createtorrentdialog
│   └── thememanager
├── webui/         # Browser-based remote UI
│   ├── webserver.h/.cpp
│   ├── index.html
│   └── webuiresources.qrc
├── fonts/ icons/ images/
└── main.cpp
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Building

### Dependencies

- C++17 compiler (GCC, Clang, MSVC)
- Qt 6 (Widgets + Network modules)
- libtorrent-rasterbar
- CMake 3.16+

**Ubuntu/Debian:**
```bash
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools libtorrent-rasterbar-dev
```

**Arch Linux:**
```bash
sudo pacman -S cmake qt6-base libtorrent-rasterbar
```

### Compile
```bash
cmake -B build
cmake --build build
./build/BATorrent
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Roadmap

- [x] WebUI (remote management via browser)
- [x] Auto-shutdown when downloads complete
- [ ] RSS feed auto-download
- [ ] Bandwidth scheduler (time-based speed limits)
- [ ] IP filter / blocklist

## License

MIT
