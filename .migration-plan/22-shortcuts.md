# 22 — Shortcuts Reference (ShortcutsWindow.qml)

## Scope

Port the QWidget `ShortcutsDialog` to a QML top-level window. This is the
simplest screen in the migration: **fully static data, no C++ bridge required**.
The data is a hard-coded list of key/description pairs grouped into four
sections, rendered as scrollable group headers + two-column rows, with a single
Close button in a footer.

## Source of truth (original QWidget)

- `src/gui/shortcutsdialog.h:6-11` — `ShortcutsDialog : QDialog`, ctor + `sizeHint()` override.
- `src/gui/shortcutsdialog.cpp:26-131` — the entire dialog:
  - `src/gui/shortcutsdialog.cpp:28` — window title `tr("Keyboard Shortcuts")`.
  - `src/gui/shortcutsdialog.cpp:29` — `setMinimumSize(560, 600)`.
  - `src/gui/shortcutsdialog.cpp:31-66` — the `groups` data table (the payload to port).
  - `src/gui/shortcutsdialog.cpp:72-116` — `QScrollArea` → content `QVBoxLayout`,
    contentsMargins `(24,24,24,24)`, spacing `28` between groups.
  - `src/gui/shortcutsdialog.cpp:82-88` — group title label: object name `shortcutGroupTitle`, font `pointSize+1`, bold.
  - `src/gui/shortcutsdialog.cpp:90-93` — per-group `QGridLayout`, horizontal spacing `24`, vertical spacing `10`, column 1 stretches.
  - `src/gui/shortcutsdialog.cpp:96-108` — key label (object `shortcutKey`, left/vcenter, min width `140`) + desc label (object `shortcutDesc`, word-wrap), both top-aligned.
  - `src/gui/shortcutsdialog.cpp:118-130` — footer (`shortcutFooter`), margins `(24,16,24,16)`, right-aligned **Close** button (`primaryButton`, default) wired to `accept()`.
- `src/gui/shortcutsdialog.cpp:133-135` — `sizeHint()` returns `QSize(560, 600)`.
- `src/gui/mainwindow.cpp:312-314` — old launch point: Help menu → `&Keyboard Shortcuts`, **shortcut `F1`**, opens `ShortcutsDialog` via `dlg.exec()`. This menu item + F1 must be reproduced in `Main.qml`.

Note: the F1 row is self-documenting (`{"F1", tr("Show this shortcuts reference")}`,
`src/gui/shortcutsdialog.cpp:36`), so the new window must itself be reachable by F1.

## Shortcut data table (exact, 4 groups / 26 entries)

Verbatim from `src/gui/shortcutsdialog.cpp:31-66`. Wrap every key string and
description in `qsTr()` in QML to preserve translation parity with the original
`tr()` calls.

### Group 1 — "Application" (`:32`)

| Keys | Description |
|------|-------------|
| `Ctrl+,` | Open Settings |
| `Ctrl+Q` | Quit BATorrent |
| `Ctrl+W` | Close window |
| `F1` | Show this shortcuts reference |
| `Ctrl+Shift+L` | Open log viewer |

### Group 2 — "Torrents" (`:39`)

| Keys | Description |
|------|-------------|
| `Ctrl+O` | Add torrent from file |
| `Ctrl+U` | Add torrent from magnet/URL |
| `Ctrl+N` | Create new torrent |
| `Delete` | Remove selected torrent |
| `Shift+Delete` | Remove selected torrent + data |
| `Ctrl+A` | Select all torrents |
| `Space` | Pause/resume selected |
| `Ctrl+R` | Force recheck selected |
| `Ctrl+F` | Focus search/filter box |

### Group 3 — "Queue" (`:50`)

| Keys | Description |
|------|-------------|
| `Ctrl+Up` | Move up in queue |
| `Ctrl+Down` | Move down in queue |
| `Ctrl+Home` | Move to top of queue |
| `Ctrl+End` | Move to bottom of queue |

### Group 4 — "View" (`:56`)

| Keys | Description |
|------|-------------|
| `Ctrl+1` | Show all torrents |
| `Ctrl+2` | Show downloading |
| `Ctrl+3` | Show seeding |
| `Ctrl+4` | Show completed |
| `Ctrl+5` | Show active |
| `Ctrl+6` | Show inactive |
| `F5` | Refresh view |
| `Ctrl+Shift+T` | Toggle theme |

## Bridge additions

**Zero.** No new C++, no additions to `src/gui/qmlposterbridge.h`
(`:14-18` already exposes session/torrentModel/torrentFilter/themeBridge/rss —
none are needed here). The data lives inline in the QML as a `ListModel` /
JS array. This screen does not read or mutate any backend state.

## Design-system mapping

Use the existing window + widget components (no new widgets needed):

- Top-level window: mirror `src/qml/SettingsWindow.qml:7-15` — `ApplicationWindow`,
  `color: Theme.background`, `flags: Qt.Window`. Set `width: 560`, `height: 600`,
  `minimumWidth: 560`, `minimumHeight: 600` to match the QWidget min size
  (`src/gui/shortcutsdialog.cpp:29`, `:133-135`).
- Header `ToolBar`: copy `src/qml/SettingsWindow.qml:19-31`, title `qsTr("Keyboard Shortcuts")`.
- Footer `ToolBar`: copy `src/qml/SettingsWindow.qml:33-41` — right-aligned
  `BatButton` with `primary: true` (the original Close was `primaryButton`,
  `src/gui/shortcutsdialog.cpp:125`) and `onClicked: root.close()`.
- Body: `ScrollView` (pattern `src/qml/SettingsWindow.qml:43-46`) with
  `contentWidth: availableWidth`, margins `Theme.spacingL`.
- Group title: `BatLabel` (`src/qml/widgets/BatLabel.qml`) with
  `font.pixelSize: Theme.fontSizeL` + `font.bold: true` (original was bold,
  pointSize+1 — `src/gui/shortcutsdialog.cpp:84-87`).
- Key cell: `BatLabel` with `Layout.minimumWidth: 140`
  (`src/gui/shortcutsdialog.cpp:100`). Optional polish — wrap each key in a
  keycap-style chip using `BatBadge` (`src/qml/widgets/BatBadge.qml`) or a small
  `Rectangle { radius: Theme.radiusS; color: Theme.surfaceAlt }`; original was a
  plain monospace-ish label, so a plain `BatLabel` is the faithful 1:1 port —
  do the chip only after the literal port is in place (per the port-first rule).
- Desc cell: `BatLabel` with `wrapMode: Text.WordWrap`, `Layout.fillWidth: true`
  (mirrors `shortcutDesc` word-wrap + column stretch, `src/gui/shortcutsdialog.cpp:93,104`).
- Spacing constants from `src/qml/theme/Theme.qml`: use `Theme.spacingL` (20) for
  group gaps (original 28 → closest is `spacingXL` 32 or `spacingL` 20; prefer
  `Theme.spacingL`), `Theme.spacingS`/`Theme.spacingM` for row spacing.

## Recommended QML structure

`Repeater` over an inline `ListModel` of groups; each group is a `ColumnLayout`
containing the title `BatLabel` and an inner `Repeater` over a JS array of
`{keys, desc}` rows laid out with `GridLayout { columns: 2 }`. Because nested
`ListModel` is awkward in QML, the cleanest approach is a `readonly property var
groups: [ { title: ..., rows: [ {keys, desc}, ... ] }, ... ]` JS array and a
`Repeater { model: root.groups }`. All strings wrapped in `qsTr()`.

## Wiring into Main.qml

Follow the existing window-launcher pattern in `src/qml/Main.qml:40-71`
(the `settingsWindowComponent` / `openSettings()` lazy-create-and-show idiom):

1. Add a `Component { id: shortcutsWindowComponent; ShortcutsWindow {} }`
   (mirror `src/qml/Main.qml:45-48`).
2. Add `property var shortcutsWindow: null` (mirror `:59-60`) and an
   `openShortcuts()` function that lazily `createObject(window)` then
   `show()` + `raise()` (mirror `openSettings()` at `:62-66`).
3. Add a **Help** menu to the `MenuBar` (`src/qml/Main.qml:19-36`) — the QML
   menu currently has only File/View and no Help menu, so this is a new menu:
   ```
   Menu {
       title: qsTr("&Help")
       MenuItem {
           text: qsTr("&Keyboard Shortcuts")
           onTriggered: window.openShortcuts()
       }
   }
   ```
4. Add the **F1** accelerator (the original used `QKeySequence(Qt::Key_F1)` /
   `QKeySequence::HelpContents`, `src/gui/mainwindow.cpp:304,312`). Either set
   `MenuItem { shortcut: StandardKey.HelpContents }` on the menu item, or add a
   top-level `Shortcut { sequence: StandardKey.HelpContents; onActivated:
   window.openShortcuts() }` inside `Main.qml`.

## CMake registration

Add the new file to `qt_add_qml_module(batorrent ... QML_FILES ...)` in
`CMakeLists.txt:61-80` (alongside the other `src/qml/*.qml` entries), e.g.
after `src/qml/SettingsWindow.qml` (`:66`):
```
    src/qml/ShortcutsWindow.qml
```

## Implementation checklist

- [ ] Create `src/qml/ShortcutsWindow.qml` as an `ApplicationWindow`
      (copy header/footer/ScrollView scaffold from `SettingsWindow.qml`).
- [ ] Set title `qsTr("Keyboard Shortcuts")`, size 560×600, min 560×600,
      `color: Theme.background`, `flags: Qt.Window`.
- [ ] Add `readonly property var groups` JS array with the 4 groups / 26 rows
      exactly as in the data table above; all strings via `qsTr()`.
- [ ] `Repeater` over groups → bold group title `BatLabel` + nested
      `GridLayout(columns:2)` of key `BatLabel` (min width 140) and
      word-wrapping desc `BatLabel`.
- [ ] Footer `ToolBar` with right-aligned primary `BatButton` "Close" →
      `root.close()`.
- [ ] In `Main.qml`: add `shortcutsWindowComponent`, `shortcutsWindow` property,
      `openShortcuts()`, a Help menu item, and an F1 `Shortcut`/StandardKey.
- [ ] Add `src/qml/ShortcutsWindow.qml` to `CMakeLists.txt` `QML_FILES`.
- [ ] Build + open via Help menu and via F1; verify all 4 groups, 26 rows,
      scroll, theme follows `Theme.dark`, Close button closes the window.
- [ ] After 1:1 parity confirmed, optionally upgrade key cells to keycap chips.

## Effort estimate

**Very low — ~1 hour.** Static content, no bridge, no model contract, no
threading. Bulk of the work is transcribing the 26-row table and copying the
`SettingsWindow.qml` scaffold; the only "logic" is the Main.qml launcher +
F1/Help-menu wiring, which is a direct copy of the existing Settings/RSS
launcher idiom.
