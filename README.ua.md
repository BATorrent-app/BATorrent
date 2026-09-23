<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <a href="README.ru.md">Русский</a> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <b>Українська</b>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>BitTorrent-клієнт, який показує ваші завантаження як обкладинки, а не як рядки електронної таблиці.</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Платформи" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>

<p align="center">
  <a href="https://batorrent.com/assets/trailer.mp4"><img src="src/images/trailer-poster.jpg" alt="Дивитися трейлер BATorrent (1:30)" width="860"></a>
</p>

BATorrent є десктопним торент-клієнтом на рушії [libtorrent](https://www.libtorrent.org/), тому самому, що використовують qBittorrent і Deluge. Інтерфейс читає назву кожного торента, знаходить відповідний постер (фільми й серіали з TMDB, ігри з IGDB) і показує ваші завантаження сіткою обкладинок замість списку назв файлів. Під обкладинками працює повноцінний клієнт на [пропатченій збірці цього рушія](#рушій).

Він безкоштовний і з відкритим кодом, без реклами, телеметрії, «Pro»-версії та облікового запису. Єдиний мережевий запит, який він робить сам, це перевірка оновлень на GitHub, і її можна вимкнути перемикачем. Якщо хочете переконатися, код лежить у [`updater.cpp`](src/services/integrations/updater.cpp).

## Навіщо я його зробив

Я розробник-одинак із Бразилії. Мені був потрібен торент-клієнт, який серйозно ставиться до приватності, нативно працює на Windows, macOS і Linux і не виглядає так, ніби його намалювали у 2009 році. Такого, що мені сподобався б, я не знайшов, тож написав свій. Він під ліцензією MIT, тому якби проєкт колись додав телеметрію чи рекламу, будь-хто зміг би зробити форк коду й випускати його без них. Інтерфейс перекладено дев'ятьма мовами.

## Інтерфейс

<p align="center">
  <img src="src/images/shot-list.jpg" alt="Компактний список для тих, кому деталі важливіші за оформлення" width="860">
</p>

<p align="center">
  <img src="src/images/shot-palette-v43.jpg" alt="Палітра команд (Ctrl/⌘+K): нечіткий пошук будь-якого торента чи дії" width="860">
</p>

<p align="center">
  <img src="src/images/shot-theme.jpg" alt="Sakura, одна з вбудованих тем" width="860">
</p>

- **Обкладинки.** Клієнт визначає постер за назвою торента й показує його в сітці. Один клік перемикає на компактний список, коли деталі важливіші за оформлення.
- **Шість тем.** Dark, Light, Midnight, Sakura, Dark Star і Custom, де ви самі обираєте фон і акцентний колір. Кожна тема може показувати аніме-арт як акцент, якщо його ввімкнути.
- **Палітра команд.** Ctrl/⌘+K відкриває нечіткий пошук будь-якого торента чи дії: призупинити все, перемкнути альтернативну швидкість, перейти на будь-яку сторінку. Миша для цього не потрібна.
- **Стан наживо.** Графік швидкості в реальному часі, смуги прогресу з кольором за станом і спливне вікно в треї з поточними швидкостями та часом до завершення.

## Що він уміє

**Перегляд у застосунку.** Є вбудований відеоплеєр (на FFmpeg, тому MKV, AVI і WebM відтворюються напряму), і дивитися можна ще під час завантаження, бо клієнт спершу завантажує початок файлу. Він сам шукає й завантажує субтитри (через SubDL), автоматично підхоплює файли `.srt`/`.vtt`, що лежать поруч, і дозволяє підправляти синхронізацію на льоту. Після завершення може оновити бібліотеку Plex, Jellyfin або Emby.

**Миттєве відтворення через debrid.** Підключіть обліковий запис [Real-Debrid](https://real-debrid.com) або [TorBox](https://torbox.app), і якщо magnet уже є в кеші сервісу, BATorrent розблоковує посилання й транслює його просто у вбудований плеєр. На ваш комп'ютер тоді нічого не завантажується і нічого не роздається.

**Ігри теж.** Ігрові торенти також отримують обкладинки (через IGDB). Шукайте в каталогах ігор, завантажуйте, а потім встановлюйте й запускайте прямо із застосунку. Так ваша піратська бібліотека поводиться трохи як список у Steam, а не як тека з інсталяторами.

**Огляд.** Головна сторінка в стилі Netflix, яку можна гортати (популярні постери, банер, що змінюється), щоб знайти щось для завантаження, не виходячи із застосунку.

<p align="center">
  <img src="src/images/shot-discover.jpg" alt="Огляд: головна сторінка з популярними постерами" width="860">
</p>

**Приватність.** Прив'язка до конкретного VPN-інтерфейсу з kill switch, який обриває весь трафік, якщо тунель падає. Також є режим для приватних трекерів, пресет для Tor, анонімний handshake і блокування клієнтів-«п'явок» (anti-leecher). Вбудований тест на витік IP допоможе перевірити, що все працює.

**Пошук і додавання.** Вбудований пошук (зокрема відкриті джерела СНД/RuTor, для яких не потрібен вхід), Smart Paste, що розпізнає magnet, `.torrent`, посилання `thunder://` або info hash по Ctrl+V, автозавантаження з RSS із regex-фільтрами, тека, за якою стежить клієнт, і перетягування файлів.

<p align="center">
  <img src="src/images/shot-search.jpg" alt="Вбудований пошук: обкладинки, рейтинги та найкращий збіг одразу" width="860">
</p>

**Віддалене керування.** WebUI у браузері з під'єднанням через QR: відскануйте код телефоном, і не доведеться вводити IP-адреси. QR генерується на вашому комп'ютері, і адреса нікуди з нього не йде.

**Упорядкування.** Автоматичне розпакування архівів після завершення, сортування за категоріями й тегами, ліміти рейтингу й часу для окремих торентів і глобально, розклад швидкості за годинами й днями.

**Сповіщення.** Нативні сповіщення робочого столу, повідомлення в Telegram і Discord Rich Presence.

<details>
<summary><b>Повний список можливостей</b></summary>

Пріоритет окремих файлів, послідовне завантаження, автоматичне додавання трекерів, керування розкладкою вмісту, regex для виключення файлів, окрема тимчасова тека для завантажень, стан «Завершено» з вікнами роздачі, автопауза при помилках файлів, глобальні й окремі для кожного торента ліміти рейтингу та часу, планувальник швидкості за годинами й днями, імпорт із qBittorrent, створення `.torrent`, інспектор торентів, списки блокування IP, шифрування протоколу, дзеркало оновлень на Gitee, автовимкнення комп'ютера після завершення завантажень, помічник для додавання винятку в Windows Defender, повне резервне копіювання й відновлення, історія нещодавно видалених, примусовий запуск, вбудований переглядач журналу з діагностикою та тестом на витік IP, форматування з урахуванням локалі й гарячі клавіші.

</details>

## Рушій

Більшість торент-застосунків використовують стандартний libtorrent. BATorrent постачається з невеликим пропатченим форком, тому може змінювати поведінку рушія там, куди публічний API не дістає:

- **Швидший розгін конвеєра.** На каналі з великою пропускною здатністю й високою затримкою стандартний конвеєр запитів росте по одному кроку, а форк нарощує його геометрично і заповнює широкий канал за малу частку обмінів туди й назад. У власному A/B-бенчмарку проєкту це дало приблизно +27% на швидкому каналі, без характерних для стандартної версії провалів від запуску до запуску, і гірших результатів він не показує ніколи.
- **Перевага пірам із вашої країни.** Офлайн-база GeoIP (db-ip Lite) позначає кожен пір країною, і коли є вибір, ранжування пірів у форку віддає перевагу пірам із вашої країни. Зазвичай це означає меншу затримку й менше транскордонних маршрутів з обмеженою швидкістю.

Обидві функції вмикаються у форку під час компіляції, у стандартній збірці вони вимкнені. Застосовуються вони як версіоновані патчі в [`third_party/patches/`](third_party/patches), а не як вбудована копія бібліотеки.

## Встановлення

| Платформа | Завантаження | Вимоги |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6), [інсталятор](https://batorrent.com/win) або [портативна версія](https://batorrent.com/portable) | Windows 10 або новіша |
| **macOS** | `brew install --cask Mateuscruz19/batorrent/batorrent` або [`.dmg`](https://batorrent.com/mac) | macOS 12+, Apple Silicon |
| **Linux** | [AppImage](https://batorrent.com/linux) | glibc 2.35+ |

Коли застосунок запущено, перетягніть у вікно файл `.torrent` або magnet-посилання.

<sub><b>Примітка для macOS:</b> застосунок поки не нотаризовано (програма розробника Apple є платною підпискою). Найпростіше встановити його через Homebrew, бо <code>brew</code> знімає прапорець карантину і застосунок відкривається без запиту Gatekeeper. Якщо ж ви берете <code>.dmg</code>, першого разу клацніть застосунок правою кнопкою й виберіть <b>Відкрити</b>.</sub>

<details>
<summary><b>Збирання з вихідного коду</b></summary>

**Вимоги:** C++17, CMake 3.16+, Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`, `Quick`, `QuickWidgets`, `QuickControls2`), libtorrent-rasterbar 2.0+, Boost і, за бажанням, Qt6Keychain.

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

- Набір тестів на Catch2 (модульні, безпеки, пам'яті) запускається на кожній CI-збірці, і нова поведінка бекенду додається разом із тестом.
- Збірка чисто проходить AddressSanitizer і UndefinedBehaviorSanitizer.
- Перед кожним релізом код перевіряють на безпеку пам'яті й потоків, автентифікацію WebUI, ін'єкції, path traversal, валідацію введення та поводження із секретами. Секрети зберігаються у сховищі ключів ОС, а не відкритим текстом, і WebUI стає доступним у мережі лише після того, як ви встановите пароль.

</details>

## Як долучитися

Issues і pull requests вітаються. Якщо зміна нетривіальна, спершу відкрийте issue, щоб ми домовилися про підхід. Звіт про помилку найкорисніший, коли в ньому є ваша платформа й версія (з `Довідка → Про програму`) і кроки для відтворення. Особливо вдячний за переклади.

## Ліцензія і торговельна марка

**Код** поширюється під ліцензією [MIT](LICENSE), © 2024-2026 Mateus Cruz. Ви можете зробити форк і випускати власну збірку.

**Назва «BATorrent» і логотип** належать проєкту, і ліцензія коду на них не поширюється. Якщо ви розповсюджуєте форк, будь ласка, дайте йому власну назву, щоб користувачі могли відрізнити офіційну збірку. Подробиці в [TRADEMARK.md](TRADEMARK.md). Сумлінні форки й внески вітаються.

Зроблено в Бразилії.
