<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <b>Deutsch</b> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>Ein BitTorrent-Client, der deine Downloads als Cover zeigt statt als Tabellenzeilen.</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Plattformen" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="https://batorrent.com/assets/trailer.mp4"><img src="src/images/trailer-poster.jpg" alt="Den BATorrent-Trailer ansehen (1:30)" width="860"></a>
</p>

BATorrent ist ein Desktop-Torrent-Client auf Basis der [libtorrent](https://www.libtorrent.org/)-Engine, die auch qBittorrent und Deluge verwenden. Die Oberfläche liest den Namen jedes Torrents, sucht das passende Poster (Filme und Serien über TMDB, Spiele über IGDB) und zeigt deine Downloads als Raster aus Covern statt als Liste von Dateinamen. Darunter steckt ein vollwertiger Client, der auf einer [gepatchten Version dieser Engine](#die-engine) läuft.

Er ist kostenlos und Open Source, ohne Werbung, Telemetrie, "Pro"-Stufe oder Konto. Die einzige Netzwerkanfrage, die er von sich aus stellt, ist die Update-Prüfung bei GitHub, und die lässt sich per Schalter abstellen. Wer das nachprüfen will, findet den Code in [`updater.cpp`](src/services/integrations/updater.cpp).

## Warum ich ihn gebaut habe

Ich bin ein einzelner Entwickler in Brasilien. Ich wollte einen Torrent-Client, der Privatsphäre ernst nimmt, nativ unter Windows, macOS und Linux läuft und nicht aussieht, als stamme sein Design aus dem Jahr 2009. Ich habe keinen gefunden, der mir gefiel, also habe ich selbst einen geschrieben. Er steht unter der MIT-Lizenz: Sollte das Projekt je Telemetrie oder Werbung einbauen, könnte jeder den Code forken und ohne beides veröffentlichen. Die Oberfläche ist in neun Sprachen übersetzt.

## Die Oberfläche

<p align="center">
  <img src="src/images/shot-list.jpg" alt="Kompakte Listenansicht, wenn dir Details wichtiger sind als Optik" width="860">
</p>

<p align="center">
  <img src="src/images/shot-palette-v43.jpg" alt="Befehlspalette (Strg/⌘+K): jeden Torrent und jede Aktion per unscharfer Suche finden" width="860">
</p>

<p align="center">
  <img src="src/images/shot-theme.jpg" alt="Sakura, eines der mitgelieferten Themes" width="860">
</p>

- **Cover.** Er ermittelt Poster anhand des Torrent-Namens und zeigt sie in einem Raster. Mit einem Klick wechselst du zu einer kompakten Liste, wenn dir Details wichtiger sind als Optik.
- **Sechs Themes.** Dark, Light, Midnight, Sakura, Dark Star und ein Custom-Theme, bei dem du Hintergrund und Akzentfarbe selbst wählst. Jedes unterstützt optional Anime-Akzentgrafiken.
- **Befehlspalette.** Strg/⌘+K öffnet eine unscharfe Suche nach jedem Torrent und jeder Aktion: alles pausieren, alternative Geschwindigkeit umschalten, zu einer beliebigen Seite springen. Die Maus brauchst du dafür nicht.
- **Live-Status.** Ein Geschwindigkeitsdiagramm in Echtzeit, Fortschrittsbalken in der Farbe des jeweiligen Status und ein Tray-Popup mit aktuellen Geschwindigkeiten und Restzeit.

## Was er kann

**Direkt in der App ansehen.** Es gibt einen eingebauten Videoplayer (auf FFmpeg-Basis, spielt also MKV, AVI und WebM direkt ab), und du kannst schon während des Downloads mit dem Ansehen beginnen, weil er den Anfang der Datei zuerst lädt. Er sucht und lädt Untertitel für dich (über SubDL), lädt danebenliegende `.srt`/`.vtt`-Dateien automatisch und lässt dich die Synchronisation live nachjustieren. Nach Abschluss kann er eine Plex-, Jellyfin- oder Emby-Bibliothek aktualisieren.

**Sofortige Wiedergabe mit Debrid.** Verbinde ein [Real-Debrid](https://real-debrid.com)- oder [TorBox](https://torbox.app)-Konto. Liegt ein Magnet dort bereits im Cache, entsperrt BATorrent den Link und streamt ihn direkt in den eingebauten Player, sodass auf deinem Rechner nichts heruntergeladen oder geseedet wird.

**Auch Spiele.** Spiele-Torrents bekommen ebenfalls Cover (über IGDB). Durchsuche Spielekataloge, lade herunter und installiere und starte die Spiele dann aus der App heraus. So verhält sich deine Raubkopien-Sammlung ein bisschen wie eine Steam-Bibliothek statt wie ein Ordner voller Setup-Dateien.

**Entdecken.** Eine Startseite zum Stöbern im Netflix-Stil (Poster aktueller Trends, ein wechselndes Titelbild), auf der du etwas zum Herunterladen findest, ohne die App zu verlassen.

<p align="center">
  <img src="src/images/shot-discover.jpg" alt="Entdecken: eine Startseite zum Stöbern mit Postern aktueller Trends" width="860">
</p>

**Privatsphäre.** Binde den Client an eine bestimmte VPN-Schnittstelle, mit einem Kill Switch, der sämtlichen Verkehr kappt, sobald der Tunnel abbricht. Dazu kommen ein Modus für private Tracker, eine Tor-Voreinstellung, anonyme Handshakes und das Blockieren von Leecher-Clients. Mit dem eingebauten IP-Leak-Test kannst du prüfen, ob alles greift.

**Finden und hinzufügen.** Eingebaute Suche (auch in offenen CIS/RuTor-Quellen, die keinen Login brauchen), Smart Paste, das bei Strg+V einen Magnet, eine `.torrent`-Datei, einen `thunder://`-Link oder einen Info-Hash erkennt, RSS-Auto-Download mit Regex-Filtern, ein überwachter Ordner und Drag & Drop.

<p align="center">
  <img src="src/images/shot-search.jpg" alt="Eingebaute Suche: Cover, Bewertungen und sofort der beste Treffer" width="860">
</p>

**Fernsteuerung.** Eine WebUI im Browser mit QR-Kopplung: Scanne den Code mit dem Handy, statt IP-Adressen abzutippen. Der QR-Code wird auf deinem Rechner erzeugt, und die Adresse verlässt ihn nie.

**Ordnung halten.** Archive nach Abschluss automatisch entpacken, mit Kategorien und Tags sortieren, Ratio- und Zeitlimits global und pro Torrent festlegen und die Bandbreite nach Uhrzeit und Wochentag planen.

**Benachrichtigungen.** Native Desktop-Benachrichtigungen, Telegram-Nachrichten und Discord Rich Presence.

<details>
<summary><b>Vollständige Funktionsliste</b></summary>

Priorität pro Datei, sequenzieller Download, automatisches Hinzufügen von Trackern, Steuerung des Inhaltslayouts, Regex zum Ausschließen von Dateien, separater temporärer Download-Pfad, ein Status "Abgeschlossen" mit Seeding-Zeitfenstern, automatisches Pausieren bei Dateifehlern, globale und torrentbezogene Ratio- und Zeitlimits, ein Bandbreitenplaner nach Uhrzeit und Wochentag, Import aus qBittorrent, Erstellen von `.torrent`-Dateien, ein Torrent-Inspektor, IP-Sperrlisten, Protokollverschlüsselung, ein Gitee-Mirror für Updates, automatisches Herunterfahren nach Abschluss der Downloads, eine Hilfe für Ausnahmen im Windows Defender, vollständiges Backup und Wiederherstellen, ein Verlauf kürzlich entfernter Torrents, Start erzwingen, ein eingebauter Log-Viewer mit Diagnose und IP-Leak-Test, an das Gebietsschema angepasste Formatierung und Tastenkürzel.

</details>

## Die Engine

Die meisten Torrent-Apps binden libtorrent unverändert ein. BATorrent liefert einen kleinen gepatchten Fork davon mit und kann so Verhalten der Engine ändern, an das die öffentliche API nicht herankommt:

- **Schnellerer Pipeline-Aufbau.** Auf einer Verbindung mit hoher Bandbreite und hoher Latenz wächst die Anfrage-Pipeline im Original Schritt für Schritt. Der Fork lässt sie geometrisch wachsen und füllt eine breite Leitung so in einem Bruchteil der Roundtrips. Im projekteigenen A/B-Benchmark waren das auf einer schnellen Verbindung rund +27 %, ohne die Aussetzer, die das Original von Lauf zu Lauf zeigt, und in keinem Fall langsamer.
- **Bevorzugung von Peers aus dem eigenen Land.** Eine Offline-GeoIP-Datenbank (db-ip Lite) ordnet jeden Peer einem Land zu, und das Peer-Ranking des Forks bevorzugt bei freier Wahl Peers aus deinem Land. Das bedeutet meist geringere Latenz und weniger gedrosselte grenzüberschreitende Routen.

Beides sind Compile-Time-Funktionen des Forks, in einem Standard-Build abgeschaltet, und sie werden als versionierte Patches unter [`third_party/patches/`](third_party/patches) eingespielt statt als eingebettete Kopie.

## Installation

| Plattform | Download | Voraussetzungen |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6), [Installer](https://batorrent.com/win) oder [Portable](https://batorrent.com/portable) | Windows 10 oder neuer |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` oder die [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Sobald die App läuft, ziehst du eine `.torrent`-Datei oder einen Magnet-Link auf das Fenster.

<sub><b>Hinweis zu macOS:</b> Die App ist noch nicht notarisiert (Apples Entwicklerprogramm ist ein kostenpflichtiges Abo). Homebrew ist der einfachste Weg, weil <code>brew</code> das Quarantäne-Flag entfernt und die App sich dadurch ohne Gatekeeper-Abfrage öffnet. Wenn du stattdessen die <code>.dmg</code> nimmst, klicke beim ersten Start mit der rechten Maustaste auf die App und wähle <b>Öffnen</b>.</sub>

<details>
<summary><b>Aus dem Quellcode bauen</b></summary>

**Voraussetzungen:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`), libtorrent-rasterbar 2.0+, Boost und optional Qt6Keychain.

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```

Unter macOS: `brew install qt libtorrent-rasterbar boost openssl`.
Unter Windows: der Qt-Installer plus `vcpkg install libtorrent:x64-windows`.

</details>

<details>
<summary><b>Qualität und Sicherheit</b></summary>

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- Eine Catch2-Testsuite (Unit-, Sicherheits- und Speichertests) läuft bei jedem CI-Build; neues Backend-Verhalten kommt immer mit einem Test.
- Der Build läuft sauber unter AddressSanitizer und UndefinedBehaviorSanitizer.
- Vor jedem Release wird der Code auf Speicher- und Thread-Sicherheit, WebUI-Authentifizierung, Injection, Path Traversal, Eingabevalidierung und den Umgang mit Geheimnissen geprüft. Geheimnisse liegen im Schlüsselbund des Betriebssystems statt im Klartext, und die WebUI ist erst dann im Netzwerk erreichbar, wenn du ein Passwort gesetzt hast.

</details>

## Mitwirken

Issues und Pull Requests sind willkommen. Bei allem, was über Kleinigkeiten hinausgeht, eröffne bitte zuerst ein Issue, damit wir uns auf einen Ansatz einigen können. Fehlerberichte helfen am meisten, wenn sie deine Plattform und Version (unter `Hilfe → Über`) und die Schritte zum Reproduzieren enthalten. Über Übersetzungen freue ich mich besonders.

## Lizenz und Marke

Der **Code** steht unter [MIT](LICENSE), © 2024-2026 Mateus Cruz. Du darfst ihn forken und deinen eigenen Build veröffentlichen.

Der **Name "BATorrent" und das Logo** gehören dem Projekt und fallen nicht unter die Code-Lizenz. Wenn du einen Fork weiterverbreitest, gib ihm bitte einen eigenen Namen, damit Nutzer erkennen, welcher Build der offizielle ist. Die Einzelheiten stehen in [TRADEMARK.md](TRADEMARK.md). Forks und Beiträge in gutem Glauben sind willkommen.

Entwickelt in Brasilien.
