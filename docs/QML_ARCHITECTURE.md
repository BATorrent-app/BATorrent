# BATorrent QML Architecture Reference

## Architecture Decision

**Pure QML UI** with `QQmlApplicationEngine` + C++ backend exposed via `QML_ELEMENT`.
No QWidgets for UI — only linked for `Qt.labs.platform` (SystemTrayIcon).

### Why not QQuickWidget (hybrid)
- Disables threaded render loop (loses vsync animations, Animator types don't work)
- Extra GPU pass (offscreen rendering)
- Added complexity for no benefit

### Why not 100% QWidgets
- No real shadows, blur, or smooth animations
- QSS is limited compared to QML property bindings
- Qt Company recommends QML for new desktop apps since 2023

---

## main.cpp Pattern

```cpp
#include <QApplication>  // NOT QGuiApplication — needed for Qt.labs.platform
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("BATorrent");
    app.setApplicationVersion(APP_VERSION);

    QQmlApplicationEngine engine;
    engine.loadFromModule("BATorrent", "Main");

    return app.exec();
}
```

Note: `QApplication` (not `QGuiApplication`) is required because `Qt.labs.platform`
(SystemTrayIcon) depends on QtWidgets internally on some platforms.

---

## C++ → QML Exposure

**DO NOT use** `rootContext()->setContextProperty()` — it's deprecated, slow, and
invisible to qmllint/QML compiler.

**Use `QML_ELEMENT` macro:**

```cpp
class SessionManager : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // ...
};
```

CMake registers it automatically via `qt_add_qml_module(... SOURCES ...)`.

---

## CMake Pattern

```cmake
qt_add_executable(BATorrent src/main.cpp)

qt_add_qml_module(BATorrent
    URI BATorrent
    VERSION 1.0
    QML_FILES
        src/qml/Main.qml
        src/qml/theme/Theme.qml
        src/qml/components/Toolbar.qml
        src/qml/components/FilterBar.qml
        src/qml/components/PosterGrid.qml
        src/qml/components/PosterCard.qml
        src/qml/components/DetailsPanel.qml
        src/qml/components/StatusBar.qml
    SOURCES
        src/models/TorrentListModel.h src/models/TorrentListModel.cpp
        src/models/ThemeManager.h src/models/ThemeManager.cpp
    RESOURCES
        src/icons/grid.svg
        src/icons/list.svg
        src/images/logo.svg
        src/images/sakura-branch.png
)
```

No manual `.qrc` files needed — `qt_add_qml_module` handles resource embedding.

---

## Theme System

Singleton pattern — all colors, fonts, spacing in one file:

```qml
// Theme.qml
pragma Singleton
import QtQuick

QtObject {
    // Switched at runtime by ThemeManager C++ singleton
    property string current: "dark"

    readonly property color bg: {
        if (current === "sakura") return "#FDE6EF"
        if (current === "light") return "#F7F6F4"
        if (current === "midnight") return "#08070D"
        return "#0E0A0A"
    }
    // ... all other colors

    // Typography scale (Major Second 1.125, base 11pt)
    readonly property int fontDisplay: 18
    readonly property int fontHeading: 15
    readonly property int fontSubheading: 12
    readonly property int fontBody: 11
    readonly property int fontCaption: 9

    // Spacing
    readonly property int spacingXs: 4
    readonly property int spacingSm: 8
    readonly property int spacingMd: 12
    readonly property int spacingLg: 16
    readonly property int spacingXl: 24
}
```

Access everywhere: `Theme.bg`, `Theme.fontBody`, `Theme.spacingMd`.

---

## File Structure

```
src/
├── main.cpp                    Entry point (QApplication + QQmlApplicationEngine)
├── backend/
│   ├── sessionmanager.h/cpp    libtorrent wrapper (QML_SINGLETON)
│   ├── metadataresolver.h/cpp  TMDB/IGDB API (QML_SINGLETON)
│   ├── torrentlistmodel.h/cpp  QAbstractListModel (QML_ELEMENT)
│   └── thememanager.h/cpp      Theme state (QML_SINGLETON)
├── qml/
│   ├── Main.qml                Root layout (ApplicationWindow)
│   ├── theme/
│   │   └── Theme.qml           Color/font/spacing singleton
│   ├── components/
│   │   ├── Toolbar.qml
│   │   ├── FilterBar.qml
│   │   ├── PosterGrid.qml
│   │   ├── PosterCard.qml
│   │   ├── TorrentTable.qml
│   │   ├── DetailsPanel.qml
│   │   ├── SpeedGraph.qml
│   │   └── StatusBar.qml
│   └── dialogs/
│       ├── AddTorrentDialog.qml
│       └── SettingsDialog.qml
└── resources/
    ├── icons/
    └── images/
```

---

## Model Pattern for QML

Use `QAbstractListModel` (not TableModel — QML views are role-based):

```cpp
class TorrentListModel : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT
public:
    enum Role {
        NameRole = Qt::UserRole + 1,
        ProgressRole,
        StateRole,
        PosterPathRole,
        // ...
    };
    Q_ENUM(Role)

    QHash<int, QByteArray> roleNames() const override {
        return {
            {NameRole, "name"},
            {ProgressRole, "progress"},
            {StateRole, "stateKey"},
            {PosterPathRole, "posterPath"},
        };
    }
};
```

**Delegates access roles as `required property`:**
```qml
delegate: PosterCard {
    required property int index
    required property string name
    required property real progress
    required property string posterPath
}
```

---

## Platform Integration

| Feature | Solution |
|---------|----------|
| System tray | `Qt.labs.platform.SystemTrayIcon` (requires QtWidgets link) |
| Native menus | `Qt.labs.platform.MenuBar` or C++ QMenuBar |
| File dialogs | `QtQuick.Dialogs.FileDialog` (native on all platforms) |
| Folder dialogs | `QtQuick.Dialogs.FolderDialog` |
| Message dialogs | `QtQuick.Dialogs.MessageDialog` |
| Shortcuts | `Shortcut { sequence: "Ctrl+O"; onActivated: ... }` |

---

## Performance Rules

- `GridView.reuseItems: true` for 100+ items
- `Image.asynchronous: true` + `sourceSize` always set
- Max 1-2 `MultiEffect` per visible screen
- `layer.enabled` only when effect is active
- No JS function calls in hot bindings
- `Animator` types for opacity/scale/rotation (render thread)
- `cacheBuffer: 320` on GridView for pre-loading

---

## Keyboard Navigation

- `activeFocusOnTab: true` on interactive items
- `Shortcut {}` for global hotkeys
- `Keys.onPressed` for per-item handling
- `FocusScope` for grouping
- `KeyNavigation.tab` / `KeyNavigation.backtab` for custom order

---

## Testing

```qml
import QtQuick
import QtTest

TestCase {
    name: "PosterCardTest"
    function test_titleDisplay() {
        var card = createTemporaryQmlObject(
            'import BATorrent; PosterCard { name: "Test" }', testCase)
        compare(card.name, "Test")
    }
}
```

Run with `-platform offscreen` for CI (headless).

---

## Lessons Learned (during port)

### Rectangle `clip: true` does NOT clip to rounded corners
`clip: true` clips children to the **bounding rectangle** only — rounded corners are
ignored. To clip an Image (no `radius` property) to a rounded shape, use
`MultiEffect` with a `maskSource`:

```qml
Image { id: img; visible: false; layer.enabled: true }
Rectangle { id: mask; visible: false; layer.enabled: true; topLeftRadius: 12; topRightRadius: 12; color: "white" }
MultiEffect { source: img; anchors.fill: img; maskEnabled: true; maskSource: mask }
```

The mask and source must both be layered (`layer.enabled: true`). The MultiEffect is
the visible one, source stays invisible.

### Qt 6.7+ per-corner radius
`Rectangle` has `topLeftRadius`/`topRightRadius`/`bottomLeftRadius`/`bottomRightRadius`
since 6.7. Useful when you want rounded top, square bottom (like cards above a footer).
**They only affect the Rectangle's own paint** — not children clipping.

### Property change signals collide with explicit signals
`property string searchText` auto-generates `searchTextChanged()`. If you also declare
`signal searchTextChanged(string text)` → compile error "Duplicate signal name".
Fix: rename one (e.g. `signal searchEdited(string)`).

### Q_PROPERTY function shadowing in C++
If you already have `int selectedPeers() const` and add
`QVariantList selectedPeers() const`, MOC fails because Q_PROPERTY getters cannot
overload by return type. Rename one (e.g. `selectedPeerList`).

### Layout sizing inside Layouts
Inside `RowLayout`/`ColumnLayout`/`GridLayout`, **never set bare `width`/`height`** on
direct children — use `Layout.preferredWidth`/`Layout.fillWidth`/etc. Direct width
is silently ignored.

`Layout.margins: N` on a Layout child sets margins between it and its parent. To add
**internal padding** inside a RowLayout, wrap it in an Item with `anchors.fill: parent`
+ `anchors.margins`.

### Repeater inside StackLayout
Repeater creates children as siblings in its parent (not under itself). So a StackLayout
with an explicit child + Repeater(4) has 5 children at indices 0-4. Works for tab content,
but explicit Items are clearer.

### TabBar spreads tabs across full width
Default `TabBar` distributes `TabButton`s evenly across its width. To get compact,
left-aligned tabs (like QTabWidget), replace TabBar with a `Row` of clickable
Rectangles with manual `TapHandler`s + underline accent.

### Menu styling (QtQuick.Controls.Basic)
Default `Menu` looks system-generic. To match theme:
- Override `background:` Rectangle (Theme.panel + border + radius)
- Override `delegate: MenuItem { contentItem: ...; background: ... }` — checkmark for
  checkable Actions can be drawn as a Label with "✓" colored by Theme.accent
- `modal: true` dims the screen behind. Set `modal: false` for context-menu UX (or
  `dim: false` if you need modal behavior without dimming).

### Animator vs NumberAnimation in `Behavior`
`ScaleAnimator` works in `Behavior on scale {}` but can be flaky when combined with
nested layers/effects. `NumberAnimation { easing.type: Easing.OutCubic }` is more
predictable in `Behavior` blocks. Reserve `Animator` types for `Transition` blocks
where they truly run on the render thread.

### Image inside layered (visible: false) parent
An `Image` inside a `Rectangle` with `visible: false; layer.enabled: true` may not
trigger its asynchronous load in some Qt versions. Workaround: place the Image at
sibling level, give IT `layer.enabled: true; visible: false`, and reference it from
the `MultiEffect.source` directly.

### GridView "responsive snap" perception
Computing `cellWidth = available / round(available / target)` causes cards to shrink
when crossing column thresholds (the user perceives "bigger window → smaller cards").
Fixed `cellWidth` + `floor(available / cellWidth)` columns + centering the leftover
margin is cleaner. (Netflix/Spotify pattern.)

### List/Grid view transition without StackLayout
StackLayout swaps instantly. For crossfade + scale animation between two views:
sibling Items both `anchors.fill: parent`, each with `Behavior on opacity` + `Behavior
on scale`, driven by a `bool posterMode` property. Use `visible: opacity > 0.01` to
avoid capturing input on the invisible one. Snappier easing (130ms `InOutQuad`) avoids
seeing both layers during transition.

### GridView remove + displaced race
With `remove: Transition` only, kept items snap-teleport to new positions while
others fade out — feels like "the new ones appear faster than the old ones disappear".
Add `removeDisplaced: Transition { SequentialAnimation { PauseAnimation { duration: 150 }; NumberAnimation { properties: "x,y"; duration: 180 } } }`
to delay the layout settle until after the remove animation starts. Plain `displaced`
without the pause causes the slide-across-grid problem (cards flying long distances).

### QSortFilterProxyModel from QML
Subclass it in C++, add `Q_INVOKABLE setFilterState(QString)` /
`setSearchText(QString)` / `mapToSource(int proxyRow)`. Override `filterAcceptsRow`
to read source roles via `sourceModel()->data(idx, RoleEnum)`. roleNames passes through
to QML automatically — `model.torrentName` works on the proxy. The grid clicks return
proxy row indices, so always `mapToSource` before forwarding to selection-by-source-index
session methods.

### CMake modules for QML features
QtQuick.Shapes needs `Qt6::QuickShapes` in `find_package` AND in `target_link_libraries`.
Reconfigure after adding to `find_package` (`cmake -S . -B build`) — incremental builds
don't re-detect the package.

### Brand state colors clash with graph differentiation
The brand is red, so `stateDownloading` and `stateSeeding` (used for chips on cards)
are both red shades. For SpeedGraph download/upload distinction, override locally
to graph-specific colors (e.g. download = `stateDownloading` red, upload = `#fbbf24` amber).
Keep state chip colors for cards as-is.

### Theme colors with rich text in Labels
`Label.color: Theme.text` colors the whole text. To color just one part (e.g. red ↓
arrow + white value), use `textFormat: Text.StyledText; text: "<font color='#dc2626'>↓</font> " + value`.
StyledText parses a small HTML subset.

### Tab order in DetailsPanel = tabs at top
The original `QTabWidget` puts tabs at TOP of the panel with content below — meta info
(poster + title) goes INSIDE the General tab content, NOT in a header above the
TabBar. Putting header above tabs creates the "tabs in the middle of the panel"
look the user flagged.

### Context menu shared between views
For the same context menu on PosterGrid AND TorrentTable: keep the Menu instantiated
inside one view, expose `function openContextMenu(x, y)` from it. Sibling view
forwards its `contextRequested(row, x, y)` signal, maps coordinates via
`mapToItem(commonAncestor, x, y)`, and calls the function.

---

## Sources

- [Qt Docs - QQmlApplicationEngine](https://doc.qt.io/qt-6/qqmlapplicationengine.html)
- [Qt Docs - qt_add_qml_module](https://doc.qt.io/qt-6/qt-add-qml-module.html)
- [Qt Docs - C++ Models for QML](https://doc.qt.io/qt-6/qtquick-modelviewsdata-cppmodels.html)
- [Qt Docs - Qt Quick Best Practices](https://doc.qt.io/qt-6/qtquick-bestpractices.html)
- [Qt Docs - Keyboard Focus](https://doc.qt.io/qt-6/qtquick-input-focus.html)
- [Qt Docs - Qt Quick Styles](https://doc.qt.io/qt-6/qtquickcontrols-styles.html)
- [Why ContextProperties are Bad](https://raymii.org/s/articles/Qt_QML_Integrate_Cpp_with_QML_and_why_ContextProperties_are_bad.html)
- [Qt Labs Platform](https://doc.qt.io/qt-6/qtlabsplatform-index.html)
- [Qt 6.5 QML Modules](https://www.qt.io/blog/whats-new-for-qml-modules-in-6.5)
