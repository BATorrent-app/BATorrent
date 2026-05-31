# Screen 13 — Create Torrent

Single source of truth for finishing the Create Torrent screen. All paths absolute. Evidence cited as `path:line`.

---

## 0. TL;DR for the implementer

This is **not a port** — it is a **greenfield feature**. There is **no QWidget original** (`createtorrentdialog.cpp/.h` do not exist anywhere in the repo, confirmed below), and there is **no backend create-torrent API** in `SessionManager`. You are building three layers from scratch:

1. **Backend**: a create-torrent routine using libtorrent 2.0.11's `create_torrent` API, run on a **background thread** (hashing blocks for seconds-to-minutes on large inputs).
2. **Bridge**: new `Q_INVOKABLE` methods + signals exposed to QML. The codebase currently has **zero `Q_INVOKABLE` methods** (grep confirmed) — so you also establish that convention here.
3. **QML**: replace the 45-line stub `CreateTorrentDialog.qml` with the full form.

The menu entry and CMake registration **already exist** — see §5.

---

## 1. Current state — evidence

### 1.1 The QML is a stub
`/Users/mateuscruz/Projects/BAT-Torrent/src/qml/CreateTorrentDialog.qml` (45 lines total):
- Declares `BatDialog`, `title: "Create Torrent"`, `standardButtons: 0`, `width: 560`.
- Has properties `busy`, `sourcePath`, `sourceIsFolder` (CreateTorrentDialog.qml:13-15).
- Has a `reset()` function that already references **non-existent** ids: `commentField`, `trackersField`, `webSeedsField`, `privateCheck`, `pieceCombo`, `progressBar` (CreateTorrentDialog.qml:19-28). These ids must be created by the implementer; right now `reset()` would throw ReferenceError if called.
- Body is a placeholder: a "Source" label and `Text { text: "TODO: wire to backend" }` (CreateTorrentDialog.qml:40-43).

### 1.2 There is no QWidget original to port
- `find` over `/src` for `*.cpp/*.h/*.qml` (full list in this analysis) shows **no** `createtorrentdialog.*`. The `gui/` dir contains only `qmlposterbridge.{h,cpp}` and `themebridge.{h,cpp}` (CMakeLists.txt:15-16).
- The task brief's reference to `src/gui/createtorrentdialog.cpp/.h` is incorrect — those files do not exist. Do not look for them.

### 1.3 There is no backend create-torrent API
`/Users/mateuscruz/Projects/BAT-Torrent/src/torrent/sessionmanager.h` exposes only:
- `addTorrentFile(path, savePath)`, `addMagnet(uri, savePath)`, `removeTorrent`, `pauseTorrent`, `resumeTorrent`, `raw()` (sessionmanager.h:35-41).
- Signals: `torrentAdded`, `torrentRemoved`, `statsUpdated` (sessionmanager.h:44-46).
- **No** create/make/generate method. Grep for `create_torrent|add_files|set_piece_hashes|set_priv|add_tracker|add_url_seed|set_comment|piece_size` in `sessionmanager.cpp` returned **zero hits**.

### 1.4 The existing wired dialogs reference invokables that DON'T exist yet
This is a landmine. `AddTorrentDialog.qml:22` calls `bridge.session.inspectTorrentFile(path)`, `AddTorrentDialog.qml:23` reads `fileInfo.savePathSuggestion`; `MagnetDialog.qml:17` calls `bridge.session.defaultSavePath()`; `MagnetDialog.qml:56` and `:56` call `bridge.session.urlToLocalPath(...)`. **None of these methods exist** in `sessionmanager.h` and there are **no `Q_INVOKABLE`** declarations anywhere (grep over `/src` for `Q_INVOKABLE` = 0 hits; grep for `inspectTorrentFile|defaultSavePath|urlToLocalPath|savePathSuggestion` = 0 hits).
Implication: the "already-wired examples" are themselves only QML-side wired; their backend is also unimplemented. Treat them as **UI patterns to copy**, not as proof the bridge layer works. Our Create Torrent feature must add its own `Q_INVOKABLE` methods and they must be `Q_INVOKABLE` (the helpers above will presumably be added by sibling tasks; we cannot depend on them, so we provide our own `defaultSavePath`-equivalent default).

### 1.5 No threading exists yet
Grep for `QThread|QtConcurrent|std::thread|QRunnable|QFutureWatcher` over `/src` = **0 hits**. The whole codebase is synchronous (SessionManager polls libtorrent alerts on a 1s `QTimer`, sessionmanager.cpp:31-33). Create-torrent hashing **must not** run on the GUI thread; you are introducing the first background-work pattern. Use `QtConcurrent::run` + `QFutureWatcher`, or a dedicated `QThread` worker. `QtConcurrent` requires linking `Qt6::Concurrent` (see §5).

---

## 2. Fields the screen must expose

Mapped from the libtorrent `create_torrent` API (header at `/opt/homebrew/include/libtorrent/create_torrent.hpp`, version 2.0.11 per `/opt/homebrew/include/libtorrent/version.hpp:21`) and standard create-torrent UX. The stub's `reset()` already names most of these ids — honor those names.

| Field | QML control | id (match stub) | libtorrent mapping |
|---|---|---|---|
| Source file **or** folder | read-only `BatTextField` + two `BatButton`s ("File…", "Folder…") | `sourcePath` prop + display field | `lt::add_files(fs, path, predicate, flags)` (create_torrent.hpp:229-231) |
| Trackers (one per line, blank line = tier break) | `BatTextArea` | `trackersField` | `ct.add_tracker(url, tier)` (create_torrent.hpp:119) |
| Web seeds (one URL per line) | `BatTextArea` | `webSeedsField` | `ct.add_url_seed(url)` (create_torrent.hpp:116) |
| Piece size | `BatComboBox` | `pieceCombo` | `piece_size` arg of `create_torrent(fs, piece_size, flags)` ctor (create_torrent.hpp:66) |
| Private flag | `BatCheckBox` | `privateCheck` | `ct.set_priv(true)` (create_torrent.hpp:120) |
| Comment | `BatTextField` or `BatTextArea` | `commentField` | `ct.set_comment(str)` (create_torrent.hpp:101) |
| Output `.torrent` path | read-only `BatTextField` + "Save As…" `BatButton` | `outputPath` (add this; not in stub) | destination for `lt::bencode(out, ct.generate())` |
| Start seeding after create | `BatCheckBox` (optional, recommended) | `startSeedCheck` | after save, call `addTorrentFile(outputPath, sourceParentDir)` |
| Progress | `BatProgressBar` | `progressBar` | driven by `set_piece_hashes` progress callback (create_torrent.hpp:233-234) |
| Status text | `Label` | `statusLabel` (add) | "Hashing… N/M pieces", "Done", error |

### 2.1 Piece-size combo values
Provide `["Auto", "16 KiB", "32 KiB", ... "16 MiB"]`. "Auto" → pass `piece_size = 0` to the ctor (libtorrent auto-selects; ctor default is `0`, create_torrent.hpp:66). Otherwise pass bytes (e.g. 256 KiB = `262144`). Piece size **must be a power of two** ≥ 16 KiB — only offer power-of-two values.

### 2.2 Creator string
Set `ct.set_creator("BATorrent 1.0.0")` (create_torrent.hpp:102) — not user-facing, hardcode from `QCoreApplication::applicationName/Version`.

### 2.3 Create flags (optional, advanced — can be deferred)
`create_flags_t` constants are members of `lt::create_torrent`: `v1_only = 4_bit`, `v2_only = 3_bit`, `optimize_alignment = 0_bit`, `canonical_files = 5_bit`, `modification_time = 6_bit`, `symlinks = 7_bit` (create_torrent.hpp:133-139). For v1 first pass, you may default flags to `{}` (creates hybrid v1+v2). If you want to match classic clients, expose a "Compatibility" combo: Hybrid (default) / v1 only (`lt::create_torrent::v1_only`) / v2 only (`lt::create_torrent::v2_only`). **Recommended to defer** — ship hybrid default.

---

## 3. Creation flow (backend)

libtorrent 2.0.11, header `/opt/homebrew/include/libtorrent/create_torrent.hpp`. Reference sequence (runs on the **worker thread**):

```cpp
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/torrent_info.hpp>   // already included via sessionmanager
#include <fstream>

lt::file_storage fs;
lt::add_files(fs, sourcePath.toStdString());           // create_torrent.hpp:231 (no-predicate overload)
lt::create_torrent ct(fs, pieceSize /* 0 = auto */);   // create_torrent.hpp:66

for (tracker line) ct.add_tracker(url, tier);           // :119
for (web seed)     ct.add_url_seed(url);                // :116
if (priv)          ct.set_priv(true);                   // :120
if (!comment.isEmpty()) ct.set_comment(comment.c_str());// :101
ct.set_creator("BATorrent 1.0.0");                      // :102

lt::error_code ec;
// parent dir of source = root for hashing
std::string parent = QFileInfo(sourcePath).absolutePath().toStdString();
lt::set_piece_hashes(ct, parent,
    [&](lt::piece_index_t p){ /* emit progress: int(p)/ct.num_pieces() */ },
    ec);                                                 // create_torrent.hpp:233-234
if (ec) { /* emit error */ return; }

std::vector<char> buf;
lt::bencode(std::back_inserter(buf), ct.generate());     // generate() create_torrent.hpp:95
std::ofstream out(outputPath.toStdString(), std::ios::binary);
out.write(buf.data(), buf.size());
// emit finished(outputPath)
```

Key API facts:
- `add_files(file_storage&, std::string const& path, create_flags_t = {})` — no-predicate overload at create_torrent.hpp:231; predicate overload (to skip files) at create_torrent.hpp:229-230.
- `create_torrent(file_storage& fs, int piece_size = 0, create_flags_t = {})` — create_torrent.hpp:66. `piece_size == 0` ⇒ auto.
- `set_piece_hashes(create_torrent&, std::string const& root_path, std::function<void(piece_index_t)> const&, error_code&)` — create_torrent.hpp:233-234. **This is the long-running call.** The callback fires once per piece; use it to compute `0..1` progress against `ct.num_pieces()` (create_torrent.hpp:126).
- `generate()` returns an `lt::entry` (create_torrent.hpp:95); bencode it to bytes.

### 3.1 Tracker tier parsing
Trackers textarea: split by newline. Convention to support tiers: increment `tier` on a blank line, otherwise all tier 0. Simplest acceptable v1: every tracker tier 0.

### 3.2 Source root subtlety
`set_piece_hashes` takes the directory that **contains** the files added by `add_files`. When `add_files(fs, "/a/b/movie.mkv")`, libtorrent stores the path relative to `/a/b`, so pass `/a/b` (the parent) as the hash root. Use `QFileInfo(sourcePath).absolutePath()`. For a folder source `/a/b/myfolder`, `add_files` records `myfolder/...` and the root is still `/a/b`. Verify against libtorrent docs if files come up empty.

---

## 4. Bridge additions

The cleanest design: put the create-torrent logic in **`SessionManager`** (it already owns the lt::session and is exposed as `bridge.session`, main.cpp:30 / qmlposterbridge.h:17,26). The existing QML calls `bridge.session.addTorrentFile(...)` (Main.qml:57), so `bridge.session.createTorrent(...)` is consistent. Add the threading inside SessionManager.

### 4.1 New methods on `SessionManager` (`sessionmanager.h`)
```cpp
public:
    // Kicks off background hashing; results come back via signals below.
    Q_INVOKABLE void createTorrent(const QVariantMap &opts);
    Q_INVOKABLE QString defaultSavePath() const;        // = defaultDownloadDir(); reuse the file-local helper
    Q_INVOKABLE QString suggestOutputPath(const QString &sourcePath) const; // <source>.torrent next to source

signals:
    void createTorrentProgress(double fraction, int pieceDone, int pieceTotal);
    void createTorrentFinished(const QString &outputPath);
    void createTorrentFailed(const QString &message);
```
`opts` (QVariantMap) keys passed from QML: `source` (string), `output` (string), `pieceSize` (int bytes, 0=auto), `priv` (bool), `comment` (string), `trackers` (string, raw textarea), `webSeeds` (string, raw textarea), `startSeeding` (bool).

Note: **add `Q_INVOKABLE`** — this is the first use in the codebase. The class is already a `QObject` with `Q_OBJECT` (sessionmanager.h:26-27) so no extra setup beyond AUTOMOC (already on, CMakeLists.txt:6).

### 4.2 Threading inside `createTorrent` (sessionmanager.cpp)
- Use `QtConcurrent::run([=]{ ... })` or a `QThread` worker. The lambda runs the §3 sequence.
- The `set_piece_hashes` progress callback runs on the worker thread → **must not** touch GUI. Marshal back with `QMetaObject::invokeMethod(this, [=]{ emit createTorrentProgress(...); }, Qt::QueuedConnection)` (or emit a signal connected `Qt::QueuedConnection`). Since these are `SessionManager` signals and the object lives on the GUI thread, a direct `emit` from the worker is unsafe — queue it.
- On completion / `ec` error, queue `createTorrentFinished` / `createTorrentFailed`.
- If `startSeeding`, after the file is written call `addTorrentFile(output, QFileInfo(source).absolutePath())` — but do that **back on the GUI thread** (queued), because `async_add_torrent` should be issued from where the session is driven. addTorrentFile is at sessionmanager.cpp:41-47.

### 4.3 `QmlPosterBridge`
No change strictly required if methods live on `SessionManager` (already exposed). `qmlposterbridge.h:17` exposes `session` as a CONSTANT property; QML reaches it as `bridge.session.createTorrent(...)`. **Zero new properties needed** on the bridge.
(Alternative if you prefer the bridge to own this: add `Q_INVOKABLE void createTorrent(QVariantMap)` + the 3 signals to `QmlPosterBridge` and forward — more boilerplate, no benefit. Recommend keeping it on SessionManager.)

### 4.4 Count of bridge additions
- **3 `Q_INVOKABLE` methods**: `createTorrent`, `defaultSavePath`, `suggestOutputPath`.
- **3 signals**: `createTorrentProgress`, `createTorrentFinished`, `createTorrentFailed`.
- **0** new `QmlPosterBridge` properties.
Total = **6 bridge-surface additions** on `SessionManager`.

---

## 5. Build / registration

- `CreateTorrentDialog.qml` is **already registered** in the QML module (CMakeLists.txt:39). No CMake change for the QML file.
- All widgets you'll use are registered: `BatComboBox` (CMakeLists.txt:45), `BatCheckBox` (:46), `BatProgressBar` (:48), `BatTextArea` (:49), `BatSpinBox` (:50), `BatButton` (:43), `BatTextField` (:44), `BatPanel` (:47), `BatDialog` (:42), `Theme` (:53).
- **CMake changes needed**: if you use `QtConcurrent`, add `Concurrent` to `find_package(Qt6 ... COMPONENTS ...)` (CMakeLists.txt:8 currently `Quick Quick Controls2 Widgets Network` — note `Quick` is duplicated; leave as-is or fix) and link `Qt6::Concurrent` in a `target_link_libraries`. **Caution**: CMakeLists.txt has **no visible `target_link_libraries`** in the portion read (file ends at line 55 / the read shows the qml module block). Verify the link section exists below; if libtorrent + Qt libs are linked elsewhere, add `Qt6::Concurrent` there. Confirm how `LibtorrentRasterbar` is linked (find_package at :9) — the create_torrent symbols come from the same lib, so no new lib, just the headers in §3.
- No new `.cpp/.h` files are required if logic goes into existing `sessionmanager.cpp` (already in the sources list, CMakeLists.txt:17). If you make a separate `TorrentCreator` worker class, add its `.cpp` to `qt_add_executable` (CMakeLists.txt:13-29).

---

## 6. QML implementation — pattern to follow

Copy structure from `MagnetDialog.qml` / `AddTorrentDialog.qml` / `RemoveDialog.qml`:
- Root is `BatDialog` with `standardButtons: 0`; build your own Cancel / Create buttons in a trailing `RowLayout` with `Item { Layout.fillWidth: true }` spacer (AddTorrentDialog.qml:83-100, MagnetDialog.qml:59-75).
- File/folder pickers: use `QtQuick.Dialogs` `FileDialog` (for source file + output .torrent) and `FolderDialog` (for source folder), mirroring MagnetDialog.qml:54-57. Convert URL→path the same way the others do (`bridge.session.urlToLocalPath(...)` — but that invokable does not yet exist, §1.4; either rely on a sibling task adding it, or do `decodeURIComponent(url.toString().replace(/^file:\/\//, ""))` in QML, or add a `Q_INVOKABLE QString urlToLocalPath(QUrl)` yourself). **Recommend adding `urlToLocalPath` + `defaultSavePath` to SessionManager as part of this task** since AddTorrent/Magnet need them too and they are trivial — but scope-wise they belong to those screens; coordinate. At minimum your `suggestOutputPath` covers the output default.
- Inputs: `BatTextField` (source display, comment, output), `BatTextArea` (trackers, web seeds), `BatComboBox` (piece size), `BatCheckBox` (private, start seeding), `BatProgressBar` (progress).
- Buttons: `BatButton { primary: true; ... }` for the Create action (AddTorrentDialog.qml:91-99).
- Theme tokens only: `Theme.spacingM/S/L`, `Theme.fontM/L`, `Theme.textPrimary/textSecondary`, `Theme.accent`, `Theme.surfaceAlt` (Theme.qml:5-27).
- `BatDialog` is `modal`, centered, surface bg, header from `title` (BatDialog.qml:6-26). Keep `width: 560`; consider `600`.

### 6.1 Wiring inside the dialog (signals from backend)
The dialog is created lazily in Main.qml:32-35 and only `reset()`/`open()` are called — **the Create button's onClicked and the backend signal connections must live inside `CreateTorrentDialog.qml`** (unlike MagnetDialog which surfaces an `accepted` signal handled in Main.qml:51-53). Two valid approaches:
1. **Self-contained (recommended)**: inside the dialog, on Create click call `bridge.session.createTorrent({...})`, and add `Connections { target: bridge.session; function onCreateTorrentProgress(f,d,t){...}; function onCreateTorrentFinished(p){...}; function onCreateTorrentFailed(m){...} }` to update `progressBar`, `statusLabel`, and `busy`.
2. Expose a `signal requestCreate(var opts)` and handle in Main.qml like the others — but then progress signals still need a `Connections` block somewhere. Approach 1 is cleaner here.
- During hashing set `busy = true`, disable inputs (`enabled: !root.busy`) and the Create button, show `progressBar`. On finished: close (and optionally toast). On failed: show `statusLabel` in `Theme.danger`, re-enable.

### 6.2 reset() must match real ids
The stub's `reset()` (CreateTorrentDialog.qml:19-28) references `commentField`, `trackersField`, `webSeedsField`, `privateCheck`, `pieceCombo`, `progressBar`. Create controls with exactly those ids (plus `sourcePath` prop it already clears, and add `outputPath`/`startSeedCheck`/`statusLabel` resets). Menu calls `createDialog.reset()` then `.open()` (Main.qml:34-35), so `reset()` must be safe to call before first show.

---

## 7. Implementation checklist

Backend (`sessionmanager.h` / `.cpp`):
- [ ] Add includes: `<libtorrent/create_torrent.hpp>`, `<libtorrent/file_storage.hpp>`, `<libtorrent/bencode.hpp>`, `<QVariantMap>`, `<QtConcurrent>` (or QThread), `<fstream>`.
- [ ] Declare 3 `Q_INVOKABLE` methods + 3 signals (§4.1).
- [ ] Implement `defaultSavePath()` returning the existing `defaultDownloadDir()` (sessionmanager.cpp:17-19 — currently file-local; reuse).
- [ ] Implement `suggestOutputPath(source)` → `<source-basename>.torrent` next to source (or in Downloads).
- [ ] Implement `createTorrent(opts)`: parse opts, launch worker thread, run §3 sequence.
- [ ] Wire `set_piece_hashes` callback → queued `createTorrentProgress` emits (compute fraction vs `ct.num_pieces()`).
- [ ] On success: bencode + write file → queued `createTorrentFinished`; if `startSeeding`, queued `addTorrentFile`.
- [ ] On `error_code` / exception: queued `createTorrentFailed(message)`.
- [ ] Ensure all signal emits crossing thread boundary are `Qt::QueuedConnection`.

Build:
- [ ] Add `Concurrent` to `find_package(Qt6 COMPONENTS ...)` (CMakeLists.txt:8) and link `Qt6::Concurrent` (locate/confirm the `target_link_libraries` section — not in the read range; verify it exists and add there).
- [ ] If a separate worker class is used, add its `.cpp` to `qt_add_executable` (CMakeLists.txt:13-29).

QML (`CreateTorrentDialog.qml` — full rewrite of the stub):
- [ ] Build the form: source file/folder pickers, trackers `BatTextArea`, web seeds `BatTextArea`, piece-size `BatComboBox`, private `BatCheckBox`, comment field, output path + Save As picker, start-seeding `BatCheckBox`, `BatProgressBar`, status `Label`.
- [ ] Give controls the ids `reset()` expects (§6.2) and extend `reset()` for new controls.
- [ ] Cancel + Create buttons (Create `primary: true`, `enabled: sourcePath && outputPath && !busy`).
- [ ] On Create: assemble opts map, call `bridge.session.createTorrent(opts)`, set `busy = true`.
- [ ] `Connections { target: bridge.session }` for the 3 backend signals → update progress/status/busy, close on finish.
- [ ] URL→path conversion for pickers (use existing helper if added, else inline in QML — §6).
- [ ] No CMake change for the QML file (already registered, CMakeLists.txt:39).

Verification:
- [ ] Create a single-file torrent and a folder torrent; confirm `.torrent` opens in another client and piece hashes verify.
- [ ] Progress bar advances; UI stays responsive (proves threading).
- [ ] Private flag, comment, trackers, web seeds present in the output (inspect with `transmission-show` or re-add via Add Torrent).
- [ ] "Start seeding" adds the torrent and it goes to seeding state.
- [ ] Error path (e.g. unreadable source) surfaces `createTorrentFailed` without crashing.

---

## 8. Risks / notes

- **Threading is new to this codebase** — get the queued-emit marshalling right or you'll get random crashes from touching QObject signals across threads.
- **Sibling-task dependency**: `urlToLocalPath` / `defaultSavePath` are referenced by AddTorrent & Magnet but don't exist yet (§1.4). Don't assume they're present; either add the trivial ones you need or convert URLs in QML.
- **libtorrent 2.0.11 produces hybrid v1+v2 torrents by default.** Some old trackers/clients dislike v2/hybrid. If compatibility matters, expose the flags combo (§2.3) and default-or-offer `v1_only`.
- **`set_piece_hashes` blocks** for the entire input size — never call it on the GUI thread.
- **CMake `target_link_libraries` not seen** in the read range (file shown through line 55, qml module block). Confirm Qt/libtorrent linkage section before adding `Qt6::Concurrent`.
- **Don't depend on the "wired examples" backend working** — they are UI-pattern references only (§1.4).
