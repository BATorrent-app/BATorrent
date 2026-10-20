<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <b>Українська</b>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="120">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  Торент-клієнт, який показує завантаження обкладинками, відтворює їх просто під час завантаження<br>
  і тримає ваші фільми, серіали та ігри в одній бібліотеці.
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="#встановлення"><b>Завантажити</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com/assets/trailer.mp4"><b>Дивитися трейлер</b></a> &nbsp;·&nbsp;
  <a href="https://batorrent.com"><b>Сайт</b></a>
</p>

<p align="center">
  <img src="docs/images/grid.jpg" alt="Вкладка «Завантаження» у вигляді сітки: вибрано серіал, його епізоди праворуч" width="900">
</p>

BATorrent це десктопний торент-клієнт на [libtorrent](https://www.libtorrent.org/), тому самому рушії, який використовують qBittorrent і Deluge. Він читає назву кожного торента, знаходить відповідний постер (TMDB для фільмів і серіалів, IGDB для ігор) і показує завантаження обкладинками замість імен файлів. Епізоди одного серіалу збираються разом, відео можна дивитися в застосунку ще до кінця завантаження, а ігри встановлюються й запускаються з того самого вікна.

Він безкоштовний і з відкритим кодом. Без реклами, без акаунта, без «Pro»-версії, без аналітики. Окрім самих торентів, він виходить у мережу заради трьох речей: обкладинки й описи з TMDB, IGDB і Steam для того, що є у вашій бібліотеці або що ви відкриваєте в «Знайти»; перевірка оновлень на GitHub, яку можна вимкнути ([`updater.cpp`](src/services/integrations/updater.cpp)); і, в офіційних збірках, звіт про збій, якщо застосунок аварійно завершився.

## Три вкладки

Угорі застосунку три вкладки: **Завантаження**, **Знайти** і **HUB**.

### Завантаження

Ваші торенти сіткою обкладинок або щільним списком, а збоку подробиці того, що вибрано. Серіал згортається в один рядок з епізодами всередині, тож дванадцять епізодів займають один рядок, а не дванадцять.

<p align="center">
  <img src="docs/images/list.jpg" alt="Вкладка «Завантаження» у вигляді списку, серіал розгорнуто по епізодах" width="900">
</p>

### Знайти і HUB

**Знайти** це місце, де шукають щось нове: головна сторінка з популярними фільмами, серіалами та іграми і пошук по всіх ваших джерелах, який ставить найкращий збіг першим. Кожен тайтл має власну сторінку. На сторінці фільму перелічено його релізи, відсортовані за тим, скільки людей їх роздає, з рекомендованим варіантом, а нижче решта фільмів його колекції. На сторінці серіалу епізоди розбито за сезонами, і видно, що у вас уже є і що ви переглянули.

<table>
  <tr>
    <td width="50%"><img src="docs/images/find.jpg" alt="Знайти: сторінка фільму з його релізами та рештою колекції"></td>
    <td width="50%"><img src="docs/images/series.jpg" alt="Знайти: сторінка серіалу з епізодами, переглянутими і тими, що є в бібліотеці"></td>
  </tr>
  <tr>
    <td align="center"><sub>Фільм у «Знайти»</sub></td>
    <td align="center"><sub>Серіал у «Знайти»</sub></td>
  </tr>
</table>

**HUB** це бібліотека того, що у вас уже є: «Продовжити перегляд», «Продовжити гру» і все інше, розкладене за іграми, серіалами та фільмами.

<p align="center">
  <img src="docs/images/hub.jpg" alt="HUB: продовжити перегляд, продовжити гру і бібліотека" width="900">
</p>

## Плеєр

Вбудований плеєр працює на FFmpeg, тому MKV, AVI і WebM відтворюються напряму. Початок файлу завантажується першим, тож дивитися можна, поки докачується решта. Плеєр запам'ятовує, де ви зупинилися, пропускає заставки, а наприкінці пропонує наступний епізод або наступний фільм колекції. Він сам знаходить субтитри (через SubDL) або підхоплює `.srt`, що лежить поруч із файлом. Декодування відбувається в окремому ізольованому процесі, тому пошкоджений файл не покладе разом із собою весь застосунок.

<p align="center">
  <img src="docs/images/player.jpg" alt="Вбудований плеєр, наступний епізод уже напоготові" width="900">
</p>

Якщо у вас є акаунт [Real-Debrid](https://real-debrid.com) або [TorBox](https://torbox.app) і торент уже є в них у кеші, відео йде потоком звідти, і на ваш комп'ютер нічого не завантажується і нічого не роздається.

## Що ще він уміє

- **Ігри.** Обкладинки з IGDB, встановлення й запуск із HUB, ігровий час і досягнення. Завантажені ігри виглядають як лаунчер, а не як тека з інсталяторами.
- **VPN.** Імпортуйте WireGuard `.conf` (Proton, Mullvad, IVPN...) і підключайтеся. Через нього йде лише BATorrent, решта комп'ютера лишається на звичайному з'єднанні, і в систему нічого не встановлюється. Якщо з'єднання падає, він перепідключається сам, kill switch зупиняє торенти, доки зв'язок не повернеться, а на серверах, які це підтримують, відкриває порт, щоб піри могли до вас достукатися.
- **Приватність.** Тест витоку, який показує IP, що його бачать ваші піри, прив'язка до мережевого інтерфейсу, режим приватних трекерів, пресет Tor і блокування відомих клієнтів-п'явок.
- **Додавання.** Вставте magnet, посилання або info-hash будь-де через Ctrl+V. RSS з regex-фільтрами, тека спостереження, drag and drop і звичайні завантаження по HTTP/HTTPS.
- **Віддалене керування.** WebUI, який відкривається на телефоні за QR-кодом. Код генерується на вашому комп'ютері.
- **Упорядкування.** Категорії й теги, авторозпакування архівів, ліміти рейтингу й часу, розклад швидкості за годинами й днями і альтернативна швидкість (черепаха) в один клік.
- **Вигляд.** Сім тем, кожна з власним шрифтом і артом, або ваша власна; чистий фон, арт теми або ваше зображення. Палітра команд за Ctrl/⌘+K. Дев'ять мов і коротке налаштування під час першого запуску.

<p align="center">
  <img src="docs/images/themes.jpg" alt="Вкладка «Завантаження» в семи темах: Стандартна, Кіно, Світла, Midnight, Sakura, Dark Star і Matrix" width="900">
</p>

<details>
<summary><b>Повний список</b></summary>

Пріоритет окремих файлів, послідовне завантаження, додавання трекерів, керування розкладкою вмісту, regex для виключення файлів, окрема тимчасова тека, вікна роздачі, автопауза, коли файли зникають, оновлення бібліотеки Plex/Jellyfin/Emby, сповіщення в Telegram і Discord, імпорт із qBittorrent, створення `.torrent`, IP-блоклисти, шифрування протоколу, дзеркало оновлень на Gitee, стабільний і бета-канал оновлень, вимкнення після завершення завантажень, помічник для виключення у Windows Defender, резервне копіювання й відновлення, історія нещодавно видалених, примусовий запуск, перегляд логів із діагностикою, рядок меню за Alt, фільми однієї колекції згруповано в бібліотеці і гарячі клавіші майже для всього.

</details>

## Рушій

BATorrent постачається з невеликим пропатченим форком libtorrent, який збирається з версіонованих патчів у [`third_party/patches/`](third_party/patches):

- **Швидший розгін.** На швидкому каналі з великою затримкою стандартний libtorrent нарощує конвеєр запитів по одному кроку. Форк нарощує його геометрично й заповнює канал за значно меншу кількість обмінів. В A/B-прогонах проєкту це дало приблизно на 27% швидше на швидкому з'єднанні й жодного разу не було повільніше за стандартну версію.
- **Піри з власної країни.** Офлайн-база GeoIP (db-ip Lite) позначає кожен пір країною, і коли є вибір, форк віддає перевагу пірам із вашої країни, що зазвичай означає меншу затримку.

Обидві можливості вмикаються під час компіляції і вимкнені в стандартній збірці.

## Встановлення

| Платформа | Завантаження | Вимоги |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Інсталятор](https://batorrent.com/win) · [Портативна версія](https://batorrent.com/portable) | Windows 10 або новіша |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` або [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Після запуску перетягніть у вікно `.torrent` або magnet-посилання.

<sub><b>macOS:</b> застосунок поки не нотаризовано (програма розробника Apple це платна підписка). Найпростіше ставити через Homebrew, бо він знімає прапорець карантину. Якщо ви берете <code>.dmg</code>, першого разу клацніть по застосунку правою кнопкою і виберіть <b>Відкрити</b>.</sub>

<details>
<summary><b>Збірка з вихідного коду</b></summary>

**Вимоги:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`), libtorrent-rasterbar 2.0+, Boost, zlib і, за бажанням, Qt6Keychain.

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```

На macOS: `brew install qt libtorrent-rasterbar boost openssl`.
На Windows: інсталятор Qt і `vcpkg install libtorrent:x64-windows`.

</details>

<details>
<summary><b>Якість і безпека</b></summary>

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- Набір тестів на Catch2 (модульні, безпека, пам'ять) і тести QML проганяються в кожній CI-збірці.
- Збірка проходить під AddressSanitizer і UndefinedBehaviorSanitizer.
- Секрети зберігаються в keychain ОС, а WebUI стає доступним із мережі лише після того, як ви задасте пароль.

</details>

## Навіщо я його зробив

Я один розробник із Бразилії. Мені був потрібен торент-клієнт, який серйозно ставиться до приватності, нативно працює на Windows, macOS і Linux і не виглядає так, ніби його зробили у 2009 році. Такого, що мені сподобався б, я не знайшов, тому написав свій. Він під ліцензією MIT, тож якщо в ньому колись з'являться реклама чи стеження, будь-хто зможе форкнути його і випустити версію без них.

## Участь

Issues і pull requests вітаються. Якщо зміна більша за дрібне виправлення, спершу відкрийте issue, щоб домовитися про підхід. У звіті про помилку найкорисніше вказати платформу, версію (з `Довідка → Про BATorrent`) і кроки для відтворення. Перекладам особливо раді. Якщо ви користуєтеся інструментами ШІ, спершу прочитайте, [як я використовую ШІ](USING_AI.md).

## Ліцензія і торговельна марка

**Код** поширюється за ліцензією [MIT](LICENSE), © 2024-2026 Mateus Cruz. Ви можете форкнути його і випускати власну збірку.

**Назва «BATorrent» і логотип** не підпадають під ліцензію на код. Якщо ви випускаєте форк, дайте йому, будь ласка, власну назву, щоб було зрозуміло, яка збірка офіційна. Подробиці в [TRADEMARK.md](TRADEMARK.md).

<p align="center"><sub>Зроблено в Бразилії.</sub></p>
