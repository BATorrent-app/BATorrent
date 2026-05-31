# Screen 11 — Search (QWidget → QML migration plan)

Single source of truth for finishing the Search screen migration.

## Files in scope

- QML (stub, design-only): `/Users/mateuscruz/Projects/BAT-Torrent/src/qml/SearchWindow.qml` (185 lines)
- Old QWidget: `/Users/mateuscruz/Projects/BAT-Torrent/src/gui/searchdialog.cpp` (580 lines), `/Users/mateuscruz/Projects/BAT-Torrent/src/gui/searchdialog.h` (60 lines)
- Backend: `/Users/mateuscruz/Projects/BAT-Torrent/src/app/addonmanager.cpp` (684 lines), `/Users/mateuscruz/Projects/BAT-Torrent/src/app/addonmanager.h` (155 lines)
- Bridge header (no Search bridge yet): `/Users/mateuscruz/Projects/BAT-Torrent/src/gui/qmlposterbridge.h`
- Wired-dialog patterns to copy: `/Users/mateuscruz/Projects/BAT-Torrent/src/qml/AddTorrentDialog.qml`, `MagnetDialog.qml`, `RemoveDialog.qml`
- Shared widgets: `/Users/mateuscruz/Projects/BAT-Torrent/src/qml/widgets/{TFld,TSelect,TChip,EmptyState,BtnFlat,IconImg}.qml`

## Status summary

- **What's there (QML):** A static, design-only `SearchWindow.qml`. It is a top-level `Window` (correct shape — Search is a window, not a `BatDialog`). It has a titlebar, a search field (`TFld`) with hard-coded text `"forza horizon 6"`, a fake provider selector (a plain `Rectangle`, **not** a `TSelect`), a results header row (NOME / PROVEDOR / TAMANHO / SEEDS / LEECH / IDADE / +), a `ListView` bound to a **hard-coded `ListModel`** of 6 fake Forza rows, and a footer with a fake "42 resultados · 3 provedores" label + a Close button. **Nothing is wired** — no bridge reference, no `session`, no `AddonManager`, no signals, no real search, no category selector, no status/loading text, no stream drill-down, no back button, no game/repacker column.
- **What's missing:** literally all behavior. No `QmlSearchBridge` exists in `qmlposterbridge.h`. The QML has zero of the four search modes the QWidget supports (Stremio catalog→streams, legacy single-URL torrent, multi-provider torrent, games). It also drops design columns the backend can't fill (IDADE/age is not in any backend struct — see Gotchas).
- **Bridge additions:** **1 new class** (`QmlSearchBridge`) with **1 model-ish property + ~6 properties + ~5 Q_INVOKABLEs + ~4 signals**, registered once in `main.cpp`. Plus 1 line in `CMakeLists.txt` if the bridge lives in a new file (recommended: extend existing `qmlposterbridge.h/.cpp`, so no new file).
- **Est. effort:** ~1.5–2 days. Bridge ~0.5 day; QML rewrite (source selector, category selector, dynamic columns per mode, stream drill-down + back, status line, empty/loading states, wiring add/download) ~1 day; testing the 4 modes ~0.25 day.

---

## 1. The four search modes (the core complexity)

`SearchDialog` is **not** a single torrent search. The source `QComboBox` (`m_sourceCombo`) switches between four logically distinct flows, each with a **different result schema and a different result table**. The QML must replicate all four. Evidence: `searchdialog.cpp:136-150` (source combo population), `searchdialog.cpp:312-371` (`onSourceChanged` / `performSearch` dispatch).

| Source `data` value | Mode | Backend call | Result struct | Columns | Activation |
|---|---|---|---|---|---|
| `"stremio"` | Stremio catalog → streams (2-level) | `searchCatalog(query)` then `getStreams(type,id)` on row activate | `CatalogItem` → `StreamResult` | catalog: Name/Type/Year; streams: Quality/Size/Addon | double-click catalog row loads streams; double-click stream row adds magnet |
| `"legacy"` | Legacy single-URL torrent API | `searchTorrents(query, categoryCode)` | `TorrentSearchResult` | Name/Size/Seeds/Leech | double-click adds magnet |
| `"games"` | Games (legacy URL, cat=400, repacker detection) | `searchTorrents(query, 400)` | `TorrentSearchResult` | Name/Repacker/Size/Seeds/Leech | double-click adds magnet |
| `"provider:N"` | Multi-engine provider (e.g. apibay, Nyaa) | `searchWithProvider(N, query)` | `TorrentSearchResult` | Name/Size/Seeds/Leech | double-click adds magnet |

Source combo population logic (`searchdialog.cpp:136-146`):
1. Always add `"stremio"` (label `search_source_stremio`).
2. **Only if** `AddonManager::instance().torrentSearchEnabled()`: add `"legacy"` (label `search_source_torrents`) and `"games"` (label `search_source_games`).
3. For each enabled `SearchProvider` at index `i`: add label = provider name, data = `"provider:N"` (where N is the **index into the full `searchProviders()` list**, including disabled ones — see `searchdialog.cpp:143-146`, the loop indexes `i` over all providers but only `addItem`s the enabled ones, so `provider:N` carries the absolute index).

The category `QComboBox` (`m_categoryCombo`) is **only visible in `"legacy"` mode** (`searchdialog.cpp:160` hides by default, `:320` `setVisible(data=="legacy")`). Its items (`searchdialog.cpp:152-158`): All=0, Audio=100, Video=200, Apps=300, Games=400, Other=500.

The placeholder text changes per mode (`searchdialog.cpp:316-326`): games → `search_placeholder_games`, legacy/provider → `search_placeholder_torrent`, stremio → `search_placeholder`.

---

## 2. Element-by-element mapping (old → QML → backend)

### 2.1 Search input
- Old: `m_searchEdit` `QLineEdit`, clear-button enabled, `returnPressed` → `performSearch` (`searchdialog.cpp:172-181`).
- QML now: `TFld` with literal `text: "forza horizon 6"` (`SearchWindow.qml:61-66`). No clear button, no Enter handler.
- Target: `TFld { id: queryFld; ... }` with `onAccepted`/Enter → `bridge.search(...)`. `TFld` already wraps a `TextField` with an icon prop (`widgets/TFld.qml`). Add an explicit search button too (the old dialog has a primary "Buscar" button at `searchdialog.cpp:177-182` and again in the footer at `:279-284`).
- Backend: query is `m_searchEdit->text().trimmed()`, empty → no-op (`searchdialog.cpp:332-333`). Replicate the trim+empty guard in the bridge.

### 2.2 Source / provider selector
- Old: `m_sourceCombo` `QComboBox`, `currentIndexChanged` → `onSourceChanged` (`searchdialog.cpp:148-150`).
- QML now: a fake static `Rectangle` "Todos os provedores" + chevron, no dropdown, no logic (`SearchWindow.qml:67-83`).
- Target: replace with `TSelect` (`widgets/TSelect.qml` — the existing dropdown widget used elsewhere). Model = the source list built from the bridge (see `QmlSearchBridge.sources`). On change, store the selected source key and toggle category-selector visibility + placeholder text.

### 2.3 Category selector (legacy only)
- Old: `m_categoryCombo`, hidden unless legacy (`searchdialog.cpp:152-161`, `:320`).
- QML now: **completely missing.**
- Target: a second `TSelect`, `visible: currentSourceKey === "legacy"`, model = the 6 fixed categories. Value passed as int code to `bridge.search`.

### 2.4 Results header + table columns
- QML now: one fixed 6-column header NOME/PROVEDOR/TAMANHO/SEEDS/LEECH/IDADE + a `+` action column (`SearchWindow.qml:87-106`), and one `ListView` over the fake model (`SearchWindow.qml:109-167`).
- Target: columns must change per mode. Old dialog uses **four separate `QTableWidget`s** shown/hidden (`searchdialog.cpp:189-246`, `switchTo*` at `:476-510`). In QML, do **one `ListView`** whose header + delegate adapt to the active mode (a `currentMode` string property + per-mode column definitions), or keep it pragmatic with a column set computed from mode. Column specs by mode:

  - **Catalog** (`searchdialog.cpp:190-191`, `:373-384`): Name (stretch), Type, Year.
  - **Streams** (`searchdialog.cpp:203-204`, `:404-423`): Quality/title (stretch), Size (mono, `formatSize`), Addon name.
  - **Torrent / Provider** (`searchdialog.cpp:217-220`, `:436-465`): Name (stretch), Size (mono), Seeds (green if >0 else dim, `tm.stateSeedingColor()/dimColor()`), Leech (warning color `tm.warningColor()`).
  - **Games** (`searchdialog.cpp:233-236`, `:528-571`): Name (stretch), Repacker (color-coded — see 2.7), Size (mono), Seeds, Leech.

  **The design's PROVEDOR chip and IDADE columns have no backend source** — see Gotchas. Recommended: drop IDADE entirely; show PROVEDOR chip only when meaningful (provider/source name), otherwise omit. Keep Seeds/Leech color rules from the old code.

### 2.5 Row activation (the "+ / download / add" action)
- Old: every table activates on **double-click** (`cellDoubleClicked`, `searchdialog.cpp:199,212,228,244`).
  - Catalog row → `onItemDoubleClicked` → switch to streams + `getStreams(item.type, item.id)` (`searchdialog.cpp:386-402`).
  - Stream row → `onStreamDoubleClicked` → `m_session->addMagnet(stream.magnet, m_savePath)` if `magnet:` (`searchdialog.cpp:425-434`).
  - Torrent/Game row → `onTorrentDoubleClicked` / `onGameDoubleClicked` → `m_session->addMagnet(result.magnet, m_savePath)` (`searchdialog.cpp:467-474`, `:573-580`).
- QML now: the `+` button (`SearchWindow.qml:146-163`) has a hover-only `MouseArea` with **no `onClicked`**. The row `MouseArea` has `acceptedButtons: Qt.NoButton` (`SearchWindow.qml:165`).
- Target: wire both double-click on the row **and** click on the `+` button to `bridge.activateResult(mode, index)`. The bridge resolves: catalog → drill into streams; everything else → add magnet via session. (Adding a magnet is the equivalent of "download".)

### 2.6 Stream drill-down + Back button
- Old: catalog→streams is a navigation step. `m_backBtn` (`searchdialog.cpp:264-269`) is **only shown in streams view** (`switchToStreams` shows it, all others hide it). Back → `switchToCatalog` (`:268`).
- QML now: **no back button, no two-level navigation at all.**
- Target: a `currentMode === "streams"` state showing a Back button (use `BtnFlat`) in the bar/footer that returns to the catalog list (which must be preserved while drilling down). Status text while loading: `search_loading_streams` (`searchdialog.cpp:391`).

### 2.7 Repacker detection + colors (games mode)
- Old: `detectRepacker(name)` static (`searchdialog.cpp:512-526`) returns FitGirl/DODI/Online-Fix/ElAmigos/Xatab/R.G. Mechanics/GOG/CODEX/PLAZA/SKIDROW or "". Colors (`searchdialog.cpp:543-552`): FitGirl `#E91E63`, DODI `tm.stateDownloadingColor()`, Online-Fix `tm.stateSeedingColor()`, GOG `#9C27B0`, else `tm.dimColor()`.
- Target: do detection + color in the **bridge** (since it owns the model rows) and expose `repacker` + `repackerColor` (or a color key) per game row. Keep the same substring rules. Theme colors map to `Theme.*` in QML (downloading/seeding/dim).

### 2.8 Status / result-count line
- Old: `m_statusLabel` mono font (`searchdialog.cpp:250-258`). Set to: `search_searching` on start (`:342,350,357,368`); `search_done.arg(count)` on finish (`:291,306-307`); `search_streams_done.arg(count)` (`:295`); error string on `torrentSearchError` (`:309`); `search_no_catalog` / `search_no_stream` guards (`:362,397`).
- QML now: a single static `"42 resultados · 3 provedores"` label (`SearchWindow.qml:179`).
- Target: bind footer/status text to a `bridge.statusText` property (or a `searching` bool + `resultCount`). Show an `EmptyState` (`widgets/EmptyState.qml`) when no results and not searching.

### 2.9 Footer
- Old footer (`searchdialog.cpp:261-286`): Back (left, conditional) · stretch · Cancel/Close · primary "Buscar". Close → `reject()`.
- QML now: status label · stretch · Close `BtnFlat` → `win.close()` (`SearchWindow.qml:170-183`).
- Target: keep Close; add Back (conditional, streams mode); the primary Buscar can live next to the search field (matches the design `.sbar`). Match old behavior either way.

---

## 3. Backend API the bridge must call (exact signatures)

All on the `AddonManager` singleton, `AddonManager::instance()` (`addonmanager.h:74`). It is a `QObject` with **async signals** — results arrive via signals, not return values.

Methods:
- `bool torrentSearchEnabled() const` — `addonmanager.h:108` / `.cpp:407` (gates legacy+games in source list).
- `QList<SearchProvider> searchProviders() const` — `addonmanager.h:101` / `.cpp:584` (populate provider entries; struct has `name`, `enabled`, `addonmanager.h:55-68`).
- `void searchCatalog(const QString &query)` — `addonmanager.h:89` / `.cpp:218`. Has generation-counter de-dup (`m_catalogGen`, `.cpp:222,241`) — typing fast is safe.
- `void getStreams(const QString &type, const QString &id)` — `addonmanager.h:92` / `.cpp:283`. Generation-guarded (`m_streamGen`).
- `void searchTorrents(const QString &query, int category)` — `addonmanager.h:112` / `.cpp:431` (legacy + games; games passes `400`).
- `void searchWithProvider(int providerIndex, const QString &query, int category=0)` — `addonmanager.h:105` / `.cpp:653`.
- `bool hasCatalogAddon() const` / `bool hasStreamAddon() const` — `addonmanager.h:85-86` / `.cpp:119,126` (guards before catalog/stream search).

Signals to connect (all `addonmanager.h:114-124`):
- `catalogResults(QList<CatalogItem>)` `.cpp:271` — incremental, may fire multiple times per search (per addon/type).
- `catalogFinished()` `.cpp:273,279`.
- `streamResults(QList<StreamResult>)` `.cpp:348`.
- `streamFinished()` `.cpp:350,355`.
- `torrentSearchResults(QList<TorrentSearchResult>)` `.cpp:498,681` — used by legacy, games, **and** provider modes (shared signal; old dialog disambiguates via `m_isGameSearch`, `searchdialog.cpp:296-302`).
- `torrentSearchFinished()` `.cpp:499,682`.
- `torrentSearchError(QString)` `.cpp:434,453,460,677`.

Result structs (`addonmanager.h`):
- `CatalogItem { id, type, name, poster, year }` — `addonmanager.h:26-32`.
- `StreamResult { addonName, title, magnet, quality, size }` — `addonmanager.h:35-41`.
- `TorrentSearchResult { name, infoHash, magnet, size, seeders, leechers, category }` — `addonmanager.h:44-52`.

Session call for "add"/"download":
- The old dialog calls `m_session->addMagnet(magnet, m_savePath)` directly (`searchdialog.cpp:431,472,578`). The bridge already has `QmlSessionBridge::addMagnetUri(const QString &uri)` (`qmlposterbridge.h:156`) — **but it does not take a save path** (it uses the default). For parity, the Search bridge should call `SessionManager::addMagnet(magnet, savePath)` itself (it can hold a `SessionManager*` like `QmlSessionBridge` does, `qmlposterbridge.h:219`), OR accept that magnets go to the default save path via `session.addMagnetUri`. **Recommendation:** give `QmlSearchBridge` a `SessionManager*` and call `addMagnet(magnet, savePath)` so the save-path semantics match the old dialog. The save path in the old dialog is the ctor arg `m_savePath` (`searchdialog.cpp:34-35`); in QML, default to `session.defaultSavePath()` (`qmlposterbridge.h:164`).

Helper: `formatSize(qint64)` from `../app/utils.h` (`searchdialog.cpp:7`, used at `:417,450,556`) — the bridge should format sizes to strings before exposing them to QML (QML rows are display strings, matching the AddTorrentDialog pattern where the bridge pre-formats `totalSize`/file sizes).

Translation keys used (for i18n parity — `searchdialog.cpp`): `search_title`, `search_eyebrow`, `search_heading`, `search_subtitle`, `search_section_source`, `search_section_query`, `search_section_results`, `search_source_stremio/torrents/games`, `search_cat_all/audio/video/apps/games/other`, `search_placeholder[_games/_torrent]`, `search_btn`, `search_back`, `search_col_name/type/year/quality/size/addon/seeds/leechers/repacker`, `search_searching`, `search_done`, `search_streams_done`, `search_loading_streams`, `search_no_catalog`, `search_no_stream`, `search_added`, `btn_cancel`. The current QML hard-codes Portuguese; decide whether to expose these via the bridge or keep literal strings (rest of new QML appears to hard-code PT — `MagnetDialog.qml` etc.).

---

## 4. Proposed bridge additions — `QmlSearchBridge`

Add to `/Users/mateuscruz/Projects/BAT-Torrent/src/gui/qmlposterbridge.h` (and `.cpp`). Mirrors the `QmlRssBridge` pattern (wraps a singleton, exposes `QVariantList` + invokables + a `*Changed` signal — see `qmlposterbridge.h:253-271`). Holds a `SessionManager*` for adds (like `QmlSessionBridge`).

```cpp
class SessionManager; // already fwd-declared

class QmlSearchBridge : public QObject
{
    Q_OBJECT
    // Source/provider dropdown entries: [{ key, label }, ...]
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
    // Fixed category entries for legacy mode: [{ code, label }, ...]
    Q_PROPERTY(QVariantList categories READ categories CONSTANT)
    // Current result rows (schema varies by mode; each is a QVariantMap
    // pre-formatted for display). QML binds a ListView/Repeater to this.
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    // "catalog" | "streams" | "torrent" | "games"
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    Q_PROPERTY(bool inStreams READ inStreams NOTIFY modeChanged) // controls Back button

public:
    explicit QmlSearchBridge(SessionManager *session, QObject *parent = nullptr);

    QVariantList sources() const;     // built from torrentSearchEnabled + searchProviders
    QVariantList categories() const;  // 6 fixed entries
    QVariantList results() const;
    QString mode() const;
    bool searching() const;
    QString statusText() const;
    bool inStreams() const;

    // sourceKey: "stremio" | "legacy" | "games" | "provider:N"; categoryCode used only for legacy
    Q_INVOKABLE void search(const QString &sourceKey, const QString &query, int categoryCode = 0);
    Q_INVOKABLE void cancel();                  // bump a local generation so late results are dropped
    Q_INVOKABLE void activateResult(int index); // catalog→drill to streams; else add magnet to session
    Q_INVOKABLE void back();                    // streams → catalog
    Q_INVOKABLE QString savePath() const;       // defaults to session default save path

signals:
    void sourcesChanged();
    void resultsChanged();
    void modeChanged();
    void searchingChanged();
    void statusChanged();

private:
    SessionManager *m_session;
    QString m_mode;
    QString m_savePath;
    QVariantList m_results;
    QList<CatalogItem> m_catalogCache;   // preserved for back()
    QList<StreamResult> m_streamCache;
    QList<TorrentSearchResult> m_torrentCache;
    bool m_isGameSearch = false;
    bool m_searching = false;
    QString m_status;
    // connect AddonManager signals once in ctor; route by current mode
};
```

**Counts:** 1 class, 7 Q_PROPERTY (sources, categories, results, mode, searching, statusText, inStreams), 5 Q_INVOKABLE (search, cancel, activateResult, back, savePath), 5 signals.

**Design notes / decisions:**
- Each `results` row is a `QVariantMap` with display-ready string fields + a stable role set so the QML delegate can render any mode. Suggested keys: `{ name, sub, sizeStr, seeds, leech, seedsColorKey, repacker, repackerColorKey, addonName, year, type }` — populate only what the mode needs. Pre-format `size` via `formatSize`.
- `activateResult(index)` uses `m_mode`: catalog → `getStreams(catalog[index].type, catalog[index].id)` and set mode to "streams"; streams → `addMagnet(stream.magnet, savePath)`; torrent/games → `addMagnet(result.magnet, savePath)`. Mirrors `searchdialog.cpp:386-402, 425-434, 467-474, 573-580`.
- `cancel()` is the new affordance the task asks for; the old dialog had none. Implement with a local generation counter so a stale `*Results` signal that lands after cancel is ignored (the AddonManager already de-dups catalog/stream internally, but legacy/provider have no guard — `searchTorrents`/`searchWithProvider` fire whatever lands; a bridge-side gen counter is the safe fix).
- `search()` must replicate the guards: empty-query no-op (`searchdialog.cpp:332`); `hasCatalogAddon()` check before stremio (`:361-364`); `hasStreamAddon()` check before streams (`:396-399`); set `m_isGameSearch` for the games branch so `torrentSearchResults` is routed to the games column set (`:298-301`).
- `sources()` rebuilds each time it's read (cheap) or on a `sourcesChanged` emitted when settings change. The source list depends on `torrentSearchEnabled()` and the enabled providers — both can change in Settings, so emit `sourcesChanged` when the Search window opens (or connect to AddonManager if it ever signals provider changes; it currently does not).

### Registration (main.cpp) — exact location confirmed
The QML path is the `--qml` branch in `main.cpp:133-190`. `SessionManager session;` is at `main.cpp:136`; the bridges are created at `main.cpp:138-143`; the `setContextProperty` block is `main.cpp:181-185` (`torrentModel`/`torrentFilter`/`themeBridge`/`session`/`rss`), all on `engine.rootContext()` (`main.cpp:180`). Add **two lines** mirroring the `rssBridge` pattern:

```cpp
// near main.cpp:143, after rssBridge:
auto *searchBridge = new QmlSearchBridge(&session, &app);   // &session is the SessionManager
// near main.cpp:185, after the "rss" line:
engine.rootContext()->setContextProperty("search", searchBridge);
```

`#include "app/addonmanager.h"` is **not** needed in `main.cpp` (the bridge `.cpp` includes it); `main.cpp` already includes `gui/qmlposterbridge.h` (`main.cpp:18`).

### Window launch — already wired
`SearchWindow.qml` is a top-level `Window`. It is **already instantiated** in `Main.qml:1737` as `SearchWindow { id: searchWin; visible: false }` (alongside `rssWin` `:1738` and `settingsWin` `:1739`), and already opened via `searchWin.show()` from two places: the native menu item (`Main.qml:183`, "Buscar torrents…") and the toolbar button (`Main.qml:374`, `TBtn { label: "Buscar"; ... onClicked: searchWin.show() }`). **No `Main.qml` changes are needed for launching** — the only work is rewriting `SearchWindow.qml`'s body and adding the bridge. (Note: `Main.qml:487-523` is a *different* search — an inline filter box over the local torrent list, calling `torrentFilter.setSearchText` — unrelated to this screen.)

### CMake
No new source file needed if `QmlSearchBridge` is added to the existing `qmlposterbridge.h/.cpp` (already in the build). `SearchWindow.qml` is presumably already in the QML resource set (it exists alongside the other windows); confirm it's listed in the `qt_add_qml_module`/`.qrc` in `CMakeLists.txt` and add it if missing.

---

## 5. Gotchas / design-vs-backend mismatches

1. **IDADE (age) column has NO backend source.** None of `CatalogItem`/`StreamResult`/`TorrentSearchResult` carries an age/date field (`addonmanager.h:26-52`). The design `SearchWindow.qml:103,144` shows it. **Drop the IDADE column** (or leave blank). Do not invent it.
2. **PROVEDOR chip has no per-row source in torrent modes.** `TorrentSearchResult` has `category`, not a provider/source name (`addonmanager.h:51`). The "provider" is the *search source* (same for all rows in a search), not per-row. Either drop the chip or set it to the active source name. The design's per-row varied providers ("The Pirate Bay" / "1337x" / "RuTracker") are **fake** — the real backend returns rows from one source at a time.
3. **Columns change per mode** — the single fixed header in the current QML is wrong for catalog/streams/games. Must be dynamic (4 column sets).
4. **`torrentSearchResults` is shared** across legacy/games/provider. Disambiguate with the bridge's own `m_mode`/`m_isGameSearch` exactly like `searchdialog.cpp:296-302`.
5. **Catalog results arrive incrementally** (`catalogResults` may fire several times, once per addon/type, `addonmanager.cpp:230-275`). Don't reset the list on each emission — the signal already carries the full accumulated `m_catalogResults`; just replace `results` wholesale each emit.
6. **Save path:** `QmlSessionBridge::addMagnetUri` ignores save path. Use a direct `SessionManager::addMagnet(magnet, savePath)` from the Search bridge for parity with `searchdialog.cpp:431,472,578`.
7. **`search.svg` / `chevron.svg` icons** referenced as `qrc:/icons/...` in the stub (`SearchWindow.qml:64,80`) — confirm they exist in the resource set (other widgets use the same scheme, so likely fine).
8. **No `cancel` in the old dialog** — the task asks to add it; there is no QWidget behavior to mirror, so implement it as a bridge-side generation guard + reset of `searching`/`statusText`.

---

## 6. Implementation checklist

**Bridge (`qmlposterbridge.h` + `.cpp`):**
- [ ] Add `QmlSearchBridge` class per section 4 (ctor takes `SessionManager*`; include `addonmanager.h` for the structs/singleton).
- [ ] Implement `sources()`: always `{key:"stremio"}`; if `torrentSearchEnabled()` add `legacy` + `games`; append enabled providers as `provider:N` with absolute index (mirror `searchdialog.cpp:136-146`).
- [ ] Implement `categories()`: 6 fixed entries All/Audio/Video/Apps/Games/Other = 0/100/200/300/400/500.
- [ ] Connect all 7 AddonManager signals in ctor; route by `m_mode`/`m_isGameSearch`; build `results` `QVariantList` with pre-formatted (`formatSize`) display fields + seed/leech color keys + repacker (move `detectRepacker` + color logic from `searchdialog.cpp:512-571`).
- [ ] Implement `search()` with empty-query guard, `hasCatalogAddon`/`hasStreamAddon` guards, mode dispatch (stremio→`searchCatalog`; legacy→`searchTorrents(q,cat)`; games→`searchTorrents(q,400)`+`m_isGameSearch=true`; provider→`searchWithProvider(N,q)`), set `searching=true` + status `search_searching`, bump local gen.
- [ ] Implement `activateResult(index)` (catalog→`getStreams`+mode=streams; else `addMagnet(magnet, savePath)`), `back()` (restore catalog from cache), `cancel()` (bump gen, clear searching/status), `savePath()`.
- [ ] Emit `statusChanged`/`searchingChanged`/`resultsChanged`/`modeChanged` appropriately (searching/done/streams-done/error/no-catalog/no-stream).
- [ ] Register `searchBridge` as context property `"search"` in `main.cpp` (next to `rss`).
- [ ] Ensure `SearchWindow.qml` is in the QML module/resource list in `CMakeLists.txt`.

**QML (`SearchWindow.qml` — rewrite the body, keep the `Window` shell + design tokens):**
- [ ] Delete the hard-coded `ListModel` (`SearchWindow.qml:16-24`).
- [ ] Replace the fake provider `Rectangle` with a `TSelect` bound to `search.sources`; on change store `currentSourceKey`, toggle category visibility + placeholder.
- [ ] Add a category `TSelect` (`visible: currentSourceKey === "legacy"`) bound to `search.categories`.
- [ ] Wire `TFld` (clear the literal text, add Enter handler) + a primary "Buscar" button → `search.search(currentSourceKey, queryFld.text, currentCategoryCode)`.
- [ ] Make the results header + delegate adapt to `search.mode` (4 column sets per section 2.4); apply seed/leech/repacker colors via `Theme.*`.
- [ ] Bind `ListView.model: search.results`; double-click row and `+`-button click → `search.activateResult(index)`.
- [ ] Add a Back button (`BtnFlat`) visible when `search.inStreams`, → `search.back()`.
- [ ] Bind status/count line to `search.statusText`; show `EmptyState` when `!search.searching && search.results.length === 0`; show a loading affordance when `search.searching`.
- [ ] Add a Cancel control (button or in footer) → `search.cancel()` visible while `search.searching`.
- [ ] Keep Close → `win.close()`.
- [ ] No `Main.qml` change needed — `searchWin` is already instantiated (`Main.qml:1737`) and opened from the menu (`:183`) and toolbar (`:374`).

**Verify (4 modes):**
- [ ] Stremio: search → catalog rows → double-click → streams rows → Back → catalog; double-click stream adds magnet.
- [ ] Legacy (requires `torrentSearchEnabled` + URL in settings): category selector visible; results + seed/leech colors; double-click/+ adds magnet.
- [ ] Games: repacker column + colors; cat 400.
- [ ] Provider (apibay / Nyaa default providers): results + add.
- [ ] `cancel()` mid-search stops the spinner and ignores late results.
