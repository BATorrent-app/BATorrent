# BATorrent — Архітектура

Карта того, як зібраний BATorrent: де що живе, як взаємодіють шари
та неочевидні підводні камені. Призначено для всіх (людей або ШІ), хто збирається змінювати код. Про те, *що* робить програма, читайте в [README](../README.ua.md); про інженерні угоди та конвенції — у [`CLAUDE.md`](../CLAUDE.md).

BATorrent — десктопний BitTorrent-клієнт: рушій **libtorrent**, обгорнутий у C++, з інтерфейсом на **Qt 6 / QML** та опційним браузерним **WebUI**. Один процес, три платформи (Windows / macOS / Linux).

```
        ┌──────────────────────────────────────────────────────────┐
        │  QML UI (src/qml)         WebUI (src/webui/index.html)   │
        │      ▲                          ▲                        │
        │      │ context properties       │ HTTP/JSON              │
        │  ┌───┴────────────────┐   ┌─────┴───────┐               │
        │  │ C++↔QML мости      │   │ WebServer   │               │
        │  │ (qmlposterbridge)  │   │ (webui)     │               │
        │  └───┬────────────────┘   └─────┬───────┘               │
        │      │                          │                        │
        │  ┌───▼──────────────────────────▼───┐                   │
        │  │ SessionManager (torrent/)        │  ← рушій libtorrent│
        │  │ + сервіси app/ (metadata, rss,   │                   │
        │  │   search, secrets, updates, …)   │                   │
        │  └──────────────────────────────────┘                   │
        └──────────────────────────────────────────────────────────┘
```

UI ніколи не звертається до libtorrent напряму. Все проходить через
`SessionManager` та сервіси `app/`; QML і WebUI — це два
*представлення* одного й того самого ядра.

---

## Структура директорій

```
src/
  main.cpp            Точка входу: будує QML-рушій, реєструє всі мости як
                      context properties, сервер єдиного екземпляра, міграції при запуску.
  torrent/
    sessionmanager.*  Обгортка libtorrent — ГОЛОВНИЙ рушій. Додавання/видалення/пауза,
                      налаштування на рівні торрента, піри/файли/трекери, ліміти, проксі…
    types.h           Прості структури, що перетинають межу (TorrentInfo, PeerInfo…).
  app/                Бекенд-сервіси, повністю незалежні від UI (без Qt Widgets/QML):
    metadataresolver  Постерне мистецтво (TMDB для фільмів/серіалів, IGDB для ігор).
    nameparser        Назва торренту → чистий заголовок + тип контенту (фільм/серіал/гра).
    addonmanager      Пошукові провайдери (JSON-API шаблони) + список публічних трекерів.
    rssmanager        RSS-стрічки + правила автозавантаження за регулярними виразами.
    secretstore       Секрети в системному сховищі ключів ОС (Qt6Keychain). Див. підводні камені.
    updater           Перевірка оновлень на GitHub (пропускається в Store-збірках).
    notifier          Нативні сповіщення ОС.
    discordrpc        Discord Rich Presence.
    geoip             Прапори країн пірів (визначаються за IP, кешуються).
    translator        i18n: завантажує :/translations/<код>.json (8 мов).
    logger            Журнал програми (пише власний файл — див. підводні камені).
    qrcodegen         Генератор QR-матриці (сполучення з телефоном).
    utils             Допоміжні засоби форматування (розміри, швидкості), загальні константи.
  gui/
    qmlposterbridge.* ВЕСЬ шар C++↔QML мостів (всі мости живуть тут).
  webui/
    webserver.*       Мінімальний HTTP-сервер: роздає index.html + /api/* JSON. Аутентифікація.
    index.html        Самодостатній WebUI SPA (HTML/CSS/JS, стилізований як програма).
  qml/                UI (див. «Шар QML» нижче).
  fonts/ icons/ images/  Вбудовані ресурси (шрифт Inter, SVG-іконки, логотип).
  resources.qrc       Вбудовує qml/ + ресурси. webui/webuiresources.qrc вбудовує index.html.
tests/                Тестові набори Catch2 (unit / security / memory / nameparser).
dist/                 Пакування: маніфест flatpak, формула homebrew, .desktop.
installer/            Скрипт Inno Setup для Windows + MSIX (Store).
.github/workflows/    CI/CD (збірка + стіна статичного/динамічного аналізу — див. нижче).
```

---

## Шар C++↔QML мостів

Всі мости — це `QObject`-и, визначені в `src/gui/qmlposterbridge.{h,cpp}` і
зареєстровані як **context properties** у `main.cpp`, тому QML звертається до них за
глобальним ім'ям. Кожен міст — тонкий адаптер: він відкриває `Q_PROPERTY` /
`Q_INVOKABLE` для QML і делегує виклики бекенд-сервісу. **Це є контракт — якщо
метод тут не викликається жодним `.qml`, він або мертвий, або функція,
яку так і не підключили (міграція залишила кілька таких; перевіряйте перед видаленням).**

| Ім'я в QML      | Клас мосту              | Відповідальність (бекенд)                                      |
|-----------------|-------------------------|----------------------------------------------------------------|
| `session`       | `QmlSessionBridge`      | Операції з торрентами + поточний вибір (піри/файли/трекери, ліміти, розумна вставка). Містить `GeoIpResolver`. → `SessionManager` |
| `settings`      | `QmlSettingsBridge`     | Всі налаштування; WebUI/сполучення; секрети; експорт/імпорт. → QSettings + `SecretStore` + `WebServer` |
| `themeBridge`   | `QmlThemeBridge`        | Назва теми / аніме-акцент / власна палітра / іконка трея |
| `torrentModel` / `torrentFilter` | `QmlPosterModel` → `QmlTorrentFilterProxy` | Модель сітки/списку торрентів з постерами + фільтрація/сортування |
| `rss`           | `QmlRssBridge`          | RSS-стрічки + правила. → `RssManager`                         |
| `search`        | `QmlSearchBridge`       | Пошук торрентів по провайдерах. → `AddonManager`              |
| `addons`        | `QmlAddonBridge`        | Керування пошуковими провайдерами + список автотрекерів. → `AddonManager` |
| `pairing`       | `QmlPairingBridge`      | LAN URL, QR-матриця, буфер обміну. → `qrcodegen` + мережеві інтерфейси |
| `logs`          | `QmlLogBridge`          | Переглядач журналу. → `Logger`                                |
| `notifications` | `QmlNotificationBridge` | Запускає нативні + Telegram/Discord сповіщення на події торрентів |
| `updater`       | `QmlUpdaterBridge`      | Перевірка оновлень на GitHub. → `Updater` (null у Store-збірці) |
| `i18n`          | `QmlI18nBridge`         | Пошук `t(key)` + переключення мови в реальному часі. → `Translator` |

Конвеєр моделей: `QmlPosterModel` (рядки = торренти + вирішені постери) →
`QmlTorrentFilterProxy` (`setSourceModel(posterModel)`, сортує за сідами /
фільтрує за вкладкою+пошуком) → відкривається QML як `torrentModel` і `torrentFilter`.

---

## Шар QML (`src/qml`)

QML — **єдиний** UI (застарілий QWidget UI видалений; резервного `--legacy`
режиму не існує). `Main.qml` — кореневе вікно: сітка/список торрентів, панель
інструментів, вбудоване меню, панель подробиць, іконка трея та хост для кожного діалогу.

- **Екрани / діалоги** (`*.qml`): `Main`, `SettingsWindow`, `SearchWindow`,
  `RssWindow`, `AddTorrentDialog`, `CreateTorrentDialog`, `InspectorDialog`,
  `StatisticsWindow`, `DiagnosticsWindow`, `LogViewerWindow`, `PairingDialog`,
  `MagnetDialog`, `RemoveDialog`, `RemovedHistoryWindow`, `WelcomeDialog`,
  `AboutDialog`, `ShortcutsWindow`, `ReleaseNotesDialog`, `UpdateDialog`,
  `AddAddonDialog`, `Splash`. `BatDialog` — загальна база діалогу;
  `InputPromptDialog` (`inputPrompt.openWith(title, prompt, value, placeholder, cb)`)
  — багаторазовий діалог введення тексту.
- **Віджети** (`widgets/`): багаторазові компоненти — `BtnFlat`, `TFld`/`TArea`/`TSelect`/
  `TChk`/`TToggle`/`TChip` (поля вводу), `PathFld`, `Eyebrow`, `IconImg`,
  `PosterThumb`, `ToastHost`, `EmptyState`, та панелі подробиць
  `DetailFiles` / `DetailPeers` / `DetailTrackers` / `DetailPieces`.
- **Тема** (`theme/Theme.qml`): синглтон, що містить **кожен** токен кольору/відступу/шрифту,
  прив'язаний до назви теми (dark / light / midnight / sakura / darkstar /
  custom). Ніщо за межами `Theme.qml` не використовує hex-значення напряму.

---

## Збірка · Тести · Реліз

```bash
# Dev-збірка (показано для macOS) → запускати зібраний бінарник напряму
./scripts/dev-build.sh
./build/BATorrent.app/Contents/MacOS/BATorrent      # НЕ `open` (запустить встановлену копію)

# Тести (Catch2) — налаштувати з кореня репозиторію з увімкненою опцією
cmake -B build-tests -DBAT_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tests --target test_unit test_security test_memory test_nameparser
./build-tests/tests/test_unit                        # очікуємо "All tests passed"
```

**Реліз** запускається тегом: надсилання тегу `v*` запускає CI для збірки всіх трьох
платформ і публікації (GitHub Release → Microsoft Store MSIX → Homebrew tap →
Flathub). Звичайний push у `main` релізу **не** запускає. CI також проводить широкий
аналіз: `build` (+ Catch2), `codeql`, `sanitizers` (ASan/UBSan), `tsan`,
`clang-tidy`, `cppcheck`, `semgrep`, `osv-scanner`, `scorecard`, плюс Store-збірка
та AI-рев'ю коду.

---

## Конвенції та підводні камені

- **Коментарів навмисно мало** (див. `CLAUDE.md`): лише коли *чому* неочевидно.
  Не коментуйте *що* робить добре названий код.
- **Трюк з прив'язкою i18n**: пишіть UI-рядки як `(i18n.language, i18n.t("key"))` —
  звернення до `i18n.language` змушує прив'язку переобчислюватися при переключенні мови.
  Голий `i18n.t("key")` не перекладеться при зміні мови.
- **Лише токени теми**: кольори/відступи/шрифти беруться з `Theme.*`. Жодних
  raw hex у QML поза `Theme.qml`.
- **Секрети / запит до keychain на macOS**: *хеш* пароля WebUI живе в
  `QSettings` (це лише SHA-256, а не використовуваний credential); *відкритий текст*
  пароля живе в keychain. Такий поділ існує тому, що macOS-збірка непідписана
  (немає облікового запису розробника Apple), тому читання елемента keychain при
  кожному холодному старті показувало б запит до login-keychain. Справжні секрети
  (пароль проксі, токени Plex/Jellyfin, відкритий текст пароля WebUI) все одно
  проходять через `SecretStore` → системний keychain.
- **Вбудовування через `.qrc` не перевіряється при C++ збірці**: після редагування
  `resources.qrc` перевіряйте через `grep -c <Name> build/qrc_resources.cpp` —
  відсутній запис призведе до збою лише під час *виконання* QML, а не при компіляції.
- **Кросплатформний QML**: на Windows використовуйте URL типу `file:` (наприклад,
  `win.fileUrl`), вбудований шрифт **Inter** (не системний), `pixelSize` (не
  `pointSize`) і враховуйте нативний рендеринг тексту. Програма пише **власний**
  файл журналу — Windows DebugView для неї марний.
- **Запускайте свіжозібраний бінарник напряму**, а не через `open` (який запустить
  встановлену копію).
