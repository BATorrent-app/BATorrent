# 01 — Design System & Canonical Wired-Screen Pattern

Single source of truth for porting the remaining QWidget screens to QML 1:1.
Everything below is read directly from source; citations are `path:line`.
Two items are NOT in this analysis scope and are flagged at the end: the
`SettingsWindow.qml` Window recipe details and the `Main.qml` open/close
call-sites (only the verified-from-elsewhere facts are stated).

---

## 1. Theme tokens cheat-sheet

Singleton at `src/qml/theme/Theme.qml` (`pragma Singleton`, `QtObject`).
Import with `import "theme"` (root screens) or `import "../theme"` (widgets),
reference as `Theme.<token>`. **Never hardcode hex/rgba outside Theme.qml**
(Theme.qml:12). Theme picks values off `themeBridge` so changing theme is live.

### State / control (read or call)
| Token | Kind | Notes | Ref |
|---|---|---|---|
| `Theme.name` | string | `dark`\|`light`\|`midnight`\|`sakura`\|`darkstar`; from `themeBridge.themeName` | Theme.qml:19 |
| `Theme.anime` | bool | anime art on/off; from `themeBridge.anime` | Theme.qml:20 |
| `Theme.setName(n)` | func | persists via bridge | Theme.qml:22 |
| `Theme.setAnime(on)` | func | persists via bridge | Theme.qml:26 |
| `Theme.cycle()` | func | next theme in `[dark,light,midnight,sakura,darkstar]`; call ONLY from SettingsWindow | Theme.qml:177 |
| `Theme.isLight` | bool ro | true for `light`,`sakura` | Theme.qml:32 |
| `Theme.isDark` | bool ro | `!isLight` | Theme.qml:33 |

### Spacing — `int` ro (`--s1..--s6`)
`Theme.sp1`=4, `sp2`=8, `sp3`=12, `sp4`=16, `sp5`=24, `sp6`=32 (Theme.qml:36-41).

### Fonts — `string` ro (families only; no size/weight tokens — set those inline)
`Theme.fontSans` (Theme.qml:44), `Theme.fontMono` (Theme.qml:45).

### Surfaces — `color` ro
`Theme.bg` (window/card bg, Theme.qml:48) · `Theme.panel` (panel/card-in-body, :54) ·
`Theme.elev` (titlebar+footer bars, :60) · `Theme.field` (recessed input bg, :67).

### Hairlines / hovers / tints — `color` ro
`Theme.hair` (1px rule + input border, :74) · `Theme.hairSoft` (row separators, :80) ·
`Theme.hover` (hover wash, :86) · `Theme.sel` (selection/chip-red bg, :93) ·
`Theme.accentTint` (strong accent tint, :100) · `Theme.track` (progress/scroll track, :104).

### Text ramp — `color` ro
`Theme.t1` primary (:111) · `Theme.t2` secondary/title (:117) · `Theme.t3` tertiary (:123) ·
`Theme.t4` faint/labels/idle-icon (:129).

### Accents — `color` ro
`Theme.accent` fill `--red` (:137) · `Theme.accentDark` hover-dark (:141) ·
`Theme.accentText` accent-over-bg `--red-t` (:145) · `Theme.amber` seeding fill (:152) ·
`Theme.up` upload text (:156) · `Theme.pausedFill` (:160) · `Theme.grn` health green (:165).

### Anime art — ro
`Theme.animeSource` (qrc URL, empty for light, :168) · `Theme.animeBottom` (darkstar only, :173) ·
`Theme.hasAnime` (`anime && animeSource!==""` — gate rendering on this, :174).

### Inline type scale observed across screens (use these literals, there are no tokens)
- Eyebrow caption: 9 / Bold / letterSpacing 1.8 / AllUppercase (Eyebrow.qml:9-14).
- Column-header caption: 10 / DemiBold / letterSpacing 0.6 (DetailFiles.qml:24).
- Big screen/dialog title: 19 / DemiBold / letterSpacing -0.3 (MagnetDialog.qml:25-28).
- Section sub-title: 16 / DemiBold (RemoveDialog.qml:44-46).
- Field label: 11 / DemiBold, color t3 (MagnetDialog.qml:38-41).
- Body text: 12 / 12.5 Sans, color t1/t2/t3.
- Mono numeric/meta: 11 / 11.5 / 12 Mono.
- Dialog titlebar text: 12.5 / DemiBold, color t2 (BatDialog.qml:80-82).
- Footer hint: 10.5, color t4 (BatDialog.qml:142-144).

---

## 2. Reusable widgets (all VERIFIED from source)

`import "widgets"` — qmldir registers all 16 (widgets/qmldir:1-16). Inside the
widgets dir they `import "../theme"`.

### BtnFlat — `src/qml/widgets/BtnFlat.qml` (Rectangle)
Flat/pill button. Footer Cancel (neutral) and OK (`primary`) both use it.
- `text: string` (:9) · `icon: string` qrc, optional left icon (:10) · `sm: bool` 28px vs 32px (:11) · `primary: bool` accent fill + white text (:12).
- signal `clicked()` (:13).
- Sizes: h 32 (sm 28), radius 7, white text when primary, hover boosts border/text (:15-26).
```qml
BtnFlat { text: "Cancelar"; onClicked: dlg.close() }
BtnFlat { primary: true; text: "Salvar"; onClicked: { session.doX(); dlg.close() } }
BtnFlat { sm: true; icon: "qrc:/icons/open.svg"; text: "Abrir" }
```

### IconImg — `src/qml/widgets/IconImg.qml` (Item)
Tinted SVG/PNG via MultiEffect colorization.
- `src: string` (:7) · `tint: color` default `#9ca3af` (:8) · `s: int` square px, default 16 (:9). Renders at 2x sourceSize for crispness (:17).
```qml
IconImg { src: "qrc:/icons/close.svg"; tint: Theme.t4; s: 13 }
```

### Eyebrow — `src/qml/widgets/Eyebrow.qml` (Text)
Tiny uppercase caption. Set `text` directly (it IS a Text).
- `red: bool` → color `Theme.accent` else `Theme.t4` (:8-9). 9px/Bold/spacing 1.8/uppercase.
```qml
Eyebrow { text: "ADICIONAR"; red: true }
```

### PathFld — `src/qml/widgets/PathFld.qml` (RowLayout)
Mono path field + "Procurar…" button (gap `Theme.sp2`).
- `text: alias` → inner field text (:8) · `browseLabel: string` default "Procurar…" (:9).
- signal `browseClicked()` (:10).
```qml
PathFld { id: pathFld; Layout.fillWidth: true; text: session.defaultSavePath();
          onBrowseClicked: folderPicker.open() }
```

### TArea — `src/qml/widgets/TArea.qml` (Rectangle)
Multiline mono textarea, focus border = accent.
- `text: alias` (:10) · `placeholder: alias` (:11). Default 400x88; set height via `Layout.preferredHeight`.
```qml
TArea { id: magnetArea; Layout.fillWidth: true; Layout.preferredHeight: 88
        placeholder: "magnet:?xt=urn:btih:..." }
```

### TChip — `src/qml/widgets/TChip.qml` (Rectangle)
Small mono pill (h 18, radius 999).
- `text: string` (:9) · `red: bool` → accent text + `Theme.sel` bg (:10).
```qml
TChip { text: "1080p" }
TChip { text: "VERIFICADO"; red: true }
```

### TChk — `src/qml/widgets/TChk.qml` (Rectangle)
17x17 checkbox (Canvas check), supports tri-state.
- `on: bool` (:10) · `partial: bool` (:11). signal `toggled(bool on)` (:12).
- NOTE: handler arg arrives positionally → use `onToggled: function(v){...}`. Toggling clears `partial` (:53). It manages its own `on` on click, so mirror external state by binding `on:` and also writing back in `onToggled`.
```qml
TChk { id: chk; on: dlg.deleteFiles; onToggled: function(v){ dlg.deleteFiles = v } }
```

### TFld — `src/qml/widgets/TFld.qml` (Rectangle)
Single-line input row (h 34, radius 8), focus border = accent.
- `icon: string` optional left icon (:12) · `text: alias` (:13) · `placeholder: alias` (:14) · `mono: bool` switches to mono 11.5 (:15) · `readonly: bool` (:16).
```qml
TFld { Layout.fillWidth: true; icon: "qrc:/icons/search.svg"; placeholder: "Buscar" }
```

### TSelect — `src/qml/widgets/TSelect.qml` (ComboBox, Controls.Basic)
Styled dropdown (h 30). It's a real ComboBox → use standard ComboBox API: `model`, `currentIndex`, `currentText`, `displayText`, signal `activated(index)`. Delegate expects `modelData` (string list) (:64-78).
```qml
TSelect { model: ["Auto","Sempre","Nunca"]; currentIndex: 0
          onActivated: function(i){ /* ... */ } }
```

### TToggle — `src/qml/widgets/TToggle.qml` (Rectangle)
38x21 switch, animated knob.
- `on: bool` (:9). signal `toggled(bool on)` (:10). Same positional-arg + self-toggling note as TChk.
```qml
TToggle { on: true; onToggled: function(v){ session.setSelectedForceStart(v) } }
```

### PosterThumb — `src/qml/widgets/PosterThumb.qml` (Item)
30x40 rounded poster, falls back to dimmed logo.
- `posterUrl: string` (:9) · `letter: string` (declared, unused fallback) (:10). For local files build `"file://" + posterPath` (see AddTorrentDialog.qml:26).
```qml
PosterThumb { posterUrl: model.posterPath ? "file://" + model.posterPath : "" }
```

### EmptyState — `src/qml/widgets/EmptyState.qml` (ColumnLayout)
Centered empty-list block (icon + title + 2 CTAs + drag hint). Text is hardcoded PT-BR.
- signals `openClicked()` (:8), `magnetClicked()` (:9).
```qml
EmptyState { anchors.centerIn: parent
             onOpenClicked: openDialog.open(); onMagnetClicked: magnetDialog.open() }
```

### Detail* panes (detail-panel tabs) — all `ColumnLayout`, data via `var` arrays
| File | Property | Row shape | Ref |
|---|---|---|---|
| `DetailFiles.qml` | `files: var` | `{path,size,progress,priority}` | :9 |
| `DetailPeers.qml` | `peers: var` | `{ip,port,client,downSpeed,upSpeed,progress}` | :9 |
| `DetailPieces.qml` | `pieces: var` (array of bool) | — (computes done/total) | :8 |
| `DetailTrackers.qml` | `trackers: var` | `{url,tier,status}` | :9 |

Feed them straight from the bridge:
```qml
DetailFiles    { files: session.selectedFiles }
DetailPeers    { peers: session.selectedPeerList }
DetailPieces   { pieces: session.selectedPieces }
DetailTrackers { trackers: session.selectedTrackers }
```
They render their own column header, an empty-state Text, and a `ListView`
(separators `Theme.hairSoft`, margins `Theme.sp5`). No signals — read-only.

---

## 3. Canonical recipes

### 3.1 (a) Modal dialog — `BatDialog`-based  (VERIFIED, `src/qml/BatDialog.qml`)

BatDialog is an `Item` (NOT a Window), `anchors.fill: parent`, `z: 200`,
declared inside Main.qml so its backdrop covers the whole app (BatDialog.qml:1-15).
It supplies all chrome: backdrop (with shadow), 13px card on `Theme.bg`, 36px
titlebar with close-X, a **scrollable 24px-padded body**, and a 56px footer with
Cancel/OK.

Public API:
| Member | Kind | Default | Ref |
|---|---|---|---|
| `opened` | bool | false | :15 |
| `open()` / `close()` | func | toggles `opened` | :17-18 |
| `title` | alias→titlebar | — | :20 |
| `cardW` / `cardH` | int | 480 / 460 | :21-22 |
| `footHint` | string (faint, left in footer) | "" | :23 |
| `okText` / `cancelText` | string | "OK" / "Cancelar" | :24-25 |
| `showFooter` / `showCancel` / `showOk` | bool | true | :26-28 |
| `bodyContent` | **default** alias → body ColumnLayout `.data` | — | :29 |
| signal `accepted()` | fires before close on OK | — | :31 |
| signal `rejected()` | fires before close on Cancel / X / backdrop | — | :32 |

Body contract (do NOT re-pad or wrap in your own Column):
- Inner `ColumnLayout id:bodyHost`, x/y = `Theme.sp5` (24), width = `bodyScroll.width - 2*Theme.sp5`, **child spacing `Theme.sp4` (16)** (BatDialog.qml:120-126).
- So every direct child you declare becomes a vertical, 16px-gapped, 24px-padded, scrollable row. Use `Layout.fillWidth: true` on children.
- Backdrop click and X both `rejected()` then `close()` (:39,:104). Footer OK = `accepted()+close()` (:158); Cancel = `rejected()+close()` (:152).

Recipe (call the bridge in `onAccepted`):
```qml
import QtQuick
import QtQuick.Layouts
import "theme"
import "widgets"

BatDialog {
    id: dlg
    title: "Adicionar link magnet"
    cardW: 480; cardH: 470
    okText: "Adicionar"
    footHint: "Aceita vários links, um por linha"

    onOpenedChanged: if (opened) magnetArea.text = ""   // reset on open

    ColumnLayout {                 // a grouped sub-block (label + control)
        Layout.fillWidth: true
        spacing: 7
        Text { text: "Cole o link"; color: Theme.t3; font.pointSize: 11
               font.weight: Font.DemiBold; font.family: Theme.fontSans }
        TArea { id: magnetArea; Layout.fillWidth: true; Layout.preferredHeight: 88
                placeholder: "magnet:?xt=urn:btih:..." }
    }

    onAccepted: session.addMagnetUri(magnetArea.text)
}
```
Patterns to copy verbatim:
- MagnetDialog.qml — label+TArea+note card+PathFld+TToggle; resets text on open (:16).
- RemoveDialog.qml — destructive: warn-icon block, StyledText body, clickable option card whose MouseArea and TChk both write one `deleteFiles` bool (:100-104).
- AddTorrentDialog.qml — data-driven: `loadPreview(p,path)` fills a `ListModel`, `priorities()` builds the int array, OK should call `session.addTorrentWithPrefs(torrentPath, savePath, priorities())`; listens to `session.previewPosterReady` via a `Connections` block (:47-52).

### 3.2 (b) Standalone top-level Window (like SettingsWindow)

Project rule (memory `feedback_settings_window`): Settings/Rss/Search are
**independent top-level Windows**, not in-app BatDialog overlays. They therefore
use the OS window frame (no BatDialog titlebar/footer chrome) and are shown via
`.show()` / `visible`, not `open()`. Existing examples to mirror exactly:
`src/qml/SettingsWindow.qml`, `src/qml/RssWindow.qml`, `src/qml/SearchWindow.qml`.

Skeleton (validate exact root type/flags against SettingsWindow.qml before coding):
```qml
import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import "theme"
import "widgets"

Window {
    id: win
    width: 720; height: 560
    title: "Preferências"
    color: Theme.bg            // window must paint its own bg (no card)
    // body: your own padding + ColumnLayout; sections = Eyebrow header
    // + rows of label/TToggle/TSelect/TFld with Theme.sp4/sp5 gaps.
    // Theme switcher lives here: Theme.cycle()/Theme.setName()/Theme.setAnime().
}
```
Differences vs a BatDialog modal:
- You own the layout/padding entirely (no auto 24px body, no auto footer).
- Open/close = `win.show()` / `win.close()` / `win.visible = true`, not `open()`.
- The theme switcher and `Theme.cycle()` belong here (Theme.qml:176).

[NOT RE-READ IN THIS PASS — confirm before relying on exact numbers]
`SettingsWindow.qml` (32 K), `RssWindow.qml`, `SearchWindow.qml`: capture the
exact root element (`Window` vs `ApplicationWindow`), `flags`, default size,
and section layout, and append them here.

### 3.3 (c) How fields call into the bridge  (VERIFIED, `src/gui/qmlposterbridge.h`)

Five context objects are exposed globally and callable by name from any QML scope:
`session` (QmlSessionBridge), `torrentModel` (QmlPosterModel), `torrentFilter`
(QmlTorrentFilterProxy), `themeBridge` (QmlThemeBridge), `rss` (QmlRssBridge).
(Theme proves global access: it reads `themeBridge.themeName`, Theme.qml:19.)

Convention: read widget values via their aliases (`fld.text`, `chk.on`,
`tg.on`, `sel.currentText/currentIndex`), then call the matching bridge method —
in a dialog do it in `onAccepted`, not per keystroke.

Bridge methods/properties you will actually wire (header line refs):
- `session.addMagnetUri(uri)` :156 · `addTorrentFile(path)` :155 · `previewTorrent(path)→map` :157 · `addTorrentWithPrefs(path, savePath, priorities)` :159 · `resolvePreview(hash,name)` :158 · `defaultSavePath()` :164.
- Selection actions: `pauseSelected/resumeSelected/removeSelected/removeSelectedWithFiles` :149-152 · `toggleSelectedPause` :173 · `forceRecheckSelected/forceReannounceSelected` :169-170 · `queueUp/DownSelected` :171-172 · `markSelectedCompleted/unmark` :167-168 · `setSelectedForceStart(bool)/setSelectedSuperSeeding(bool)` :165-166 · `copyMagnetLink/copyInfoHash/openSaveFolder` :161-163 · `smartPaste` :174 · `pauseAll/resumeAll` :153-154.
- Selection state (Q_PROPERTY, bind directly, `selectionChanged` notifies): `selectedName/SavePath/Size/Hash/Downloaded/Uploaded/Speed/DownSpeed/UpSpeed/Eta/Ratio/State/Poster/Description/MetaTitle/MetaInfo` (:99-116), `selectedSeeds/Peers` int (:109-110), `selectedForceStart/SuperSeeding/Completed/AtFullProgress/Paused/hasSelection` bool (:117-126), and the four `var` arrays in §2 (:122-125).
- Global stats (Q_PROPERTY, `statsChanged`): `torrentCount/activeCount/downloadingCount/seedingCount/pausedCount/completedCount` (:84-89), `totalDownSpeed/totalUpSpeed/totalDownloaded/totalUploaded/globalRatio` (:90-94), `downloadHistory/uploadHistory/historyMaxBytes` for graphs (:95-97).
- Selecting a row: `session.setSelectedIndex(i)` / `setSelectedRows([..])` / `selectedRows()` (:146-148).
- `torrentFilter`: `setFilterState(state)` :64 · `setSearchText(text)` :65 · `setSortColumn(col, asc)` :66 · `clearSort()` :67 · `mapToSource/mapFromSource(row)` :68-69.
- `rss`: prop `feeds` (var) :256 · `itemsForFeed(i)` :261 · `addFeed(url)` :262 · `removeFeed(i)` :263 · `setFeedEnabled(i,bool)` :264 · `setAutoDownload(i,bool)` :265 · `checkAllFeeds()` :266 · `downloadItem(feed,item)` :267.
- Async results arrive as signals — handle with `Connections { target: session; function onPreviewPosterReady(hash,poster){...} }` (pattern at AddTorrentDialog.qml:47-52). Signals: `statsChanged/selectionChanged/historyChanged/queueRefreshNeeded/queueMoved(from,to)/previewPosterReady(hash,path)` (:208-213).
- `torrentModel` roles for delegates: `name,stateKey,infoHash,progress,posterPath,metaTitle,stateString,downSpeed,upSpeed,size,category,numPeers,downRate,upRate,sizeBytes` (enum :23-39; lowercased role names in `roleNames()`).

### 3.4 (d) How Main.qml opens dialogs/windows

VERIFIED from the design contract (BatDialog.qml:1-15) + dialog conventions:
- Each modal is declared once as a child of the Main root: `BatDialog { id: xDlg; ... }`. Open from a button/menu handler with `xDlg.open()`; it closes itself via footer/X/backdrop (no manual close needed). For data-driven dialogs call a setup method first, e.g. `addDialog.loadPreview(session.previewTorrent(path), path); addDialog.open()`.
- Standalone Windows (Settings/Rss/Search) are shown with `.show()` / `visible = true`, never `open()` (§3.2).

[NOT RE-READ IN THIS PASS — confirm exact call sites] `Main.qml` (82 K): verify
how the native menu / toolbar buttons trigger each `open()` / `.show()`, and that
`theme`+`widgets` dirs are imported once at top. Append the concrete call sites here.

---

## Items the implementer must still pull from source (only two)
1. `SettingsWindow.qml` / `RssWindow.qml` / `SearchWindow.qml` — exact Window root type, flags, size, section layout (§3.2).
2. `Main.qml` — exact open/close call sites for each dialog and Window (§3.4).
Everything else in this document is verified from the cited source.
