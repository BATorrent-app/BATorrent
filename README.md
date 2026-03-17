<p align="center">
  <img src="src/images/logo1.png" alt="BATorrent Logo" width="200">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  A lightweight BitTorrent client built with C++, Qt 6, and libtorrent-rasterbar.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-17-red?style=flat-square&logo=c%2B%2B"/>
  <img src="https://img.shields.io/badge/Qt-6-red?style=flat-square&logo=qt"/>
  <img src="https://img.shields.io/badge/Linux-only-red?style=flat-square&logo=linux"/>
  <img src="https://img.shields.io/badge/License-MIT-red?style=flat-square"/>
</p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## About

BATorrent is a desktop BitTorrent client for Linux, Windows and Mac, focusing on simplicity and performance. Built from scratch as a learning project using modern C++ and the Qt framework.

<img width="1176" height="790" alt="image" src="https://github.com/user-attachments/assets/776da3b7-5fe6-49eb-87cc-b985dcc8cfda" />
<img width="1172" height="782" alt="image" src="https://github.com/user-attachments/assets/bd1708c3-9987-4240-b37a-69c0aaa289e2" />
<img width="1177" height="795" alt="image" src="https://github.com/user-attachments/assets/2a0be1f2-b5b5-45b8-8e17-db67a0a35345" />
<img width="540" height="550" alt="image" src="https://github.com/user-attachments/assets/a572b3cd-ad96-4453-af1b-677e8c498907" />

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Tech Stack

| | Technology | Role |
|---|---|---|
| ⚙️ | **C++17** | Core language |
| 🖼️ | **Qt 6** | GUI framework |
| 🔗 | **libtorrent-rasterbar** | BitTorrent protocol |
| 🔨 | **CMake** | Build system |

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Features

### Torrent Management
- Add torrents via `.torrent` files or magnet links
- Drag & drop support for `.torrent` files and magnet links
- Command-line arguments support (pass `.torrent` files or `magnet:` URIs directly)
- Create `.torrent` files from local content
- Import torrents from qBittorrent (auto-detects BT_backup folder with fastresume data)
- Pause, resume, and remove individual or all torrents at once
- Remove torrents with option to delete downloaded files
- Sequential download mode (per-torrent toggle)
- Per-file priority control (skip, low, normal, high)
- Automatic resume data saving (every 5 minutes + on exit)
- Seed ratio limit with auto-pause when reached

### Details & Monitoring
- **General tab** — name, save path, size, downloaded, progress, speeds, state, peers/seeds count, ratio
- **Peers tab** — IP, port, client, download/upload speed, progress per peer
- **Files tab** — file list with individual size, progress, and priority selector
- **Trackers tab** — tracker URLs, tier, status, with ability to add custom trackers
- Real-time speed graph (download & upload)
- Status bar with torrent count and global speeds

### Network & Protocol
- DHT (Distributed Hash Table) for trackerless torrents
- PEX (Peer Exchange) enabled by default
- UPnP and NAT-PMP for automatic port forwarding
- Protocol encryption (enabled, forced, or disabled)
- Configurable max connections limit
- Global download and upload speed limits

### Interface
- 3 themes: Dark, Light, and Midnight
- Multi-language support (English and Portuguese), with auto-detection from system locale
- System tray icon with quick actions (show, pause all, resume all, quit)
- Minimize to tray on close
- Sortable and resizable columns with persistent layout
- Filter torrents by state (All Active, Downloading, Seeding, Paused, Finished)
- Search bar to filter torrents by name
- Context menu with resume, pause, sequential toggle, open folder, remove
- Keyboard shortcuts (Space to toggle pause, Ctrl+A to select all, Ctrl+O to open)
- Splash screen animation on startup
- Welcome dialog on first launch
- Desktop notifications on download completion

### Updates & Distribution
- Built-in auto-update system (checks GitHub releases on startup)
- In-app update download with progress bar
- Windows installer via Inno Setup
- Linux `.desktop` file for app launcher integration
- Cross-platform: Linux, Windows, and macOS

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Building

### Dependencies

- GCC (C++17 support)
- Qt 6 (Widgets module)
- libtorrent-rasterbar
- CMake 3.16+
```bash
sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools libtorrent-rasterbar-dev
```

### Compile
```bash
cmake -B build
cmake --build build
./build/BATorrent
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=cc0000&height=3&width=100%25" width="100%"/>

## Status

> 🚧 Early development — actively being built.

## License

MIT
