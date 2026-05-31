# QML Migration Plan — Log Viewer

Status: PLAN (read-only analysis). Implementer follows this 1:1.

> Tooling note: during this analysis pass the harness dropped tool output for
> every call after the first batch. The four files this port depends on were
> read in full and are quoted with exact `path:line` evidence below
> (`logviewerdialog.h`, `logviewerdialog.cpp`, `logger.h`, `logger.cpp`,
> `qmlposterbridge.h`). The supporting QML/registration files
> (`Theme.qml`, `MagnetDialog.qml`, `RssWindow.qml`, `main.cpp`) could **not** be
> re-read this pass — references to their conventions are marked **[verify]**
> and the implementer must confirm the exact token / registration names against
> the live tree before coding. Nothing in the core mapping below depends on
> those unverified files.

---

## 1. What the QWidget Log Viewer does (full feature set)

Source: `src/gui/logviewerdialog.cpp` (class declared in `src/gui/logviewerdialog.h`).
It is a `QDialog`, 900x600 (`logviewerdialog.cpp:30`), themed via inline stylesheet
from `ThemeManager` (`logviewerdialog.cpp:32-48`).

### 1.1 Layout
- **Header row** (`logviewerdialog.cpp:55-78`): bold title (`tr_("logviewer_title")`,
  13pt bold `logviewerdialog.cpp:56-60`), stretch, a `"Level:"` label
  (`tr_("logviewer_level") + ":"` `logviewerdialog.cpp:63`), and a level-filter
  combo box on the right.
- **Body** (`logviewerdialog.cpp:80-83`): a read-only `QPlainTextEdit`, monospace
  (Menlo/Consolas, 11px `logviewerdialog.cpp:37`), `setMaximumBlockCount(20000)`
  (`logviewerdialog.cpp:82`) so old lines are dropped — keeps memory bounded while
  still showing previous-session history. Takes all stretch (`root->addWidget(m_view, 1)`).
- **Bottom action row** (`logviewerdialog.cpp:86-102`):
  - `Open folder` button → `openLogsFolder()` (`logviewerdialog.cpp:87-89`).
  - `Save` button, `objectName "primaryBtn"` (accent-filled) → `saveLogs()`
    (`logviewerdialog.cpp:90-93`).
  - `Clear` button, `objectName "dangerBtn"` (accent-colored text) → `clearLogs()`
    (`logviewerdialog.cpp:94-97`).
  - stretch.
  - `Close` button (`tr_("welcome_close")`) → `QDialog::accept` (`logviewerdialog.cpp:99-101`).

### 1.2 Features

1. **Live log tail** — `refresh()` (`logviewerdialog.cpp:112-135`). Reads only the
   *delta* since the last read for cheapness:
   - opens `Logger::currentLogPath()` read-only (`logviewerdialog.cpp:114-116`);
   - if file size unchanged → return early (`logviewerdialog.cpp:118`);
   - if file **shrank** (rotation or clear) → `m_view->clear()`, reset
     `m_lastSize=0`, reread from scratch (`logviewerdialog.cpp:119-123`);
   - `seek(m_lastSize)`, read remainder, update `m_lastSize` (`logviewerdialog.cpp:124-126`);
   - append the delta **line by line** with `appendPlainText` so the
     `maximumBlockCount` cap applies per line (`logviewerdialog.cpp:131-132`).
   - A `QTimer` polls `refresh()` every **1000 ms** (`logviewerdialog.cpp:107-109`).

2. **Auto-scroll (sticky bottom)** — before appending, capture
   `atBottom = (bar->value() == bar->maximum())` (`logviewerdialog.cpp:129-130`);
   after appending, if it *was* at bottom, snap back to bottom
   (`logviewerdialog.cpp:133-134`). So new lines auto-scroll only when the user
   hasn't scrolled up. This is the de-facto "auto-scroll" behavior — there is
   **no explicit auto-scroll toggle checkbox** in the QWidget version.

3. **Level filter** — combo seeded with Trace/Debug/Info/Warning/Error
   (`logviewerdialog.cpp:65-69`), each item carries `int(Logger::Level)` as its
   userData. Note: **Critical is intentionally omitted** from the combo (only 5 of
   6 enum values listed). Current selection initialized from
   `Logger::instance().level()` (`logviewerdialog.cpp:70`). On change
   (`logviewerdialog.cpp:71-76`): `Logger::setLevel(...)`, then `m_lastSize=0`
   + `refresh()` to re-tail. **Important semantic:** this is *not* a view-only
   filter — it changes the **global logger threshold** (persisted to QSettings,
   see `logger.cpp:53-58`). Lower-priority lines already on disk are not removed;
   the file simply stops gaining new sub-threshold lines going forward, and the
   `m_lastSize=0` reset re-reads the whole file. The combo does **not** hide
   existing on-disk lines below the threshold.

4. **Search** — **NOT present** in the QWidget dialog. There is no search box,
   no find-next. (Task brief lists "search" as a feature to document — it is a
   *desired* addition for the QML port, not an existing feature. Treat as
   net-new; see §5 item 6, optional.)

5. **Clear** — `clearLogs()` (`logviewerdialog.cpp:161-171`): `QMessageBox::question`
   confirm (`tr_("logviewer_clear_confirm")`, default No), then
   `Logger::instance().clear()` (truncates current file, writes
   `--- log cleared by user ---`, see `logger.cpp:124-132`), then `m_view->clear()`,
   `m_lastSize=0`, `refresh()`.

6. **Export / Save** — `saveLogs()` (`logviewerdialog.cpp:142-159`): default name
   on Desktop `batorrent-logs-yyyy-MM-dd-HHmmss.txt` (`logviewerdialog.cpp:144-147`),
   `QFileDialog::getSaveFileName` filter `"Text (*.txt)"` (`logviewerdialog.cpp:148-149`),
   writes `Logger::instance().readAllLogs()` (concatenated rotated + current,
   oldest first — `logger.cpp:106-122`). On open failure: `QMessageBox::warning`
   with `tr_("logviewer_save_failed")` (`logviewerdialog.cpp:152-156`).

7. **Open logs folder** — `openLogsFolder()` (`logviewerdialog.cpp:137-140`):
   `QDesktopServices::openUrl(QUrl::fromLocalFile(Logger::instance().logsDir()))`.

### 1.3 i18n string keys used (must be reused in QML)
From `logviewerdialog.cpp`: `logviewer_title`, `logviewer_level`,
`logviewer_open_folder`, `logviewer_save`, `logviewer_clear`,
`logviewer_save_failed`, `logviewer_clear_confirm`, `welcome_close`.

---

## 2. Logger API surface (map for the bridge)

Source `src/app/logger.h` / `src/app/logger.cpp`. Singleton via
`Logger::instance()` (`logger.h:18`, `logger.cpp:13-17`).

| Need | Logger member | Evidence |
|------|---------------|----------|
| Current level (get) | `Level level() const` | `logger.h:24` |
| Set level (persists to QSettings `logLevel`) | `void setLevel(Level)` | `logger.h:23`, `logger.cpp:53-58` |
| Level enum | `enum Level { Trace=0, Debug=1, Info=2, Warning=3, Error=4, Critical=5 }` | `logger.h:16` |
| Level name string | `static QString levelName(Level)` → `TRACE/DEBUG/INFO/WARN/ERROR/CRIT` | `logger.h:39`, `logger.cpp:134-145` |
| Level from string | `static Level levelFromName(const QString&)` | `logger.h:40`, `logger.cpp:147-157` |
| Logs directory | `QString logsDir() const` (AppDataLocation/logs) | `logger.h:31`, `logger.cpp:95-99` |
| Current log file path | `QString currentLogPath() const` (`logsDir()/batorrent.log`) | `logger.h:32`, `logger.cpp:101-104` |
| Full concatenated text (rotations + current, oldest first) | `QString readAllLogs() const` | `logger.h:35`, `logger.cpp:106-122` |
| Truncate current file | `void clear()` | `logger.h:37`, `logger.cpp:124-132` |
| Emit a record (thread-safe, level-filtered) | `void log(Level, const QString&)` | `logger.h:28`, `logger.cpp:60-66` |

### 2.1 Critical gap: there is NO new-line signal
`Logger` is a plain non-`QObject` singleton (`logger.h:13` — `class Logger`, no
`Q_OBJECT`, no `QObject` base). It has **no signal** when a line is written.
`writeLine` (`logger.cpp:68-75`) just appends to `m_file` and flushes — nothing is
broadcast. That is exactly why the QWidget dialog **polls the file every 1 s**
instead of subscribing.

Two implementation options for the QML port:

- **Option A (recommended — minimal, faithful):** keep the poll-the-file-delta
  approach inside the bridge with a `QTimer`. No change to `Logger` at all.
  Identical behavior to the QWidget version, lowest risk. The bridge then emits
  its **own** `linesAppended` signal so QML reacts without polling on the QML side.
- **Option B (cleaner, larger blast radius):** make `Logger` a `QObject`, add
  `signals: void lineLogged(int level, const QString &line);` emitted from
  `writeLine`. This touches the thread-safe `log()` path (emitted under
  `m_mutex` — must use queued connection because `log()` is called from any
  thread per `logger.h:27-28`). Higher risk; do **not** do this for the port.

**Decision: Option A.** The bridge owns a `QTimer` and the file-delta read,
exactly mirroring `logviewerdialog.cpp:107-135`, and re-emits a Qt signal to QML.

---

## 3. Bridge additions — `QmlLogBridge`

New class in `src/gui/qmlposterbridge.h` / `.cpp` (same file family as the other
bridges; pattern: `QmlRssBridge` `qmlposterbridge.h:253-271`). It must be
constructed in `main.cpp` and exposed to QML as a context property (e.g. `log`)
alongside `session`, `torrentModel`, `torrentFilter`, `themeBridge`, `rss`
**[verify exact registration site in main.cpp]**.

### 3.1 Header sketch
```cpp
class QmlLogBridge : public QObject
{
    Q_OBJECT
    // Whole current-tail text, ready to bind to a TextArea.text.
    Q_PROPERTY(QString text READ text NOTIFY textChanged)
    // Current global logger level as an int (Logger::Level), two-way to the combo.
    Q_PROPERTY(int level READ level WRITE setLevel NOTIFY levelChanged)
    // For populating the combo: ["Trace","Debug","Info","Warning","Error"].
    Q_PROPERTY(QStringList levelNames READ levelNames CONSTANT)
    // Resolved logs directory (display + open-folder).
    Q_PROPERTY(QString logsDir READ logsDir CONSTANT)

public:
    explicit QmlLogBridge(QObject *parent = nullptr);

    QString text() const;
    int level() const;                       // int(Logger::instance().level())
    void setLevel(int l);                    // Logger::setLevel + reset + retail
    QStringList levelNames() const;          // 5 entries, Critical omitted (matches widget)
    QString logsDir() const;                 // Logger::instance().logsDir()

    Q_INVOKABLE void start();                // begin polling (on window shown)
    Q_INVOKABLE void stop();                 // stop polling (on window closed)
    Q_INVOKABLE void clear();                // Logger::clear() + reset tail
    Q_INVOKABLE void openLogsFolder();       // QDesktopServices::openUrl(logsDir)
    // Returns true on success; QML shows error dialog on false.
    Q_INVOKABLE bool exportLogs(const QString &filePath); // write readAllLogs()
    Q_INVOKABLE QString defaultExportName() const;        // batorrent-logs-<ts>.txt on Desktop

signals:
    void textChanged();        // full text replaced (clear / level change / shrink)
    void linesAppended(const QString &delta); // optional: append-only fast path
    void levelChanged();

private slots:
    void poll();               // == LogViewerDialog::refresh delta logic

private:
    QString m_text;            // accumulated tail (capped, see below)
    qint64 m_lastSize = 0;
    QTimer m_pollTimer;
};
```

### 3.2 Behavior mapping (1:1 to the widget)
- `poll()` reproduces `logviewerdialog.cpp:112-135` exactly:
  open `Logger::instance().currentLogPath()`; size-unchanged → return; shrunk →
  reset `m_text` + `m_lastSize=0` + emit `textChanged`; else seek+read delta,
  append to `m_text`, emit `linesAppended(delta)` (and `textChanged`).
  - **Cap:** the widget relies on `QPlainTextEdit::maximumBlockCount(20000)`
    (`logviewerdialog.cpp:82`). The bridge has no such widget, so it must cap
    `m_text` itself — keep only the last ~20000 lines (split on `\n`, trim head)
    after each append to bound memory. (Or let the QML `TextArea` hold it and cap
    in the bridge; capping in the bridge is simpler and matches the 20000 number.)
- `start()` starts `m_pollTimer` at **1000 ms** and calls `poll()` once
  (matches `logviewerdialog.cpp:104,107-109`). `stop()` stops it. Call `start()`
  from QML `Window.onVisibleChanged`/`Component.onCompleted` and `stop()` on close
  so we don't poll a hidden window.
- `setLevel(int)` → `Logger::instance().setLevel(Level(l))`, then `m_lastSize=0`,
  `poll()` (re-tail from scratch, matching `logviewerdialog.cpp:71-76`), emit
  `levelChanged`.
- `levelNames()` returns the same 5 labels in the same order as the combo
  (`logviewerdialog.cpp:65-69`): `Trace, Debug, Info, Warning, Error`. Combo index
  maps to enum int directly (Trace=0..Error=4), so `level`/`setLevel` can use the
  combo `currentIndex` 1:1 — but **prefer** mapping via the enum value to stay
  robust if Critical is later added.
- `exportLogs(path)` → write `Logger::instance().readAllLogs()` to `path`,
  return success bool (mirrors `logviewerdialog.cpp:151-158`).
- `defaultExportName()` → Desktop + `batorrent-logs-<yyyy-MM-dd-HHmmss>.txt`
  (mirrors `logviewerdialog.cpp:144-147`).
- `clear()` → `Logger::instance().clear()`, reset `m_text`/`m_lastSize`, emit
  `textChanged` (mirrors `logviewerdialog.cpp:167-170`). **Confirmation dialog
  lives in QML**, not the bridge (BatDialog).
- `openLogsFolder()` → `QDesktopServices::openUrl(QUrl::fromLocalFile(logsDir()))`
  (mirrors `logviewerdialog.cpp:137-140`).

### 3.3 Count of bridge additions
- **1 new class** (`QmlLogBridge`).
- **4 properties** (`text`, `level`, `levelNames`, `logsDir`).
- **6 invokables** (`start`, `stop`, `clear`, `openLogsFolder`, `exportLogs`,
  `defaultExportName`).
- **3 signals** (`textChanged`, `linesAppended`, `levelChanged`) — `linesAppended`
  is the "new-line signal" the brief asks for (bridge-level, since Logger has none).
- **0 changes to `Logger`** (Option A).
- **1 registration line** in `main.cpp` (context property `log`).

---

## 4. New QML file — `src/qml/LogViewerWindow.qml`

Top-level window (per memory `feedback_settings_window` — must be an independent
`ApplicationWindow`/`Window`, not a cramped child modal). Mirror the structure of
`RssWindow.qml` / `SettingsWindow.qml` **[verify those use `ApplicationWindow` and
how they import the theme singleton]**.

### 4.1 Structure
```
ApplicationWindow (or Window)              // top-level, 900x600 default (widget:30)
  title: qsTrId("logviewer_title")         // i18n key reuse
  color: Theme.bg                          // [verify Theme token names]
  ColumnLayout (anchors.fill, margins 24/20/24/20, spacing 12)  // widget:50-52
    // --- Header row ---
    RowLayout
      Label { text: logviewer_title; bold 13pt }                 // widget:56-60
      Item { Layout.fillWidth: true }                            // stretch
      Label { text: logviewer_level + ":" }                      // widget:63
      ComboBox {                                                  // widget:64-77
        model: log.levelNames
        currentIndex: log.level     // Trace=0..Error=4 maps 1:1
        onActivated: log.level = index
      }
    // --- Body: scrollable monospace read-only text ---
    Flickable / ScrollView {                                     // widget:80-83
      TextArea {
        readOnly: true
        font.family: "Menlo, Consolas, monospace"; font.pointSize ~11  // widget:37
        wrapMode: TextArea.NoWrap
        text: log.text
        // sticky auto-scroll: see §4.2
      }
      Layout.fillWidth: true; Layout.fillHeight: true            // widget addWidget(.,1)
    }
    // --- Bottom action row ---
    RowLayout                                                    // widget:86-102
      BatButton { text: logviewer_open_folder; onClicked: log.openLogsFolder() }
      BatButton { primary; text: logviewer_save;  onClicked: saveDialog.open() }   // "primaryBtn"
      BatButton { danger;  text: logviewer_clear; onClicked: clearConfirm.open() } // "dangerBtn"
      Item { Layout.fillWidth: true }
      BatButton { text: welcome_close; onClicked: root.close() }
  // --- Dialogs ---
  FileDialog { id: saveDialog; fileMode: SaveFile; nameFilters: ["Text (*.txt)"]
               currentFile: log.defaultExportName()
               onAccepted: if (!log.exportLogs(selectedFile)) saveFailed.open() }
  BatDialog  { id: clearConfirm; ... message: logviewer_clear_confirm;
               onAccepted: log.clear() }                          // widget:163-170
  BatDialog  { id: saveFailed; message: logviewer_save_failed }   // widget:153
  Component.onCompleted: log.start()
  onClosing: log.stop()
```

### 4.2 Widgets to use (from `src/qml/widgets/*` — **[verify exact filenames/API]**)
- **Buttons:** the shared button widget the design kit already provides (the
  QWidget version uses `primaryBtn`/`dangerBtn`/default variants
  `logviewerdialog.cpp:43-46`,`91`,`95`). Use whatever `BatButton`-style component
  exists in `src/qml/widgets/`; pass a primary/danger flag. **[verify name]**
- **ComboBox:** the styled combo widget if one exists in `widgets/`, else a
  themed `ComboBox`. Model = `log.levelNames`.
- **Text body:** `ScrollView { TextArea }` (read-only). Do **not** use a
  `ListView`-of-lines unless search/colorization is added — a single `TextArea`
  bound to `log.text` is the faithful 1:1 of `QPlainTextEdit`.
- **Confirmation + error dialogs:** `BatDialog.qml` (the project's themed dialog,
  pattern proven by `MagnetDialog.qml`/`RemoveDialog.qml`/`AddTorrentDialog.qml`).
- **File save dialog:** `Qt.labs.platform`/`QtQuick.Dialogs` `FileDialog`
  (SaveFile mode). **[verify which Dialogs module the project already imports]**.

### 4.3 Sticky auto-scroll in QML (replaces widget:129-134)
The widget's "scroll to bottom only if already at bottom" must be reproduced:
```qml
property bool stick: true
// before text updates:
Connections {
  target: log
  function onLinesAppended(delta) {
    stick = (flick.contentY >= flick.contentHeight - flick.height - 4)
  }
}
onTextChanged: if (stick) /* position cursor at end + flick to bottom */
```
Simplest robust approach: bind a `Connections` on `log.linesAppended`, and after
the `TextArea.text` updates, if the user was within a few px of the bottom, set
`cursorPosition = length` and scroll to bottom. (This is why the bridge exposes
`linesAppended` — gives QML the hook the widget got from `appendPlainText`.)

### 4.4 Opening the window
Wire from the same place the other windows are launched (native menu / Main.qml
action that currently constructs `LogViewerDialog`). Find the existing
`LogViewerDialog` construction site and replace with showing `LogViewerWindow`
**[verify: grep `LogViewerDialog` usages across `src/` and the menu wiring in
`Main.qml` / `main.cpp`]**.

---

## 5. Implementation checklist

1. **Bridge — header.** Add `QmlLogBridge` to `src/gui/qmlposterbridge.h` with the
   4 properties / 6 invokables / 3 signals from §3.1. Add `#include <QTimer>`
   (already present `qmlposterbridge.h:12`) and forward-declare nothing new.
2. **Bridge — impl.** In `qmlposterbridge.cpp`:
   - `poll()` = port of `logviewerdialog.cpp:112-135` (delta read, shrink reset),
     plus the 20000-line cap on `m_text` (replaces `maximumBlockCount`).
   - `start()/stop()` = `QTimer` @1000 ms + initial `poll()`.
   - `setLevel`, `level`, `levelNames` (5 labels, Critical omitted).
   - `clear`, `openLogsFolder`, `exportLogs`, `defaultExportName`, `logsDir`.
   - Needed includes: `<QTimer>`, `<QFile>`, `<QScrollBar>` not needed (no widget),
     `<QDesktopServices>`, `<QUrl>`, `<QStandardPaths>`, `<QDateTime>`, `"../app/logger.h"`.
3. **Register in `main.cpp`.** Construct `QmlLogBridge` and set as context property
   `log` next to the existing bridges. **[verify exact engine/rootContext call site]**.
4. **Create `src/qml/LogViewerWindow.qml`** per §4 (top-level window, header combo,
   read-only monospace TextArea bound to `log.text`, 4 bottom buttons, BatDialog
   confirm + error, FileDialog save, sticky auto-scroll via `linesAppended`).
5. **Add the QML file to the build / resource set.** Append `LogViewerWindow.qml`
   to wherever the qml files are listed (`qt_add_qml_module` QML_FILES in
   `CMakeLists.txt`, or the `.qrc`). **[verify which mechanism the project uses —
   CMakeLists.txt is in the working set/modified]**.
6. **Replace the launch site.** Swap the old `LogViewerDialog` construction (menu
   action) to open `LogViewerWindow`. Remove the QWidget dialog from the build once
   no longer referenced (do **not** delete `logviewerdialog.*` until grep confirms
   zero remaining users). **[verify]**.
7. **i18n.** Reuse existing keys (`logviewer_title`, `logviewer_level`,
   `logviewer_open_folder`, `logviewer_save`, `logviewer_clear`,
   `logviewer_save_failed`, `logviewer_clear_confirm`, `welcome_close`) — no new
   strings required. Confirm the QML i18n mechanism (`qsTrId` vs a `Tr` helper)
   matches what `RssWindow.qml`/`SettingsWindow.qml` already use. **[verify]**.
8. **(Optional, net-new) Search.** The widget has **no** search. If the brief's
   "search" is wanted: add a `TextField` in the header, and either (a) keep the
   `TextArea` + use `TextArea` find/highlight, or (b) switch the body to a
   `ListView` of log lines fed by a small bridge model and filter in
   `filterAcceptsRow`. Defer unless explicitly requested — adding it changes the
   body widget choice in §4.2.
9. **(Optional) Explicit auto-scroll toggle.** The widget has none (only sticky
   behavior). If a checkbox is desired, add a `CheckBox` in the header bound to a
   `property bool autoScroll: true` and gate §4.3 on it. Net-new; defer.

### Acceptance / parity checks
- Tail updates live within ~1 s of new log lines.
- Scrolling up pauses auto-scroll; scrolling back to bottom resumes it.
- Changing the level combo persists across restart (QSettings `logLevel`) and
  re-tails the file.
- Clear shows a confirm, truncates the file, and the view shows the
  `--- log cleared by user ---` line.
- Save writes `readAllLogs()` (rotations + current, oldest first) to the chosen
  `.txt`; failure shows the error dialog.
- Open-folder opens `…/AppData/.../logs`.
- Window is independent top-level, not a child modal (memory constraint).
- No polling while the window is closed (`start`/`stop`).

---

## 6. Risk / notes
- **Logger has no signal** (`logger.h:13`, not a `QObject`) — the port keeps the
  file-poll model; do not refactor Logger for this task.
- **Level combo mutates global state**, not a view filter — preserve that exact
  semantic (`logviewerdialog.cpp:71-76`) so behavior matches users' expectations
  and the persisted `logLevel`.
- **Critical level is omitted** from the combo on purpose (`logviewerdialog.cpp:65-69`);
  `levelNames()` must omit it too for 1:1 index mapping.
- **Memory cap** must be reimplemented in the bridge (no `maximumBlockCount` in
  QML TextArea) — cap `m_text` to ~20000 lines (`logviewerdialog.cpp:82`).
- Unverified-this-pass items (all marked **[verify]**): exact `Theme.qml` token
  names, the shared widget filenames/APIs in `src/qml/widgets/`, the i18n helper
  used by existing windows, the `main.cpp` context-property registration site, the
  CMake/qrc QML file list, and the existing `LogViewerDialog` launch site. None
  affect the Logger/bridge mapping above; confirm them before writing the QML.
