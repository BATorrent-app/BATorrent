# BATorrent 4.0 — "Hub" Roadmap

> **Living document.** Tick the checkboxes as steps land so progress survives
> across work sessions. Major release: **v4.0.0**. Branch: `feat/hub-4.0`.

## Vision
Turn BATorrent into a native media hub: **find** (Discover) → **download**
(Downloads) → **search deeply** (Search) → **consume** (HUB: watch movies with
resume / launch games). Built around the existing cover-art identity.

## Design principles
- **Animations are first-class** — the project values them highly. Page/route
  transitions, rotating Discover hero, poster hover/scale, rail selection,
  loading shimmers. Smooth and premium, never janky. Budget time for motion.
- Cover-art forward; consistent card components reused across Discover / Search / HUB.

## Architecture — nav-rail shell
`Main.qml` → `[NavRail | StackLayout]`. Rail: logo · Downloads · Discover ·
Search · HUB · (spacer) · Settings(bottom). Pages swap in the content stack
(with transitions); Settings reuses its existing window. Default page: Downloads.

## Distribution model (Kodi-style)
One binary, clean by default. No torrent sources/catalogs bundled — the user
adds them as **add-ons** (existing `AddonManager`, Stremio-style). Store builds:
**Discover off by default** (`BAT_STORE_BUILD`); everything else identical. The
GitHub build can offer an easier in-app add-on browser; the store stays neutral
(users add sources by URL there).

## Build steps
- [~] **① Nav-rail shell** — `NavRail.qml` (animated: accent bar, hover/active fades) + `DiscoverView`/`SearchView`/`HubView` stubs created & registered in qrc. **Remaining:** wrap Main.qml content in `[rail | StackLayout]` (move toolbar+grid into a Downloads page; bind `currentIndex`; rail `settingsClicked`→ settings window; animated page transitions).
- [ ] **② Search page** — promote `SearchWindow` to a content page `SearchView.qml` (poster grid, repacker chips, complex filters: repacker/size/seeders/source/category). Reuses `QmlSearchBridge`.
- [ ] **③ Discover** — `src/app/discoveryservice.{h,cpp}` (TMDB `/trending` + `/popular`, IGDB popular; 12h disk cache) + `DiscoverView.qml` (rotating hero + horizontal poster rows). Click → `QmlSearchBridge::search("all", title)`.
- [ ] **④ Engine** — `src/webui/streamserver.{h,cpp}` (127.0.0.1, `GET /stream/<hash>/<idx>`, 206/Range, incremental write) + SessionManager `handleByInfoHash` & `prioritizeRange` (`set_piece_deadline`/`have_piece`) + `PlayerWindow.qml` (QtMultimedia FFmpeg backend, resume, external-player fallback).
- [ ] **⑤ HUB — movies** — library of completed video torrents → embedded play + **resume** (position per infohash+file in QSettings) → "Continue watching".
- [ ] **⑥ HUB — games** — launcher: Install (run setup) → set/auto-detect game exe → Play + playtime (extend Discord RPC). Windows-first.
- [ ] **⑦ Store gate + keys + bump** — `#ifndef BAT_STORE_BUILD` hides **only** Discover; add the `BAT_TMDB_KEY/BAT_IGDB_ID/BAT_IGDB_SECRET` env block to `store.yml`'s build step; bump `project(... VERSION 4.0.0)` (CMake/iss/msix) + CHANGELOG.

## Already landed (rides into 4.0 — also shippable as a quick 3.0.4)
- [x] Game search: token match + repacker labeling (`gamesourcemanager.cpp`; `detectRepacker` in `qmlposterbridge.cpp`).
- [x] VCRedist runtime shipped (`build.yml` windeployqt `--compiler-runtime`) — clean-Windows launch fix.
- [x] `thunder://` decode in Smart Paste (`qmlposterbridge.cpp`).
- [x] "Stream while downloading" hardened (`.!bt` path, cross-platform players, identity guard, give-up, resume-if-paused).
- [x] Version bumped to 3.0.4 + CHANGELOG/READMEs (will rebump to 4.0.0 at step ⑦).

## Key files / reuse
- Classification: `ContentType` (`nameparser.h`), per-torrent typeStr.
- Covers + API keys: `MetadataResolver` (TMDB `TmdbBaseUrl`/`tmdbApiKey()`, IGDB `ensureIgdbToken()`).
- Completed state: `info.completed` / `completedCount()` (`qmlposterbridge.cpp`).
- Launch: `launchMediaPlayer()` + `QProcess::startDetached` (`qmlposterbridge.cpp`).
- Add-ons: `AddonManager` (`addonmanager.cpp`).
- Piece APIs: `prioritizeFilePieceBoundaries` + `.!bt` resolver (`sessionmanager.cpp`).
