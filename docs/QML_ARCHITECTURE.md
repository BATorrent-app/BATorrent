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
