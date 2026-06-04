🌐 [English](README.md) | [Português](README.pt-BR.md) | [中文](README.zh-CN.md) | [日本語](README.ja.md) | **Русский** | [Español](README.es.md) | [Deutsch](README.de.md)

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="160">
</p>

<h1 align="center">BATorrent</h1>

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

## Обзор

Современный кроссплатформенный BitTorrent клиент с акцентом на приватность, производительность и простоту. BATorrent сочетает проверенный движок libtorrent-rasterbar с тщательно настроенным интерфейсом на Qt 6, удалённым управлением через WebUI, автоматической загрузкой по RSS, поиском совместимым со Stremio, изоляцией трафика через VPN и встроенной интеграцией с медиасерверами.

> **Никакой телеметрии, аналитики и скрытых запросов.** Единственный исходящий запрос, который приложение выполняет без вашего участия — проверка обновлений на GitHub, которую можно отключить в Настройках. Убедитесь сами: [`src/app/updater.cpp`](src/app/updater.cpp).

![Главное окно — тёмная тема](src/images/image1.png)

![Главное окно — светлая тема](src/images/image2.png)

![Панель подробностей](src/images/image3.png)

![Диалог настроек](src/images/image4.png)

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Скачать

Готовые сборки для последнего релиза:

| Платформа | Формат | Примечания |
|---|---|---|
| Windows | [Установщик (`.exe`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) · [Портативная версия (`.zip`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Windows 10+ (x86_64) |
| macOS | **`brew install --cask Mateuscruz19/batorrent/batorrent`** (рекомендуется) · [Образ диска (`.dmg`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) | macOS 12+ (Apple Silicon) |
| Linux | [AppImage](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Glibc 2.35+ (x86_64) |

> **macOS — о предупреждении безопасности:** сборка пока не нотаризована (программа разработчика Apple платная — препятствие для проекта одного человека). **Homebrew — самый удобный путь:** `brew` снимает флаг карантина при установке, поэтому приложение просто открывается без окна Gatekeeper. Если вы скачали `.dmg`, при первом запуске щёлкните по приложению правой кнопкой и выберите **Открыть**, чтобы обойти предупреждение о «неустановленном разработчике».

Все артефакты создаются воркфлоу [Build & Release](.github/workflows/build.yml) при каждом теговом релизе.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Возможности

### Торренты
- Файлы `.torrent` и magnet-ссылки с сохранением данных докачки
- **Декодирование thunder://-ссылок** — на китайских форумах раздачи часто публикуются в формате Xunlei `thunder://`; BATorrent автоматически декодирует их через Умную вставку (Ctrl+V)
- **Умная вставка** — нажмите Ctrl+V с magnet-ссылкой, info hash или thunder://-ссылкой в буфере обмена, и приложение сразу предложит добавить загрузку
- **Инспектор торрентов** — предварительный просмотр файла `.torrent` (название, размер, файлы, трекеры, хеш, автор, флаг приватности) перед началом загрузки
- Приоритет файлов, последовательная загрузка, ручная перепроверка и переанонсирование
- Автоматическая инъекция трекеров из [ngosang/trackerslist](https://github.com/ngosang/trackerslist)
- Система тегов (произвольные, несколько тегов на торрент наряду с одной категорией)
- **Структура содержимого** — Оригинал, Создать подпапку или Без подпапки управляет расположением многофайловых торрентов на диске
- **Шаблоны исключения файлов** — regex-правила для автоматического пропуска файлов (например, `.nfo`, `.txt`, `sample`) при добавлении торрента
- **Временный путь загрузки** — сначала скачивает в промежуточную папку, автоматически перемещает в папку сохранения по завершении (защита от сканирования частичных файлов медиасервером)
- **Автораспаковка архивов** — автоматически распаковывает `.rar`/`.zip`/`.7z` по завершении, со списком паролей для защищённых архивов (использует 7-Zip или WinRAR на Windows, `unrar`/`unzip` на macOS/Linux)
- Категории, перетаскивание для изменения порядка, контекстное меню
- Импорт состояния из qBittorrent
- Создание новых файлов `.torrent` из любого файла или папки

### Управление состоянием
- Состояние **Завершён** — задаётся вручную или автоматически после настраиваемого периода раздачи (1, 3, 7, 14 или 30 дней). Отличается от статусов «Раздаёт»/«Финиширован», сохраняется между перезапусками.
- Кнопка **Остановить** замораживает завершённый торрент без его удаления; **Продолжить** снимает пометку и возвращает торрент в пул раздачи.
- Правила остановки раздачи: глобальный лимит рейтинга и максимальное время раздачи с переопределением на уровне отдельного торрента.
- **Автопауза при ошибках файлов** — если libtorrent не может прочитать файлы завершённого торрента (например, отключён внешний диск), торрент ставится на паузу вместо повторной загрузки.

### Поиск и обнаружение
- **Автозагрузка по RSS** с фильтрами на основе регулярных выражений, индивидуальными путями сохранения, расписанием обновлений и обнаружением дубликатов. Поддерживает magnet-ссылки, URL-адреса `.torrent` и теги `<enclosure>`.
- **Поиск совместимый со Stremio** для фильмов и сериалов через встроенные аддоны Cinemeta и Torrentio.

### Потоковое воспроизведение
- Воспроизведение во время загрузки — `.mp4`, `.mkv`, `.avi`, `.mov`, `.wmv`, `.flv`, `.webm`, `.m4v`, `.ts`.
- Автоматическое определение VLC и IINA, с откатом на системный проигрыватель по умолчанию.

### VPN и приватность
- **Привязка к интерфейсу** направляет весь торрент-трафик через выбранный сетевой интерфейс (например, `tun0`, `utun4`).
- **Kill switch** ставит на паузу все активные торренты в момент отключения привязанного интерфейса с возможностью автоматического возобновления при его восстановлении.
- **Режим PT** — одним переключателем обеспечивает совместимость с приватными трекерами: отключает DHT/PEX/LSD, принудительно использует анонимное рукопожатие, анонсирует на каждый уровень (tier). Обязателен для M-Team, TorrentLeech и большинства трекеров с учётом рейтинга.
- **Блокировка анти-личеров** — автоматическое обнаружение и бан клиентов Xunlei (Thunder), QQDownload, Baidu Netdisk P2P и других, которые скачивают, но не раздают. Определяется по префиксу peer_id в BitTorrent-рукопожатии.
- **Анонимный режим** — скрывает имя и версию клиента при рукопожатии, отключает UPnP/NAT-PMP.
- **Пресет для Tor** — один клик заполняет настройки SOCKS5 127.0.0.1:9050.
- **Принудительный IPv4** — отклоняет IPv6-пиров для предотвращения утечек по v6, когда VPN-туннель не покрывает IPv6.
- Определение VPN для WireGuard, Mullvad, NordLynx, ProtonVPN, а также универсальных tun/tap-интерфейсов.
- Поддержка прокси SOCKS5 и HTTP с аутентификацией.
- Поддержка списков блокировки IP (формат P2P).
- Шифрование протокола: включено / принудительное / отключено.

### WebUI
- Панель управления в браузере по адресу `http://localhost:8080` (порт и удалённый доступ настраиваются).
- **QR-код для сопряжения** — отсканируйте QR-код телефоном, чтобы мгновенно открыть WebUI без ввода IP-адреса. QR генерируется локально на чистом C++ (ваш LAN-адрес никогда не покидает машину).
- REST API с ответами в формате JSON; добавление по magnet-ссылке или загрузка файла `.torrent`; пауза/возобновление/удаление; просмотр файлов и пиров для каждого торрента.
- Basic Auth с SHA-256-хешированием, тёмный интерфейс в стиле основной темы, полностью адаптивная мобильная вёрстка.

### Полоса пропускания и расписание
- Независимые лимиты загрузки и отдачи.
- Альтернативный скоростной профиль с расписанием по часам и дням недели (поддержка ночных диапазонов).
- Лимиты рейтинга раздачи и времени раздачи: глобальные и индивидуальные для каждого торрента.

### Интеграция с медиасерверами
- Уведомляет **Plex**, **Jellyfin** или **Emby** о необходимости сканирования библиотеки при завершении загрузки.
- Настраиваемый URL и токен / API-ключ для каждого сервера.

### Уведомления и интеграции
- **Telegram-вебхук** — уведомления о завершении загрузки, срабатывании kill switch, автозагрузке по RSS и ошибках торрентов отправляются в любой чат Telegram через токен бота. Чекбоксы для каждого события + кнопка «Тест».
- **Discord Rich Presence** — показывает «Downloading X · 67%» в вашем профиле Discord с кнопками «Download BATorrent» и «View on GitHub». Работает из коробки.

### Интерфейс
- **Шесть тем** — Тёмная, Светлая (тёплая кремовая палитра «Comfortable»), Midnight, Sakura, Dark Star и полностью **Пользовательская** тема (собственное фоновое изображение + акцентные цвета), каждая с опциональным **аниме-акцентным артом**.
- **Автоматические обложки** — подгружает постеры фильмов/сериалов (TMDB) и арты игр (IGDB) по названию торрента для **сеточного просмотра** постеров; переключается на компактный список.
- График скорости в реальном времени, панель подробностей (Общее · Пиры · Файлы · Трекеры · Части), прогресс-бары с цветовой кодировкой состояния, уведомления в трее с фокусировкой по клику.
- Пользовательское всплывающее окно в трее (кроссплатформенное) с текущими скоростями, превью активных торрентов с расчётным временем, статусом VPN и кнопкой выхода.
- Фильтры-пиллы с подсчётом в реальном времени (Все / Активные / Загружаются / Раздаются / Завершены / На паузе / Финишировали / В очереди), строка поиска и фильтр по категориям.
- Перетаскивание как для файлов `.torrent`, так и для magnet-ссылок.
- **Семь языков интерфейса** с автоопределением: English, Português (BR), Español, Deutsch, Русский, 日本語, 中文 — 1 000+ переведённых строк с откатом на английский для отсутствующих ключей.
- Отображение скорости в байтах (KB/s, MB/s) или битах (Kbps, Mbps) — переключается в Настройках.
- Форматирование чисел с учётом локали (например, `1 234,5` для русской локали).

### Система
- Автообновление с настраиваемым источником: **GitHub** (по умолчанию), **Gitee** (зеркало для Китая) или отключено.
- Автовыключение компьютера при завершении всех загрузок (обратный отсчёт 60 секунд с возможностью отмены).
- **Исключение в Windows Defender** — один клик добавляет папку загрузок в список исключений Defender, чтобы он перестал помечать и сканировать скачанные файлы (с повышением прав через UAC).
- **Полное резервное копирование/восстановление** всех настроек и данных докачки в одном архиве для переноса между машинами.
- **История недавно удалённых** (последние 50 торрентов, восстановление в один клик).
- **Принудительный запуск** — обход лимита активных загрузок в очереди для отдельного торрента.
- Встроенный **просмотрщик логов** с ротацией файлов (5 МБ/файл, хранить 3), фильтром по уровню, экспортом для баг-репортов и CLI-флагом `--debug`.
- **Диалог диагностики** — проверка состояния (путь сохранения, порт, DHT, шифрование, VPN, блокировка личеров) + тест на утечку IP (через api.ipify.org).
- Аргументы CLI: при запуске можно передать любое количество путей к файлам `.torrent` или URI `magnet:`; последующие запуски перенаправляются на работающий экземпляр.
- Автоматическое отображение списка изменений при первом запуске после обновления версии.
- Горячие клавиши и диалог быстрой справки по `?`.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Начало работы

1. Скачайте сборку для вашей платформы со [страницы релизов](https://github.com/Mateuscruz19/BATorrent/releases/latest).
2. При первом запуске приветственный диалог поможет настроить путь сохранения, тему и язык.
3. Перетащите файл `.torrent` или magnet-ссылку на окно — или используйте **Файл → Открыть торрент** / **Файл → Добавить Magnet**.
4. По желанию: привяжите исходящий интерфейс в **Настройки → VPN** и включите kill switch перед добавлением конфиденциальных торрентов.

> **Совет по VPN:** когда включена **Привязка к интерфейсу**, все анонсы и соединения с пирами проходят только через выбранный интерфейс. Если интерфейс отключается, kill switch ставит всё на паузу до его восстановления.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Сборка из исходного кода

### Требования
- Компилятор C++17 (GCC 11+, Clang 14+ или MSVC 19.30+)
- CMake 3.16+
- Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`)
- libtorrent-rasterbar 2.0+
- Boost (транзитивная зависимость libtorrent)
- Опционально: Qt6Keychain (хранит учётные данные в системном хранилище ключей вместо открытого текста в QSettings)

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

Установите Qt 6 через официальный установщик и libtorrent через vcpkg:

```powershell
vcpkg install libtorrent:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

### Тесты

Набор тестов подключается опционально:

```bash
cmake -B build -DBAT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Структура проекта

```
src/
├── torrent/      обёртка libtorrent, данные докачки, очередь, правила раздачи
├── app/          переводчик, обновления, RSS, аддоны, хранилище секретов, GeoIP
├── gui/          интерфейс на Qt Widgets (главное окно, диалоги, панель подробностей, трей)
├── webui/        встроенный HTTP-сервер + браузерный интерфейс
└── main.cpp      инициализация единственного экземпляра, разбор CLI, оформление
.github/
└── workflows/    Linux AppImage, macOS DMG, Windows installer + zip
installer/        скрипт Inno Setup для установщика Windows
dist/             desktop-файл и ресурсы для упаковки под Linux
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Участие в разработке

Сообщения об ошибках и пул-реквесты приветствуются. Для значительных изменений, пожалуйста, сначала создайте issue для обсуждения подхода. Готовые сборки можно создать для любой ветки через воркфлоу **Build & Release** (`workflow_dispatch`).

При сообщении об ошибке приложите:
- Платформу + версию (`Помощь → О программе`)
- Шаги для воспроизведения
- Соответствующую секцию из `~/.local/share/BATorrent/` (Linux), `~/Library/Application Support/BATorrent/` (macOS) или `%APPDATA%\BATorrent\` (Windows), если проблема связана с данными докачки или настройками.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Лицензия

[MIT](LICENSE) © 2024–2026 Mateus Cruz
