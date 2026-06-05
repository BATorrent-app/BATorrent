# BATorrent — Architecture

A map of how BATorrent is put together: where things live, how the layers talk,
and the non-obvious gotchas. Aimed at anyone (human or AI) about to change the
code. For *what* the app does, see the [README](../README.md); for the engineering
contract and conventions, see [`CLAUDE.md`](../CLAUDE.md).

BATorrent is a desktop BitTorrent client: a **libtorrent** engine wrapped in C++,
driven by a **Qt 6 / QML** UI, with an optional browser **WebUI**. One process,
three platforms (Windows / macOS / Linux).

```
        ┌──────────────────────────────────────────────────────────┐
        │  QML UI (src/qml)         WebUI (src/webui/index.html)     │
        │      ▲                          ▲                          │
        │      │ context properties       │ HTTP/JSON                │
        │  ┌───┴────────────────┐   ┌─────┴───────┐                  │
        │  │ C++↔QML bridges    │   │ WebServer   │                  │
        │  │ (qmlposterbridge)  │   │ (webui)     │                  │
        │  └───┬────────────────┘   └─────┬───────┘                  │
        │      │                          │                          │
        │  ┌───▼──────────────────────────▼───┐                      │
        │  │ SessionManager (torrent/)        │  ← libtorrent engine │
        │  │ + app/ services (metadata, rss,  │                      │
        │  │   search, secrets, updates, …)   │                      │
        │  └──────────────────────────────────┘                      │
        └──────────────────────────────────────────────────────────┘
```

The UI never touches libtorrent directly. Everything flows through
`SessionManager` and the `app/` services; the QML and WebUI front-ends are two
*views* over the same core.

---

## Directory layout

```
src/
  main.cpp            App entry: builds the QML engine, wires every bridge as a
                      context property, single-instance server, startup migrations.
  torrent/
    sessionmanager.*  The libtorrent wrapper — THE engine. Add/remove/pause,
                      per-torrent settings, peers/files/trackers, limits, proxy…
    types.h           Plain structs crossing the boundary (TorrentInfo, PeerInfo…).
  app/                Backend services, all UI-agnostic (no Qt Widgets/QML):
    metadataresolver  Poster art (TMDB for film/TV, IGDB for games).
    nameparser        Torrent name → clean title + content type (movie/series/game).
    addonmanager      Search providers (JSON-API templates) + auto public-tracker list.
    rssmanager        RSS feeds + auto-download regex rules.
    secretstore       Secrets in the OS keychain (Qt6Keychain). See gotchas.
    updater           GitHub release update check (skipped in Store builds).
    notifier          Native OS notifications.
    discordrpc        Discord Rich Presence.
    geoip             Peer country flags (resolved by IP, cached).
    translator        i18n: loads :/translations/<code>.json (7 languages).
    logger            App log (the app writes its own file — see gotchas).
    qrcodegen         QR matrix generator (phone pairing).
    utils             Formatting helpers (sizes, speeds), shared constants.
  gui/
    qmlposterbridge.* The ENTIRE C++↔QML bridge layer (all bridges live here).
  webui/
    webserver.*       Tiny HTTP server: serves index.html + /api/* JSON. Auth.
    index.html        Self-contained WebUI SPA (HTML/CSS/JS, themed like the app).
  qml/                The UI (see "QML layer" below).
  fonts/ icons/ images/  Bundled assets (Inter font, SVG icons, logo).
  resources.qrc       Embeds qml/ + assets. webui/webuiresources.qrc embeds index.html.
tests/                Catch2 suites (unit / security / memory / nameparser).
dist/                 Packaging: flatpak manifest, homebrew formula, .desktop.
installer/            Windows Inno Setup script + MSIX (Store).
.github/workflows/    CI/CD (build + a wall of static/dynamic analysis — see below).
```

---

## The C++↔QML bridge layer

All bridges are `QObject`s defined in `src/gui/qmlposterbridge.{h,cpp}` and
registered as **context properties** in `main.cpp`, so QML reaches them by a
global name. Each bridge is a thin adapter: it exposes `Q_PROPERTY` /
`Q_INVOKABLE` to QML and forwards to a backend service. **This is the contract —
if a method here isn't called by any `.qml`, it's either dead or a feature that
was never wired (the migration left a few of the latter; verify before deleting).**

| QML name        | Bridge class            | Responsibility (backend)                              |
|-----------------|-------------------------|-------------------------------------------------------|
| `session`       | `QmlSessionBridge`      | Torrent ops + current selection (peers/files/trackers, limits, smart-paste). Owns a `GeoIpResolver`. → `SessionManager` |
| `settings`      | `QmlSettingsBridge`     | Every preference; WebUI/pairing; secrets; export/import. → QSettings + `SecretStore` + `WebServer` |
| `themeBridge`   | `QmlThemeBridge`        | Theme name / anime accent / custom palette / tray icon |
| `torrentModel` / `torrentFilter` | `QmlPosterModel` → `QmlTorrentFilterProxy` | The torrent grid/list model with poster art + filtering/sorting |
| `rss`           | `QmlRssBridge`          | RSS feeds + rules. → `RssManager`                     |
| `search`        | `QmlSearchBridge`       | Torrent search across providers. → `AddonManager`     |
| `addons`        | `QmlAddonBridge`        | Manage search providers + auto-tracker list. → `AddonManager` |
| `pairing`       | `QmlPairingBridge`      | LAN URL, QR matrix, clipboard. → `qrcodegen` + net interfaces |
| `logs`          | `QmlLogBridge`          | Log viewer. → `Logger`                                |
| `notifications` | `QmlNotificationBridge` | Fires native + Telegram/Discord alerts on torrent events |
| `updater`       | `QmlUpdaterBridge`      | GitHub update check. → `Updater` (null in Store build) |
| `i18n`          | `QmlI18nBridge`         | `t(key)` lookup + live language switch. → `Translator` |

The model pipeline: `QmlPosterModel` (rows = torrents + resolved posters) →
`QmlTorrentFilterProxy` (`setSourceModel(posterModel)`, sorts by seeders /
filters by tab+search) → exposed to QML as both `torrentModel` and `torrentFilter`.

---

## QML layer (`src/qml`)

QML is the **only** UI (the legacy QWidget UI was removed; there is no `--legacy`
fallback). `Main.qml` is the root window — torrent grid/list, toolbar, in-window
menu bar, detail panel, tray icon, and the host for every dialog.

- **Screens / dialogs** (`*.qml`): `Main`, `SettingsWindow`, `SearchWindow`,
  `RssWindow`, `AddTorrentDialog`, `CreateTorrentDialog`, `InspectorDialog`,
  `StatisticsWindow`, `DiagnosticsWindow`, `LogViewerWindow`, `PairingDialog`,
  `MagnetDialog`, `RemoveDialog`, `RemovedHistoryWindow`, `WelcomeDialog`,
  `AboutDialog`, `ShortcutsWindow`, `ReleaseNotesDialog`, `UpdateDialog`,
  `AddAddonDialog`, `Splash`. `BatDialog` is the shared dialog base;
  `InputPromptDialog` (`inputPrompt.openWith(title, prompt, value, placeholder, cb)`)
  is the reusable text-input dialog.
- **Widgets** (`widgets/`): reusable pieces — `BtnFlat`, `TFld`/`TArea`/`TSelect`/
  `TChk`/`TToggle`/`TChip` (inputs), `PathFld`, `Eyebrow`, `IconImg`,
  `PosterThumb`, `ToastHost`, `EmptyState`, and the detail-panel panes
  `DetailFiles` / `DetailPeers` / `DetailTrackers` / `DetailPieces`.
- **Theme** (`theme/Theme.qml`): a singleton holding **every** color/space/font
  token, keyed by theme name (dark / light / midnight / sakura / darkstar /
  custom). Nothing outside `Theme.qml` hardcodes a hex value.

---

## Build · Test · Release

```bash
# Dev build (macOS shown) → run the produced binary directly
./scripts/dev-build.sh
./build/BATorrent.app/Contents/MacOS/BATorrent      # NOT `open` (grabs installed copy)

# Tests (Catch2) — configure from the repo root with the option ON
cmake -B build-tests -DBAT_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build-tests --target test_unit test_security test_memory test_nameparser
./build-tests/tests/test_unit                        # expect "All tests passed"
```

**Release** is tag-driven: pushing a `v*` tag triggers CI to build all three
platforms and publish (GitHub Release → Microsoft Store MSIX → Homebrew tap →
Flathub). A plain push to `main` does **not** release. CI also runs a wide
analysis wall: `build` (+ Catch2), `codeql`, `sanitizers` (ASan/UBSan), `tsan`,
`clang-tidy`, `cppcheck`, `semgrep`, `osv-scanner`, `scorecard`, plus the Store
build and AI code review.

---

## Conventions & gotchas

- **Comments are rare by design** (see `CLAUDE.md`): only when the *why* is
  non-obvious. Don't narrate *what* well-named code already says.
- **i18n binding trick**: write UI strings as `(i18n.language, i18n.t("key"))` —
  referencing `i18n.language` makes the binding re-evaluate on a live language
  switch. A bare `i18n.t("key")` won't re-translate.
- **Theme tokens only**: colors/spacing/fonts come from `Theme.*`. No raw hex in
  QML outside `Theme.qml`.
- **Secrets / the macOS keychain prompt**: the WebUI password *hash* lives in
  `QSettings` (it's only a SHA-256, not a usable credential); the *plaintext*
  password lives in the keychain. This split exists because the macOS build is
  unsigned (no Apple dev account), so reading a keychain item at every cold start
  would pop a login-keychain prompt. Real secrets (proxy pass, Plex/Jellyfin
  tokens, WebUI plaintext) still go through `SecretStore` → OS keychain.
- **`.qrc` embeds are not checked by the C++ build**: after editing
  `resources.qrc`, verify with `grep -c <Name> build/qrc_resources.cpp` — a
  missing entry only fails at QML *runtime*, not at compile time.
- **Cross-platform QML**: on Windows use `file:` URLs (e.g. `win.fileUrl`), the
  bundled **Inter** font (not the system font), `pixelSize` (not `pointSize`),
  and mind native text rendering. The app writes its **own** log file — Windows
  DebugView is useless for it.
- **Run the freshly built binary directly**, not `open` (which launches the
  installed copy instead).
