# 15 — Main Window Actions: QWidget → QML Parity

> DATA-QUALITY NOTE
>
> Verified line-by-line this session: `src/gui/qmlposterbridge.h` + `.cpp`
> (full), `src/torrent/sessionmanager.h` (full), `src/qml/Main.qml` (read in full
> — context menu, native menu bar, toolbar, shortcuts).
>
> NOT readable this session (tool channel outage — repeated empty returns on
> Read/Bash/Grep): `src/gui/mainwindow.cpp` (128 KB) and `src/gui/mainwindow.h`.
> Both DO exist on disk; an earlier draft wrongly claimed they were deleted —
> that was a mistake caused by `Glob` being unavailable in this environment and
> erroring out, not by the files being absent. The original widget UI is intact.
>
> Consequence: the parity diff below is built from the **bridge surface** (the
> exact set of slots the QML actually calls) plus `Main.qml`. That is reliable
> for "what is wired in QML today." The exact original-widget action **labels,
> ordering, and keyboard shortcuts** in `mainwindow.cpp` could not be transcribed
> and are reconstructed from the bridge/SessionManager capabilities. Before
> implementing, do step 0 of the checklist: read `mainwindow.cpp` and confirm the
> handful of rows marked **[CONFIRM]**.

---

## 1. Architecture: how QML actions reach the backend

`QmlPosterBridge` exposes 5 context properties to QML (`qmlposterbridge.h:81`+,
constructed `qmlposterbridge.cpp`):

- `session` → `QmlSessionBridge` (NOT `SessionManager` directly)
- `torrentModel` → `QmlPosterModel`
- `torrentFilter` → `QmlTorrentFilterProxy`
- `themeBridge` → `QmlThemeBridge`
- `rss` → `QmlRssBridge`

CRITICAL: QML does **not** call `SessionManager` invokables. It calls
`session.<slot>()` on `QmlSessionBridge`, which internally forwards to
`SessionManager` using a single **selection index** (`m_selectedIndex` /
`m_selectedRows`, `qmlposterbridge.cpp:368-383`). So adding any new per-torrent
action means adding a **`Q_INVOKABLE` on `QmlSessionBridge`** that operates on the
current selection — even when `SessionManager` already has the underlying method.
This is the bulk of the work and the reason the "new C++" count is non-trivial.

`QmlSessionBridge` per-torrent invokables that ALREADY exist
(`qmlposterbridge.h:146-174`):
`setSelectedIndex/Rows`, `pauseSelected`, `resumeSelected`, `removeSelected`,
`removeSelectedWithFiles`, `pauseAll`, `resumeAll`, `addTorrentFile`,
`addMagnetUri`, `previewTorrent`, `addTorrentWithPrefs`, `copyMagnetLink`,
`copyInfoHash`, `openSaveFolder`, `defaultSavePath`, `setSelectedForceStart`,
`setSelectedSuperSeeding`, `markSelectedCompleted`, `unmarkSelectedCompleted`,
`forceRecheckSelected`, `forceReannounceSelected`, `queueUpSelected`,
`queueDownSelected`, `toggleSelectedPause`, `smartPaste`.

`SessionManager` capabilities that exist but are **NOT yet bridged** to QML
(no `QmlSessionBridge` wrapper) — these are the gaps:
rename (`setName`-equivalent is via libtorrent; SM has no public setName, see
below), `moveStorage` :62, `renameFile` :59, `setFilePriority` :51,
`setSequentialDownload` :52, per-torrent rate caps `setTorrentDownloadLimit`
/`setTorrentUploadLimit` :178-179, queue top/bottom (SM only has
`setTorrentQueuePosition` :285 — top/bottom must be computed), categories
`setTorrentCategory`/`categories` :71-72, tags
`setTorrentTags`/`torrentTags`/`allTags` :80-82, trackers
`addTracker`/`replaceTrackers` :49,66, recently-removed
`recentlyRemoved`/`restoreRemoved`/`clearRemovedHistory` :227-229, export
`.torrent` (NO SM method — see §6 C-4), import qBittorrent
`importFromQBittorrent` :377, `stopSeedingTorrent` :163, `torrentRootPath` :234.

> NOTE on rename: `SessionManager` has **no** public "set display name" method —
> there is `renameFile` (per-file) :59 but not a torrent-rename. qBittorrent-style
> "Rename torrent" would need a new SM method or is out of scope. **[CONFIRM]**
> against `mainwindow.cpp` whether the widget UI even had a rename-torrent action.

---

## 2. What Main.qml ALREADY has (verified, `src/qml/Main.qml`)

### 2a. Right-click context menu (`Main.qml:113-153`, `ctxMenu`)
Shown on right-click of a grid tile (`:907-910`) or list row (`:1198-1201`):
1. Pausar → `session.pauseSelected()` :142
2. Retomar → `session.resumeSelected()` :143
3. Abrir pasta → `session.openSaveFolder()` :145
4. Copiar link magnet → `session.copyMagnetLink()` :146
5. Copiar hash → `session.copyInfoHash()` :147
6. Forçar verificação → `session.forceRecheckSelected()` :149
7. Forçar reanúncio → `session.forceReannounceSelected()` :150
8. Remover… → `removeDlg.open()` :152

### 2b. Native menu bar (`Main.qml:158-195`, `Platform.MenuBar`)
- **Arquivo**: Abrir torrent… (`StandardKey.Open`→openFileDlg) :161; Adicionar
  magnet… (`Ctrl+M`→magnetDlg) :162; Criar torrent… (createDlg) :163; Sair
  (`StandardKey.Quit`) :165.
- **Torrent**: Pausar :169; Retomar :170; Pausar todos (`pauseAll`) :172; Retomar
  todos (`resumeAll`) :173; Remover… (`StandardKey.Delete`) :175.
- **Configurações**: Preferências… (`StandardKey.Preferences`→settingsWin) :179;
  Addons… (addAddonDlg) :180; RSS… (rssWin) :181; Buscar torrents…
  (searchWin) :183; Teste de velocidade (opens fast.com) :185.
- **Ajuda**: Boas-vindas :189; Notas de versão :190; Doar :192; Sobre :193.

### 2c. Toolbar (`Main.qml:362-378`)
Abrir, Magnet, Pausar, Retomar, Parar (=pause), Remover, Buscar, RSS, Config.

### 2d. Shortcuts (`Main.qml`)
Only `StandardKey.SelectAll` :155. Plus the menu-attached `shortcut:` on
Open/Magnet/Quit/Delete/Preferences. **No Space, F5, Ctrl+Up/Down, etc.**

### 2e. Dialogs already ported (in `src/qml/`)
AddTorrentDialog, MagnetDialog, RemoveDialog, CreateTorrentDialog, AddAddonDialog,
AboutDialog, ReleaseNotesDialog, WelcomeDialog, SearchWindow, SettingsWindow,
RssWindow. (Equivalent widget dialogs that have NO QML yet: `logviewerdialog`,
`diagnosticsdialog`, `statisticsdialog`, `removedhistorydialog`,
`torrentinspectordialog`, `shortcutsdialog`, `pairingdialog` — see §4/§5.)

---

## 3. Context-menu items MISSING in QML

Each row: the action → the `QmlSessionBridge` wrapper needed (✗ = must be added,
✓ = already exists) → the underlying `SessionManager` method.

| Action | Bridge wrapper | SessionManager backing |
|---|---|---|
| Force start (toggle) | ✓ `setSelectedForceStart(on)` :165 + read `selectedForceStart` :194 | `setForceStart`/`isForceStart` sm:172-173 |
| Super seeding (toggle) | ✓ `setSelectedSuperSeeding(on)` :166 + `selectedSuperSeeding` :195 | `setSuperSeeding`/`isSuperSeeding` sm:167-168 |
| Mark / unmark completed | ✓ `markSelectedCompleted` / `unmarkSelectedCompleted` :167-168 | `markCompleted`/`unmarkCompleted` sm:187-188 |
| Queue → Up / Down | ✓ `queueUpSelected` / `queueDownSelected` :171-172 | `setTorrentQueuePosition` sm:285 |
| Queue → Top / Bottom | ✗ add `queueTopSelected`/`queueBottomSelected` | compute pos 0 / count-1, then `setTorrentQueuePosition` |
| Pause/Resume single toggle | ✓ `toggleSelectedPause` :173 | pause/resume sm:39-40 |
| Stop seeding now | ✗ add `stopSeedingSelected` | `stopSeedingTorrent` sm:163 |
| Set location… (move storage) | ✗ add `moveSelectedStorage(path)` (+ getter for prefill) | `moveStorage` sm:62 |
| Set download limit… | ✗ add `setSelectedDownloadLimit(kbps)` (+ getter) | `setTorrentDownloadLimit`/`torrentDownloadLimit` sm:178,180 |
| Set upload limit… | ✗ add `setSelectedUploadLimit(kbps)` (+ getter) | `setTorrentUploadLimit`/`torrentUploadLimit` sm:179,181 |
| Sequential download (toggle) | ✗ add `setSelectedSequential(on)` (+ getter — SM has no isSequential getter, **[CONFIRM]** / may need new SM getter) | `setSequentialDownload` sm:52 |
| Category → set / new | ✗ add `setSelectedCategory(name)`, expose `categories()`/`addCategory` | `setTorrentCategory`/`categories` sm:71-72 (no `addCategory`; category is created implicitly by `setCategorySavePath` or by assigning — **[CONFIRM]**) |
| Tags → toggle / add | ✗ add `selectedTags()`, `setSelectedTags(list)`, expose `allTags()` | `torrentTags`/`setTorrentTags`/`allTags` sm:80-82 |
| Add tracker… | ✗ add `addTrackerToSelected(url)` | `addTracker` sm:49 |
| Remove tracker | ✗ add `removeTrackerFromSelected(url)` | `replaceTrackers` (drop one) sm:66 |
| Rename file… (in Files tab) | ✗ add `renameSelectedFile(fileIdx, name)` | `renameFile` sm:59 |
| Set file priority (in Files tab) | ✗ add `setSelectedFilePriority(fileIdx, prio)` | `setFilePriority` sm:51 |
| Export .torrent… | ✗ add `exportSelected(path)` | **NO SM method** — see §6 C-4 |
| Copy name | ✗ add `copySelectedName` | trivial (clipboard) |
| Open file (not just folder) | ✗ add `openSelectedFile` | `torrentRootPath` sm:234 + QDesktopServices |
| Properties / Inspect | ✗ open a QML inspector window | reuse `selectedTrackers`/`selectedPeers`/`selectedFiles` already on bridge :199-202 |
| Remove + delete files | ✓ `removeSelectedWithFiles` :152 (RemoveDialog already has the checkbox) | `removeTorrent(idx,true)` sm:38 |

Toggle items must reflect state for the **focus** torrent via the existing
`selected*` properties (`selectedForceStart`, `selectedSuperSeeding`,
`selectedPaused` :194-198). Note multi-select forwarding already works for
pause/resume/remove/queue (`qmlposterbridge.cpp:385-582`); new toggles should
follow the same `m_selectedRows` loop pattern.

## 4. Menu-bar actions MISSING in QML

| Menu action | Bridge wrapper | Backing |
|---|---|---|
| Import from qBittorrent | ✗ add `importQbittorrent(savePath)` | `importFromQBittorrent` sm:377 |
| Export settings / Import settings | ✗ add (read/write QSettings to file) | **NO SM method** — new helper, **[CONFIRM]** widget had it |
| Full backup / Full restore | ✗ add | **NO SM method** — new helper, **[CONFIRM]** (commit `4fde8ce` "Backup" suggests it existed) |
| Recently removed… (window) | ✗ expose `recentlyRemoved()`, `restoreRemoved(hash)`, `clearRemovedHistory()` on bridge + new `RemovedHistoryWindow.qml` | sm:227-229; widget `removedhistorydialog.cpp` exists |
| Check for update | ✗ expose `Updater` (new bridge property OR invokable) | `app/updater.h` — not on bridge |
| Statistics… | ✗ expose `globalDownloaded/Uploaded/globalRatio` (bridge already has these as `total*`/`globalRatio` :92-94) + `detailedStats()`/`sessionStats` | sm:354-371; widget `statisticsdialog.cpp` |
| Diagnostics… | ✗ new window + expose `detailedStats()` | sm `DetailedStats` :363; widget `diagnosticsdialog.cpp` |
| Log viewer… | ✗ expose `Logger` on bridge + new window | `app/logger.h`; widget `logviewerdialog.cpp` |
| Torrent inspector… | ✗ new window | widget `torrentinspectordialog.cpp` |
| Keyboard shortcuts… (help) | ✗ new static window | widget `shortcutsdialog.cpp` |
| WebUI pairing / QR… | ✗ expose `WebServer` + pairing | `webui/webserver.*`; widget `pairingdialog.cpp` |

**[CONFIRM]** the exact set/labels against `mainwindow.cpp`'s `menuBar()` setup —
this list is reconstructed from the widget-dialog file names + SessionManager
surface, not transcribed from the menu code.

## 5. Keyboard shortcuts MISSING in QML

Main.qml currently has only SelectAll + menu-attached Open/Magnet/Quit/Delete/
Preferences. The widget app almost certainly bound more (it ships a
`shortcutsdialog.cpp`, implying a documented set). **[CONFIRM] by grepping
`mainwindow.cpp` for `setShortcut` / `QKeySequence` / `QShortcut`** and mirror each
with a QML `Shortcut { sequence: … }`. Likely-missing: Space (pause/resume toggle
→ `toggleSelectedPause`), F5 (recheck → `forceRecheckSelected`), Ctrl+Up/Down
(queue → `queueUpSelected`/`queueDownSelected`), Ctrl+F (focus search), Enter/
double-click (open folder — list already does this `:1205-1207`).

## 6. New C++ / bridge additions required

Count of distinct additions: **~22 `QmlSessionBridge` invokables + getters, 3
new bridge properties, and up to 4 brand-new backend helpers.** Breakdown:

- **C-1 `QmlSessionBridge` per-torrent invokables (✗ rows in §3/§4).** ~22 new
  `Q_INVOKABLE`s wrapping existing `SessionManager` methods against the current
  selection: `queueTopSelected`, `queueBottomSelected`, `stopSeedingSelected`,
  `moveSelectedStorage`, `setSelectedDownloadLimit`/`UploadLimit` (+getters),
  `setSelectedSequential` (+getter), `setSelectedCategory` (+`categories`
  exposure), `selectedTags`/`setSelectedTags`/`allTags`, `addTrackerToSelected`,
  `removeTrackerFromSelected`, `renameSelectedFile`, `setSelectedFilePriority`,
  `copySelectedName`, `openSelectedFile`, `importQbittorrent`. Each follows the
  existing pattern (`qmlposterbridge.cpp:527-535` for the simple ones,
  `:385-414`/`:537-582` for multi-select loops). LOW risk — mechanical.
- **C-2 Recently-removed exposure.** Add `recentlyRemoved()→QVariantList`,
  `restoreRemoved(hash)`, `clearRemovedHistory()` to `QmlSessionBridge` (wrap
  `SessionManager::recentlyRemoved`/`restoreRemoved`/`clearRemovedHistory`
  sm:227-229; convert the `RemovedEntry` struct sm:220-226 to a `QVariantMap`).
  New `RemovedHistoryWindow.qml`.
- **C-3 New bridge properties for managers not yet exposed.** Mirror the existing
  5-property pattern in `qmlposterbridge.h:81`+ / ctor:
  - `Updater* updater` (for Check-for-update) — `app/updater.h`.
  - `Logger* logger` (for log viewer) — `app/logger.h`.
  - `WebServer*` (for WebUI pairing/QR) — `webui/webserver.h`.
  Each: forward-declare, add `Q_PROPERTY(... CONSTANT)`, construct in ctor.
  Only add the ones whose menu items you actually keep (gate on §4 **[CONFIRM]**).
- **C-4 Brand-new backend helpers (no SessionManager method today).**
  - Export `.torrent`: write `torrent_info` of the selected handle to a file
    (libtorrent `create_torrent` from existing `torrent_info`). The widget had
    this (commit `dfae0db` "torrent export"); confirm where it lived — likely a
    free function in `mainwindow.cpp`, must move to SM or the bridge.
  - Export/Import settings + Full backup/Restore: QSettings dump/load + file
    copy of the app-data dir. Confirm the widget implementation (commit
    `4fde8ce` "Backup") and port it.
- **C-5 Categories submenu data.** `SessionManager::categories()` sm:72 returns
  the list, but there is **no public `addCategory`**; a new category is created by
  assigning it (`setTorrentCategory`) and/or `setCategorySavePath` sm:73.
  Expose `categories()` and emit on change (SM has no `categoriesChanged` signal —
  **[CONFIRM]**; may need to add one or just re-read on menu open).

> Clipboard helpers are already done correctly: `copyMagnetLink`/`copyInfoHash`
> call `QGuiApplication::clipboard()->setText(...)` directly
> (`qmlposterbridge.cpp:428-440`). `copySelectedName` should follow suit.

## 7. Implementation checklist (ordered)

- [ ] **0. Recover ground truth (do FIRST — blocked this session by tool outage).**
  Read `src/gui/mainwindow.cpp` + `.h` end-to-end. Specifically:
  grep `menuBar`/`addMenu`/`addAction` for the real menu tree;
  grep `setShortcut`/`QKeySequence`/`QShortcut` for the real shortcut set;
  grep `customContextMenuRequested`/`exec(`/`QMenu` for the real per-torrent
  context menu. Resolve every **[CONFIRM]** above and strike anything already
  present in `Main.qml`.
- [ ] 1. Add the §6 C-1 `QmlSessionBridge` invokables + selection-state getters.
- [ ] 2. Build `TorrentContextMenu.qml` (reuse the inline `CtxItem` styling at
  `Main.qml:123-141`) wiring all §3 items; add submenus for Queue, Category,
  Tags; make toggles reflect `selected*` state.
- [ ] 3. Small input dialogs (copy the `MagnetDialog.qml`/`BatDialog` pattern):
  Rate limit (dl/ul), Set location (folder picker), New category, Add tag, Add
  tracker, Add peer, Rename file.
- [ ] 4. Menu-bar additions (§4): Import qBittorrent, Export/Import settings,
  Backup/Restore — each a file dialog → C-1/C-4 invokable.
- [ ] 5. Recently-removed: C-2 bridge methods + `RemovedHistoryWindow.qml`.
- [ ] 6. Properties/Inspector window using existing
  `selectedTrackers/Peers/Files` (`qmlposterbridge.h:199-202`).
- [ ] 7. C-3 bridge properties (Updater/Logger/WebServer) + their menu items
  (Check for update, Log viewer, Pairing) — only the ones the widget had.
- [ ] 8. C-4 backend helpers: export `.torrent`, settings export/import, backup/
  restore (port from the widget implementation).
- [ ] 9. Keyboard shortcuts (§5): add QML `Shortcut` elements mirroring the
  recovered set.
- [ ] 10. Statistics / Diagnostics / Log / Shortcuts-help windows (port the
  remaining widget dialogs).
- [ ] 11. **Tray is OUT OF SCOPE.** `traypopup.cpp` actions are not part of this
  pass — note them for a follow-up doc.

## 8. Effort estimate

- Step 0 (recover + diff mainwindow.cpp, resolve [CONFIRM]s): ~0.5 day.
- C-1 bridge invokables + getters (~22, mechanical): ~1 day.
- Context menu + submenus + ~7 small dialogs: ~2 days.
- Menu-bar (import/export/backup/restore) + C-4 helpers: ~1.5 days.
- Recently-removed + inspector + C-3 manager properties + their windows: ~1.5 days.
- Shortcuts + remaining ported windows (stats/diag/log/help): ~1 day.
- **Total ≈ 6.5–7.5 days.** Risk is concentrated in C-4 (export .torrent,
  settings backup/restore) where there is no ready SessionManager method and the
  widget logic must be located and ported; everything else is wrapping an
  existing `SessionManager` call in a selection-aware `QmlSessionBridge` slot.
