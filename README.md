<p align="center">
  <img src="src/images/logo1.png" alt="BATorrent Logo" width="200">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  A lightweight BitTorrent client built with C++, Qt 6, and libtorrent-rasterbar.
</p>

---

## About

BATorrent is a desktop BitTorrent client for Linux, focusing on simplicity and performance. Built from scratch as a learning project using modern C++ and the Qt framework.

<img width="997" height="681" alt="image" src="https://github.com/user-attachments/assets/0f0ff8c7-7dc9-482c-80cd-5bb4a66f9276" />
<img width="540" height="550" alt="image" src="https://github.com/user-attachments/assets/a572b3cd-ad96-4453-af1b-677e8c498907" />


## Tech Stack

- **C++17** — core language
- **Qt 6** — GUI framework
- **libtorrent-rasterbar** — BitTorrent protocol implementation
- **CMake** — build system

## Building

### Dependencies

- GCC (C++17 support)
- Qt 6 (Widgets module)
- libtorrent-rasterbar
- CMake 3.16+

sudo apt install build-essential cmake qt6-base-dev qt6-base-dev-tools libtorrent-rasterbar-dev

### Compile

```bash
cmake -B build
cmake --build build
./build/BATorrent
```

## Status

Early development — actively being built.

## License

MIT
