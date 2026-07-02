<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <b>Español</b> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>El cliente BitTorrent con cara — carátulas de películas, seis temas, cero anuncios.</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>


<p align="center">
  <img src="src/images/shot-grid-v43.jpg" alt="BATorrent — ábrelo y listo, las carátulas se resuelven solas" width="860">
</p>

La mayoría de los clientes de torrent parecen un formulario de Hacienda. Este muestra tus descargas como un **muro de carátulas de películas, series y juegos** — lo mismo que reconoces en Netflix o Steam — y te deja vestirlo con seis temas (o tu propio fondo de pantalla). Por dentro lleva el probado motor **libtorrent**, así que no es un juguete bonito: es un cliente de verdad que da la casualidad de que tiene buen gusto.

> **Sin anuncios. Sin telemetría. Sin versión "Pro". Sin cuenta.** La única petición que hace por su cuenta es la comprobación de actualizaciones en GitHub, y puedes desactivarla. El código está aquí mismo — lee [`updater.cpp`](src/services/integrations/updater.cpp) y compruébalo tú mismo.


## Por qué existe

Soy un desarrollador solo en Brasil. Quería un cliente de torrent que se tomara en serio la privacidad, corriera de forma nativa en cualquier escritorio y no pareciera hecho en 2009 — y como no encontré ninguno, hice el mío. Es gratis y con **licencia MIT**: sin trampas, sin telemetría apareciendo más tarde y sin que puedan venderlo en silencio a una empresa que le añada anuncios. Ocho idiomas, porque "útil" no debería significar "solo en inglés".

## El aspecto

<p align="center">
  <img src="src/images/themes.gif" alt="Cambiando entre los temas integrados" width="860">
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

- **Carátulas automáticas** — lee el nombre del torrent y trae el póster real (películas y series vía TMDB, juegos vía IGDB) a una vista de cuadrícula. Un clic cambia a una lista compacta.
- **Seis temas** — Dark, Light, Midnight, Sakura, Dark Star y uno totalmente **Personalizado** (tu propio fondo + colores de acento), cada uno con arte de acento anime opcional.
- Gráfico de velocidad en tiempo real, progreso coloreado por estado, un popup en la bandeja con velocidades en vivo y ETA — los detalles que hacen que se *sienta* terminado.

## Lo que de verdad hace

| | |
|---|---|
| 🔒 **Privacidad primero** | Vinculación a la interfaz de la VPN + **kill switch** (corta todo el tráfico si el túnel cae), modo PT para trackers privados, preajuste Tor, handshake anónimo, bloqueo anti-leecher |
| 🔎 **Buscar y añadir** | Búsqueda integrada (incl. fuentes abiertas CIS/RuTor, sin login), Pegado Inteligente (magnet / `.torrent` / `thunder://` / hash con Ctrl+V), descarga automática por RSS con filtros regex, arrastrar y soltar |
| 📱 **Contrólalo desde cualquier sitio** | WebUI en el navegador con **emparejamiento por QR** — escanea desde el móvil, sin teclear IPs. El QR se genera localmente; tu dirección nunca sale de la máquina |
| 📺 **Ver y organizar** | Reproduce mientras descargas, extracción automática de archivos, categorías + etiquetas, actualización de la biblioteca Plex/Jellyfin/Emby al completar |
| 🔔 **Mantente al tanto** | Notificaciones nativas de escritorio, alertas de Telegram, Discord Rich Presence ("Descargando X · 67%") |

<details>
<summary><b>…y la cola larga</b> (haz clic para expandir)</summary>

Prioridad por archivo · descarga secuencial · inyección automática de trackers · control del diseño del contenido · regex de archivos excluidos · carpeta temporal de descarga · estado Completado con ventanas de seeding · auto-pausa ante errores de archivo · límites globales + por torrent de ratio/tiempo · programador de ancho de banda (hora + día) · importar desde qBittorrent · crear archivos `.torrent` · inspector de torrent · listas de bloqueo de IP · cifrado de protocolo · mirror de actualización Gitee · apagado automático al terminar · exclusión de Windows Defender · copia de seguridad/restauración completa · historial de eliminados recientemente · forzar inicio · visor de registro integrado + diagnósticos + prueba de fuga de IP · formato según la configuración regional · atajos de teclado.

</details>


## Conseguirlo

| Plataforma | | |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Instalador](https://github.com/BATorrent-app/BATorrent/releases/latest) · [Portátil](https://github.com/BATorrent-app/BATorrent/releases/latest) | Windows 10+ |
| **macOS** | **`brew install --cask Mateuscruz19/batorrent/batorrent`** · [`.dmg`](https://github.com/BATorrent-app/BATorrent/releases/latest) | macOS 12+ · Apple Silicon |
| **Linux** | [AppImage](https://github.com/BATorrent-app/BATorrent/releases/latest) | glibc 2.35+ |

Después solo arrastra un `.torrent` o un magnet a la ventana. Eso es todo.

<sub>**macOS:** todavía sin notarizar (el programa de desarrollador de Apple es de pago). Homebrew es la vía más suave — `brew` quita la marca de cuarentena, así que abre sin el aviso de Gatekeeper. Con el `.dmg`, clic derecho → **Abrir** la primera vez.</sub>


<details>
<summary><b>Compilar desde el código fuente y notas de ingeniería</b></summary>

### Requisitos
C++17 · CMake 3.16+ · Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`) · libtorrent-rasterbar 2.0+ · Boost · Qt6Keychain (opcional).

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```
(macOS: `brew install qt libtorrent-rasterbar boost openssl`. Windows: instalador de Qt + `vcpkg install libtorrent:x64-windows`.)

### Calidad y seguridad

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- **Pruebas** — suite Catch2 (unidad, seguridad, memoria) en cada build de CI; el nuevo comportamiento del backend lleva su prueba.
- **Sanitizers** — pasa limpio bajo AddressSanitizer + UndefinedBehaviorSanitizer (0 fugas / use-after-free / UB).
- **Revisado** antes de cada release en cuanto a seguridad de memoria/hilos, autenticación de la WebUI, inyección, path traversal, validación de entrada y manejo de secretos. Los secretos viven en el keychain del sistema, nunca en texto plano; la WebUI solo se abre a la red cuando defines una contraseña.

</details>

## Contribuir

Issues y PRs son bienvenidos — para cualquier cosa no trivial, abre una issue primero. Informes de error: incluye tu plataforma + versión (`Ayuda → Acerca de`) y los pasos para reproducirlo. Las traducciones se agradecen especialmente.

## Licencia

[MIT](LICENSE) © 2024–2026 Mateus Cruz · hecho en Brasil 🦇
