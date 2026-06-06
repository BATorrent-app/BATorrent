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
- [x] **① Nav-rail shell** — `NavRail.qml` (animated: accent bar, hover/active fades) + `DiscoverView`/`SearchView`/`HubView` stubs (registered in qrc). Main.qml wrapped: MenuBar stays global; below it `RowLayout{ NavRail | StackLayout{ Downloads(=original column) , Discover, Search, HUB } }`; `minimumWidth` 1100→1288. **Compile-tested + launched on macOS — no QML errors.** Removed the duplicate brand logo from the Downloads toolbar (rail now owns the identity). Collapsible: bottom chevron toggles icon-only mode (64px), labels fade out, icons recenter, tooltips in collapsed mode, state persisted via `settings` (`navRailCollapsed`, stored 0/1 for cross-platform QSettings). Lapidar: i18n the rail labels (incl. "Collapse"/"Expand"); animated page transitions; Discover/HUB icons.
- [~] **② Search page** — 1:1 port of `SearchWindow` content into the `SearchView.qml` page landed (search bar, source/category selectors, results list w/ repacker chips, footer + game-source manager), reusing `QmlSearchBridge`. Rail/toolbar/menu/empty-state now navigate to page 2 instead of opening the window; `searchWinLoader` removed (`SearchWindow.qml` now orphaned in qrc — delete in a cleanup pass). **Remaining (redesign):** decided UX = keep the results **list** + **detail-on-click** panel showing one resolved cover (the picked item), synopsis, files, size, seeds/source, add button (reuse Discover's cover resolution; 1 lookup, not N). Plus complex filters (repacker/size/seeders/source/category). To do **after ③** so it reuses the shared poster-card/cover work.
- [x] **③ Discover** — `src/app/discoveryservice.{h,cpp}` exposes `rows`/`hero` (QVariantList) from TMDB trending+popular (movie/tv) + IGDB **popularity_primitives** (Trending Games = games of the moment, e.g. CS2/LoL/PUBG) + IGDB recent (New Releases); posters are remote URLs (QML Image cache), only the assembled lists are disk-cached (12h, versioned). `DiscoverView.qml` = rotating hero (backdrop crossfade, 7s) + horizontal rows of reusable `widgets/PosterCard.qml` (2:3 cover, hover scale/ring, rating pill). Row order interleaves games high: Trending Movies · **Trending Games** · Trending Series · **New Releases** · Popular Movies · Popular Series. Click poster/hero → `DiscoverView.openSearch(title)` → Main routes to `search.search("all", title)` + Search page. Bridge `discovery` registered in main.cpp. **Compile-tested + verified on macOS.** Lapidar: i18n the row labels (EN keys added, other langs in the final batch); store gate (step ⑦).
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
