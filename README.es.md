<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <b>Español</b> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="120">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  Un cliente de torrent que muestra tus descargas como carátulas, las reproduce mientras se descargan<br>
  y reúne tus películas, series y juegos en una sola biblioteca.
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Descargas" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="Licencia" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Plataformas" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="#instalación"><b>Descargar</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com/assets/trailer.mp4"><b>Ver el tráiler</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com"><b>Sitio web</b></a>
</p>

<p align="center">
  <img src="docs/images/grid.jpg" alt="La pestaña Descargas en vista de cuadrícula, con una serie seleccionada y sus episodios a la derecha" width="900">
</p>

BATorrent es un cliente de torrent de escritorio construido sobre [libtorrent](https://www.libtorrent.org/), el motor que usan qBittorrent y Deluge. Lee el nombre de cada torrent, busca el póster que le corresponde (TMDB para películas y series, IGDB para juegos) y muestra tus descargas como carátulas en lugar de nombres de archivo. Los episodios de una misma serie se agrupan, el video se reproduce en la app antes de que termine la descarga, y los juegos se pueden instalar y ejecutar desde la misma ventana.

Es gratuito y de código abierto. Sin anuncios, sin cuenta, sin versión "Pro", sin analíticas. Aparte de los propios torrents, se conecta a internet para tres cosas: carátulas y detalles de TMDB, IGDB y Steam para lo que está en tu biblioteca o lo que abres en Encontrar; la comprobación de actualizaciones en GitHub, que puedes desactivar ([`updater.cpp`](src/services/integrations/updater.cpp)); y, en las compilaciones oficiales, un informe de fallo si la app se cierra inesperadamente (que también se puede desactivar).

## Tres pestañas

La app tiene tres pestañas arriba: **Descargas**, **Encontrar** y **HUB**.

### Descargas

Tus torrents como una cuadrícula de carátulas o una lista compacta, con los detalles de lo que selecciones al lado. Las series se agrupan en una fila con sus episodios dentro, así que doce episodios de una serie ocupan una línea en lugar de doce.

<p align="center">
  <img src="docs/images/list.jpg" alt="La pestaña Descargas en vista de lista, con una serie desplegada en sus episodios" width="900">
</p>

### Encontrar y HUB

**Encontrar** es donde buscas algo nuevo: una portada con las películas, series y juegos del momento, y una búsqueda que recorre tus fuentes y pone primero el mejor resultado. Cada título tiene su propia página. La de una película lista sus versiones, ordenadas por cuánta gente las comparte, con una recomendada y el resto de su colección debajo. La de una serie lista sus episodios por temporada, con lo que tienes y lo que ya viste.

<table>
  <tr>
    <td width="50%"><img src="docs/images/find.jpg" alt="Encontrar: la página de una película con sus versiones y el resto de su colección"></td>
    <td width="50%"><img src="docs/images/series.jpg" alt="Encontrar: la página de una serie con sus episodios, vistos y en tu biblioteca"></td>
  </tr>
  <tr>
    <td align="center"><sub>Una película en Encontrar</sub></td>
    <td align="center"><sub>Una serie en Encontrar</sub></td>
  </tr>
</table>

**HUB** es la biblioteca de lo que ya tienes: seguir viendo, seguir jugando, y todo agrupado en juegos, series y películas.

<p align="center">
  <img src="docs/images/hub.jpg" alt="HUB: seguir viendo, seguir jugando y la biblioteca" width="900">
</p>

## El reproductor

El reproductor integrado usa FFmpeg, así que MKV, AVI y WebM se reproducen directamente. Descarga primero el comienzo del archivo, así que puedes ver mientras llega el resto. Recuerda dónde te quedaste, salta las intros y al final ofrece el siguiente episodio, o la siguiente película de una colección. Busca subtítulos por ti (a través de SubDL) o carga el `.srt` que está junto al archivo. La decodificación corre en un proceso aparte y aislado, así que un archivo dañado no puede tumbar la app.

<p align="center">
  <img src="docs/images/player.jpg" alt="El reproductor integrado, con el siguiente episodio preparado" width="900">
</p>

Si tienes una cuenta de [Real-Debrid](https://real-debrid.com) o [TorBox](https://torbox.app) y el torrent ya está en su caché, lo transmite desde ahí, y no se descarga ni se comparte nada en tu equipo.

## Qué más hace

- **Juegos.** Carátulas de IGDB, instalar y ejecutar desde el HUB, tiempo de juego y logros. Tus descargas de juegos acaban pareciendo un launcher en vez de una carpeta de instaladores.
- **VPN.** Importa un `.conf` de WireGuard (Proton, Mullvad, IVPN...) y conéctate. Solo BATorrent pasa por ella; el resto del equipo mantiene su conexión normal, y no se instala nada en el sistema. Se reconecta sola si se cae, el kill switch detiene los torrents hasta que vuelva, y en los servidores que lo ofrecen abre un puerto para que los peers puedan llegar a ti.
- **Privacidad.** Una prueba de fugas que muestra la IP que ven tus peers, vinculación a una interfaz de red, un modo para trackers privados, un preset de Tor y bloqueo de clientes leecher conocidos.
- **Añadir cosas.** Pega un magnet, un enlace o un info-hash en cualquier parte con Ctrl+V. RSS con filtros regex, una carpeta vigilada, arrastrar y soltar, y descargas HTTP/HTTPS normales.
- **Control remoto.** Una WebUI que abres en el móvil escaneando un código QR. El código se genera en tu equipo.
- **Organización.** Categorías y etiquetas, extracción automática de archivos comprimidos, límites de ratio y de tiempo, un horario de ancho de banda por hora y día, y una velocidad alternativa (la tortuga) a un clic.
- **Apariencia.** Siete temas, cada uno con su propia tipografía y arte, o uno tuyo; un fondo limpio, el arte del tema o una imagen tuya. Una paleta de comandos con Ctrl/⌘+K. Nueve idiomas, y una configuración breve la primera vez que la abres.

<p align="center">
  <img src="docs/images/themes.jpg" alt="La pestaña Descargas en los siete temas: Default, Cinema, Light, Midnight, Sakura, Dark Star y Matrix" width="900">
</p>

<details>
<summary><b>La lista completa</b></summary>

Prioridad por archivo, descarga secuencial, inyección de trackers, control de la estructura del contenido, regex de archivos excluidos, una carpeta temporal aparte, ventanas de seeding, pausa automática cuando faltan archivos, actualización de bibliotecas de Plex/Jellyfin/Emby, notificaciones por Telegram y Discord, importación desde qBittorrent, creación de `.torrent`, listas de bloqueo de IP, cifrado del protocolo, un mirror de actualizaciones en Gitee, canales de actualización estable y beta, apagado al terminar las descargas, un asistente para la exclusión de Windows Defender, copia de seguridad y restauración, historial de eliminados recientes, inicio forzado, un visor de registros con diagnóstico, una barra de menús con Alt, las películas de una colección agrupadas en la biblioteca, y atajos de teclado para casi todo.

</details>

## El motor

BATorrent incluye un pequeño fork parcheado de libtorrent, aplicado como parches versionados en [`third_party/patches/`](third_party/patches):

- **Arranque más rápido.** En una conexión rápida con latencia alta, libtorrent original hace crecer su cola de peticiones de a un paso. El fork la hace crecer de forma geométrica, así que llena la conexión en muchos menos viajes de ida y vuelta. En las pruebas A/B del proyecto fue cerca de un 27% más rápido en una conexión rápida, y nunca más lento que el original.
- **Peers del mismo país.** Una base de datos GeoIP offline (db-ip Lite) etiqueta cada peer por país y, cuando hay opción, el fork prefiere peers de tu propio país, lo que normalmente significa menos latencia.

Las dos son opciones de compilación y están desactivadas en una compilación estándar.

## Instalación

| Plataforma | Descarga | Requisitos |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Instalador](https://batorrent.com/win) · [Portable](https://batorrent.com/portable) | Windows 10 o posterior |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` o el [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Una vez abierta, suelta un `.torrent` o un enlace magnet en la ventana.

<sub><b>macOS:</b> la app todavía no está notarizada (el programa de desarrolladores de Apple es una suscripción de pago). Homebrew es la vía más sencilla porque quita la marca de cuarentena. Con el <code>.dmg</code>, haz clic derecho en la app y elige <b>Abrir</b> la primera vez.</sub>

<details>
<summary><b>Compilar desde el código fuente</b></summary>

**Requisitos:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`), libtorrent-rasterbar 2.0+, Boost, zlib y, opcionalmente, Qt6Keychain.

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

- Una suite de Catch2 (unitarias, seguridad, memoria) y las pruebas de QML se ejecutan en cada build de CI.
- El build pasa con AddressSanitizer y UndefinedBehaviorSanitizer.
- Los secretos se guardan en el llavero del sistema, y la WebUI solo se abre a la red después de que definas una contraseña.

</details>

## Por qué lo hice

Soy un desarrollador solo, en Brasil. Quería un cliente de torrent que se tomara en serio la privacidad, funcionara de forma nativa en Windows, macOS y Linux, y no pareciera hecho en 2009. No encontré ninguno que me gustara, así que lo escribí. Tiene licencia MIT, así que si algún día incluyera anuncios o rastreo, cualquiera podría hacer un fork y distribuirlo sin ellos.

## Contribuir

Los issues y pull requests son bienvenidos. Para cualquier cosa más grande que un arreglo pequeño, abre primero un issue para que acordemos el enfoque. Los reportes de errores ayudan más con tu plataforma, la versión (en `Help → About`) y los pasos para reproducirlo. Las traducciones son especialmente bienvenidas. Si usas herramientas de IA, lee primero [cómo uso la IA](USING_AI.md).

## Licencia y marca

El **código** es [MIT](LICENSE), © 2024-2026 Mateus Cruz. Puedes hacer un fork y distribuir tu propia compilación.

El **nombre "BATorrent" y el logo** no están cubiertos por la licencia del código. Si distribuyes un fork, ponle un nombre propio para que la gente sepa cuál es la versión oficial. Detalles en [TRADEMARK.md](TRADEMARK.md).

<p align="center"><sub>Hecho en Brasil.</sub></p>
