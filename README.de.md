<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <b>Deutsch</b> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>Der BitTorrent-Client mit Gesicht — Filmcover, sechs Themes, null Werbung.</i>
</p>

<p align="center">
  <a href="https://github.com/Mateuscruz19/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/Mateuscruz19/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/Mateuscruz19/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/Mateuscruz19/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/Mateuscruz19/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

<p align="center">
  <img src="src/images/with_startup.gif" alt="BATorrent — öffnen und loslegen, Cover werden automatisch geladen" width="860">
</p>

Die meisten Torrent-Clients sehen aus wie ein Steuerformular. Dieser zeigt deine Downloads als **Wand aus Film-, Serien- und Spielcovern** — dasselbe, was du von Netflix oder Steam kennst — und lässt sich in sechs Themes (oder dein eigenes Hintergrundbild) kleiden. Unter der Haube steckt die bewährte **libtorrent**-Engine, also kein hübsches Spielzeug: ein echter Client, der zufällig auch Geschmack hat.

> **Keine Werbung. Keine Telemetrie. Keine „Pro"-Stufe. Kein Konto.** Die einzige Anfrage, die er von sich aus stellt, ist die GitHub-Update-Prüfung, und die kannst du abschalten. Der Quellcode ist direkt hier — lies [`updater.cpp`](src/app/updater.cpp) und überzeug dich selbst.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Warum es das gibt

Ich bin ein einzelner Entwickler in Brasilien. Ich wollte einen Torrent-Client, der Privatsphäre ernst nimmt, nativ auf jedem Desktop läuft und nicht aussieht, als wäre er 2009 gebaut worden — und weil ich keinen fand, habe ich meinen eigenen gebaut. Er ist kostenlos und **MIT-lizenziert**: keine Haken, keine Telemetrie, die sich später einschleicht, und er kann nicht still an eine Firma verkauft werden, die Werbung dranschraubt. Acht Sprachen, denn „nützlich" sollte nicht „nur Englisch" heißen.

## Das Aussehen

<p align="center">
  <img src="src/images/themes.gif" alt="Wechsel zwischen den integrierten Themes" width="860">
</p>

<p align="center">
  <img src="src/images/shot-grid.jpg" alt="Cover-art grid" width="860">
</p>

<p align="center">
  <img src="src/images/shot-list.jpg" alt="List view" width="425">
  <img src="src/images/shot-theme.jpg" alt="Sakura theme" width="425">
</p>

- **Automatische Cover** — liest den Torrent-Namen und holt das echte Poster (Filme & Serien über TMDB, Spiele über IGDB) in eine Rasteransicht. Ein Klick wechselt zur kompakten Liste.
- **Sechs Themes** — Dark, Light, Midnight, Sakura, Dark Star und ein vollständig **anpassbares** (eigener Hintergrund + Akzentfarben), jeweils mit optionaler Anime-Akzentgrafik.
- Echtzeit-Geschwindigkeitsdiagramm, zustandsgefärbter Fortschritt, ein reichhaltiges Tray-Popup mit Live-Geschwindigkeiten und Restzeit — die Details, die es *fertig* wirken lassen.

## Was er wirklich kann

| | |
|---|---|
| 🔒 **Privatsphäre zuerst** | VPN-Interface-Bindung + **Kill Switch** (kappt allen Verkehr, wenn der Tunnel abbricht), PT-Modus für private Tracker, Tor-Voreinstellung, anonymer Handshake, Anti-Leecher-Blockade |
| 🔎 **Finden & hinzufügen** | Integrierte Suche (inkl. offene CIS/RuTor-Quellen, ohne Login), Smart Paste (Magnet / `thunder://` / Hash per Strg+V), RSS-Auto-Download mit Regex-Filtern, Drag & Drop |
| 📱 **Von überall steuern** | Browser-WebUI mit **QR-Kopplung** — vom Handy scannen, keine IPs tippen. Der QR-Code wird lokal erzeugt; deine Adresse verlässt den Rechner nie |
| 📺 **Ansehen & ordnen** | Streamen während des Downloads, automatisches Entpacken von Archiven, Kategorien + Tags, Plex/Jellyfin/Emby-Bibliotheksaktualisierung bei Abschluss |
| 🔔 **Auf dem Laufenden bleiben** | Native Desktop-Benachrichtigungen, Telegram-Alerts, Discord Rich Presence („Lädt X · 67%") |

<details>
<summary><b>…und der lange Rest</b> (zum Aufklappen klicken)</summary>

Priorität pro Datei · sequenzieller Download · automatisches Tracker-Einspielen · Inhalts-Layout-Steuerung · Regex für ausgeschlossene Dateien · temporärer Download-Pfad · Status „Abgeschlossen" mit Seeding-Fenstern · Auto-Pause bei Dateifehlern · globale + pro-Torrent Ratio-/Zeitlimits · Bandbreiten-Planer (Stunde + Tag) · Import aus qBittorrent · `.torrent`-Dateien erstellen · Torrent-Inspektor · IP-Sperrlisten · Protokollverschlüsselung · Gitee-Update-Mirror · automatisches Herunterfahren nach Abschluss · Windows-Defender-Ausnahme · vollständiges Backup/Restore · Verlauf kürzlich entfernter Torrents · Start erzwingen · integrierter Log-Viewer + Diagnose + IP-Leak-Test · gebietsschema-bewusste Formatierung · Tastenkürzel.

</details>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Holen

| Plattform | | |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Installer](https://github.com/Mateuscruz19/BATorrent/releases/latest) · [Portabel](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Windows 10+ |
| **macOS** | **`brew install --cask Mateuscruz19/batorrent/batorrent`** · [`.dmg`](https://github.com/Mateuscruz19/BATorrent/releases/latest) | macOS 12+ · Apple Silicon |
| **Linux** | [AppImage](https://github.com/Mateuscruz19/BATorrent/releases/latest) | glibc 2.35+ |

Dann einfach eine `.torrent`-Datei oder einen Magnet auf das Fenster ziehen. Das war's.

<sub>**macOS:** noch nicht notarisiert (Apples Entwicklerprogramm ist kostenpflichtig). Homebrew ist der reibungsloseste Weg — `brew` entfernt das Quarantäne-Flag, sodass die App ohne Gatekeeper-Dialog öffnet. Bei der `.dmg` beim ersten Mal Rechtsklick → **Öffnen**.</sub>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

<details>
<summary><b>Aus dem Quellcode bauen & technische Notizen</b></summary>

### Voraussetzungen
C++17 · CMake 3.16+ · Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`) · libtorrent-rasterbar 2.0+ · Boost · Qt6Keychain (optional).

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```
(macOS: `brew install qt libtorrent-rasterbar boost openssl`. Windows: Qt-Installer + `vcpkg install libtorrent:x64-windows`.)

### Qualität & Sicherheit

<p>
  <a href="https://github.com/Mateuscruz19/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/Mateuscruz19/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/Mateuscruz19/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/Mateuscruz19/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- **Tests** — Catch2-Suite (Unit, Sicherheit, Speicher) bei jedem CI-Build; neues Backend-Verhalten bekommt einen Test.
- **Sanitizers** — läuft sauber unter AddressSanitizer + UndefinedBehaviorSanitizer (0 Lecks / Use-after-free / UB).
- **Geprüft** vor jedem Release auf Speicher-/Thread-Sicherheit, WebUI-Authentifizierung, Injection, Path Traversal, Eingabevalidierung und Umgang mit Geheimnissen. Geheimnisse liegen im OS-Keychain, nie im Klartext; die WebUI öffnet sich erst zum Netzwerk, wenn du ein Passwort setzt.

</details>

## Mitwirken

Issues und PRs willkommen — für alles Nicht-Triviale bitte zuerst ein Issue eröffnen. Fehlerberichte: nenne deine Plattform + Version (`Hilfe → Über`) und die Schritte zur Reproduktion. Übersetzungen sind besonders willkommen.

## Lizenz

[MIT](LICENSE) © 2024–2026 Mateus Cruz · made in Brazil 🦇
