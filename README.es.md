<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <b>Español</b> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>Un cliente BitTorrent que muestra tus descargas como carátulas y no como filas de una hoja de cálculo.</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="https://batorrent.com/assets/trailer.mp4"><img src="src/images/trailer-poster.jpg" alt="Mira el tráiler de BATorrent (1:30)" width="860"></a>
</p>

BATorrent es un cliente de torrent de escritorio construido sobre el motor [libtorrent](https://www.libtorrent.org/), el mismo que usan qBittorrent y Deluge. La interfaz lee el nombre de cada torrent, busca el póster que le corresponde (películas y series en TMDB, juegos en IGDB) y organiza tus descargas en una cuadrícula de carátulas en lugar de una lista de nombres de archivo. Por debajo hay un cliente completo que corre sobre una [versión parcheada de ese motor](#el-motor).

Es gratuito y de código abierto, sin anuncios, telemetría, versión "Pro" ni cuenta. La única petición de red que hace por su cuenta es la comprobación de actualizaciones en GitHub, y hay una opción para desactivarla. Si quieres verificarlo, el código está en [`updater.cpp`](src/services/integrations/updater.cpp).

## Por qué lo hice

Soy un desarrollador solo, en Brasil. Quería un cliente de torrent que se tomara en serio la privacidad, funcionara de forma nativa en Windows, macOS y Linux, y no pareciera diseñado en 2009. No encontré ninguno que me gustara, así que escribí el mío. Tiene licencia MIT, así que si algún día el proyecto añadiera telemetría o anuncios, cualquiera podría hacer un fork del código y distribuirlo sin ellos. La interfaz está traducida a nueve idiomas.

## La interfaz

<p align="center">
  <img src="src/images/shot-list.jpg" alt="Vista de lista compacta, para cuando prefieres detalle a decoración" width="860">
</p>

<p align="center">
  <img src="src/images/shot-palette-v43.jpg" alt="Paleta de comandos (Ctrl/⌘+K): búsqueda difusa de cualquier torrent o acción" width="860">
</p>

<p align="center">
  <img src="src/images/shot-theme.jpg" alt="Sakura, uno de los temas integrados" width="860">
</p>

- **Carátulas.** Obtiene los pósteres a partir del nombre del torrent y los muestra en una cuadrícula. Con un clic cambias a una lista compacta cuando prefieres detalle a decoración.
- **Seis temas.** Dark, Light, Midnight, Sakura, Dark Star y un tema Custom en el que eliges tu propio fondo y color de acento. Todos admiten arte de acento anime opcional.
- **Paleta de comandos.** Ctrl/⌘+K abre un buscador difuso para cualquier torrent o acción: pausar todo, activar la velocidad alternativa, saltar a cualquier página. Nada de eso requiere el mouse.
- **Estado en vivo.** Un gráfico de velocidad en tiempo real, barras de progreso con colores según el estado y un popup en la bandeja con las velocidades actuales y el tiempo restante.

## Qué hace

**Míralo en la app.** Trae un reproductor de video integrado (basado en FFmpeg, así que reproduce MKV, AVI y WebM directamente) y puedes empezar a ver mientras el archivo todavía se descarga, porque primero baja el comienzo. Busca y descarga subtítulos por ti (a través de SubDL), carga automáticamente los archivos `.srt`/`.vtt` que están junto al video y te deja ajustar la sincronía en vivo. Al terminar, puede actualizar una biblioteca de Plex, Jellyfin o Emby.

**Reproducción instantánea con debrid.** Conecta una cuenta de [Real-Debrid](https://real-debrid.com) o [TorBox](https://torbox.app) y, cuando un magnet ya está en su caché, BATorrent desbloquea el enlace y lo transmite directamente al reproductor integrado, sin descargar ni compartir nada en tu equipo.

**También juegos.** Los torrents de juegos también reciben su carátula (a través de IGDB). Busca en catálogos de juegos, descarga, instala y ejecuta desde la propia app, para que tu biblioteca pirata se comporte un poco como una lista de Steam en vez de una carpeta llena de instaladores.

**Descubrir.** Una página de inicio navegable al estilo de Netflix (pósteres en tendencia, un destacado que va rotando) para encontrar algo que bajar sin salir de la app.

<p align="center">
  <img src="src/images/shot-discover.jpg" alt="Descubrir: una página de inicio navegable con pósteres en tendencia" width="860">
</p>

**Privacidad.** Vincula el tráfico a una interfaz de VPN específica, con un kill switch que corta todo si el túnel se cae. También tiene un modo para trackers privados, un preajuste para Tor, handshakes anónimos y bloqueo de clientes anti-leecher. Incluye una prueba de fuga de IP para confirmar que todo funciona.

**Buscar y añadir.** Búsqueda integrada (incluidas fuentes abiertas CIS/RuTor que no piden login), Smart Paste, que reconoce un magnet, un `.torrent`, un enlace `thunder://` o un info hash al pulsar Ctrl+V, descarga automática por RSS con filtros regex, una carpeta vigilada y arrastrar y soltar.

<p align="center">
  <img src="src/images/shot-search.jpg" alt="Búsqueda integrada: carátulas, calificaciones y el mejor resultado al instante" width="860">
</p>

**Control remoto.** Una WebUI en el navegador con emparejamiento por QR: escanea el código con el teléfono en lugar de escribir direcciones IP. El QR se genera en tu equipo y la dirección nunca sale de él.

**Organizar.** Extracción automática de archivos comprimidos al terminar, orden por categorías y etiquetas, límites de ratio y de tiempo globales y por torrent, y programación del ancho de banda por hora y día.

**Notificaciones.** Alertas nativas de escritorio, mensajes de Telegram y Discord Rich Presence.

<details>
<summary><b>Lista completa de funciones</b></summary>

Prioridad por archivo, descarga secuencial, inyección automática de trackers, control del diseño del contenido, regex para excluir archivos, ruta temporal de descarga separada, un estado de completado con ventanas de seeding, pausa automática ante errores de archivo, límites de ratio y de tiempo globales y por torrent, un programador de ancho de banda por hora y día, importación desde qBittorrent, creación de archivos `.torrent`, un inspector de torrents, listas de bloqueo de IP, cifrado del protocolo, un mirror de actualizaciones en Gitee, apagado automático al terminar las descargas, un asistente para la exclusión en Windows Defender, copia de seguridad y restauración completas, historial de eliminados recientemente, inicio forzado, un visor de registros integrado con diagnósticos y prueba de fuga de IP, formato según la configuración regional y atajos de teclado.

</details>

## El motor

La mayoría de las apps de torrent enlazan libtorrent tal cual. BATorrent incluye un pequeño fork parcheado, lo que le permite cambiar comportamientos del motor a los que la API pública no llega:

- **Arranque más rápido del pipeline.** En un enlace de mucho ancho de banda y alta latencia, el pipeline de peticiones estándar crece de a un paso; el fork lo hace crecer de forma geométrica, así que llena una conexión amplia en una fracción de las idas y vueltas. En el benchmark A/B del propio proyecto midió alrededor de +27% en un enlace rápido, sin los atascos que la versión estándar tiene de una ejecución a otra, y nunca rinde peor.
- **Preferencia por peers del mismo país.** Una base de datos GeoIP sin conexión (db-ip Lite) etiqueta cada peer por país, y el ranking de peers del fork prefiere los de tu propio país cuando puede elegir, lo que suele significar menos latencia y menos rutas internacionales con throttling.

Ambas son funciones del fork que se activan al compilar, están desactivadas en un build estándar y se aplican como parches versionados en [`third_party/patches/`](third_party/patches) en lugar de una copia incluida en el repositorio.

## Instalación

| Plataforma | Descarga | Requisitos |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6), [Instalador](https://batorrent.com/win) o [Portátil](https://batorrent.com/portable) | Windows 10 o posterior |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` o el [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Con el programa abierto, arrastra un archivo `.torrent` o un enlace magnet a la ventana.

<sub><b>Nota sobre macOS:</b> la app todavía no está notarizada (el programa de desarrolladores de Apple es una suscripción de pago). Homebrew es la vía más sencilla porque <code>brew</code> quita la marca de cuarentena, así que se abre sin el aviso de Gatekeeper. Si usas el <code>.dmg</code>, la primera vez haz clic derecho en la app y elige <b>Abrir</b>.</sub>

<details>
<summary><b>Compilar desde el código fuente</b></summary>

**Requisitos:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`), libtorrent-rasterbar 2.0+, Boost y, de forma opcional, Qt6Keychain.

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```

En macOS: `brew install qt libtorrent-rasterbar boost openssl`.
En Windows: el instalador de Qt más `vcpkg install libtorrent:x64-windows`.

</details>

<details>
<summary><b>Calidad y seguridad</b></summary>

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- Una suite de pruebas Catch2 (unitarias, de seguridad y de memoria) se ejecuta en cada build de CI; todo comportamiento nuevo del backend llega con su prueba.
- El build pasa limpio con AddressSanitizer y UndefinedBehaviorSanitizer.
- Antes de cada release se revisa el código en cuanto a seguridad de memoria y de hilos, autenticación de la WebUI, inyección, path traversal, validación de entradas y manejo de secretos. Los secretos se guardan en el llavero del sistema y no en texto plano, y la WebUI solo se abre a la red después de que defines una contraseña.

</details>

## Contribuir

Los issues y pull requests son bienvenidos. Para cualquier cambio no trivial, abre primero un issue para que acordemos el enfoque. Los reportes de errores son más útiles si incluyen tu plataforma y versión (en `Ayuda → Acerca de`) y los pasos para reproducir el problema. Las traducciones se agradecen especialmente.

## Licencia y marca

El **código** está bajo licencia [MIT](LICENSE), © 2024-2026 Mateus Cruz. Puedes hacer un fork y distribuir tu propio build.

El **nombre "BATorrent" y el logo** pertenecen al proyecto y no están cubiertos por la licencia del código. Si redistribuyes un fork, por favor ponle un nombre propio para que los usuarios sepan cuál es el build oficial. Los detalles están en [TRADEMARK.md](TRADEMARK.md). Los forks y contribuciones de buena fe son bienvenidos.

Hecho en Brasil.
