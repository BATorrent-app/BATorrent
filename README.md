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

BATorrent is a desktop BitTorrent client for Linux, focusing on simplicity and performance. Built from scratch as a learning project using modern C++ and the Qt framework.

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
