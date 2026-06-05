🌐 [English](README.md) | [Português](README.pt-BR.md) | [中文](README.zh-CN.md) | [日本語](README.ja.md) | [Русский](README.ru.md) | [Español](README.es.md) | [Deutsch](README.de.md) | **Українська**

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

## Огляд

Сучасний кросплатформний BitTorrent-клієнт з акцентом на конфіденційність, продуктивність і простоту. BATorrent поєднує перевірений рушій libtorrent-rasterbar з ретельно налаштованим інтерфейсом на Qt 6, віддаленим керуванням через WebUI, автоматичним завантаженням по RSS, пошуком сумісним зі Stremio, ізоляцією трафіку через VPN та вбудованою інтеграцією з медіасерверами.

> **Жодної телеметрії, аналітики і прихованих запитів.** Єдиний вихідний запит, який програма виконує без вашої участі — перевірка оновлень на GitHub, яку можна вимкнути в Налаштуваннях. Переконайтеся самі: [`src/app/updater.cpp`](src/app/updater.cpp).

![Головне вікно — темна тема](src/images/image1.png)

![Головне вікно — світла тема](src/images/image2.png)

![Панель подробиць](src/images/image3.png)

![Діалог налаштувань](src/images/image4.png)

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Завантажити

Готові збірки для останнього релізу:

| Платформа | Формат | Примітки |
|---|---|---|
| Windows | [Інсталятор (`.exe`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) · [Портативна версія (`.zip`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Windows 10+ (x86_64) |
| macOS | **`brew install --cask Mateuscruz19/batorrent/batorrent`** (рекомендується) · [Образ диска (`.dmg`)](https://github.com/Mateuscruz19/BATorrent/releases/latest) | macOS 12+ (Apple Silicon) |
| Linux | [AppImage](https://github.com/Mateuscruz19/BATorrent/releases/latest) | Glibc 2.35+ (x86_64) |

> **macOS — про попередження безпеки:** збірка поки що не нотаризована (програма розробника Apple платна — перешкода для проекту однієї людини). **Homebrew — найзручніший шлях:** `brew` знімає прапорець карантину при встановленні, тому програма просто відкривається без вікна Gatekeeper. Якщо ви завантажили `.dmg`, при першому запуску клацніть по програмі правою кнопкою миші й виберіть **Відкрити**, щоб обійти попередження про «невстановленого розробника».

Всі артефакти створюються воркфлоу [Build & Release](.github/workflows/build.yml) при кожному теговому релізі.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Можливості

### Торренти
- Файли `.torrent` і magnet-посилання зі збереженням даних докачки
- **Декодування thunder://-посилань** — на китайських форумах роздачі часто публікуються у форматі Xunlei `thunder://`; BATorrent автоматично декодує їх через Розумну вставку (Ctrl+V)
- **Розумна вставка** — натисніть Ctrl+V з magnet-посиланням, info hash або thunder://-посиланням у буфері обміну, і програма одразу запропонує додати завантаження
- **Інспектор торрентів** — попередній перегляд файлу `.torrent` (назва, розмір, файли, трекери, хеш, автор, прапорець приватності) перед початком завантаження
- Пріоритет файлів, послідовне завантаження, ручна перевірка і переоголошення
- Автоматична ін'єкція трекерів з [ngosang/trackerslist](https://github.com/ngosang/trackerslist)
- Система тегів (довільні, кілька тегів на торрент поряд з однією категорією)
- **Структура вмісту** — Оригінальна, Створити підпапку або Без підпапки керує розташуванням багатофайлових торрентів на диску
- **Шаблони виключення файлів** — regex-правила для автоматичного пропуску файлів (наприклад, `.nfo`, `.txt`, `sample`) при додаванні торренту
- **Тимчасовий шлях завантаження** — спочатку завантажує до проміжної папки, автоматично переміщає до папки збереження після завершення (захист від сканування часткових файлів медіасервером)
- **Авторозпакування архівів** — автоматично розпаковує `.rar`/`.zip`/`.7z` після завершення, зі списком паролів для захищених архівів (використовує 7-Zip або WinRAR на Windows, `unrar`/`unzip` на macOS/Linux)
- Категорії, перетягування для зміни порядку, контекстне меню
- Імпорт стану з qBittorrent
- Створення нових файлів `.torrent` з будь-якого файлу або папки

### Керування станом
- Стан **Завершено** — задається вручну або автоматично після налаштовуваного періоду роздачі (1, 3, 7, 14 або 30 днів). Відрізняється від статусів «Роздача»/«Готово», зберігається між перезапусками.
- Кнопка **Зупинити** заморожує завершений торрент без його видалення; **Продовжити** знімає позначку і повертає торрент до пулу роздачі.
- Правила зупинки роздачі: глобальний ліміт рейтингу і максимальний час роздачі з можливістю перевизначення для окремого торренту.
- **Автопауза при помилках файлів** — якщо libtorrent не може прочитати файли завершеного торренту (наприклад, відключено зовнішній диск), торрент ставиться на паузу замість повторного завантаження.

### Пошук і виявлення
- **Автозавантаження по RSS** з фільтрами на основі регулярних виразів, індивідуальними шляхами збереження, розкладом оновлень і виявленням дублікатів. Підтримує magnet-посилання, URL-адреси `.torrent` і теги `<enclosure>`.
- **Пошук сумісний зі Stremio** для фільмів і серіалів через вбудовані додатки Cinemeta і Torrentio.

### Потокове відтворення
- Відтворення під час завантаження — `.mp4`, `.mkv`, `.avi`, `.mov`, `.wmv`, `.flv`, `.webm`, `.m4v`, `.ts`.
- Автоматичне визначення VLC і IINA, з відкатом на системний програвач за замовчуванням.

### VPN і конфіденційність
- **Прив'язка до інтерфейсу** спрямовує весь торрент-трафік через вибраний мережевий інтерфейс (наприклад, `tun0`, `utun4`).
- **Аварійне відключення** ставить на паузу всі активні торренти в момент відключення прив'язаного інтерфейсу з можливістю автоматичного поновлення при його відновленні.
- **Режим PT** — одним перемикачем забезпечує сумісність з приватними трекерами: вимикає DHT/PEX/LSD, примусово використовує анонімне рукостискання, оголошує на кожен рівень (tier). Обов'язковий для M-Team, TorrentLeech і більшості трекерів з обліком рейтингу.
- **Блокування антиличерів** — автоматичне виявлення та бан клієнтів Xunlei (Thunder), QQDownload, Baidu Netdisk P2P та інших, які завантажують, але не роздають. Визначається за префіксом peer_id у BitTorrent-рукостисканні.
- **Анонімний режим** — приховує ім'я і версію клієнта при рукостисканні, вимикає UPnP/NAT-PMP.
- **Пресет для Tor** — один клік заповнює налаштування SOCKS5 127.0.0.1:9050.
- **Примусовий IPv4** — відхиляє IPv6-піри для запобігання витокам по v6, коли VPN-тунель не покриває IPv6.
- Визначення VPN для WireGuard, Mullvad, NordLynx, ProtonVPN, а також універсальних tun/tap-інтерфейсів.
- Підтримка проксі SOCKS5 і HTTP з аутентифікацією.
- Підтримка списків блокування IP (формат P2P).
- Шифрування протоколу: увімкнено / примусово / вимкнено.

### WebUI
- Панель керування в браузері за адресою `http://localhost:8080` (порт і віддалений доступ налаштовуються).
- **QR-код для сполучення** — відскануйте QR-код телефоном, щоб миттєво відкрити WebUI без введення IP-адреси. QR генерується локально на чистому C++ (ваша LAN-адреса ніколи не залишає машину).
- REST API з відповідями у форматі JSON; додавання по magnet-посиланню або завантаження файлу `.torrent`; пауза/поновлення/видалення; перегляд файлів і пірів для кожного торренту.
- Basic Auth з SHA-256-хешуванням, темний інтерфейс у стилі основної теми, повністю адаптивне мобільне верстання.

### Пропускна здатність і розклад
- Незалежні ліміти завантаження і роздачі.
- Альтернативний швидкісний профіль з розкладом по годинах і днях тижня (підтримка нічних діапазонів).
- Ліміти рейтингу роздачі і часу роздачі: глобальні та індивідуальні для кожного торренту.

### Інтеграція з медіасерверами
- Сповіщає **Plex**, **Jellyfin** або **Emby** про необхідність сканування бібліотеки після завершення завантаження.
- Налаштовуваний URL і токен / API-ключ для кожного сервера.

### Сповіщення та інтеграції
- **Telegram-вебхук** — сповіщення про завершення завантаження, спрацювання аварійного відключення, автозавантаження по RSS і помилки торрентів надсилаються в будь-який чат Telegram через токен бота. Прапорці для кожної події + кнопка «Тест».
- **Discord Rich Presence** — показує «Downloading X · 67%» у вашому профілі Discord з кнопками «Download BATorrent» і «View on GitHub». Працює з коробки.

### Інтерфейс
- **Шість тем** — Темна, Світла (тепла кремова палітра «Comfortable»), Midnight, Sakura, Dark Star і повністю **Власна** тема (власне фонове зображення + акцентні кольори), кожна з опціональним **аніме-акцентним артом**.
- **Автоматичні обкладинки** — підвантажує постери фільмів/серіалів (TMDB) і арти ігор (IGDB) за назвою торренту для **сіткового перегляду** постерів; перемикається на компактний список.
- Графік швидкості в реальному часі, панель подробиць (Загальне · Піри · Файли · Трекери · Частини), прогрес-бари з кольоровим кодуванням стану, сповіщення в треї з фокусуванням по кліку.
- Власне спливаюче вікно в треї (кросплатформне) з поточними швидкостями, прев'ю активних торрентів з розрахунковим часом, статусом VPN і кнопкою виходу.
- Фільтри-пілюлі з підрахунком у реальному часі (Всі / Активні / Завантаження / Роздача / Завершені / На паузі / Готові / В черзі), рядок пошуку і фільтр по категоріях.
- Перетягування як для файлів `.torrent`, так і для magnet-посилань.
- **Вісім мов інтерфейсу** з автовизначенням: English, Português (BR), Español, Deutsch, Русский, 日本語, 中文, Українська — 1 000+ перекладених рядків з відкатом на англійську для відсутніх ключів.
- Відображення швидкості в байтах (КБ/с, МБ/с) або бітах (Кбіт/с, Мбіт/с) — перемикається в Налаштуваннях.
- Форматування чисел з урахуванням локалі.

### Система
- Автооновлення з налаштовуваним джерелом: **GitHub** (за замовчуванням), **Gitee** (дзеркало для Китаю) або вимкнено.
- Автовимкнення комп'ютера після завершення всіх завантажень (зворотний відлік 60 секунд з можливістю скасування).
- **Виключення в Windows Defender** — один клік додає папку завантажень до списку виключень Defender, щоб він перестав позначати і сканувати завантажені файли (з підвищенням прав через UAC).
- **Повне резервне копіювання/відновлення** всіх налаштувань і даних докачки в одному архіві для перенесення між машинами.
- **Історія нещодавно видалених** (останні 50 торрентів, відновлення в один клік).
- **Примусовий запуск** — обхід ліміту активних завантажень у черзі для окремого торренту.
- Вбудований **переглядач журналів** з ротацією файлів (5 МБ/файл, зберігати 3), фільтром по рівню, експортом для баг-репортів і CLI-прапорцем `--debug`.
- **Діалог діагностики** — перевірка стану (шлях збереження, порт, DHT, шифрування, VPN, блокування личерів) + тест на витік IP (через api.ipify.org).
- Аргументи CLI: при запуску можна передати будь-яку кількість шляхів до файлів `.torrent` або URI `magnet:`; наступні запуски перенаправляються на запущений екземпляр.
- Автоматичне відображення списку змін при першому запуску після оновлення версії.
- Гарячі клавіші і діалог швидкої довідки по `?`.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Початок роботи

1. Завантажте збірку для вашої платформи зі [сторінки релізів](https://github.com/Mateuscruz19/BATorrent/releases/latest).
2. При першому запуску вітальний діалог допоможе налаштувати шлях збереження, тему і мову.
3. Перетягніть файл `.torrent` або magnet-посилання на вікно — або скористайтеся **Файл → Відкрити торрент** / **Файл → Додати Magnet**.
4. За бажанням: прив'яжіть вихідний інтерфейс у **Налаштування → VPN** і увімкніть аварійне відключення перед додаванням конфіденційних торрентів.

> **Порада щодо VPN:** коли увімкнена **Прив'язка до інтерфейсу**, всі оголошення і з'єднання з пірами проходять лише через вибраний інтерфейс. Якщо інтерфейс відключається, аварійне відключення ставить все на паузу до його відновлення.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Збірка з вихідного коду

### Вимоги
- Компілятор C++17 (GCC 11+, Clang 14+ або MSVC 19.30+)
- CMake 3.16+
- Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`)
- libtorrent-rasterbar 2.0+
- Boost (транзитивна залежність libtorrent)
- Опціонально: Qt6Keychain (зберігає облікові дані в системному сховищі ключів замість відкритого тексту в QSettings)

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

Встановіть Qt 6 через офіційний інсталятор і libtorrent через vcpkg:

```powershell
vcpkg install libtorrent:x64-windows
cmake -B build -DCMAKE_TOOLCHAIN_FILE="$env:VCPKG_ROOT\scripts\buildsystems\vcpkg.cmake"
cmake --build build --config Release
```

### Тести

Набір тестів підключається опціонально:

```bash
cmake -B build -DBAT_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Структура проекту

```
src/
├── torrent/      обгортка libtorrent, дані докачки, черга, правила роздачі
├── app/          перекладач, оновлення, RSS, додатки, сховище секретів, GeoIP
├── gui/          інтерфейс на Qt Widgets (головне вікно, діалоги, панель подробиць, трей)
├── webui/        вбудований HTTP-сервер + браузерний інтерфейс
└── main.cpp      ініціалізація єдиного екземпляра, розбір CLI, оформлення
.github/
└── workflows/    Linux AppImage, macOS DMG, Windows installer + zip
installer/        скрипт Inno Setup для інсталятора Windows
dist/             desktop-файл і ресурси для пакування під Linux
```

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Участь у розробці

Повідомлення про помилки і пул-реквести вітаються. Для значних змін, будь ласка, спочатку створіть issue для обговорення підходу. Готові збірки можна створити для будь-якої гілки через воркфлоу **Build & Release** (`workflow_dispatch`).

При повідомленні про помилку додайте:
- Платформу + версію (`Довідка → Про програму`)
- Кроки для відтворення
- Відповідну секцію з `~/.local/share/BATorrent/` (Linux), `~/Library/Application Support/BATorrent/` (macOS) або `%APPDATA%\BATorrent\` (Windows), якщо проблема пов'язана з даними докачки або налаштуваннями.

<img src="https://capsule-render.vercel.app/api?type=rect&color=dc2626&height=3&width=100%25" width="100%"/>

## Ліцензія

[MIT](LICENSE) © 2024–2026 Mateus Cruz
