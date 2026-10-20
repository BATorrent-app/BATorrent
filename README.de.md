<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <b>Deutsch</b> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="120">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  Ein Torrent-Client, der deine Downloads als Cover zeigt, sie schon während des Downloads abspielt<br>
  und deine Filme, Serien und Spiele in einer Bibliothek zusammenführt.
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="Lizenz" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Plattformen" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="#installation"><b>Herunterladen</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com/assets/trailer.mp4"><b>Trailer ansehen</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com"><b>Website</b></a>
</p>

<p align="center">
  <img src="docs/images/grid.jpg" alt="Der Tab Downloads in der Rasteransicht, mit einer ausgewählten Serie und ihren Folgen rechts" width="900">
</p>

BATorrent ist ein Desktop-Torrent-Client auf Basis von [libtorrent](https://www.libtorrent.org/), der Engine, die auch qBittorrent und Deluge verwenden. Er liest den Namen jedes Torrents, sucht das passende Poster (TMDB für Filme und Serien, IGDB für Spiele) und zeigt deine Downloads als Cover statt als Dateinamen. Folgen derselben Serie werden zusammengefasst, Videos laufen in der App, bevor der Download fertig ist, und Spiele lassen sich im selben Fenster installieren und starten.

Er ist kostenlos und Open Source. Keine Werbung, kein Konto, keine "Pro"-Stufe, keine Analytics. Abgesehen von den Torrents selbst geht er für drei Dinge online: Cover und Details von TMDB, IGDB und Steam für das, was in deiner Bibliothek liegt oder was du in Finden öffnest; die Update-Prüfung bei GitHub, die du abschalten kannst ([`updater.cpp`](src/services/integrations/updater.cpp)); und, in den offiziellen Builds, einen Absturzbericht, falls die App abstürzt.

## Drei Tabs

Die App hat oben drei Tabs: **Downloads**, **Finden** und **HUB**.

### Downloads

Deine Torrents als Raster aus Covern oder als kompakte Liste, mit den Details zur aktuellen Auswahl an der Seite. Serien werden zu einer Zeile mit ihren Folgen darin zusammengeklappt, sodass zwölf Folgen einer Serie eine Zeile belegen statt zwölf.

<p align="center">
  <img src="docs/images/list.jpg" alt="Der Tab Downloads in der Listenansicht, mit einer zu ihren Folgen aufgeklappten Serie" width="900">
</p>

### Finden und HUB

In **Finden** suchst du nach etwas Neuem: eine Startseite mit angesagten Filmen, Serien und Spielen und eine Suche, die über deine Quellen läuft und den besten Treffer zuerst zeigt. Jeder Titel hat eine eigene Seite. Die eines Films listet seine Releases, sortiert danach, wie viele sie teilen, mit einer Empfehlung und dem Rest seiner Filmreihe darunter. Die einer Serie listet ihre Folgen nach Staffel, mit dem, was du hast und was du schon gesehen hast.

<table>
  <tr>
    <td width="50%"><img src="docs/images/find.jpg" alt="Finden: die Seite eines Films mit seinen Releases und dem Rest seiner Filmreihe"></td>
    <td width="50%"><img src="docs/images/series.jpg" alt="Finden: die Seite einer Serie mit ihren Folgen, gesehen und in deiner Bibliothek"></td>
  </tr>
  <tr>
    <td align="center"><sub>Ein Film in Finden</sub></td>
    <td align="center"><sub>Eine Serie in Finden</sub></td>
  </tr>
</table>

**HUB** ist die Bibliothek mit dem, was du schon hast: weiterschauen, weiterspielen und alles nach Spielen, Serien und Filmen sortiert.

<p align="center">
  <img src="docs/images/hub.jpg" alt="HUB: weiterschauen, weiterspielen und die Bibliothek" width="900">
</p>

## Der Player

Der eingebaute Player nutzt FFmpeg, MKV, AVI und WebM laufen also direkt. Er lädt den Anfang der Datei zuerst, sodass du schauen kannst, während der Rest noch kommt. Er merkt sich, wo du aufgehört hast, überspringt Intros und bietet am Ende die nächste Folge an, oder den nächsten Film einer Reihe. Er sucht Untertitel für dich (über SubDL) oder lädt die `.srt`, die neben der Datei liegt. Das Dekodieren läuft in einem separaten Prozess in einer Sandbox, damit eine kaputte Datei nicht die ganze App mitreißt.

<p align="center">
  <img src="docs/images/player.jpg" alt="Der eingebaute Player, mit der nächsten Folge in der Warteschlange" width="900">
</p>

Wenn du ein [Real-Debrid](https://real-debrid.com)- oder [TorBox](https://torbox.app)-Konto hast und der Torrent dort schon im Cache liegt, streamt er stattdessen von dort, und auf deinem Rechner wird nichts heruntergeladen oder geseedet.

## Was er sonst noch kann

- **Spiele.** Cover von IGDB, installieren und starten aus dem HUB, Spielzeit und Erfolge. Deine Spiele-Downloads sehen am Ende wie ein Launcher aus statt wie ein Ordner voller Setup-Dateien.
- **VPN.** Importiere eine WireGuard-`.conf` (Proton, Mullvad, IVPN...) und verbinde dich. Nur BATorrent läuft darüber; der Rest des Rechners behält seine normale Verbindung, und am System wird nichts installiert. Bricht die Verbindung ab, verbindet er sich von selbst neu, der Kill Switch stoppt die Torrents, bis sie wieder steht, und auf Servern, die es anbieten, öffnet er einen Port, damit Peers dich erreichen.
- **Privatsphäre.** Ein Leak-Test, der die IP zeigt, die deine Peers sehen, Bindung an eine Netzwerkschnittstelle, ein Modus für private Tracker, ein Tor-Preset und das Blockieren bekannter Leecher-Clients.
- **Hinzufügen.** Füge einen Magnet, einen Link oder einen Info-Hash überall mit Strg+V ein. RSS mit Regex-Filtern, ein überwachter Ordner, Drag and Drop und normale HTTP/HTTPS-Downloads.
- **Fernsteuerung.** Eine WebUI, die du auf dem Handy öffnest, indem du einen QR-Code scannst. Der Code wird auf deinem Rechner erzeugt.
- **Organisieren.** Kategorien und Tags, automatisches Entpacken von Archiven, Ratio- und Zeitlimits, ein Bandbreitenplan nach Stunde und Tag und eine alternative Geschwindigkeit (die Schildkröte) nur einen Klick entfernt.
- **Aussehen.** Sieben Themes, jedes mit eigener Schrift und eigenen Grafiken, oder ein eigenes; ein schlichter Hintergrund, die Grafik des Themes oder dein eigenes Bild. Eine Befehlspalette auf Strg/⌘+K. Neun Sprachen und eine kurze Einrichtung beim ersten Start.

<p align="center">
  <img src="docs/images/themes.jpg" alt="Der Tab Downloads in den sieben Themes: Default, Cinema, Light, Midnight, Sakura, Dark Star und Matrix" width="900">
</p>

<details>
<summary><b>Die vollständige Liste</b></summary>

Priorität pro Datei, sequenzieller Download, Tracker-Injektion, Steuerung des Inhaltslayouts, Regex für ausgeschlossene Dateien, ein separater Temp-Ordner, Seeding-Zeitfenster, automatisches Pausieren, wenn Dateien fehlen, Aktualisierung von Plex-/Jellyfin-/Emby-Bibliotheken, Benachrichtigungen über Telegram und Discord, Import aus qBittorrent, Erstellen von `.torrent`-Dateien, IP-Sperrlisten, Protokollverschlüsselung, ein Gitee-Update-Mirror, stabile und Beta-Update-Kanäle, Herunterfahren nach Abschluss der Downloads, ein Helfer für die Windows-Defender-Ausnahme, Sicherung und Wiederherstellung, ein Verlauf zuletzt entfernter Torrents, erzwungener Start, ein Log-Viewer mit Diagnose, eine Menüleiste auf Alt, Filme einer Reihe in der Bibliothek zusammengefasst und Tastenkürzel für fast alles.

</details>

## Die Engine

BATorrent bringt einen kleinen gepatchten Fork von libtorrent mit, angewendet als versionierte Patches unter [`third_party/patches/`](third_party/patches):

- **Schnellerer Anlauf.** Auf einer schnellen Leitung mit hoher Latenz vergrößert das originale libtorrent seine Request-Pipeline Schritt für Schritt. Der Fork vergrößert sie geometrisch und lastet die Leitung so mit deutlich weniger Round-Trips aus. In den A/B-Läufen des Projekts war das auf einer schnellen Verbindung etwa 27 % schneller und nie langsamer als das Original.
- **Peers aus demselben Land.** Eine Offline-GeoIP-Datenbank (db-ip Lite) ordnet jedem Peer ein Land zu, und wenn es eine Wahl gibt, bevorzugt der Fork Peers aus deinem eigenen Land, was meist eine geringere Latenz bedeutet.

Beides sind Compile-Time-Schalter und in einem Standard-Build ausgeschaltet.

## Installation

| Plattform | Download | Voraussetzungen |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Installer](https://batorrent.com/win) · [Portable](https://batorrent.com/portable) | Windows 10 oder neuer |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` oder die [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Sobald sie offen ist, zieh eine `.torrent`-Datei oder einen Magnet-Link ins Fenster.

<sub><b>macOS:</b> Die App ist noch nicht notarisiert (Apples Entwicklerprogramm ist ein kostenpflichtiges Abo). Homebrew ist der einfachste Weg, weil es das Quarantäne-Flag entfernt. Bei der <code>.dmg</code> klickst du beim ersten Mal mit der rechten Maustaste auf die App und wählst <b>Öffnen</b>.</sub>

<details>
<summary><b>Aus dem Quellcode bauen</b></summary>

**Voraussetzungen:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`), libtorrent-rasterbar 2.0+, Boost, zlib und optional Qt6Keychain.

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

- Eine Catch2-Suite (Unit, Sicherheit, Speicher) und die QML-Tests laufen bei jedem CI-Build.
- Der Build läuft sauber unter AddressSanitizer und UndefinedBehaviorSanitizer.
- Geheimnisse liegen im Schlüsselbund des Betriebssystems, und die WebUI öffnet sich erst für das Netzwerk, nachdem du ein Passwort gesetzt hast.

</details>

## Warum ich ihn gebaut habe

Ich bin ein einzelner Entwickler in Brasilien. Ich wollte einen Torrent-Client, der Privatsphäre ernst nimmt, nativ unter Windows, macOS und Linux läuft und nicht aussieht, als wäre er 2009 entstanden. Ich habe keinen gefunden, der mir gefiel, also habe ich ihn selbst geschrieben. Er steht unter der MIT-Lizenz: Sollte er je Werbung oder Tracking bekommen, könnte jeder ihn forken und ohne beides veröffentlichen.

## Mitmachen

Issues und Pull Requests sind willkommen. Für alles, was größer ist als ein kleiner Fix, öffne bitte zuerst ein Issue, damit wir uns auf den Ansatz einigen können. Bugreports helfen am meisten mit deiner Plattform, der Version (unter `Help → About`) und den Schritten zum Reproduzieren. Übersetzungen sind besonders willkommen. Wenn du KI-Tools nutzt, lies vorher, [wie ich KI nutze](USING_AI.md).

## Lizenz und Marke

Der **Code** steht unter [MIT](LICENSE), © 2024-2026 Mateus Cruz. Du darfst ihn forken und deinen eigenen Build veröffentlichen.

Der **Name "BATorrent" und das Logo** fallen nicht unter die Code-Lizenz. Wenn du einen Fork veröffentlichst, gib ihm bitte einen eigenen Namen, damit man erkennt, welcher Build der offizielle ist. Details in [TRADEMARK.md](TRADEMARK.md).

<p align="center"><sub>Gemacht in Brasilien.</sub></p>
