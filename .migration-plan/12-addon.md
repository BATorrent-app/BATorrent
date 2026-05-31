# Screen 12 — Addons (Addon / Extension Management)

Migration plan for the QWidget→QML port of the Addons screen.
Status of the QML port: **stub only** — `AddAddonDialog.qml` is a static design mockup with a hard-coded `Repeater` model and zero backend wiring. It also covers **only a fraction** of the original QWidget feature set. A real port needs a richer dialog plus a new C++ bridge.

---

## 1. What exists today

### 1.1 QML side (the stub)
`src/qml/AddAddonDialog.qml` (140 lines) — a `BatDialog` titled "Adicionar addon", `cardW: 560`, `cardH: 540`, `okText: "Fechar"`, `showCancel: false`.

Sections present (all static / non-functional):
- Eyebrow `ADDONS` + title + subtitle (`AddAddonDialog.qml:16-38`).
- "URL do manifesto" field + "Instalar" button — **button has no `onClicked`, field has no id** (`AddAddonDialog.qml:40-55`).
- "DA COMUNIDADE" card with a hard-coded inline array of 4 entries (Torrentio/Stremio Catalogs/The Pirate Bay/Nyaa.si) rendered by a `Repeater` (`AddAddonDialog.qml:57-139`). The `installed`/`Instalar` state is a literal boolean in the model; buttons do nothing.

There is **no installed-addons list, no remove, no enable/disable toggle, no update, no auto-trackers card, no torrent-search card, no error handling, and no live data**. It is purely cosmetic.

> Wiring status (CONFIRMED): the dialog is **already instantiated and triggered** in `Main.qml` — `AddAddonDialog { id: addAddonDlg }` (`Main.qml:1731`), opened from the native menu item `MenuItem { text: qsTr("Addons…"); onTriggered: addAddonDlg.open() }` (`Main.qml:180`). So no new trigger is needed; only the dialog body + bridge wiring. (Note: there is no toolbar/`TBtn` for Addons like RSS has at `Main.qml:375`; consider adding one for parity, optional.)

### 1.2 QWidget side (the source of truth)
`src/gui/addondialog.h` + `src/gui/addondialog.cpp` (407 lines). It is **one scrollable dialog** (not a multi-window manager) containing **six cards**:

| # | Card | Code | Backend calls |
|---|------|------|---------------|
| 1 | **Installed addons** list + Remove button | `addondialog.cpp:171-203`, `refreshList()` 394-407 | `AddonManager::addons()`, `removeAddon(row)` |
| 2 | **Install via URL** (field + Install) | `addondialog.cpp:205-228`, `installAddon()` 378-384 | `addAddon(url)` |
| 3 | **Suggested addons** (Cinemeta, Torrentio) chip buttons | `addondialog.cpp:230-289` | `addAddon(url)`, dedupe vs `addons()` |
| 4 | **Auto trackers** (checkbox + count label) | `addondialog.cpp:291-312` | `autoTrackersEnabled()`, `setAutoTrackersEnabled()`, `trackerList().size()` |
| 5 | **Torrent search** (enable checkbox + URL field) | `addondialog.cpp:314-349` | `torrentSearchEnabled()`, `setTorrentSearchEnabled()`, `torrentSearchUrl()`, `setTorrentSearchUrl()` |
| 6 | Footer close button | `addondialog.cpp:357-366` | `accept()` |

Live behaviour (must be replicated):
- On `AddonManager::addonAdded` → `refreshList()` (`addondialog.cpp:368-370`).
- On `AddonManager::addonError` → `QMessageBox::warning` (`addondialog.cpp:371-373`). **In QML this becomes a toast or inline error string** — there is no QMessageBox.
- Remove button disabled until a row is selected (`addondialog.cpp:184, 192-194`).
- Empty-state hint label shown only when `addons().isEmpty()` (`addondialog.cpp:197-201, 405-406`) → use the existing `EmptyState.qml` widget.
- Installed-list row text is multi-line: `name\ndescription\nTypes · movie, series` (`addondialog.cpp:398-403`).

### 1.3 Backend API — `AddonManager` (singleton)
`src/app/addonmanager.h` / `.cpp`. **This class is complete and needs zero changes.** Relevant surface:

Data structs (`addonmanager.h:15-68`):
- `AddonManifest { id, name, description, url, QStringList types, QStringList resources, bool enabled }` (`:15-23`).
- `SearchProvider { id, name, urlTemplate, arrayPath, namePath, hashPath, sizePath, seedersPath, leechersPath, bool enabled, bool builtIn }` (`:55-68`).

Addon management methods:
- `QList<AddonManifest> addons() const` — `addonmanager.h:80` / impl `addonmanager.cpp:212-215`.
- `void addAddon(const QString &url)` — `addonmanager.h:77` / impl `addonmanager.cpp:133-148` (dedupes, then async `fetchManifest`). **Async**: success → `addonAdded`, failure → `addonError`.
- `void removeAddon(int index)` — `addonmanager.h:78` / impl `addonmanager.cpp:198-203` (bounds-checked, saves).
- `void setAddonEnabled(int index, bool enabled)` — `addonmanager.h:79` / impl `addonmanager.cpp:205-210` (bounds-checked, saves). **This is the enable/disable backend — the QWidget UI never exposed it; the QML port should.**
- Capability checks `hasCatalogAddon()` / `hasStreamAddon()` — `addonmanager.h:85-86`.

Auto-trackers:
- `bool autoTrackersEnabled()` / `void setAutoTrackersEnabled(bool)` — `addonmanager.h:97-98` / impl `addonmanager.cpp:393-403`.
- `QStringList trackerList()` (for the count) — `addonmanager.h:96` / impl `addonmanager.cpp:388-391`.
- `void fetchTrackerList()` — `addonmanager.h:95` / impl `addonmanager.cpp:360-386` (async, emits `trackerListUpdated`). **The QWidget dialog never offered a "refresh trackers" button; the count just reflects cache. Keep parity: show count, optionally add a refresh button bound to `fetchTrackerList()`.**

Legacy single-URL torrent search (used by card #5):
- `bool torrentSearchEnabled()` / `void setTorrentSearchEnabled(bool)` — `addonmanager.h:108-109` / impl `addonmanager.cpp:407-417`.
- `QString torrentSearchUrl()` / `void setTorrentSearchUrl(const QString&)` — `addonmanager.h:110-111` / impl `addonmanager.cpp:419-429`.

Multi-engine search providers (NOT in the QWidget addon dialog — surfaced in the Search screen, see §6):
- `searchProviders()`, `addSearchProvider()`, `removeSearchProvider()`, `setSearchProviderEnabled()`, `searchWithProvider()` — `addonmanager.h:101-105`.

Signals (`addonmanager.h:114-124`): `addonAdded(AddonManifest)`, `addonError(QString)`, `trackerListUpdated()`, plus catalog/stream/search-result signals not relevant to this screen.

There is **no `addonUpdate`/version mechanism in the backend** — addons have no version field and no "update" call. See §4.

---

## 2. Single dialog vs. full manager window — decision

**A single dialog is enough — but it must be the *full* QWidget dialog, not the current stub.**

The QWidget `AddonDialog` is itself one scrollable `QDialog` containing all six cards. The current QML stub only ported ~2 of those cards (URL install + a fake community list) and dropped the entire installed-list/remove/auto-trackers/torrent-search functionality. So this is **not** a "single dialog vs full window" question — it is "finish porting the dialog that already exists."

Recommendation: **port `addondialog.cpp` 1:1 into `AddAddonDialog.qml`** as a `BatDialog` with a scrollable body (BatDialog's body is already a `Flickable`, so long content scrolls — see `BatDialog.qml:111-127`). Per the project memory note `feedback_port_not_freestyle.md`, do the faithful 1:1 port first; do not redesign.

The card #5 (legacy torrent-search) and the `SearchProvider` management belong logically to the **Search** screen / Settings, not strictly to "Addons" — but the QWidget put torrent-search inside the Addon dialog, so to preserve parity keep it here unless a separate screen plan moves it. Flag overlap with the Search screen plan (§6).

---

## 3. Required bridge: `QmlAddonBridge`

There is no addon bridge today. Add one, modelled **exactly** on `QmlRssBridge` (`qmlposterbridge.h:252-271`) which wraps the `RssManager` singleton with a `QVariantList` property + `Q_INVOKABLE` mutators + a `…Changed` notify signal. The implementer should register it as a context property named **`addons`** in the same place `rss`/`session`/`themeBridge` are registered in `main.cpp` (mirror the existing `rss` registration line — the QmlRssBridge registration is the template).

### 3.1 Header to add — `src/gui/qmlposterbridge.h`

```cpp
// Bridge for community addons (wraps AddonManager singleton).
class QmlAddonBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList installed READ installed NOTIFY changed)
    Q_PROPERTY(QVariantList suggested READ suggested CONSTANT)
    Q_PROPERTY(bool autoTrackers READ autoTrackers WRITE setAutoTrackers NOTIFY changed)
    Q_PROPERTY(int trackerCount READ trackerCount NOTIFY changed)
    Q_PROPERTY(bool torrentSearchEnabled READ torrentSearchEnabled
               WRITE setTorrentSearchEnabled NOTIFY changed)
    Q_PROPERTY(QString torrentSearchUrl READ torrentSearchUrl
               WRITE setTorrentSearchUrl NOTIFY changed)
public:
    explicit QmlAddonBridge(QObject *parent = nullptr);

    QVariantList installed() const;          // [{ name, description, url, types(joined), enabled }]
    QVariantList suggested() const;          // hard-coded Cinemeta/Torrentio + installed-flag
    bool autoTrackers() const;
    void setAutoTrackers(bool on);
    int trackerCount() const;
    bool torrentSearchEnabled() const;
    void setTorrentSearchEnabled(bool on);
    QString torrentSearchUrl() const;
    void setTorrentSearchUrl(const QString &url);

    Q_INVOKABLE void addAddon(const QString &url);   // async; errors arrive via error()
    Q_INVOKABLE void removeAddon(int index);
    Q_INVOKABLE void setEnabled(int index, bool on);
    Q_INVOKABLE void refreshTrackers();              // -> AddonManager::fetchTrackerList()
    Q_INVOKABLE bool isInstalled(const QString &url) const; // suggested-list dedupe helper

signals:
    void changed();              // emitted on addonAdded / removed / enabled / trackers updated
    void error(const QString &message);  // -> QML shows a toast (replaces QMessageBox)
};
```

### 3.2 Implementation notes — `src/gui/qmlposterbridge.cpp`
Copy the `QmlRssBridge` ctor/impl pattern. In the ctor connect to the singleton:

```cpp
QmlAddonBridge::QmlAddonBridge(QObject *parent) : QObject(parent) {
    auto &mgr = AddonManager::instance();
    connect(&mgr, &AddonManager::addonAdded, this, [this](const AddonManifest&){ emit changed(); });
    connect(&mgr, &AddonManager::addonError, this, [this](const QString &e){ emit error(e); });
    connect(&mgr, &AddonManager::trackerListUpdated, this, &QmlAddonBridge::changed);
}
```

- `installed()` maps `AddonManager::instance().addons()` to a `QVariantList` of `QVariantMap` with keys `name`, `description`, `url`, `types` (use `a.types.join(", ")`), `enabled`. (Mirror the multi-line label logic from `addondialog.cpp:398-403`, but expose fields separately so QML formats them.)
- `removeAddon` / `setEnabled` call through then `emit changed()` (the manager's remove/setEnabled are synchronous and do **not** emit, unlike `addAddon`). This matches the QWidget which manually called `refreshList()` after remove (`addondialog.cpp:391`).
- `addAddon` is async — do **not** `emit changed()` synchronously; the `addonAdded` connection handles refresh; `addonError` → `error()`.
- `suggested()` returns the same two hard-coded entries as `addondialog.cpp:244-247` (Cinemeta `https://v3-cinemeta.strem.io`, Torrentio `https://torrentio.strem.fun`), each with an `installed` bool computed via `isInstalled(url)`.
- Add `#include "../app/addonmanager.h"` and `#include <QVariantMap>` to the cpp.

### 3.3 Registration (CONFIRMED site)
In `src/main.cpp`, inside the `--qml` branch:
- Instantiate next to the other bridges (after `main.cpp:143` `auto *rssBridge = new QmlRssBridge(&app);`):
  `auto *addonBridge = new QmlAddonBridge(&app);`
- Register next to `main.cpp:185` `engine.rootContext()->setContextProperty("rss", rssBridge);`:
  `engine.rootContext()->setContextProperty("addons", addonBridge);`

Note: `QmlRssBridge` ctor is `(QObject *parent = nullptr)` and takes no manager arg (it uses `AddonManager::instance()` / `RssManager::instance()` directly — see `qmlposterbridge.cpp:923`). `QmlAddonBridge` follows the same singleton pattern, so its ctor also needs no SessionManager. The `RssManager::instance().setSession(...)` line (`main.cpp:142`) is RSS-specific and has **no addon equivalent** — `AddonManager` needs no session wiring.

**Bridge additions count: 1 new class (`QmlAddonBridge`)** exposing **6 properties + 5 invokables + 2 signals = 13 members**, plus **1 context-property registration** in `main.cpp`.

---

## 4. Gap: there is no "update" capability

The task asks to document "update". **The backend has no addon-version / update mechanism**: `AddonManifest` has no `version` field, there is no `updateAddon()` call, and `fetchManifest` always *appends* (it dedupes on URL and refuses re-add — `addonmanager.cpp:140-145`). So "update" today can only mean **remove + re-add the same URL** (which re-fetches the manifest).

Options for the implementer:
- **Parity (recommended):** do NOT add an Update button — the QWidget never had one. Out of scope for a faithful port.
- **If update is desired:** add `void reinstallAddon(int index)` to `AddonManager` (capture URL, `removeAddon`, `addAddon`) and surface it via the bridge. This is a backend change beyond the port; call it out as a follow-up, not part of this screen.

Document this clearly so "missing update" is understood as a backend gap, not a port oversight.

---

## 5. Design-system mapping (widgets to use)

The stub already imports `"theme"` + `"widgets"`. Build on `BatDialog.qml` (body is a scrollable `Flickable`, footer has OK/Cancel). Widgets to reuse:
- `Eyebrow { text; red }` — section eyebrows (already used in stub).
- `TFld { placeholder; mono }` — URL fields (manifest URL, torrent-search URL). Give them `id`s. Use `mono: true`.
- `BtnFlat { primary; sm; text; onClicked }` — Install / Remove buttons.
- `TChk { on; onToggled: function(v){…} }` — enable/disable per-addon toggle, auto-trackers checkbox, torrent-search enable. (See `RemoveDialog.qml:77-82` for the `onToggled` callback signature.)
- `TToggle { on }` — alternative for the auto-trackers / torrent-search switches (matches `MagnetDialog.qml:126`).
- `IconImg { src; tint; s }` — row icons (stub uses `qrc:/icons/rss.svg`; consider a dedicated addon/extension icon).
- `EmptyState.qml` — shown when `addons.installed.length === 0` (replaces `addon_empty_hint`).
- Card chrome: `Rectangle { radius:11; color: Theme.panel; border.color: Theme.hair }` (as in stub `:71-77` and `RemoveDialog.qml:63-69`).
- Error reporting: replace `QMessageBox::warning` with the app's toast (`src/gui/toast.*`) or an inline `Text` bound to a `dlg.errorText` property set from the bridge `onError` handler. Use a `Connections { target: addons; function onError(msg){…} }` block (pattern: `AddTorrentDialog.qml:47-52`).

Strings: the QWidget used translator keys (`tr_("addon_title")`, `addon_installed`, `addon_install_btn`, `addon_empty_hint`, `addon_auto_trackers`, `addon_tracker_count`, `addon_torrent_search_*`, etc., scattered through `addondialog.cpp`). The QML stub currently hard-codes Portuguese. Match the existing QML convention (other ported dialogs hard-code pt-BR strings, e.g. `MagnetDialog.qml`), so hard-coded pt-BR is acceptable for parity, but reuse the exact label wording from the QWidget where possible.

---

## 6. Cross-screen overlap to flag
- **Card #5 (legacy torrent search) and `SearchProvider` management** logically belong to the **Search** screen. The QWidget bundled torrent-search-enable + URL into the Addon dialog. Coordinate with the Search-screen plan so this isn't double-implemented. If Search owns providers, the Addon dialog can drop card #5; otherwise keep it for parity.
- `setAddonEnabled()` exists in the backend but was never exposed in any QWidget UI — the QML port is an opportunity to add per-addon enable/disable toggles (low cost, the bridge already needs `setEnabled`).

---

## 7. Implementation checklist

**Backend / bridge (C++):**
1. [ ] Add `class QmlAddonBridge` to `qmlposterbridge.h` (§3.1).
2. [ ] Implement it in `qmlposterbridge.cpp` mirroring `QmlRssBridge`; connect `addonAdded`/`addonError`/`trackerListUpdated` (§3.2). Add `#include "../app/addonmanager.h"`.
3. [ ] Map `installed()` from `AddonManager::addons()` to `QVariantList` (fields: name, description, url, types-joined, enabled).
4. [ ] Implement `suggested()` + `isInstalled()` (Cinemeta + Torrentio, dedupe vs installed).
5. [ ] Instantiate + register as context property `addons` in `main.cpp` next to the existing `rss` registration.
6. [ ] (Optional, only if update wanted) add `reinstallAddon(int)` to `AddonManager` — otherwise document update as N/A (§4).

Confirmed `QmlRssBridge` impl pattern to copy (`qmlposterbridge.cpp:923-1005`): ctor connects manager signals to a `…Changed` emit; getters map `manager.list()` to `QVariantList` of `QVariantMap`; mutators call through then (for RSS) `saveFeeds()`. For addons, `AddonManager` already auto-saves inside `removeAddon`/`setAddonEnabled`/`addAddon` (`addonmanager.cpp:202,209,193`), so the bridge must NOT call any save — just call through and `emit changed()` for the synchronous ops.

**QML (`AddAddonDialog.qml`) — replace the stub body:**
7. [ ] Card 1 — **Installed addons**: `ListView`/`Repeater` over `addons.installed`; each row shows name/description/types, a `TChk`/`TToggle` bound to `enabled` calling `addons.setEnabled(index, v)`, and a Remove `BtnFlat` calling `addons.removeAddon(index)`. Show `EmptyState` when empty.
8. [ ] Card 2 — **Install via URL**: give the `TFld` an id; wire "Instalar" `onClicked` and field accepted/return to `addons.addAddon(fld.text.trim())`, then clear the field (mirror `installAddon()` `addondialog.cpp:378-384`).
9. [ ] Card 3 — **Suggested**: replace the hard-coded inline array with `addons.suggested`; install button calls `addons.addAddon(url)`; show "Instalado" badge when `modelData.installed`.
10. [ ] Card 4 — **Auto trackers**: `TChk`/`TToggle` bound to `addons.autoTrackers`; count label bound to `addons.trackerCount`; (optional) refresh button → `addons.refreshTrackers()`.
11. [ ] Card 5 — **Torrent search** (if kept, see §6): enable toggle bound to `addons.torrentSearchEnabled`; URL `TFld` bound to `addons.torrentSearchUrl` (disabled when not enabled); commit on editing-finished.
12. [ ] Error handling: `Connections { target: addons; function onError(msg) {/* toast */} }` (replaces QMessageBox).
13. [ ] Refresh: `installed` rebinds automatically via the `changed` NOTIFY signal — no manual `refreshList()` needed.
14. [ ] Footer: keep `okText: "Fechar"`, `showCancel: false`; `onAccepted` just closes.

**Wiring / verification:**
15. [ ] `Main.qml` trigger already exists (`Main.qml:180,1731`) — no change needed. (Optional: add an Addons `TBtn` to the toolbar group near `Main.qml:375` for parity with RSS.)
16. [ ] Resize `cardH` if the six cards overflow (BatDialog body scrolls, so prefer scrolling over an oversized card).
17. [ ] Manual test: install a real manifest URL, see it appear; toggle enable; remove; trigger an error (bad URL) → toast; toggle auto-trackers + see count.

---

## 8. Effort estimate
**Medium — ~5-7 hours.** Backend is fully reusable (no `AddonManager` changes unless update is wanted). Work is: ~1 bridge class (~120 lines C++, straight copy of the RSS bridge pattern) + 1 context registration, and a full rewrite of `AddAddonDialog.qml` from a 140-line static stub to a ~250-line data-driven dialog with 5-6 functional cards. Both the `Main.qml` open-trigger (`Main.qml:180,1731`) and the `main.cpp` registration site (`main.cpp:143,185`) are now confirmed, so there are no unknowns left — it is mechanical work.
