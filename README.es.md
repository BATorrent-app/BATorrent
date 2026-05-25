🌐 [English](README.md) | [Português](README.pt-BR.md) | [中文](README.zh-CN.md) | [日本語](README.ja.md) | [Русский](README.ru.md) | **[Español](README.es.md)** | [Deutsch](README.de.md)

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="160">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <em>Un cliente BitTorrent moderno y multiplataforma enfocado en privacidad, rendimiento y claridad</em>
</p>

<p align="center">
  <a href="https://github.com/Mateuscruz19/BAT-Torrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/Mateuscruz19/BAT-Torrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/Mateuscruz19/BAT-Torrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/Mateuscruz19/BAT-Torrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/Mateuscruz19/BAT-Torrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/platform-Windows%20%7C%20macOS%20%7C%20Linux-dc2626?style=flat-square">
  <img alt="C++" src="https://img.shields.io/badge/C%2B%2B-17-dc2626?style=flat-square&logo=c%2B%2B">
  <img alt="Qt" src="https://img.shields.io/badge/Qt-6-dc2626?style=flat-square&logo=qt">
</p>

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Descripción general

BATorrent es un cliente BitTorrent de escritorio que prioriza la claridad, el rendimiento y la privacidad. Combina el motor maduro libtorrent-rasterbar con una interfaz Qt 6 cuidadosamente ajustada, una WebUI de control remoto, descarga automática por RSS, búsqueda compatible con Stremio, aislamiento de tráfico con reconocimiento de VPN e integración incorporada con servidores de medios.

> **Sin telemetría, sin analíticas, sin conexiones ocultas.** La única solicitud saliente que la aplicación inicia sin tu intervención es la verificación de actualizaciones en GitHub, que puede desactivarse en Configuración. Revísalo tú mismo en [`src/app/updater.cpp`](src/app/updater.cpp).

![Ventana principal — Tema oscuro](src/images/image1.png)

![Ventana principal — Tema claro](src/images/image2.png)

![Panel de detalles](src/images/image3.png)

![Diálogo de configuración](src/images/image4.png)

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Descarga

Binarios precompilados para la última versión:

| Plataforma | Formato | Notas |
|---|---|---|
| Windows | [Instalador (`.exe`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) · [Portable (`.zip`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | Windows 10+ (x86_64) |
| macOS | [Imagen de disco (`.dmg`)](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | macOS 12+ (Apple Silicon) |
| Linux | [AppImage](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest) | Glibc 2.35+ (x86_64) |

Todos los artefactos son producidos por el flujo de trabajo [Build & Release](.github/workflows/build.yml) en cada release etiquetado.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Características

### Torrents
- Archivos `.torrent` y enlaces magnet con datos de reanudación persistentes
- **Decodificación de enlaces Thunder://** — los foros chinos comparten enlaces en el formato `thunder://` de Xunlei; BATorrent los decodifica automáticamente mediante Smart Paste (Ctrl+V)
- **Smart Paste** — Ctrl+V con un enlace magnet, info hash o enlace thunder:// en el portapapeles solicita agregarlo al instante
- **Inspector de torrents** — previsualiza un archivo `.torrent` (nombre, tamaño, archivos, trackers, hash, creador, bandera privada) antes de confirmar la descarga
- Prioridad por archivo, descarga secuencial, reverificación manual y reanuncio
- Inyección automática de trackers desde [ngosang/trackerslist](https://github.com/ngosang/trackerslist)
- Sistema de etiquetas múltiples (texto libre, varias etiquetas por torrent junto con una sola categoría)
- **Disposicion del contenido** — Original, Crear subcarpeta o Sin subcarpeta controla como se organizan los torrents multi-archivo en disco
- **Patrones de exclusion de archivos** — reglas regex para omitir archivos automaticamente (ej: `.nfo`, `.txt`, `sample`) al agregar un torrent
- **Ruta temporal de descarga** — descarga primero a una carpeta intermedia, mueve automaticamente al destino al completar (evita escaneo de parciales por servidores de medios)
- Categorías, reorden por arrastrar y soltar, y acciones contextuales con clic derecho
- Importar estado existente desde qBittorrent
- Crear nuevos archivos `.torrent` desde cualquier archivo o carpeta

### Gestión de estado
- Estado **Completado** — marcado manualmente o promovido automáticamente después de un periodo de siembra configurable (1, 3, 7, 14 o 30 días). Distinto de Sembrando/Finalizado, persistente entre reinicios.
- **Botón Detener** que congela un torrent finalizado sin eliminarlo; **Reanudar** desmarca y reingresa al grupo de siembra.
- Reglas de detención de siembra: límite de ratio global y tiempo máximo de siembra, con anulaciones por torrent.
- **Pausa automática en errores de archivo** — si libtorrent no puede leer los archivos de un torrent finalizado (p. ej. disco externo desconectado), lo pausa en lugar de volver a descargar silenciosamente.

### Descubrimiento
- **Descarga automática por RSS** con filtros regex, rutas de guardado por feed, programación de intervalos y detección de duplicados. Maneja enlaces magnet, URLs de `.torrent` y etiquetas `<enclosure>`.
- **Búsqueda compatible con Stremio** para películas y series a través de los addons integrados Cinemeta y Torrentio.

### Streaming
- Reproduce mientras descarga — `.mp4`, `.mkv`, `.avi`, `.mov`, `.wmv`, `.flv`, `.webm`, `.m4v`, `.ts`.
- Detecta automáticamente VLC e IINA, recurre al reproductor predeterminado del sistema.

### VPN y privacidad
- **Enlace de interfaz** bloquea todo el tráfico torrent a una interfaz de red elegida (p. ej. `tun0`, `utun4`).
- **Kill switch** pausa cada torrent activo en el momento en que la interfaz enlazada cae, con reanudación automática opcional cuando regresa.
- **Modo PT** — cumplimiento de trackers privados con un solo toggle: desactiva DHT/PEX/LSD, fuerza handshake anónimo, anuncia en cada nivel. Requerido por M-Team, TorrentLeach y la mayoría de comunidades con control de ratio.
- **Bloqueo anti-leecher** — detecta y banea automáticamente Xunlei (Thunder), QQDownload, Baidu Netdisk P2P y otros clientes que descargan sin devolver siembra. Detectados por el prefijo peer_id en el handshake BitTorrent.
- **Modo anónimo** — oculta el nombre/versión del cliente en los handshakes, desactiva los anuncios UPnP/NAT-PMP.
- **Preset de proxy Tor** — un clic rellena SOCKS5 127.0.0.1:9050.
- **Forzar IPv4** — rechaza pares IPv6 para prevenir fugas v6 cuando el túnel no lo cubre.
- Detección de VPN para WireGuard, Mullvad, NordLynx, ProtonVPN, tun/tap genérico.
- Proxy SOCKS5 y HTTP con autenticación.
- Soporte de lista de bloqueo de IP (formato P2P).
- Cifrado de protocolo: activado / forzado / desactivado.

### WebUI
- Panel de control basado en navegador en `http://localhost:8080` (puerto y acceso remoto configurables).
- **Vinculación por código QR** — escanea un QR desde tu teléfono para abrir la WebUI al instante, sin escribir IPs. El QR se genera localmente en C++ puro (tu dirección LAN nunca sale de la máquina).
- API REST con respuestas JSON; agregar por magnet o subir `.torrent`; pausar / reanudar / eliminar; vistas de archivos y pares por torrent.
- Auth básica con hash SHA-256, interfaz oscura acorde al tema, diseño responsive completo para móviles.

### Ancho de banda y programación
- Límites independientes de descarga y subida.
- Perfil de velocidad alternativo con programación por hora del día + día de la semana (rangos nocturnos soportados).
- Límites de ratio y tiempo de siembra por torrent y globales.

### Integración con servidores de medios
- Notifica a **Plex**, **Jellyfin** o **Emby** para escanear la biblioteca cuando se completa una descarga.
- URL y token / clave API configurables por servidor.

### Notificaciones e integraciones
- **Webhook de Telegram** — descarga completada, kill switch activado, descarga automática RSS y errores de torrent enviados a cualquier chat de Telegram mediante token de bot. Casillas por evento + botón de prueba.
- **Discord Rich Presence** — muestra "Downloading X · 67%" en tu perfil de Discord con botones "Download BATorrent" y "View on GitHub". Funciona de inmediato.

### Interfaz
- Tres temas — Oscuro, Claro (paleta cálida tipo crema "Comfortable") y Midnight — con un override global de QPalette para que los widgets simples sigan el tema activo.
- Gráfico de velocidad en tiempo real, panel de detalles (General · Pares · Archivos · Trackers · Piezas), barras de progreso coloreadas por estado, notificaciones de bandeja con foco al hacer clic.
- Popup de bandeja personalizado (multiplataforma) con velocidades en vivo, vista previa de torrents activos con ETA, estado de VPN y opción de salir.
- Filtros con conteo en vivo (Todos / Activos / Descargando / Sembrando / Completados / Pausados / Finalizados / En cola), barra de búsqueda y filtro por categoría.
- Arrastrar y soltar para archivos `.torrent` y enlaces magnet.
- **Siete idiomas de interfaz** con detección automática: English, Português (BR), Español, Deutsch, Русский, 日本語, 中文 — 630+ cadenas traducidas con respaldo en inglés para claves faltantes.
- Visualización de velocidad en bytes (KB/s, MB/s) o bits (Kbps, Mbps) — configurable en Configuración.
- Formato numérico adaptado al locale (p. ej. `1 234,5` para el locale ruso).

### Sistema
- Actualización automática con fuente configurable: **GitHub** (predeterminado) o **Gitee** (espejo para China) o desactivado.
- Apagado automático cuando todas las descargas se completan (cuenta regresiva cancelable de 60 s).
- **Respaldo y restauración completa** de toda la configuración + datos de reanudación en un solo archivo para migración entre equipos.
- **Historial de eliminados recientes** (últimos 50 torrents, restauración con un clic).
- **Inicio forzado** — omite el límite de descargas activas en cola para un solo torrent.
- **Visor de registros** integrado con rotación de archivos (5 MB/archivo, mantiene 3), filtro por nivel, exportación para reportes de errores y flag CLI `--debug`.
- **Diálogo de diagnósticos** — verificación de salud (ruta de guardado, puerto, DHT, cifrado, VPN, bloqueo de leechers) + prueba de fuga de IP saliente (vía api.ipify.org).
- Argumentos CLI: pasa cualquier cantidad de rutas `.torrent` o URIs `magnet:` al inicio; las ejecuciones posteriores reenvían a la instancia en ejecución.
- Popup automático de changelog en el primer inicio tras una actualización de versión.
- Atajos de teclado y diálogo de referencia rápida con `?`.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Primeros pasos

1. Descarga la versión para tu plataforma desde la [página de releases](https://github.com/Mateuscruz19/BAT-Torrent/releases/latest).
2. En el primer inicio, el diálogo de bienvenida te guía por la ruta de guardado predeterminada, el tema y el idioma.
3. Arrastra un archivo `.torrent` o un enlace magnet a la ventana — o usa **Archivo → Abrir Torrent** / **Archivo → Agregar Magnet**.
4. Opcional: enlaza la interfaz de salida en **Configuración → VPN** y activa el kill switch antes de agregar torrents sensibles.

> **Tip de VPN:** cuando el **Enlace de interfaz** está configurado, cada anuncio y conexión de pares sale exclusivamente por esa interfaz. Si la interfaz cae, el kill switch pausa todo hasta que regrese.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Compilar desde el código fuente

### Requisitos
- Toolchain C++17 (GCC 11+, Clang 14+ o MSVC 19.30+)
- CMake 3.16+
- Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`)
- libtorrent-rasterbar 2.0+
- Boost (dependencia transitiva de libtorrent)
- Opcional: Qt6Keychain (almacena credenciales en el keyring del SO en lugar de QSettings en texto plano)

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

Instala Qt 6 mediante el instalador oficial y libtorrent mediante vcpkg:

```powershell
vcpkg install libtorrent:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

### Pruebas

La suite de pruebas es opcional:

```bash
cmake -B build -DBAT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Estructura del proyecto

```
src/
├── torrent/      wrapper de libtorrent, datos de reanudación, cola, reglas de siembra
├── app/          traductor, actualizador, RSS, addons, almacén de secretos, GeoIP
├── gui/          interfaz Qt Widgets (ventana principal, diálogos, detalles, popup de bandeja)
├── webui/        servidor HTTP integrado + interfaz de navegador
└── main.cpp      bootstrap de instancia única, parseo de CLI, temas
.github/
└── workflows/    Linux AppImage, macOS DMG, Windows installer + zip
installer/        script de Inno Setup para el instalador de Windows
dist/             archivo desktop + assets para empaquetado en Linux
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Contribuir

Se aceptan issues y pull requests. Para cambios significativos, por favor abre un issue primero para discutir el enfoque. Los artefactos precompilados pueden ser producidos para cualquier rama mediante el flujo de trabajo **Build & Release** (`workflow_dispatch`).

Al reportar un error, adjunta:
- Plataforma + versión (`Ayuda → Acerca de`)
- Pasos para reproducir
- La sección relevante de `~/.local/share/BATorrent/` (Linux), `~/Library/Application Support/BATorrent/` (macOS) o `%APPDATA%\BATorrent\` (Windows) si involucra datos de reanudación o configuración.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Licencia

[MIT](LICENSE) © 2024–2026 Mateus Cruz
