<p align="center">
  <a href="README.md">English</a> | <a href="README.pt-BR.md">Português</a> | <a href="README.zh-CN.md">中文</a> | <a href="README.ja.md">日本語</a> | <b>Русский</b> | <a href="README.es.md">Español</a> | <a href="README.de.md">Deutsch</a> | <a href="README.ua.md">Українська</a>
</p>

<p align="center">
  <img src="src/images/logo.svg" alt="BATorrent" width="140">
</p>

<h1 align="center">BATorrent</h1>

<p align="center">
  <i>Торрент-клиент с лицом — обложки фильмов, шесть тем, ноль рекламы.</i>
</p>

<p align="center">
  <a href="https://github.com/BATorrent-app/BATorrent/releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/releases"><img alt="Downloads" src="https://img.shields.io/github/downloads/BATorrent-app/BATorrent/total?style=flat-square&color=dc2626"></a>
  <a href="LICENSE"><img alt="License" src="https://img.shields.io/github/license/BATorrent-app/BATorrent?style=flat-square&color=dc2626"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/Windows%20·%20macOS%20·%20Linux-dc2626?style=flat-square">
  <a href="https://apps.microsoft.com/detail/9n4l3tq24rc6"><img alt="Microsoft Store" src="https://img.shields.io/badge/Microsoft%20Store-get-dc2626?style=flat-square&logo=microsoft"></a>
</p>


<p align="center">
  <img src="src/images/shot-grid-v43.jpg" alt="BATorrent — открыл и пользуйся, обложки подтягиваются сами" width="860">
</p>

Большинство торрент-клиентов выглядят как налоговая декларация. Этот показывает ваши загрузки **стеной обложек фильмов, сериалов и игр** — как на Netflix или в Steam — и позволяет нарядить его в шесть тем (или ваши собственные обои). Под капотом — проверенный движок **libtorrent**, так что это не красивая игрушка: это настоящий клиент, у которого просто оказался вкус.

> **Без рекламы. Без телеметрии. Без «Pro»-версии. Без аккаунта.** Единственный запрос, который он делает сам — проверка обновлений на GitHub, и её можно отключить. Исходный код прямо здесь — откройте [`updater.cpp`](src/services/integrations/updater.cpp) и убедитесь сами.


## Зачем это нужно

Я один разработчик из Бразилии. Мне был нужен торрент-клиент, который серьёзно относится к приватности, работает нативно на любом десктопе и не выглядит так, будто его собрали в 2009 году — а раз такого не нашлось, я сделал свой. Он бесплатный и под **лицензией MIT**: без подвохов, без телеметрии, которая прокрадётся позже, и его нельзя тихо продать компании, которая прикрутит рекламу. Восемь языков, потому что «полезный» не должно означать «только на английском».

## Как это выглядит

<p align="center">
  <img src="src/images/themes.gif" alt="Переключение между встроенными темами" width="860">
</p>

<p align="center">
  <img src="src/images/shot-grid.ru.jpg" alt="Сетка обложек" width="860">
</p>

<p align="center">
  <img src="src/images/shot-list.ru.jpg" alt="Список" width="860">
</p>

<p align="center">
  <img src="src/images/shot-theme.ru.jpg" alt="Тема Sakura" width="860">
</p>

- **Автоматические обложки** — читает имя торрента и подтягивает настоящий постер (фильмы и сериалы через TMDB, игры через IGDB) в вид-сетку. Один клик переключает на компактный список.
- **Шесть тем** — Dark, Light, Midnight, Sakura, Dark Star и полностью **настраиваемая** (свой фон + акцентные цвета), каждая с опциональной аниме-графикой акцента.
- График скорости в реальном времени, прогресс с цветом по состоянию, насыщенное всплывающее окно в трее со скоростями и оставшимся временем — детали, от которых он *ощущается* завершённым.

## Что он реально умеет

| | |
|---|---|
| 🔒 **Приватность прежде всего** | Привязка к VPN-интерфейсу + **kill switch** (обрывает весь трафик, если туннель упал), PT-режим для приватных трекеров, пресет Tor, анонимный handshake, блокировка пиявок (anti-leecher) |
| 🔎 **Найти и добавить** | Встроенный поиск (вкл. открытые источники СНГ/RuTor, без логина), Умная вставка (magnet / `.torrent` / `thunder://` / hash по Ctrl+V), авто-загрузка по RSS с regex-фильтрами, drag-and-drop |
| 📱 **Управление откуда угодно** | Веб-интерфейс в браузере с **сопряжением по QR** — отсканируйте с телефона, без ввода IP. QR генерируется локально; ваш адрес никогда не покидает машину |
| 📺 **Смотреть и упорядочивать** | Просмотр во время загрузки, авто-распаковка архивов, категории + теги, обновление библиотеки Plex/Jellyfin/Emby по завершении |
| 🔔 **Быть в курсе** | Нативные уведомления рабочего стола, оповещения в Telegram, Discord Rich Presence («Загружается X · 67%») |

<details>
<summary><b>…и длинный хвост</b> (нажмите, чтобы развернуть)</summary>

Приоритет по файлам · последовательная загрузка · авто-добавление трекеров · управление раскладкой контента · regex исключаемых файлов · временная папка загрузки · состояние «Завершено» с окнами раздачи · авто-пауза при ошибках файлов · глобальные + по-торренту лимиты рейтинга/времени · планировщик полосы пропускания (час + день) · импорт из qBittorrent · создание `.torrent`-файлов · инспектор торрента · списки блокировки IP · шифрование протокола · зеркало обновлений Gitee · авто-выключение по завершении · исключение в Windows Defender · полный бэкап/восстановление · история недавно удалённых · принудительный запуск · встроенный просмотр логов + диагностика + тест утечки IP · форматирование с учётом локали · горячие клавиши.

</details>


## Скачать

| Платформа | | |
|---|---|---|
| **Windows** | [Microsoft Store](https://apps.microsoft.com/detail/9n4l3tq24rc6) · [Установщик](https://github.com/BATorrent-app/BATorrent/releases/latest) · [Портативная версия](https://github.com/BATorrent-app/BATorrent/releases/latest) | Windows 10+ |
| **macOS** | **`brew install --cask Mateuscruz19/batorrent/batorrent`** · [`.dmg`](https://github.com/BATorrent-app/BATorrent/releases/latest) | macOS 12+ · Apple Silicon |
| **Linux** | [AppImage](https://github.com/BATorrent-app/BATorrent/releases/latest) | glibc 2.35+ |

Дальше просто перетащите `.torrent` или magnet в окно. Вот и всё.

<sub>**macOS:** пока без нотаризации (программа разработчика Apple платная). Homebrew — самый гладкий путь: `brew` снимает флаг карантина, и приложение открывается без диалога Gatekeeper. С `.dmg` в первый раз — правый клик → **Открыть**.</sub>


<details>
<summary><b>Сборка из исходников и инженерные заметки</b></summary>

### Требования
C++17 · CMake 3.16+ · Qt 6 (`Widgets`, `Network`, `Svg`, `Multimedia`) · libtorrent-rasterbar 2.0+ · Boost · Qt6Keychain (опционально).

```bash
# Debian / Ubuntu
sudo apt install build-essential cmake qt6-base-dev qt6-svg-dev qt6-multimedia-dev \
    libtorrent-rasterbar-dev libboost-dev libssl-dev
cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/BATorrent
```
(macOS: `brew install qt libtorrent-rasterbar boost openssl`. Windows: установщик Qt + `vcpkg install libtorrent:x64-windows`.)

### Качество и безопасность

<p>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml"><img alt="CodeQL" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/codeql.yml/badge.svg"></a>
  <a href="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml"><img alt="Sanitizers" src="https://github.com/BATorrent-app/BATorrent/actions/workflows/sanitizers.yml/badge.svg"></a>
  <a href="https://sonarcloud.io/summary/new_code?id=Mateuscruz19_BAT-Torrent"><img alt="Quality Gate" src="https://sonarcloud.io/api/project_badges/measure?project=Mateuscruz19_BAT-Torrent&metric=alert_status"></a>
  <a href="https://www.codefactor.io/repository/github/mateuscruz19/batorrent"><img alt="CodeFactor" src="https://www.codefactor.io/repository/github/mateuscruz19/batorrent/badge"></a>
  <a href="https://www.bestpractices.dev/projects/13073"><img alt="OpenSSF Best Practices" src="https://www.bestpractices.dev/projects/13073/badge"></a>
</p>

- **Тесты** — набор Catch2 (юнит, безопасность, память) на каждой CI-сборке; новое поведение бэкенда получает тест.
- **Санитайзеры** — проходит чисто под AddressSanitizer + UndefinedBehaviorSanitizer (0 утечек / use-after-free / UB).
- **Проверка** перед каждым релизом на безопасность памяти/потоков, аутентификацию WebUI, инъекции, path traversal, валидацию ввода и обращение с секретами. Секреты хранятся в keychain ОС, никогда в открытом виде; WebUI открывается в сеть только после установки пароля.

</details>

## Участие

Issues и PR приветствуются — для всего нетривиального сначала откройте issue. Сообщения об ошибках: укажите вашу платформу + версию (`Справка → О программе`) и шаги воспроизведения. Переводы особенно приветствуются.

## Лицензия

[MIT](LICENSE) © 2024–2026 Mateus Cruz · сделано в Бразилии 🦇
