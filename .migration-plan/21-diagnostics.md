# Port 21 — Diagnostics (QWidget → QML)

Single source of truth for porting the Network Diagnostics dialog from the legacy
`QDialog` implementation to a top-level QML `Window`.

---

## 1. Current state

### 1.1 Legacy QWidget dialog
- `src/gui/diagnosticsdialog.h` (37 LOC of content)
- `src/gui/diagnosticsdialog.cpp` (~230 LOC of content; rest is trailing blank lines)
- Still compiled: listed in `CMakeLists.txt` SOURCES at `gui/diagnosticsdialog.cpp` (≈ line 64).

### 1.2 QML side — already stubbed but NOT implemented
- `src/qml/Main.qml:88` — menu item `Diagnostics` already triggers `diagnosticsWindow.show()`.
- `src/qml/Main.qml:~22` — `DiagnosticsWindow { id: diagnosticsWindow }` is **already instantiated** as a child of the root, exactly parallel to `SettingsWindow { id: settingsWindow }` and `RssWindow { id: rssWindow }`.
- **`src/qml/DiagnosticsWindow.qml` does NOT exist** — this is the file to create.
- `DiagnosticsWindow.qml` is **NOT** in the `QML_FILES` list in `CMakeLists.txt` (qt_add_qml_module block, ≈ lines 91–110). Must be added.
- There is **no** `QmlDiagnosticsBridge` yet. `qmlposterbridge.h` exposes only `session`, `torrentModel`, `torrentFilter`, `themeBridge`, `rss` (`src/gui/qmlposterbridge.h:16-20`).

Because `Main.qml` references `diagnosticsWindow.show()`, the QML will fail to load until `DiagnosticsWindow.qml` exists and is registered. **This is currently a build/load blocker on the `experiment/qml-ui` branch.**

---

## 2. What the legacy dialog shows and runs

### 2.1 Status grid (4 labeled rows) — `diagnosticsdialog.cpp:34-44`
Built by a lambda `addRow(row, label, valueLabel)`; each value label starts as `"—"` and is later set to a colored HTML span.

| Row | Label | Backing field | Set by |
|-----|-------|---------------|--------|
| 0 | `Listen status:` | `m_listenStatus` | `runPortTest()` |
| 1 | `DHT nodes:` | `m_dhtStatus` | `runDhtTest()` |
| 2 | `NAT-PMP / UPnP:` | `m_natStatus` | `runDhtTest()` |
| 3 | `Port reachability:` | `m_portStatus` | `runPortTest()` (delayed) |

Color constants — `diagnosticsdialog.cpp:21-25`:
- `kOkColor   = "#2ecc71"` (green) → map to `Theme.success`
- `kWarnColor = "#f39c12"` (amber) → map to `Theme.warning`
- `kErrColor  = "#e74c3c"` (red)   → map to `Theme.danger`

### 2.2 Button row — `diagnosticsdialog.cpp:46-56`
| Button | Slot | Action |
|--------|------|--------|
| `Run all tests` (`m_runButton`) | `runAll()` | runs port + DHT tests, shows progress, disables itself for 1.2 s |
| `Test port` | `runPortTest()` | listen-port + reachability check |
| `Test DHT` | `runDhtTest()` | DHT running/node-count + NAT status |
| `Copy report` | `copyReport()` | copies the log text to clipboard |

### 2.3 Progress bar — `diagnosticsdialog.cpp:58-61`
`m_progress` is an **indeterminate** bar (`setRange(0,0)`), hidden by default, shown only while `runAll()` is in flight.

### 2.4 Log pane — `diagnosticsdialog.cpp:63-66`
`m_log` = read-only `QPlainTextEdit`, placeholder `"Diagnostics output will appear here."`, grows (stretch factor 1). All test output is appended line-by-line via `appendLine()` (`diagnosticsdialog.cpp:77`).

### 2.5 Auto-run on open — `diagnosticsdialog.cpp:73`
`QTimer::singleShot(0, this, &DiagnosticsDialog::runAll)` → runs all tests immediately when the dialog opens. Replicate via `onVisibleChanged`/`Component.onCompleted`.

---

## 3. Each diagnostic, mapped to backend (SessionManager)

All backend methods are in `src/torrent/sessionmanager.h` (declarations) and `src/torrent/sessionmanager.cpp` (impl). Enum: `NatStatus { Unknown, Disabled, Mapping, Mapped, Failed }` (`sessionmanager.h:40`).

### 3.1 `runAll()` — `diagnosticsdialog.cpp:81-100`
1. Disable Run button, show progress.
2. Log `=== Running diagnostics ===`.
3. Log `Qt %1 / libtorrent runtime` using `qVersion()`.
4. Call `runPortTest()` then `runDhtTest()`.
5. `QTimer::singleShot(1200, ...)` → re-enable button, hide progress, log `=== Done ===`.

### 3.2 `runPortTest()` — `diagnosticsdialog.cpp:103-130`
- `int port = m_session->listenPort();` → `SessionManager::listenPort()` (decl `sessionmanager.h:73`; impl `sessionmanager.cpp:1264`, returns `m_listenPort`). **Real.**
- Log `Listen port: %1`.
- If `port <= 0` → Listen status = `Not listening` (err color); else `Listening on %1` (ok color).
- Reads `m_session->isDhtRunning()` into `dhtRunning` but **never uses it** (`Q_UNUSED`).
- Log `Querying port reachability...`.
- `QTimer::singleShot(800, ...)` → `reachable = port > 0`; if reachable → Port status `Reachable` (ok) + log `Port %1 appears reachable.`; else `Unknown` (warn) + log `Could not confirm port reachability.`
- **Note:** reachability is **faked** — it is just `port > 0`, no real external probe. Port a faithful 1:1 (same fake) first; a real probe is a future enhancement (see §7).

### 3.3 `runDhtTest()` — `diagnosticsdialog.cpp:132-176`
- `bool running = m_session->isDhtRunning();` → `SessionManager::isDhtRunning()` (decl `sessionmanager.h:85`; impl `sessionmanager.cpp:1289`, returns `m_session->is_dht_running()`). **Real.**
- `int nodes = m_session->dhtNodeCount();` → `SessionManager::dhtNodeCount()` (decl `sessionmanager.h:86`; impl `sessionmanager.cpp:1301`, returns cached `m_lastDhtNodes`). **Real (cached).**
- If running → DHT status `Running (%1 nodes)` (ok) + log; else `Disabled` (warn) + log `DHT is disabled.`
- `auto nat = m_session->natStatus();` → `SessionManager::natStatus()` (decl `sessionmanager.h:87`; impl `sessionmanager.cpp:1315`, returns `m_natStatus`). **Real.** Switch maps each enum:
  - `Mapped` → `Mapped` / ok
  - `Mapping` → `Mapping...` / warn
  - `Failed` → `Failed` / err
  - `Disabled` → `Disabled` / warn
  - default/`Unknown` → `Unknown` / warn
- Sets NAT status label + log `NAT mapping: %1`.

### 3.4 `copyReport()` — `diagnosticsdialog.cpp:207-215`
- Reads `m_log->toPlainText()`; if empty → `QMessageBox::information(..., "Nothing to copy yet.")`; else copies to `QApplication::clipboard()` and logs `Report copied to clipboard.`

### 3.5 Orphaned slots (defined in .cpp, NOT declared in .h, NOT wired to any button)
These exist in the .cpp but have **no declaration** in `diagnosticsdialog.h` and **no UI** wiring. They are dead code in the legacy dialog but signal *intended* features. Decide per §7.
- `rebuildIndex()` — `diagnosticsdialog.cpp:217-221`: logs, calls `m_session->forceRecheckAll()`, logs. Backend: `SessionManager::forceRecheckAll()` (decl `sessionmanager.h:102`; impl `sessionmanager.cpp:1400` is a **STUB** — `// recheck all; return 0;`, does nothing). Also note the .h declares it `void` but the .cpp defines it returning `int` — pre-existing inconsistency.
- `runNetworkTest()` — `diagnosticsdialog.cpp:223-230`: logs `Pinging trackers...`, calls `m_session->knownTrackers()`, logs count + each tracker. Backend: `SessionManager::knownTrackers()` (decl `sessionmanager.h:103`; impl `sessionmanager.cpp:1358` is a **STUB** — loops over handles with an empty body, always returns an empty `QStringList`).

### 3.6 Backend methods available but UNUSED by the legacy dialog
These are declared and (mostly) implemented; useful for an enriched QML port (see §7):
- `peerCount()` → `sessionmanager.h:55`, impl `sessionmanager.cpp:1320` (returns `m_lastPeerCount`). **Real.**
- `externalIp()` → `sessionmanager.h` (`QString externalIp() const`), impl `sessionmanager.cpp:1333` (returns `m_externalIp`). **Real.**
- `listenInterfaces()` → impl `sessionmanager.cpp:1370` (returns `m_listenInterfaces`). **Real.**
- `isListening()` → impl `sessionmanager.cpp:1421` (returns `m_session->is_listening()`). **Real.**
- `uploadSlots()` → impl `sessionmanager.cpp:1345` (returns `m_uploadSlots`). **Real.**
- `activeTorrentCount()` → impl `sessionmanager.cpp:1390` (returns `m_activeTorrents`). **Real.**
- `reannounceAll()` → impl `sessionmanager.cpp:1438` is a **STUB** (`// reannounce; return;`).
- `natStatusValue()` → impl `sessionmanager.cpp:1447` is a **STUB** returning `false` (and is declared `bool`, redundant with `natStatus()`).

### 3.7 SessionManager signals available for live updates — `sessionmanager.h:130-138`
```
void natStatusChanged(SessionManager::NatStatus status);
void dhtStateChanged(bool running, int nodes);
void listenStateChanged(bool listening, int port);
void externalIpChanged(const QString &ip);
void sessionStatsUpdated();
void peerCountChanged(int peers);
void logMessage(const QString &msg);
void error(const QString &msg);
```
These let the QML window update reactively via `Connections { target: bridge.session ... }` instead of one-shot polling. **Recommended** for the port (legacy used `QTimer::singleShot`; QML should subscribe to these signals where they exist, and keep the imperative one-shot run for buttons).

> **CAUTION for implementer:** several backend methods are stubs (`knownTrackers`, `forceRecheckAll`, `reannounceAll`, `natStatusValue`). Port the dialog 1:1 against what works; do **not** invent UI for stubbed backends without also implementing the backend, or flag it as out-of-scope. The 1:1 port (§4–§6) uses only the **real** methods: `listenPort`, `isDhtRunning`, `dhtNodeCount`, `natStatus`.

---

## 4. Bridge additions — `QmlDiagnosticsBridge`

The legacy dialog calls 4 real `SessionManager` methods plus formats colored/HTML status. QML can read `bridge.session` directly for the trivial getters, but a dedicated bridge is cleaner because:
- The HTML color formatting must NOT cross into QML (use `Theme` colors + status enum strings instead).
- It centralizes the report-builder + clipboard copy + Qt-version string.
- It gives a single object for `Connections` subscriptions and `Q_INVOKABLE` test triggers.

Mirror the existing `QmlRssBridge` / `QmlThemeBridge` convention: a small `QObject` subclass with `.h` + `.cpp`, constructed in `main.cpp`, wired through `QmlPosterBridge` as a `CONSTANT` property.

### 4.1 New files (mirror `qmlrssbridge.h/.cpp`)
- `src/gui/qmldiagnosticsbridge.h`
- `src/gui/qmldiagnosticsbridge.cpp`

### 4.2 Proposed `QmlDiagnosticsBridge` interface
```cpp
class QmlDiagnosticsBridge : public QObject {
    Q_OBJECT
    // live status, read by DiagnosticsWindow
    Q_PROPERTY(int listenPort   READ listenPort   NOTIFY changed)
    Q_PROPERTY(bool listening    READ listening    NOTIFY changed)
    Q_PROPERTY(bool dhtRunning   READ dhtRunning   NOTIFY changed)
    Q_PROPERTY(int dhtNodes      READ dhtNodes     NOTIFY changed)
    Q_PROPERTY(QString natStatus READ natStatus    NOTIFY changed) // "mapped"|"mapping"|"failed"|"disabled"|"unknown"
    Q_PROPERTY(QString externalIp READ externalIp  NOTIFY changed) // optional enrichment
    Q_PROPERTY(QStringList log   READ log          NOTIFY logChanged)
public:
    explicit QmlDiagnosticsBridge(SessionManager *session, QObject *parent = nullptr);

    int listenPort() const;
    bool listening() const;
    bool dhtRunning() const;
    int dhtNodes() const;
    QString natStatus() const;     // lowercase enum string for QML switch
    QString externalIp() const;
    QStringList log() const { return m_log; }

    Q_INVOKABLE void runAll();        // appends log lines, sets m_running
    Q_INVOKABLE void runPortTest();
    Q_INVOKABLE void runDhtTest();
    Q_INVOKABLE void copyReport();    // joins m_log, sets clipboard, appends confirmation
    Q_INVOKABLE void clear();         // optional: clears log
    Q_INVOKABLE QString portReachability() const; // "reachable"|"unknown" (fake: port>0, matches legacy)

    Q_PROPERTY(bool running READ running NOTIFY runningChanged) // drives indeterminate progress + button disable
    bool running() const { return m_running; }

signals:
    void changed();
    void logChanged();
    void runningChanged();
    void reportCopied(); // optional, for a toast instead of QMessageBox

private:
    void appendLine(const QString &text);
    QPointer<SessionManager> m_session;
    QStringList m_log;
    bool m_running = false;
};
```
Notes:
- `natStatus()` returns a **lowercase string** (`"mapped"`, `"mapping"`, `"failed"`, `"disabled"`, `"unknown"`) derived from the C++ `SessionManager::NatStatus` switch (porting `diagnosticsdialog.cpp:147-176`). QML maps that string to `StatusDot.state` + label + color. Do **not** pass HTML.
- The 1.2 s and 0.8 s delayed steps from the legacy dialog become `QTimer::singleShot` inside the bridge `.cpp` (preserve the exact 1200 ms / 800 ms timings and the `=== Done ===` line for a faithful port), toggling `m_running`/emitting `runningChanged()`/`logChanged()`.
- `copyReport()` uses `QGuiApplication::clipboard()` in the bridge (`#include <QClipboard>`, `#include <QGuiApplication>`). For the "nothing to copy" case, emit a signal or no-op rather than `QMessageBox` (QML shows the toast/inline message).
- Subscribe in the bridge ctor to `SessionManager::dhtStateChanged`, `natStatusChanged`, `listenStateChanged`, `externalIpChanged` → emit `changed()` so the QML status rows stay live.

### 4.3 Wire into `QmlPosterBridge` (mirror `rss`)
In `src/gui/qmlposterbridge.h`:
- Add forward decl `class QmlDiagnosticsBridge;` (near `qmlposterbridge.h:9-12`).
- Add property: `Q_PROPERTY(QmlDiagnosticsBridge *diagnostics READ diagnostics CONSTANT)` (alongside `qmlposterbridge.h:16-20`).
- Add getter `QmlDiagnosticsBridge *diagnostics() const { return m_diagnostics; }` (near line 29).
- Add setter `void setDiagnostics(QmlDiagnosticsBridge *bridge);` (near line 34).
- Add member `QPointer<QmlDiagnosticsBridge> m_diagnostics;` (near line 41).

In `src/gui/qmlposterbridge.cpp`: add `setDiagnostics(...)` impl (mirror `setRssBridge`).

### 4.4 Wire in `main.cpp` (mirror rss bridge, around `main.cpp:66-78`)
```cpp
QmlDiagnosticsBridge diagnosticsBridge(&session);
bridge.setDiagnostics(&diagnosticsBridge);
```
(`engine.rootContext()->setContextProperty("bridge", &bridge);` already exposes it transitively as `bridge.diagnostics`.)
Add `#include "gui/qmldiagnosticsbridge.h"` near `main.cpp:19-22`.

**Bridge additions count: 1 new bridge class (`QmlDiagnosticsBridge`), 2 new files (.h/.cpp), 5 edits to `QmlPosterBridge` (.h) + 1 to its .cpp, plus main.cpp construction/wiring.**

---

## 5. New file: `src/qml/DiagnosticsWindow.qml`

Follow the **top-level `Window`** pattern of `SettingsWindow.qml` (NOT `BatDialog`), because `Main.qml` opens it with `.show()` and the project rule is that such windows are independent top-level windows (see memory `feedback_settings_window.md`). Imports/skeleton from `SettingsWindow.qml:1-19`.

### 5.1 Skeleton
```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window
import "theme"
import "widgets"

Window {
    id: root
    property var diag: bridge.diagnostics
    width: 560; height: 520           // match legacy resize(560,520) -> diagnosticsdialog.cpp:30
    minimumWidth: 480; minimumHeight: 420
    title: qsTr("Network Diagnostics")  // legacy setWindowTitle -> diagnosticsdialog.cpp:29
    color: Theme.bg
    flags: Qt.Window
    modality: Qt.NonModal

    onVisibleChanged: if (visible) diag.runAll()  // legacy auto-run -> diagnosticsdialog.cpp:73

    Connections {
        target: bridge.diagnostics
        function onChanged() { /* status props auto-bind; no body needed if using property bindings */ }
        function onLogChanged() { /* logView.model is the log list; auto-updates */ }
    }
    // ... layout below
}
```

### 5.2 Layout (1:1 with legacy)
1. **Status grid** — 4 rows, each: a `StatusDot { state: ... }` + label text + value text. Use `widgets/StatusDot.qml` (`state: "ok"|"warn"|"error"|"idle"`) for the colored indicator and `Theme.success`/`Theme.warning`/`Theme.danger` for value text color. Optionally wrap in `widgets/BatCard.qml` + `widgets/SectionHeader.qml { text: "Status" }`.
   - Listen status: bind to `diag.listening`/`diag.listenPort` → "Listening on N" (ok) / "Not listening" (error).
   - DHT nodes: `diag.dhtRunning` → "Running (N nodes)" (ok) / "Disabled" (warn).
   - NAT-PMP / UPnP: `diag.natStatus` string → switch to label+state per §3.3 map.
   - Port reachability: `diag.portReachability()` → "Reachable" (ok) / "Unknown" (warn).
2. **Button row** — `RowLayout` of `widgets/BatButton.qml`:
   - `BatButton { text: qsTr("Run all tests"); primary: true; enabled: !diag.running; onClicked: diag.runAll() }`
   - `BatButton { text: qsTr("Test port"); onClicked: diag.runPortTest() }`
   - `BatButton { text: qsTr("Test DHT"); onClicked: diag.runDhtTest() }`
   - `BatButton { text: qsTr("Copy report"); onClicked: diag.copyReport() }`
   - trailing `Item { Layout.fillWidth: true }` (replaces `addStretch()`).
3. **Progress** — indeterminate `ProgressBar { indeterminate: true; visible: diag.running; Layout.fillWidth: true }` (legacy `setRange(0,0)` + visible-on-run).
4. **Log pane** — read-only scrolling text area, `Layout.fillHeight: true`. Use a `ScrollView` + `TextArea { readOnly: true; text: diag.log.join("\n"); placeholderText: qsTr("Diagnostics output will appear here.") }`, OR a `ListView { model: diag.log; delegate: Text {...} }`. Style with `Theme.surfaceAlt` bg, `Theme.border`, `Theme.radius`.
5. **Optional toast/inline message** for the empty-copy case (replaces `QMessageBox`), driven by `reportCopied()` or a returned status.

Colors: green `#2ecc71`→`Theme.success`, amber `#f39c12`→`Theme.warning`, red `#e74c3c`→`Theme.danger`. Spacing via `Theme.spacing`/`Theme.spacingLarge`; radius via `Theme.radius`/`Theme.radiusSmall`; fonts via `Theme.fontSize`/`Theme.fontSizeHeader`.

Available widgets (all in `src/qml/widgets/`): `BatButton` (props `primary`, `danger`), `BatCard` (default children container), `BatTextField`, `StatPill` (props `label`, `value`, `accent`, `icon`), `StatusDot` (prop `state`), `SectionHeader` (Text, prop `text`).

---

## 6. CMake / registration

- Add `src/qml/DiagnosticsWindow.qml` to the `QML_FILES` block in `CMakeLists.txt` (qt_add_qml_module, ≈ lines 91–110, right after `SettingsWindow.qml`).
- Add `src/gui/qmldiagnosticsbridge.cpp` to the `qt_add_executable(batorrent ...)` SOURCES block (≈ lines 48–66, after `gui/qmlrssbridge.cpp`).
- **Remove** `src/gui/diagnosticsdialog.cpp` from SOURCES once the QML port is verified (and delete `diagnosticsdialog.h/.cpp` as cleanup). Until removed it still compiles harmlessly but is dead.

---

## 7. Decisions / enrichment options (flag to user)

Faithful 1:1 port uses only working backend (`listenPort`, `isDhtRunning`, `dhtNodeCount`, `natStatus`). The following are **optional enrichments** that require backend work — call out as out-of-scope unless requested:
- **Real port reachability probe** — legacy fakes it (`port > 0`). A genuine check needs an external probe service / STUN; backend addition required.
- **Rebuild index button** — `rebuildIndex()` exists but is orphaned and its backend `forceRecheckAll()` is a **stub** (`sessionmanager.cpp:1400`). Wiring a button requires implementing the backend.
- **Network/tracker test button** — `runNetworkTest()` orphaned; backend `knownTrackers()` is a **stub** returning `[]` (`sessionmanager.cpp:1358`). Needs backend.
- **Reannounce all** — `reannounceAll()` is a **stub** (`sessionmanager.cpp:1438`).
- **Live status rows + external IP / peer count** — backend signals (`§3.7`) and getters (`peerCount`, `externalIp`, `isListening`, `listenInterfaces`) are real; cheap, high-value enrichment over the legacy poll-once behavior. Recommended.

---

## 8. Implementation checklist

Backend / bridge:
- [ ] Create `src/gui/qmldiagnosticsbridge.h` (interface per §4.2; mirror `qmlrssbridge.h`).
- [ ] Create `src/gui/qmldiagnosticsbridge.cpp`: port `runAll/runPortTest/runDhtTest/copyReport` logic from `diagnosticsdialog.cpp:81-215` into log-list + property form; map `NatStatus` enum → lowercase string; keep 1200 ms / 800 ms timings; subscribe to `SessionManager` signals (§3.7) → emit `changed()`.
- [ ] `qmlposterbridge.h`: add fwd decl, `diagnostics` `CONSTANT` property, getter, setter, `m_diagnostics` member (§4.3).
- [ ] `qmlposterbridge.cpp`: implement `setDiagnostics(...)`.
- [ ] `main.cpp`: include header, construct `QmlDiagnosticsBridge diagnosticsBridge(&session);`, call `bridge.setDiagnostics(&diagnosticsBridge);` (§4.4).

QML:
- [ ] Create `src/qml/DiagnosticsWindow.qml` as top-level `Window` (§5): status grid (4 rows w/ `StatusDot`), button row (`BatButton`), indeterminate `ProgressBar` bound to `diag.running`, read-only log pane bound to `diag.log`, auto-run on `onVisibleChanged`.
- [ ] Replace `QMessageBox` "nothing to copy" with inline message / toast.
- [ ] Confirm `Main.qml` `DiagnosticsWindow { id: diagnosticsWindow }` (line ~22) + `diagnosticsWindow.show()` (line ~88) now resolve.

CMake / cleanup:
- [ ] Add `src/qml/DiagnosticsWindow.qml` to `QML_FILES`.
- [ ] Add `src/gui/qmldiagnosticsbridge.cpp` to executable SOURCES.
- [ ] After verification, remove `gui/diagnosticsdialog.cpp` from SOURCES and delete `diagnosticsdialog.h/.cpp`.

Verify:
- [ ] Build; open menu → Diagnostics; window opens, auto-runs, status rows show correct colors, buttons work, copy-report populates clipboard, progress shows during Run all.

---

## 9. Evidence index (path:line)
- Legacy dialog ctor/layout: `src/gui/diagnosticsdialog.cpp:27-74`; title/size `:29-30`; status grid `:34-44`; buttons `:46-56`; progress `:58-61`; log `:63-66`; auto-run `:73`.
- `runAll`: `:81-100`. `runPortTest`: `:103-130`. `runDhtTest`: `:132-176`. `copyReport`: `:207-215`. Orphans: `rebuildIndex` `:217-221`, `runNetworkTest` `:223-230`. Color constants `:21-25`.
- Header members/slots: `src/gui/diagnosticsdialog.h:14-37`.
- Backend (real): `listenPort` `sessionmanager.cpp:1264`; `isDhtRunning` `:1289`; `dhtNodeCount` `:1301`; `natStatus` `:1315`; `peerCount` `:1320`; `externalIp` `:1333`; `uploadSlots` `:1345`; `listenInterfaces` `:1370`; `activeTorrentCount` `:1390`; `isListening` `:1421`.
- Backend (stub): `knownTrackers` `:1358`; `forceRecheckAll` `:1400`; `reannounceAll` `:1438`; `natStatusValue` `:1447`.
- `NatStatus` enum: `sessionmanager.h:40`. Signals: `sessionmanager.h:130-138`.
- Bridge property surface: `qmlposterbridge.h:16-20`; getters `:25-29`; setters `:31-34`; members `:37-41`. RSS bridge model: `qmlposterbridge.h:58-73`.
- QML wiring: `Main.qml:~22` (`DiagnosticsWindow { id: diagnosticsWindow }`), `Main.qml:88` (menu `diagnosticsWindow.show()`).
- Window pattern: `SettingsWindow.qml:1-21`. Connections pattern: `RssWindow.qml:7-9`. Dialog base: `BatDialog.qml:7-22`.
- Widgets: `widgets/StatusDot.qml` (`state`), `widgets/StatPill.qml` (`label/value/accent/icon`), `widgets/BatButton.qml` (`primary/danger`), `widgets/BatCard.qml`, `widgets/SectionHeader.qml`.
- Theme tokens: `theme/Theme.qml` — `bg, surface, surfaceAlt, border, text, textMuted, textDim, muted, accent, accentText, success, warning, danger, error, upColor, downColor; radius, radiusSmall, spacing, spacingLarge; fontSize, fontSizeSmall, fontSizeLarge, fontSizeHeader`.
- CMake: SOURCES block ≈ lines 48–66 (`gui/diagnosticsdialog.cpp` ≈ :64, `gui/qmlrssbridge.cpp`); `QML_FILES` block ≈ lines 91–110 (no `DiagnosticsWindow.qml`).
- main.cpp wiring: `main.cpp:19-22` includes, `:66-78` bridge construction/setters + `setContextProperty("bridge", &bridge)`.
