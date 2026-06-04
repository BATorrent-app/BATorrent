🌐 [English](README.md) | [Português](README.pt-BR.md) | [中文](README.zh-CN.md) | [日本語](README.ja.md) | [Русский](README.ru.md) | [Español](README.es.md) | **[Deutsch](README.de.md)**

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="160">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <em>Ein moderner, plattformübergreifender BitTorrent-Client mit Fokus auf Datenschutz, Leistung und Klarheit</em>
</p>

<p align="center">
  <a href="https://github.com/Mateuscruz19/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/Mateuscruz19/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/Mateuscruz19/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/Mateuscruz19/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/Mateuscruz19/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-available-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="https://github.com/Mateuscruz19/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/Mateuscruz19/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/Mateuscruz19/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/Mateuscruz19/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate Status" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Überblick

BATorrent ist ein Desktop-BitTorrent-Client, der Klarheit, Leistung und Datenschutz priorisiert. Er verbindet die bewährte libtorrent-rasterbar-Engine mit einer sorgfältig abgestimmten Qt 6-Oberfläche, einer fernsteuerbaren WebUI, automatischem RSS-Download, Stremio-kompatibler Suche, VPN-bewusster Verkehrsisolierung und integrierter Medienserver-Anbindung.

> **Keine Telemetrie, keine Analysen, keine versteckten Verbindungen.** Die einzige ausgehende Anfrage, die die App ohne dein Zutun initiiert, ist die GitHub-Release-Prüfung, die in den Einstellungen deaktiviert werden kann. Überprüfe es selbst in [`src/app/updater.cpp`](src/app/updater.cpp).

![Hauptfenster — Dunkles Theme](src/images/image1.png)

![Hauptfenster — Helles Theme](src/images/image2.png)

![Detailpanel](src/images/image3.png)

![Einstellungsdialog](src/images/image4.png)

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Download

Vorgefertigte Binärdateien für die neueste Version:

| Plattform | Format | Hinweise |
|---|---|---|
| Windows | [Installer (`.exe`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) · [Portable (`.zip`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Windows 10+ (x86_64) |
| macOS | **`brew install --cask Mateuscruz19/batorrent/batorrent`** (empfohlen) · [Disk-Image (`.dmg`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) | macOS 12+ (Apple Silicon) |
| Linux | [AppImage](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Glibc 2.35+ (x86_64) |

> **macOS — zum Sicherheitshinweis:** Der Build ist noch nicht notarisiert (Apples Entwicklerprogramm ist kostenpflichtig — eine Hürde für ein Ein-Personen-Projekt). **Homebrew ist der reibungsloseste Weg:** `brew` entfernt bei der Installation das Quarantäne-Flag, sodass die App ohne Gatekeeper-Dialog einfach öffnet. Beim `.dmg` beim ersten Mal mit Rechtsklick auf die App **Öffnen** wählen, um den Hinweis „nicht verifizierter Entwickler" zu umgehen.

Alle Artefakte werden durch den [Build & Release](.github/workflows/build.yml)-Workflow bei jedem getaggten Release erzeugt.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Funktionen

### Torrents
- `.torrent`-Dateien und Magnet-Links mit persistenten Wiederaufnahmedaten
- **Thunder://-Link-Dekodierung** — Chinesische Foren teilen Links im Xunlei-Format `thunder://`; BATorrent dekodiert sie automatisch über Smart Paste (Ctrl+V)
- **Smart Paste** — Ctrl+V mit einem Magnet-Link, Info-Hash oder thunder://-Link in der Zwischenablage fordert zum sofortigen Hinzufügen auf
- **Torrent-Inspektor** — Vorschau einer `.torrent`-Datei (Name, Größe, Dateien, Tracker, Hash, Ersteller, Privat-Flag) vor dem Download
- Priorität pro Datei, sequentieller Download, manuelle Neuprüfung und Neuankündigung
- Automatische Tracker-Injektion von [ngosang/trackerslist](https://github.com/ngosang/trackerslist)
- Multi-Tag-System (Freitext, mehrere Tags pro Torrent neben einer einzelnen Kategorie)
- **Inhaltslayout** — Original, Unterordner erstellen oder Kein Unterordner steuert die Festplattenstruktur von Multi-Datei-Torrents
- **Ausschluss-Dateimuster** — Regex-Regeln zum automatischen Uberspringen von Dateien (z.B. `.nfo`, `.txt`, `sample`) beim Hinzufugen eines Torrents
- **Temporarer Download-Pfad** — ladt zuerst in einen Staging-Ordner herunter, verschiebt automatisch zum Speicherpfad nach Abschluss (verhindert das Scannen von Teildownloads durch Medienserver)
- **Automatisches Entpacken von Archiven** — entpackt `.rar`/`.zip`/`.7z` automatisch bei Abschluss, mit einer Passwortliste für geschützte Archive (nutzt 7-Zip oder WinRAR unter Windows, `unrar`/`unzip` unter macOS/Linux)
- Kategorien, Neuanordnung per Drag-and-Drop und Kontextaktionen per Rechtsklick
- Bestehenden Status aus qBittorrent importieren
- Neue `.torrent`-Dateien aus beliebigen Dateien oder Ordnern erstellen

### Statusverwaltung
- **Abgeschlossen**-Status — manuell markiert oder automatisch nach einem konfigurierbaren Seed-Zeitraum (1, 3, 7, 14 oder 30 Tage) befördert. Unterscheidet sich von Seeden/Fertig, bleibt über Neustarts hinweg erhalten.
- **Stopp-Button** friert einen fertigen Torrent ein, ohne ihn zu entfernen; **Fortsetzen** hebt die Markierung auf und reiht ihn wieder in den Seed-Pool ein.
- Seed-Stopp-Regeln: globales Ratio-Limit und maximale Seed-Zeit, mit Überschreibungen pro Torrent.
- **Automatisches Pausieren bei Dateifehlern** — wenn libtorrent die Dateien eines fertigen Torrents nicht lesen kann (z. B. externe Festplatte getrennt), wird pausiert statt stillschweigend neu herunterzuladen.

### Entdeckung
- **Automatischer RSS-Download** mit Regex-Filtern, Speicherpfaden pro Feed, Intervallplanung und Duplikaterkennung. Verarbeitet Magnet-Links, `.torrent`-URLs und `<enclosure>`-Tags.
- **Stremio-kompatible Suche** für Filme und Serien über die integrierten Cinemeta- und Torrentio-Addons.

### Streaming
- Abspielen während des Downloads — `.mp4`, `.mkv`, `.avi`, `.mov`, `.wmv`, `.flv`, `.webm`, `.m4v`, `.ts`.
- Erkennt automatisch VLC und IINA, greift auf den Standard-Systemplayer zurück.

### VPN und Datenschutz
- **Interface-Bindung** beschränkt den gesamten Torrent-Verkehr auf eine gewählte Netzwerkschnittstelle (z. B. `tun0`, `utun4`).
- **Kill Switch** pausiert jeden aktiven Torrent in dem Moment, in dem die gebundene Schnittstelle ausfällt, mit optionaler automatischer Wiederaufnahme bei Rückkehr.
- **PT-Modus** — Private-Tracker-Konformität mit einem Schalter: deaktiviert DHT/PEX/LSD, erzwingt anonymen Handshake, kündigt in jeder Stufe an. Erforderlich für M-Team, TorrentLeech und die meisten ratio-kontrollierten Communities.
- **Anti-Leecher-Blockierung** — erkennt und sperrt automatisch Xunlei (Thunder), QQDownload, Baidu Netdisk P2P und andere Clients, die herunterladen ohne zurückzuseeden. Erkannt anhand des peer_id-Präfix im BitTorrent-Handshake.
- **Anonymer Modus** — verbirgt Client-Name/-Version in Handshakes, deaktiviert UPnP/NAT-PMP-Ankündigungen.
- **Tor-Proxy-Preset** — ein Klick füllt SOCKS5 127.0.0.1:9050 aus.
- **IPv4 erzwingen** — lehnt IPv6-Peers ab, um v6-Leaks zu verhindern, wenn der Tunnel dies nicht abdeckt.
- VPN-Erkennung für WireGuard, Mullvad, NordLynx, ProtonVPN, generisches tun/tap.
- SOCKS5- und HTTP-Proxy mit Authentifizierung.
- IP-Blocklisten-Unterstützung (P2P-Format).
- Protokollverschlüsselung: aktiviert / erzwungen / deaktiviert.

### WebUI
- Browserbasiertes Kontrollpanel unter `http://localhost:8080` (Port und Fernzugriff konfigurierbar).
- **QR-Code-Kopplung** — scanne einen QR-Code mit deinem Handy, um die WebUI sofort zu öffnen, ohne IP-Eingabe. Der QR wird lokal in reinem C++ generiert (deine LAN-Adresse verlässt nie die Maschine).
- REST-API mit JSON-Antworten; Hinzufügen per Magnet oder `.torrent`-Upload; Pausieren / Fortsetzen / Entfernen; Datei- und Peer-Ansichten pro Torrent.
- SHA-256-gehashte Basic Auth, themenangepasste dunkle Oberfläche, vollständig responsives mobiles Layout.

### Bandbreite und Planung
- Unabhängige Download- und Upload-Limits.
- Alternatives Geschwindigkeitsprofil mit Tageszeit- + Wochentagsplanung (Nachtbereiche unterstützt).
- Ratio- und Seed-Zeit-Limits pro Torrent und global.

### Medienserver-Integration
- Benachrichtigt **Plex**, **Jellyfin** oder **Emby** zum Scannen der Bibliothek, wenn ein Download abgeschlossen ist.
- Konfigurierbare URL und Token / API-Schlüssel pro Server.

### Benachrichtigungen und Integrationen
- **Telegram-Webhook** — Download abgeschlossen, Kill Switch ausgelöst, automatischer RSS-Download und Torrent-Fehler werden per Bot-Token an jeden Telegram-Chat gesendet. Kontrollkästchen pro Ereignis + Test-Button.
- **Discord Rich Presence** — zeigt "Downloading X · 67%" in deinem Discord-Profil mit den Buttons "Download BATorrent" und "View on GitHub". Funktioniert sofort.

### Oberfläche
- **Sechs Themes** — Dunkel, Hell (warme Cremepalette "Comfortable"), Midnight, Sakura, Dark Star und ein vollständig **anpassbares** Theme (eigenes Hintergrundbild + Akzentfarben), jeweils mit optionaler **Anime-Akzentgrafik**.
- **Automatische Cover-Art** — holt Filme-/Serienposter (TMDB) und Spielegrafiken (IGDB) aus dem Torrent-Namen für eine Poster-**Rasteransicht**; umschaltbar auf eine kompakte Listenansicht.
- Echtzeit-Geschwindigkeitsdiagramm, Detailpanel (Allgemein · Peers · Dateien · Tracker · Stücke), statusfarbene Fortschrittsbalken, Tray-Benachrichtigungen mit Klick-Fokus.
- Benutzerdefiniertes Tray-Popup (plattformübergreifend) mit Live-Geschwindigkeiten, Vorschau aktiver Torrents mit ETA, VPN-Status und Beenden-Option.
- Filterschaltflächen mit Live-Zählern (Alle / Aktiv / Herunterladen / Seeden / Abgeschlossen / Pausiert / Fertig / Warteschlange), Suchleiste und Kategoriefilter.
- Drag-and-Drop für `.torrent`-Dateien und Magnet-Links.
- **Sieben UI-Sprachen** mit automatischer Erkennung: English, Português (BR), Español, Deutsch, Русский, 日本語, 中文 — 1.000+ übersetzte Zeichenketten mit englischem Fallback für fehlende Schlüssel.
- Geschwindigkeitsanzeige in Bytes (KB/s, MB/s) oder Bits (Kbps, Mbps) — umschaltbar in den Einstellungen.
- Locale-bewusste Zahlenformatierung (z. B. `1 234,5` für russisches Locale).

### System
- Automatische Aktualisierung mit konfigurierbarer Quelle: **GitHub** (Standard) oder **Gitee** (China-Mirror) oder deaktiviert.
- Automatisches Herunterfahren, wenn alle Downloads abgeschlossen sind (60 s abbrechbarer Countdown).
- **Windows-Defender-Ausnahme** — ein Klick fügt den Download-Ordner zur Ausnahmeliste von Defender hinzu, sodass heruntergeladene Dateien nicht mehr markiert/gescannt werden (UAC-erhöht).
- **Vollständige Sicherung/Wiederherstellung** aller Einstellungen + Wiederaufnahmedaten in einem einzelnen Archiv für die Migration zwischen Rechnern.
- **Kürzlich entfernte** Historie (letzte 50 Torrents, Wiederherstellung mit einem Klick).
- **Erzwungener Start** — umgeht das Limit aktiver Downloads in der Warteschlange für einen einzelnen Torrent.
- Integrierter **Log-Viewer** mit Dateirotation (5 MB/Datei, behält 3), Level-Filter, Export für Fehlerberichte und `--debug`-CLI-Flag.
- **Diagnosedialog** — Gesundheitsprüfung (Speicherpfad, Port, DHT, Verschlüsselung, VPN, Leecher-Blockierung) + ausgehender IP-Leak-Test (über api.ipify.org).
- CLI-Argumente: beliebig viele `.torrent`-Pfade oder `magnet:`-URIs beim Start übergeben; nachfolgende Starts leiten an die laufende Instanz weiter.
- Automatisches Changelog-Popup beim ersten Start nach einem Versionsupdate.
- Tastenkürzel und `?`-Schnellreferenzdialog.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Erste Schritte

1. Lade den Build für deine Plattform von der [Release-Seite](https://github.com/Mateuscruz19/BATorrent/releases/latest) herunter.
2. Beim ersten Start führt der Willkommensdialog durch den Standard-Speicherpfad, das Theme und die Sprache.
3. Ziehe eine `.torrent`-Datei oder einen Magnet-Link auf das Fenster — oder nutze **Datei → Torrent öffnen** / **Datei → Magnet hinzufügen**.
4. Optional: Binde die ausgehende Schnittstelle unter **Einstellungen → VPN** und aktiviere den Kill Switch, bevor du sensible Torrents hinzufügst.

> **VPN-Tipp:** Wenn die **Interface-Bindung** gesetzt ist, läuft jede Ankündigung und Peer-Verbindung ausschließlich über diese Schnittstelle. Fällt die Schnittstelle aus, pausiert der Kill Switch alles, bis sie zurückkehrt.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Aus dem Quellcode kompilieren

### Voraussetzungen
- C++17-Toolchain (GCC 11+, Clang 14+ oder MSVC 19.30+)
- CMake 3.16+
- Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`)
- libtorrent-rasterbar 2.0+
- Boost (transitive Abhängigkeit von libtorrent)
- Optional: Qt6Keychain (speichert Zugangsdaten im OS-Schlüsselbund statt in Klartext-QSettings)

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

Installiere Qt 6 über den offiziellen Installer und libtorrent über vcpkg:

```powershell
vcpkg install libtorrent:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

### Tests

Die Test-Suite ist optional:

```bash
cmake -B build -DBAT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Projektstruktur

```
src/
├── torrent/      libtorrent-Wrapper, Wiederaufnahmedaten, Warteschlange, Seed-Regeln
├── app/          Übersetzer, Updater, RSS, Addons, Geheimspeicher, GeoIP
├── gui/          Qt Widgets-Oberfläche (Hauptfenster, Dialoge, Details, Tray-Popup)
├── webui/        eingebetteter HTTP-Server + Browser-Oberfläche
└── main.cpp      Einzelinstanz-Bootstrap, CLI-Parsing, Theming
.github/
└── workflows/    Linux AppImage, macOS DMG, Windows Installer + Zip
installer/        Inno Setup-Skript für den Windows-Installer
dist/             Desktop-Datei + Assets für Linux-Paketierung
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Mitwirken

Issues und Pull Requests sind willkommen. Bitte eröffne für größere Änderungen zuerst ein Issue, um den Ansatz zu besprechen. Vorgefertigte Artefakte können für jeden Branch über den **Build & Release**-Workflow (`workflow_dispatch`) erzeugt werden.

Beim Melden eines Fehlers bitte beifügen:
- Plattform + Version (`Hilfe → Über`)
- Schritte zur Reproduktion
- Den relevanten Abschnitt von `~/.local/share/BATorrent/` (Linux), `~/Library/Application Support/BATorrent/` (macOS) oder `%APPDATA%\BATorrent\` (Windows), falls es Wiederaufnahmedaten oder Einstellungen betrifft.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Lizenz

[MIT](LICENSE) © 2024–2026 Mateus Cruz
