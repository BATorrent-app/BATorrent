# BATorrent — living debt board

Bar: `CLAUDE.md`. Execution plan: `internal/QUALITY_PLAN.md`.
Folder reorg (navigation only): `internal/FOLDER_REORG_PLAN.md` (WS-Reorg).
**Sprint roadmap (S4–S7 + Ótimo gate):** `internal/SPRINT_ROADMAP.md`.
Update this file when a workstream lands or LOC materially changes.

**Snapshot date:** 2026-07-31  
**Branch vs origin/main:** ~37 ahead (engine/bridge/Main/discovery peels shipped; S3 F2 Search/Nav WIP).

## Gate status

| Gate | Status | Meaning |
|------|--------|---------|
| Bom | ☐ in progress | Hotspots readable + test nets on pure extracts; crash surface not structural screaming |
| Ótimo | ☐ not started | Soft ceilings mostly met; confidence to change without fear |

## Soft-ceiling hotspots (current)

| File | LOC | Phase | Owner WS | Status | Notes |
|------|-----|-------|----------|--------|-------|
| `qml/PlayerWindow.qml` | 444 | 1 | A | ◐ | 1133→865→444; Chrome/Shortcuts/Scrubber peeled; Bom ≤550 met; Chrome leaf still ~353 (soft) |
| `discovery/addonmanager.cpp` | 701 | 1 | B | ☑ | 1144→701; AddonParse + AddonCatalog + Catch2; QNAM/persistence stay |
| `main.cpp` | 154 | 1–2 | E | ☑ | 1071→154; `src/app/` peels: EngineChild, SingleInstance, BootHealth, AppRuntime, AppServices, QmlContextWiring, QmlBoot |
| `discovery/discoveryservice.cpp` | 846 | 1 | B | ◐ | TmdbParse/IgdbParse peeled; assemble/search finishers remain |
| `qml/SettingsRow.qml` | 153 | 1–2 | F | ☑ | 791→153; VpnCard + ThemeControls + FieldControls leaves; host props |
| `bridges/qmlposterbridge.cpp` | — | 1 | D | ☑ | Split into per-class TUs (10d9b14); umbrella header remains |
| `metadata/metadataresolver.cpp` | 632 | 2 | Meta | ☑ | 761→632; MetadataMatch + Catch2 (title/IGDB pick) |
| `metadata/metadatamatch.{h,cpp}` | 51+186 | 2 | Meta | ☑ | Pure fold/Jaccard/IGDB pick/type helpers |
| `qml/HubView.qml` | 704 | 1–2 | F | ◐ | HubLogic/Format/Menus/DetailDrawer peeled |
| `torrent/sessionmanager.h` | 685 | — | — | ☑ accept | Declarations only; W5 decision: leave |
| `bridges/qmlsettingsbridge.cpp` | 672 | 1–2 | D | ☐ | get/set + backup/proxy/pairing fat |
| `webui/webserver.cpp` | 660 | 2 | Web | ☐ | Optional if quiet in Sentry |
| `qml/SearchView.qml` | 279 | 1–2 | F | ☑ | 644→279; Format/Compute/Recents/WorkHeader/Footer/FitDialog leaves; host aliases |
| `qml/Main.qml` | 641 | — | — | ◐ | Composition root; App* peels done; don't grow |
| `qml/NavRail.qml` | 443 | 1–2 | F | ◐ | 610→443; Disk + DownloadSlot peeled; VPN/settings/collapse still inline; AppTour ids intact |
| `qml/NavBar.qml` | 274 | 1–2 | F | ☑ | 512→274; DownloadChip + DiskGauge + VpnChip leaves |
| `widgets/PosterTile.qml` | 528 | 1–2 | F | ☐ | S4 target; stays under widgets/ |
| `bridges/session/qmlsessionbridge.h` | 474 | 1–2 | D | ◐ | API surface; shrink only by moving logic to services; peels live under `bridges/session/` + `bridges/search/` |
| `torrent/sessionmanager_lifecycle.cpp` | 450 | 1 | C | ☑ | SessionResume trash/history helpers + Catch2 |
| `torrent/sessionmanager_persistence.cpp` | 442 | 1 | C | ☑ | SessionResume suffix/corrupt/legacy + Catch2 |
| `torrent/sessionresume.{h,cpp}` | 87+164 | 1 | C | ☑ | Pure resume/remove/finish policy (new) |
| `bridges/bridgecommon.h` | 50 | — | — | ☑ | Shared includes/fwd — **not** the 27-class mega-header (already split) |

## Already at bar (do not re-open without cause)

- Engine multi-TU (`sessionmanager_*.cpp`); alerts split; ArchiveExtractor collaborator
- Session bridge multi-TU under `bridges/session/`; search bridge multi-TU under `bridges/search/`
- Poster bridge one-per-class `.h`/`.cpp` + umbrella `qmlposterbridge.h` (dump gone)
- TmdbParse / IgdbParse / HubLogic / SessionConfig / SessionResume / AddonParse / AddonCatalog / MetadataMatch + Catch2
- i18n: **1244 keys × 9 locales** (de/en/es/ja/pt/ru/tr/uk/zh) — parity OK; need CI lock
- Main App* extracts (Tray, Notifications, Overlays, Tour, Lifecycle, Menus, LibraryController…)
- DebridPick + passwordhash PBKDF2 tests

## Active / next roster

Sprint 1–3 Fat UI Settings+Search+NavBar **done**; NavRail 443 (soft overhang —
VPN/settings/collapse still inline). **Next launch (S4):** see
`internal/SPRINT_ROADMAP.md` — Hub/PosterTile + `qml/player/` reorg (+ optional i18n CI).

## Open decisions (carry forward)

- `engineMode=ipc` default stays OFF until Sentry justifies
- Main selection JS stays QML (W5) unless a bug demands a C++ net
- `sessionmanager.h` size accepted (partial-class strategy)
- qml/ folder reorg → see `internal/FOLDER_REORG_PLAN.md` + sprint timing in
  `internal/SPRINT_ROADMAP.md` (Phase 0 bridges Done; Phase 1 `player/` = S4 after F2)
- Updater: no crypto verify on installer (needs release-pipeline hashes)
- secretstore plaintext fallback without QtKeychain (source builds only)
- ctest post-pass segfault = harness quirk; CI runs binaries directly

## Bom exit checklist

- [x] PlayerWindow ≤ ~550 (manual MKV playback still open — see signoffs)
- [x] AddonParse (+ catalog) + Catch2; addonmanager much thinner
- [x] Session persistence/remove characterization for extracted pure logic
- [x] qmlposterbridge.cpp no longer multi-class dump
- [x] Build + `BAT_QML_STRICT=warn` smoke green (WS-D)
- [x] This board updated

## Ótimo exit checklist

- [ ] Soft ceilings mostly met (documented exceptions only)
- [ ] Bridges = glue; logic in services/torrent + tests
- [ ] i18n CI on PRs
- [x] main bootstrap composed
- [ ] Fat UI (Settings/Search/Nav/Hub) within bar
- [ ] Confidence: change engine/UI without fear

## Per-sprint log

### 2026-07-31 — Sprint 3 / WS-F SearchView + Nav peel (F2)

- SearchView 644→279: `SearchFormat`, `SearchCompute`, `SearchRecents`,
  `SearchWorkHeader`, `SearchResultsFooter`, `SearchFitDialog` (HubFormat-style
  aliases; filter state stays on host; AppTour untouched)
- NavBar 512→274: `NavBarDownloadChip`, `NavBarDiskGauge`, `NavBarVpnChip`
- NavRail 610→443: `NavRailDisk`, `NavRailDownloadSlot`; `settingsItem` /
  `navRepeater` / `itemRect` kept for AppTour
- Build + `BAT_QML_STRICT=warn` + `BAT_SMOKE_EXIT_ON_FRAME`: first frame healthy
- Remaining Fat UI: HubView 704, PosterTile 528, NavRail 443 (optional further peel)

### 2026-07-31 — Sprint roadmap (S4–S7 planned)

- Wrote `internal/SPRINT_ROADMAP.md`: S3 end conditions (Search+Nav vs Search-only),
  S4 Hub/Poster + `qml/player/`, S5 reorg slices + settings/i18n, S6 AppServices +
  discovery + comments, optional S7, Ótimo → function-by-function learning gate
- Phase 0 bridges CMake paths confirmed Done; F2 qml/qrc settled

### 2026-07-31 — Sprint 3 / WS-F SettingsRow peel

- Peeled `SettingsVpnCard`, `SettingsThemeControls`, `SettingsFieldControls` from
  SettingsRow (family files, not type maps); host props (`field: rowRoot.field`)
- SettingsRow 791→153; leaves 194 / 197 / 299 — all ≤ soft ceiling
- Build + `BAT_QML_STRICT=warn` + `BAT_SMOKE_EXIT_ON_FRAME`: first frame healthy
- Remaining Fat UI: SearchView/Nav done (F2); HubView 704, PosterTile 528 → S4

### 2026-07-31 — Sprint 3 / Meta MetadataMatch

- Extracted `MetadataMatch` (foldTitle/titleSimilarity/confidentTitle,
  pickBestIgdbResult, shortenedSearchTitle, escapeApicalypse, file-type override,
  contentType + TMDB genre id map)
- `metadataresolver.cpp` 761→632 (QNAM/queue/cache stay); behaviour unchanged
- Catch2 `test_metadatamatch` (12 cases / 36 asserts): Jaccard/roman/stopwords,
  Unknown-torrent confident gate, IGDB pick + year bonus, shorten retry math
- Remaining: network orchestration + TMDB/IGDB result→MetadataResult fill still
  inline (~632 soft-ceiling ok for glue-heavy resolver)

### 2026-07-31 — Sprint 2 / WS-A PlayerWindow peel (slice 2)

- Peeled `PlayerChrome` (title + controls), `PlayerScrubber` (seek + thumb decoder),
  `PlayerShortcuts` from PlayerWindow
- PlayerWindow 865→444 (Bom ≤550); panel/menu props renamed (`optionsPanel` /
  `overflowMenu` / `subsPanel`) to avoid `foo: foo` self-shadow
- Build + `BAT_QML_STRICT=warn` boot clean; qml Loader status Ready for new leaves
- Remaining: PlayerChrome ~353 (over ~300 soft); Mateus manual MKV playback sign-off

### 2026-07-31 — Sprint 2 / WS-E main.cpp bootstrap

- Peeled `src/app/`: EngineChild, SingleInstance, BootHealth, AppRuntime,
  AppServices, QmlContextWiring, QmlBoot
- `main.cpp` 1071→154 (orchestration only); context property names unchanged
- Build + `BAT_QML_STRICT=warn` + `BAT_SMOKE_EXIT_ON_FRAME`: first frame healthy,
  no QML errors
- Remaining debt: AppServices still a fat composition TU (~342); media-refresh /
  game-catalog seed / VPN wire live there — further peels optional for Ótimo ≤400
  on every leaf

### 2026-07-31 — Sprint 1 / WS-B AddonParse

- Extracted `AddonParse` (hash/size/provider JSON/manifest/catalog/stream/apibay) and
  `AddonCatalog` (curated + provider presets + defaultProviders seed table)
- `addonmanager.cpp` 1144→701 (fetch/IO/orchestration + persistence)
- Catch2 `test_addonparse` (13 cases / 97 asserts): sizes, hashes, Jackett+TorAPI fixtures,
  streamBaseUrl language inject, catalog smoke
- Public AddonManager API unchanged (thin wrappers)

### 2026-07-31 — Sprint 1 / WS-A PlayerWindow peel (slice 1)

- Peeled `PlayerResume`, `PlayerEndCard`, `PlayerSkipChip`, `PlayerRunway` from PlayerWindow
- PlayerWindow 1133→865; host props over win.* walks in new leaves; qrc registered
- Build + `BAT_QML_STRICT=warn` + forced PlayerWindow load: no Player* QML errors
- Remaining: Chrome / Shortcuts / scrubber-thumb; target ≤550

### 2026-07-31 — Sprint 1 / WS-C SessionResume

- Extracted `SessionResume` pure helpers from persistence/lifecycle/finish alerts
- Catch2 `test_sessionresume` (16 cases): `.!bt` reconcile, corrupt quarantine vs recheck,
  legacy migrate gate, trash targets, finish mute, removed-history prune
- `sessionmanager.h` untouched (accepted debt)

### 2026-07-31 — Bridges folder layout (session/search)

- `git mv` peels into `src/bridges/session/` (`qmlsessionbridge*`) and
  `src/bridges/search/` (`qmlsearchbridge*` + util); other bridges stay at root
- Includes → root-relative `bridges/session/...` / `bridges/search/...`; no behaviour change
- **Phase 0 status: Done** — CMake + tests CMake list `bridges/session|search/...`; no
  root session/search stubs
- Remaining debt: `qmlsettingsbridge.cpp` 672 (S5); session header API surface

### 2026-07-31 — Folder reorg plan (WS-Reorg)

- Wrote `internal/FOLDER_REORG_PLAN.md`: principles, target trees, Phases 0–3,
  qrc/import blast radius, NON-goals
- **Next go after agents settle:** finish Phase 0 CMake, freeze Fat UI qrc, then
  Phase 1 slice `qml/player/`

### 2026-07-31 — Sprint 2 / WS-D Bridges dump split

- Deleted `qmlposterbridge.cpp` (773) mega-dump; one `.cpp` per class already headed:
  Rss 124 · Addon 162 · Pairing 90 · Subtitle 88 · Log 123 · Notification 68 ·
  DiscordRpc 63 · Updater 45; umbrella `qmlposterbridge.h` kept
- CMake/tests list the new TUs; `qmli18nbridge.h` stays header-only (AUTOMOC)
- AddonBridge still calls AddonManager public API only (`curatedCatalog` / `providerCatalog`)
- Smoke: `BAT_QML_STRICT=warn` boot OK (17 resumes); remaining debt: `qmlsettingsbridge.cpp` 672

### 2026-07-31 — plan refresh

- Wrote `internal/QUALITY_PLAN.md` (Bom→Ótimo multitask plan)
- Corrected myth: mega-header debt is **`qmlposterbridge.cpp` dump**, not `bridgecommon.h`
- Historical W1–W5 narrative retired from this board (git history + old commits remain); decisions preserved above

## Manual Mateus queueoffs (can't script)

- [ ] Playback once (PlayerWindow lazy) after WS-A
- [ ] Search result row + detail + game mgr after WS-B/F
- [ ] Settings click-through after WS-F
- [ ] WebUI login (PBKDF2 rehash) if touching webui
