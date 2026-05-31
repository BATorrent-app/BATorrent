# Screen 14 — RSS (QWidget → QML migration plan)

Single source of truth for finishing the RSS screen port. Status: **PARTIALLY wired.**

Files inspected (all fully read this session):
- QML (new): `src/qml/RssWindow.qml`
- Legacy QWidget: `src/gui/rssdialog.h`, `src/gui/rssdialog.cpp`
- Backend: `src/app/rssmanager.h`, `src/app/rssmanager.cpp`
- Bridge header: `src/gui/qmlposterbridge.h:252-271` (`QmlRssBridge`)
- Bridge impl: `src/gui/qmlposterbridge.cpp:923-1006` (`QmlRssBridge` method bodies — verified)
- Wiring: `src/main.cpp:142-143,185` (`rss` context property)

Stale duplicate `src/qml-new/RssWindow.qml` exists — ignore / delete; live file is `src/qml/RssWindow.qml`.

---

## 1. What currently WORKS (verified wired)

| Feature | QML site | Bridge method (impl line) | RssManager backing |
|---|---|---|---|
| Feed list (name, enabled dot, count) | `RssWindow.qml:115`, delegate `:119-164` | `feeds()` `qmlposterbridge.cpp:931` | `RssManager::feeds()` `rssmanager.cpp:114` |
| Add feed (inline overlay) | `RssWindow.qml:28-61,91` | `addFeed(url)` `qmlposterbridge.cpp:965` | `RssManager::addFeed` `rssmanager.cpp:51` |
| Remove feed (context menu) | `RssWindow.qml:162` | `removeFeed(index)` `qmlposterbridge.cpp:973` | `RssManager::removeFeed` `rssmanager.cpp:75` |
| Enable/disable feed | `RssWindow.qml:158-161`, dot `:130-133` | `setFeedEnabled` `qmlposterbridge.cpp:980` | `rssmanager.cpp:99` |
| Refresh all ("Atualizar") | `RssWindow.qml:90` | `checkAllFeeds()` `qmlposterbridge.cpp:998` | `rssmanager.cpp:121` |
| Items list (title, size, downloaded badge) | `RssWindow.qml:224`, delegate `:227-272` | `itemsForFeed(idx)` `qmlposterbridge.cpp:950` | `rssmanager.cpp:319` |
| Download single item (⬇ button) | `RssWindow.qml:265-267` | `downloadItem` `qmlposterbridge.cpp:1003` | `rssmanager.cpp:324` |
| Auto-download toggle (banner, on/off) | `RssWindow.qml:209-215` | `setAutoDownload` `qmlposterbridge.cpp:987` | sets `feed.autoDownload`, persists |
| Auto-download rule banner (read-only filterPattern) | `RssWindow.qml:199-207` | reads `feeds` map key `filterPattern` `qmlposterbridge.cpp:943` | `RssFeed::filterPattern` `rssmanager.h:20` |
| Feed count chip / empty state | `RssWindow.qml:88,168-178` | `feeds.length` | — |

**Verified map keys (no mismatch — QML reads these and the bridge provides them):**
- Feed map (`qmlposterbridge.cpp:937-945`): `index`, `name`, `url`, `enabled`, `autoDownload`, `filterPattern`, `count`. QML uses `name`,`enabled`,`count`,`filterPattern`,`autoDownload` (`RssWindow.qml:132,136,142,201,210`). ✅
- Item map (`qmlposterbridge.cpp:955-960`): `title`, `link`, `size` (already formatted via `formatSize`, `qmlposterbridge.cpp:958`), `downloaded`. QML uses `title`,`size`,`downloaded` (`:239,247,256`). ✅
- `size` is correctly a formatted string already — no bug there.

---

## 2. What is MISSING vs. the legacy `RssDialog`

### A. Live refresh / reactivity (HIGH — screen looks "dead" until reselect)
Constructor (`qmlposterbridge.cpp:923-929`) connects `RssManager::feedAdded` and `feedUpdated`, but **both only `emit feedsChanged()`** — there is NO signal that re-queries items for the already-selected feed. `win.itemList` (`RssWindow.qml:20-21`) re-evaluates only when `feedList` or `selectedFeed` changes. So:
- A `feedUpdated` fires `feedsChanged` → `feedList` getter re-runs → `itemList` *does* re-evaluate (because it depends on `feedList`). So items for the selected feed **do** refresh in practice. BUT this is fragile/indirect; the feed name only updates via the same path.
- `RssManager::feedError` (`rssmanager.h:67`) and `itemAutoDownloaded` (`:68`) are **not connected at all** — no error toast, no auto-download notification. This is a real gap.
- `checkFeed`'s `lastChecked` update (`rssmanager.cpp:165`) is not surfaced (no last-checked UI yet).

**Required:** connect `feedError` and `itemAutoDownloaded` in the constructor and re-emit new QML-visible signals. Optionally add a dedicated `itemsUpdated(int)` for robustness so the item list does not rely on the `feedList` chain.

### B. Feed settings card (edit panel) — entirely absent
Legacy settings card (`rssdialog.cpp:241-330`) edits per feed:
- `enabled` checkbox (`:257,433,454`) — QML has it only via context menu
- `autoDownload` checkbox (`:258,434,455`) — QML has toggle, but not in a settings panel
- **`filterPattern` regex field** (`:267,435,456`) — QML can only *display*, never *set*
- **`savePath` field + Browse** (`:283-299,436,457`) — absent in QML
- **`checkIntervalMin` spin (5–1440)** (`:308-313,437,458`) — absent in QML
- **Last-checked label** (`:324-330,439-443`) — absent in QML
- Save → `RssManager::updateFeed(index, feed)` (`:460`, `rssmanager.cpp:106`)

All routes through ONE backend call: `RssManager::updateFeed(int, const RssFeed&)` (`rssmanager.h:48`, `rssmanager.cpp:106`). The bridge exposes none of it (only `setAutoDownload` mutates one field via this path, `qmlposterbridge.cpp:987-996`). **Single biggest gap** — user cannot set an auto-download regex, save path, or poll interval from QML.

### C. Per-feed auto-download filter rule editing
Banner (`RssWindow.qml:199-216`) is read-only for the pattern, toggles only the bool. No path to **set** `filterPattern`. Critical because empty pattern + autoDownload = download **everything** (`autoDownloadMatching` `rssmanager.cpp:266-280`). Needs an edit affordance + bridge setter.

### D. Add-feed presets menu (nice-to-have)
Legacy add row has a presets dropdown (Nyaa / Sukebei / Linux Tracker) — `rssdialog.cpp:171-195`. QML overlay (`RssWindow.qml:28-61`) is URL-only. Low cost: hardcode list in QML, no bridge change.

### E. Item publish date + double-click-to-download
- Legacy items table has a **Date column** from `RssItem::pubDate` (`rssdialog.cpp:340-342,480-481`). QML delegate (`:237-256`) shows no date. Needs a `date` key on the item map (`qmlposterbridge.cpp:955-960`) — `it.pubDate` exists (`rssmanager.h:31`).
- Legacy supports double-click row to download (`rssdialog.cpp:352,494-500`). QML has only the ⬇ button — acceptable, consider adding row double-click for parity.

### F. Manual single-feed refresh
`RssManager::checkFeed(int)` (`rssmanager.h:53`, `rssmanager.cpp:138`) exists, not exposed. `checkAllFeeds` gates by interval (`rssmanager.cpp:127-135`) so a forced single refresh should call `checkFeed` directly. Add a per-feed "Refresh" context-menu item (optional).

### G. "Mark read" — not applicable
No read/unread concept in `RssManager`. `RssItem` has only `downloaded` (`rssmanager.h:33`) + `markDownloaded(feedUrl, link)` (`rssmanager.h:58`, `rssmanager.cpp:346`). The downloaded badge already covers the closest behavior. True "mark read" needs a NEW `RssItem` field + persistence — out of scope for a 1:1 port; skip unless the design kit shows it.

---

## 3. Bridge additions required (`QmlRssBridge`)

Header edits in `qmlposterbridge.h` after `:267`; impl in `qmlposterbridge.cpp` after `:1006`.
**Count: 3 new Q_INVOKABLE methods + 2 new signals + 2 constructor connections + 1 enriched map key (item `date`) + optional feed-map enrichment.**

### New Q_INVOKABLE methods
1. `Q_INVOKABLE void updateFeedSettings(int index, const QString &filterPattern, const QString &savePath, int checkInterval, bool enabled, bool autoDownload);`
   Mirror of `RssDialog::saveCurrentFeedSettings` (`rssdialog.cpp:446-462`). Impl pattern (copy from `setAutoDownload` `qmlposterbridge.cpp:987-996`):
   ```cpp
   auto feeds = RssManager::instance().feeds();
   if (index < 0 || index >= feeds.size()) return;
   RssFeed f = feeds[index];
   f.filterPattern = filterPattern;
   f.savePath = savePath.startsWith("file://") ? QUrl(savePath).toLocalFile() : savePath;
   f.checkIntervalMin = qBound(5, checkInterval, 1440);  // legacy range, rssdialog.cpp:309
   f.enabled = enabled;
   f.autoDownload = autoDownload;
   RssManager::instance().updateFeed(index, f);          // rebuilds timers, rssmanager.cpp:111
   RssManager::instance().saveFeeds();
   emit feedsChanged();
   ```
2. `Q_INVOKABLE void checkFeed(int index);` → `RssManager::instance().checkFeed(index);` (per-feed forced refresh, `rssmanager.cpp:138`).
3. `Q_INVOKABLE QString defaultSavePath() const;` → for the save-path placeholder. Reuse the same logic as `QmlSessionBridge::defaultSavePath` (`qmlposterbridge.cpp:646-652`) OR simply have QML read the already-registered `session.defaultSavePath()` (`qmlposterbridge.h:164`) if the session bridge is available to the RSS window's context. (If reusing `session`, this method is unnecessary — confirm the RSS window has `session` in scope; it currently only references `rss`.)

### New signals (header, after `:270`)
- `void errorOccurred(const QString &message);` ← from `RssManager::feedError`.
- `void autoDownloaded(const QString &feedName, const QString &itemTitle);` ← from `RssManager::itemAutoDownloaded`.
- (Optional) `void itemsUpdated(int feedIndex);` ← from `feedUpdated`, for robust item-list refresh.

### Constructor additions (`qmlposterbridge.cpp:925-928`, inside existing ctor)
```cpp
connect(&rss, &RssManager::feedError,          this, &QmlRssBridge::errorOccurred);
connect(&rss, &RssManager::itemAutoDownloaded, this, &QmlRssBridge::autoDownloaded);
// optional, alongside existing feedUpdated connect at line 927:
connect(&rss, &RssManager::feedUpdated, this, [this](int i, const QList<RssItem>&){ emit itemsUpdated(i); });
```
Note: existing ctor already calls `rss.loadFeeds()` (`:928`) — `RssManager` also loads in its own ctor (`rssmanager.cpp:27`), so this is a redundant reload; harmless, leave it.

### Enriched map keys
- Item map (`qmlposterbridge.cpp:955-960`): add
  ```cpp
  m["date"] = it.pubDate.isValid() ? it.pubDate.toString("yyyy-MM-dd hh:mm") : QString();
  ```
- Feed map (`qmlposterbridge.cpp:937-945`): add `savePath` (`f.savePath`), `checkInterval` (`f.checkIntervalMin`), `lastChecked` (formatted `f.lastChecked`) so the settings panel populates from the existing `feeds` property without a round-trip.

### main.cpp — already correct
`rss` context property IS registered: `src/main.cpp:185` (`setContextProperty("rss", rssBridge)`), bridge created at `:143`, session set at `:142`. No change needed for the main window. If the RSS window is opened as a separate top-level `Window` via a child `QQmlContext` (per the settings-window precedent in MEMORY), ensure `rss` (and `session` if reused for save path) is set on that context too.

---

## 4. Implementation checklist (ordered)

1. **Wire `feedError` + `itemAutoDownloaded`** in the ctor (`qmlposterbridge.cpp:925-928`) → new `errorOccurred` / `autoDownloaded` signals. Surface via the app toast mechanism used in `Main.qml`. Highest impact / lowest cost.
2. **Add the feed settings panel/dialog** (§2B) — a `BatDialog` (pattern: `MagnetDialog.qml`, `AddTorrentDialog.qml`, `RemoveDialog.qml`) or an inline right-column card. Fields: enabled `TToggle`, autoDownload `TToggle`, filter `TFld` (mono), savePath `PathFld` (`src/qml/widgets/PathFld.qml`), interval input, last-checked `Text`, Save `BtnFlat` → `rss.updateFeedSettings(...)`. Widgets available: `TFld`, `TToggle`, `TChk`, `PathFld`, `BtnFlat`, `Eyebrow` in `src/qml/widgets/`.
3. **Implement `updateFeedSettings`** (§3 method 1) in `.cpp` after `:1006`, routing through `RssManager::updateFeed`.
4. Add **`date`** to item map (`qmlposterbridge.cpp:955-960`) and render it in the item delegate (`RssWindow.qml` near `:256`).
5. Enrich feed map with `savePath`/`checkInterval`/`lastChecked` (`qmlposterbridge.cpp:937-945`) to feed the settings panel.
6. (Optional) `checkFeed(int)` bridge method + per-feed "Refresh"/"Edit settings" context-menu items (`RssWindow.qml:154-163`); presets dropdown in add overlay (§2D); double-click-to-download on item rows.
7. Delete stale `src/qml-new/RssWindow.qml`.
8. Skip "mark read" unless design kit requires it (§2G — needs new backend field).

---

## 5. Effort estimate

- Error/auto-dl signal wiring + toasts (§3 ctor + 2 signals + QML Connections): **~1 h**
- Feed settings dialog/panel QML + `updateFeedSettings` bridge method + feed-map enrichment: **~3 h**
- Date column + item-map `date` key: **~0.5 h**
- Optional polish (presets, per-feed refresh `checkFeed`, double-click): **~1 h**

**Total: ~4.5 h core, ~5.5 h with optional polish.** Backend (`RssManager`) needs **zero changes** — every gap maps to an existing API (`updateFeed`, `checkFeed`, `feedError`/`itemAutoDownloaded` signals). The screen's basic read/add/remove/download path is already live; the work is concentrated in the settings-editing UI + the regex/save-path/interval setters, which all collapse into the single `updateFeed` call.
