# 24 — Torrent Inspector (QWidget → QML port)

Single source of truth for porting `TorrentInspectorDialog` to QML.

> NOTE: During authoring, the environment's tool-output channel intermittently
> returned empty for several design-system reads (`BatDialog.qml`,
> `MagnetDialog.qml`, `AddTorrentDialog.qml`, `RemoveDialog.qml`,
> `theme/Theme.qml`, `widgets/Detail*.qml`). Those files were NOT re-read in
> this pass. Every claim about the **C++ source** (`torrentinspectordialog.*`)
> and the **bridge** (`qmlposterbridge.*`) below is cited at `path:line` from
> files that were fully read. Claims about QML widget/dialog *conventions* are
> marked **[VERIFY]** and the implementer must confirm the exact API surface
> against the live files (props of `BatDialog`, `TFld`, `TChip`, etc.) before
> coding. The bridge/back-end mapping — the load-bearing part — is fully
> verified.

---

## 1. What the QWidget does today

Source: `/Users/mateuscruz/Projects/BAT-Torrent/src/gui/torrentinspectordialog.cpp`
(150 lines) and `.../torrentinspectordialog.h` (35 lines).

`TorrentInspectorDialog` is a modal `QDialog` constructed with a single
`.torrent` *file path* plus a `SessionManager*`
(`torrentinspectordialog.h:22-23`). It parses the file with libtorrent and
shows metadata **without adding the torrent**. It exposes one piece of
out-params state: `addRequested()` (`torrentinspectordialog.h:27`), set true
when the user clicks "Add now"; the caller (MainWindow) then runs its normal
save-path UX and add flow.

### 1.1 Parse + error path
- Parses via `std::make_shared<lt::torrent_info>(filePath.toStdString())`
  inside a `try` (`torrentinspectordialog.cpp:57-59`).
- On `std::exception`, shows a word-wrapped error label
  `tr_("inspector_parse_failed").arg(e.what())` plus a single Close button and
  returns early — no tabs, no Add (`torrentinspectordialog.cpp:60-68`).

### 1.2 Header
- Bold 15pt title = `ti->name()` (`torrentinspectordialog.cpp:70-76`).

### 1.3 Key/value metadata block (rendered as an HTML `<table>`)
Rows, in order (`torrentinspectordialog.cpp:85-101`):
| Row label key | Value source | Condition |
|---|---|---|
| `inspector_size` | `formatSize(ti->total_size())` | always |
| `inspector_files` | `ti->num_files()` | always |
| `inspector_pieces` | `"%1 × %2"`.arg(`ti->num_pieces()`, `formatSize(ti->piece_length())`) | always |
| `inspector_hash` | best info-hash (see §3.1) | always |
| `inspector_creator` | `ti->creator()` | non-empty |
| `inspector_created` | `QLocale::system().toString(QDateTime::fromSecsSinceEpoch(ti->creation_date()), ShortFormat)` | `creation_date() > 0` |
| `inspector_comment` | `ti->comment()` | non-empty |
| `inspector_private` | `tr_("inspector_private_yes")` | `ti->priv()` true |

Values are HTML-escaped (`.toHtmlEscaped()`,
`torrentinspectordialog.cpp:89`). Block is selectable-by-mouse
(`torrentinspectordialog.cpp:105`).

### 1.4 Tabs (`QTabWidget`)
- **Files** tab (`inspector_tab_files`): one row per file,
  `"%1   %2"`.arg(`files.file_path(fi)`, `formatSize(files.file_size(fi))`)
  for `fi` in `0..num_files()` (`torrentinspectordialog.cpp:109-118`).
- **Trackers** tab (`inspector_tab_trackers`): one row per
  `ti->trackers()[].url`; if zero trackers, single row
  `tr_("inspector_no_trackers")` (`torrentinspectordialog.cpp:120-125`).

### 1.5 Footer buttons (`torrentinspectordialog.cpp:128-149`)
- **Copy hash** (`ctx_copy_hash`): copies full hash to clipboard, then
  relabels itself to `tr_("pairing_copied")`.
- (stretch)
- **Add now** (`inspector_add_now`, primary/accent button): sets
  `m_addRequested = true` and `accept()`.
- **Close** (`welcome_close`): `accept()`.

### 1.6 Caller wiring (must verify in MainWindow)
`addRequested()` is consumed by MainWindow after `exec()`. The QML port
replaces this out-param with a signal/JS callback (see §6.3). Grep for
`TorrentInspectorDialog` / `addRequested` in
`/Users/mateuscruz/Projects/BAT-Torrent/src/gui/mainwindow.cpp` and in
`/Users/mateuscruz/Projects/BAT-Torrent/.migration-plan/15-mainwindow-actions.md`
to find the exact entry point (menu item / context-menu action / drag-drop)
that opens the inspector and how the save path is chosen on "Add now".
**[VERIFY in mainwindow.cpp — not re-read this pass]**

### 1.7 i18n keys used (must all exist in translator before shipping)
`inspector_title`, `inspector_parse_failed`, `inspector_size`,
`inspector_files`, `inspector_pieces`, `inspector_hash`, `inspector_creator`,
`inspector_created`, `inspector_comment`, `inspector_private`,
`inspector_private_yes`, `inspector_tab_files`, `inspector_tab_trackers`,
`inspector_no_trackers`, `inspector_add_now`, `ctx_copy_hash`,
`pairing_copied`, `welcome_close`. All already exist (the QWidget uses them);
QML just needs the same strings exposed (see §5 on i18n in QML).

---

## 2. Back-end / libtorrent mapping (file:line)

All libtorrent calls live in the inspector `.cpp` and are mirrored in the
bridge's existing `previewTorrent()`:

| Datum | libtorrent call | inspector.cpp | previewTorrent (bridge.cpp) |
|---|---|---|---|
| parse | `lt::torrent_info(path)` | `:59` | `:678` |
| name | `ti->name()` | `:70` | `:685` |
| total size | `ti->total_size()` | `:91` | `:686` (`formatSize`) |
| file count | `ti->num_files()` | `:92,112` | `:687,702` |
| pieces | `ti->num_pieces()`, `ti->piece_length()` | `:94` | — (missing) |
| info hash | `ti->info_hashes().get_best()` via `ostringstream` | `:79-80` | `:690-692` |
| creator | `ti->creator()` | `:82` | — (missing) |
| creation date | `ti->creation_date()` | `:83` | — (missing) |
| comment | `ti->comment()` | `:81` | — (missing) |
| private flag | `ti->priv()` | `:100` | — (missing) |
| files list | `ti->files()` → `file_path/file_size(fi)` | `:111-117` | `:700-711` |
| trackers | `ti->trackers()[].url` | `:121` | — (missing) |

`formatSize` / `formatSpeed` are in
`/Users/mateuscruz/Projects/BAT-Torrent/src/app/utils.h` (included by both
files). Back-end is `SessionManager` only for the "Add now" path; inspection
is pure libtorrent and needs **no** SessionManager call.

### 2.1 Overlap with `QmlSessionBridge::previewTorrent()` — IMPORTANT
`previewTorrent(filePath)` already exists and is the natural home for
inspection. Verified at
`/Users/mateuscruz/Projects/BAT-Torrent/src/gui/qmlposterbridge.h:157` and
`.../qmlposterbridge.cpp:669-713`. It already returns a `QVariantMap`:

```
ok        (bool)            cpp:684 / on error cpp:680
error     (string)          cpp:681   (only when ok=false)
name      (string)          cpp:685
totalSize (string, formatted) cpp:686
fileCount (int)            cpp:687
infoHash  (string, full 40-hex) cpp:690-692
posterPath(string)         cpp:697   (only if metadata cached)
files     (list of {path,size,dir,depth}) cpp:700-711
```

It **strips a `file://` prefix** (`cpp:671-673`) and **catches parse errors**
(`cpp:679-683`), exactly matching the dialog's error path. It does the same
`info_hashes().get_best()` hash convention (`cpp:690-692`, comment at `:689`).

**Gap:** `previewTorrent()` does NOT currently return the inspector-only
fields: `pieces`/`pieceLength`/`numPieces`, `creator`, `creationDate`,
`comment`, `private`, `trackers`, and the **full** hash is present
(`infoHash`) but there's no separate "best/v2" handling needed. The `files`
entries already carry `path` + `size` (good enough for the inspector list).

**Decision:** extend `previewTorrent()` to also emit the inspector fields
(one map, one parse) rather than adding a second invokable that re-parses the
same file. This keeps AddTorrentDialog (already a `previewTorrent` consumer —
see `/Users/mateuscruz/.../AddTorrentDialog.qml` **[VERIFY]**) working
unchanged because it only reads keys it knows; the new keys are additive.

---

## 3. Conventions to preserve

### 3.1 Info-hash string
Must use the same one-liner both places use so the displayed/copied hash is
consistent across the app:
```cpp
QString::fromStdString((std::ostringstream() << ti->info_hashes().get_best()).str());
```
(`torrentinspectordialog.cpp:79-80`, `qmlposterbridge.cpp:690-692`.) The
dialog copies the **full** hash; the per-torrent `selectedHash()` getter
elides it (`qmlposterbridge.cpp:766-772`) — the inspector must NOT elide;
show + copy the full 40-char string.

### 3.2 Number formatting
`formatSize(qint64)` from `app/utils.h` for all sizes (already imported in
bridge.cpp `:9`). Don't reformat in QML.

### 3.3 Creation date
`QLocale::system().toString(QDateTime::fromSecsSinceEpoch(secs), QLocale::ShortFormat)`
— do this in C++ (bridge) and pass a ready string to QML, because QML's `Date`
+ locale handling diverges from `QLocale::system()`. (`inspector.cpp:83,98`.)

---

## 4. Bridge additions

Minimal: **extend the existing `previewTorrent()`** plus add **one helper
invokable** for clipboard. Net new invokables/signals are deliberately small.

### 4.1 Extend `QmlSessionBridge::previewTorrent()` (no signature change)
File: `qmlposterbridge.cpp:669-713`. After the existing keys, before
`return out`, add (one extra parse is already done — reuse `ti`):

```cpp
out["numPieces"]   = ti->num_pieces();
out["pieceLength"] = formatSize(ti->piece_length());
out["pieces"]      = QStringLiteral("%1 × %2")
                       .arg(ti->num_pieces()).arg(formatSize(ti->piece_length()));
QString creator = QString::fromStdString(ti->creator());
if (!creator.isEmpty()) out["creator"] = creator;
QString comment = QString::fromStdString(ti->comment());
if (!comment.isEmpty()) out["comment"] = comment;
if (ti->creation_date() > 0)
    out["created"] = QLocale::system().toString(
        QDateTime::fromSecsSinceEpoch(ti->creation_date()), QLocale::ShortFormat);
out["isPrivate"] = ti->priv();

QVariantList trackers;
for (const auto &t : ti->trackers())
    trackers << QString::fromStdString(t.url);
out["trackers"] = trackers;
```
New includes in `qmlposterbridge.cpp`: `<QDateTime>`, `<QLocale>` (the file
already pulls `<libtorrent/torrent_info.hpp>` `:21`, `app/utils.h` `:9`,
`<sstream>` `:24`).

> Keep `files` entries as `{path, size}` (already produced `:700-711`). The
> inspector list only needs `path` + `size`; `dir`/`depth` are ignored.

### 4.2 New invokable: copy an arbitrary string to clipboard
The dialog copies the **full** hash, but the bridge's `copyInfoHash()`
(`qmlposterbridge.cpp:435-440`) copies the *selected torrent's* hash, not a
previewed file's. Add a generic copy so the QML inspector can copy the full
previewed hash without a selection:

`qmlposterbridge.h` (public, near `:174`):
```cpp
Q_INVOKABLE void copyToClipboard(const QString &text);
```
`qmlposterbridge.cpp`:
```cpp
void QmlSessionBridge::copyToClipboard(const QString &text)
{
    if (!text.isEmpty()) QGuiApplication::clipboard()->setText(text);
}
```
(`<QClipboard>`/`<QGuiApplication>` already included `:12,15`.)

### 4.3 "Add now" — reuse existing invokables, no new bridge method
The dialog's "Add now" defers save-path UX to the caller. In QML the cleanest
equivalent: close the inspector and open the **already-ported**
`AddTorrentDialog.qml` for the same path (it already calls `previewTorrent`
and `addTorrentWithPrefs`, giving the user save-path + file-priority UX). So
**no** new bridge method is needed for Add. Fallback if a one-click add is
desired: call existing `addTorrentFile(path)` (`qmlposterbridge.h:155`,
`.cpp:654-661`, uses `defaultSavePath()`).

### Bridge additions count
- **1** method extended (`previewTorrent`, additive keys only — signature
  unchanged).
- **1** new invokable (`copyToClipboard`).
- **0** new signals.
- **0** changes to `previewTorrent`'s callers required (additive).

Total new public API surface: **1 invokable**.

---

## 5. QML: `InspectorWindow.qml` vs `BatDialog`

### 5.1 Recommendation
Implement as **`BatDialog`-based modal**, named `InspectorDialog.qml` (mirrors
the existing `AddTorrentDialog.qml` / `MagnetDialog.qml` / `RemoveDialog.qml`
family, all of which sit in `src/qml/` and build on `BatDialog.qml`).
Rationale: the inspector is transient, modal-by-nature ("peek before add"),
and the design kit already has the dialog chrome. A standalone `Window`
(`InspectorWindow.qml`) is only warranted if the team wants the original's
independent top-level feel (cf. the Settings-window memory). Given the size
(640×520 in the QWidget, `inspector.cpp:32`) and its tight coupling to the
add flow, a `BatDialog` is the better fit. **[VERIFY BatDialog API — not
re-read this pass]**: confirm `BatDialog`'s slot/property names (title,
content default-property, footer/buttons area, `open()`/`close()`,
width/height) against `MagnetDialog.qml` and `RemoveDialog.qml`, which are the
canonical small-dialog examples to copy.

### 5.2 Structure (1:1 with the QWidget)
```
InspectorDialog (BatDialog)
  property string filePath
  property var data: ({})        // result of bridge.previewTorrent(filePath)
  title: tr("inspector_title")

  function load() {
      data = session.previewTorrent(filePath)
      // toggles error vs content state below
  }

  // ERROR STATE  (data.ok === false)
  Column { Label(text: tr("inspector_parse_failed").arg(data.error || ""))  // wordWrap
           BtnFlat(tr("welcome_close"), onClicked: close()) }

  // CONTENT STATE (data.ok)
  Column {
    Label  title  -> data.name                       (bold ~15pt)
    Grid/Column KV block (only rows whose value present):
       size      -> data.totalSize
       files     -> data.fileCount
       pieces    -> data.pieces      (pre-formatted in bridge)
       hash      -> data.infoHash    (full, monospace, selectable)
       creator   -> data.creator     (if present)
       created   -> data.created     (if present)
       comment   -> data.comment     (if present)
       private   -> tr("inspector_private_yes")  (if data.isPrivate)
    TabBar/StackLayout (or two TChip-style tabs):
       Files:    ListView model: data.files   delegate: path + size
       Trackers: ListView model: data.trackers (strings);
                 if empty -> single row tr("inspector_no_trackers")
  }

  // FOOTER (both states differ; content footer:)
  Row {
    BtnFlat(copyLabel)  onClicked: { session.copyToClipboard(data.infoHash);
                                     copyLabel = tr("pairing_copied") }
    Item { Layout.fillWidth }                    // stretch
    BtnFlat(tr("inspector_add_now"), primary)    onClicked: requestAdd()
    BtnFlat(tr("welcome_close"))                 onClicked: close()
  }
```

### 5.3 Widgets to reuse (from `src/qml/widgets/`, **[VERIFY exact props]**)
- Buttons: **`BtnFlat`** (used everywhere; supports a primary/accent variant
  — confirm prop name, e.g. `primary:` or `accent:`).
- Tabs: the detail panel uses tab chips; reuse the same **`TChip`**-style
  toggle row that `Main.qml`'s detail tabs use, OR a `TabBar`. The closest
  existing files-list precedent is **`widgets/DetailFiles.qml`** and
  **`widgets/DetailTrackers.qml`** — copy their `ListView` + row delegate
  layout (path left, size right; tracker url full-width). **[VERIFY — not
  re-read]**: these expose `model`/`files`/`trackers` properties; the
  inspector feeds `data.files` / `data.trackers` instead of
  `session.selectedFiles`.
- KV rows: plain `Label` pairs styled with `Theme` muted-label + value, like
  the detail-panel info block. Hash row should be selectable
  (`TextEdit { readOnly:true; selectByMouse:true }` or a selectable `Label`)
  and monospace.
- Empty/error: there's an `EmptyState.qml` widget — optional for the
  zero-trackers / parse-error visuals.

### 5.4 Theme tokens (replace the QWidget stylesheet at `inspector.cpp:35-49`)
The QWidget stylesheet maps directly to `Theme.qml` tokens **[VERIFY token
names — Theme.qml not re-read this pass]**:
`bgColor→Theme.bg`, `textColor→Theme.text`, `surfaceColor→Theme.surface`,
`borderColor→Theme.border`, `mutedColor→Theme.muted`,
`accentColor→Theme.accent`. The primary button uses accent bg + white text
(`inspector.cpp:47`). Border radius 6, list item padding 4×8, tab padding
8×16, selected-tab 2px accent underline (`inspector.cpp:38-46`) — match with
the existing dialog/tab widgets which already encode these.

### 5.5 i18n in QML
QML accesses translations through the existing translator pattern used by the
other ported dialogs (e.g. `MagnetDialog.qml`). **[VERIFY]** how strings are
surfaced (likely a `tr`/`i18n` helper or a context property exposing
`Translator`). Reuse the same keys listed in §1.7 — do NOT invent new keys.
`inspector_parse_failed` and `inspector_private_yes` take/are plain strings;
`inspector_parse_failed` uses `.arg(error)` → in QML use
`tr("inspector_parse_failed").arg(data.error)` if the helper returns a
`QString`, else string-concat.

---

## 6. Wiring into Main.qml / launch path

### 6.1 Instantiate
Add the dialog instance alongside the other dialogs in `Main.qml` (search for
how `AddTorrentDialog`/`MagnetDialog`/`RemoveDialog` are declared & opened).
**[VERIFY Main.qml — 82k file, not re-read this pass.]** Pattern:
```qml
InspectorDialog { id: inspectorDialog }
function openInspector(path) { inspectorDialog.filePath = path;
                               inspectorDialog.load(); inspectorDialog.open() }
```

### 6.2 Entry points (match the QWidget's triggers)
Find where MainWindow opened `TorrentInspectorDialog` and replicate in QML:
likely a "Inspect…" context-menu / menu item and possibly drag-drop of a
`.torrent`. See
`/Users/mateuscruz/Projects/BAT-Torrent/.migration-plan/15-mainwindow-actions.md`
for the menu/context-menu inventory and add an "Inspect torrent…" action that
file-picks a `.torrent` then calls `openInspector(path)`. **[VERIFY]**

### 6.3 "Add now" handoff (replaces `addRequested()` out-param)
Inside `requestAdd()`:
```qml
function requestAdd() {
    close()
    addTorrentDialog.filePath = filePath   // reuse the ported Add dialog
    addTorrentDialog.load(); addTorrentDialog.open()
}
```
This gives the user the same post-inspect "pick save path / priorities, then
add" experience the QWidget delegated to MainWindow. Confirm
`AddTorrentDialog.qml`'s exact property/open API. **[VERIFY]**

---

## 7. Edge cases / parity checklist

- [ ] Parse failure → show `inspector_parse_failed` + Close only (no tabs, no
      Add). Driven by `data.ok === false`.
- [ ] Empty trackers → single `inspector_no_trackers` row, not a blank list.
- [ ] Optional rows (creator/created/comment/private) hidden when absent —
      bridge already omits the keys when empty (§4.1), so QML can test
      `data.creator !== undefined`.
- [ ] Hash shown **in full** and selectable; Copy copies the full hash and
      relabels the button to `pairing_copied` (button-local state, like
      `inspector.cpp:131-134`).
- [ ] `file://` URLs from drag-drop / file dialog are stripped — already
      handled inside `previewTorrent` (`cpp:671-673`); pass raw path through.
- [ ] No SessionManager mutation on inspect (pure preview). Only Add path
      touches the session.
- [ ] Re-opening for a different file must reset the Copy button label and the
      active tab.

---

## 8. Implementation checklist (ordered)

1. **Bridge** — `qmlposterbridge.cpp`: extend `previewTorrent()` with the keys
   in §4.1 (`numPieces`, `pieceLength`, `pieces`, `creator`, `comment`,
   `created`, `isPrivate`, `trackers`); add `#include <QDateTime>`,
   `<QLocale>`. (~15 LOC.)
2. **Bridge** — add `copyToClipboard(QString)` invokable: declare in
   `qmlposterbridge.h` (~`:174`), define in `.cpp` (~5 LOC).
3. **QML** — create `src/qml/InspectorDialog.qml` extending `BatDialog`,
   following `MagnetDialog.qml`/`RemoveDialog.qml` structure; implement error
   state + content state + KV block + Files/Trackers tabs + footer (§5.2).
   Reuse `BtnFlat`, the detail-files/trackers list pattern, `Theme` tokens.
4. **QML** — register the new file: add to the QML resource list (qmldir/QRC or
   `qt_add_qml_module` in `CMakeLists.txt`). **[VERIFY how Main.qml & the
   ported dialogs are registered]** — the other dialogs in `src/qml/` are the
   template.
5. **Main.qml** — instantiate `InspectorDialog`, add `openInspector(path)`
   helper, wire "Inspect torrent…" entry point (menu/context-menu/drag-drop),
   and wire `requestAdd()` → open `AddTorrentDialog`.
6. **i18n** — confirm all keys in §1.7 are reachable from QML via the existing
   translator helper; no new keys needed.
7. **Cleanup (optional, after parity confirmed)** — `torrentinspectordialog.*`
   and its MainWindow call site can be removed once the QML path is wired (do
   NOT remove until the whole QWidget shell is retired; other docs in
   `.migration-plan/` track the global cutover).
8. **Build** — ensure CMake still lists `qmlposterbridge.cpp` (it does) and the
   new `.qml` is in the module. Run a build; verify no unused-include or moc
   issues from the new bridge invokable.

### Open items the implementer MUST verify (not re-read this pass)
- `BatDialog.qml` public API (title prop, content default property, footer
  area, open/close, sizing).
- `BtnFlat` primary/accent prop name.
- `DetailFiles.qml` / `DetailTrackers.qml` delegate layout to copy.
- `Theme.qml` exact token names (bg/surface/border/muted/text/accent).
- Translator access pattern in QML (how `MagnetDialog.qml` localizes).
- `Main.qml` dialog-instantiation + entry-point pattern; QML module
  registration (qmldir/QRC/CMake).
- MainWindow's current inspector trigger + save-path-on-Add flow
  (`mainwindow.cpp`, cross-ref `15-mainwindow-actions.md`).
