// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLTHEMEBRIDGE_H
#define QMLTHEMEBRIDGE_H

#include "bridges/bridgecommon.h"

#include <QRect>

class QmlThemeBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY changed)
    Q_PROPERTY(bool followSystem READ followSystem WRITE setFollowSystem NOTIFY changed)
    // What sits behind the library: "clean" (nothing), "theme" (the art made
    // for the theme) or "user" (the user's own picture in the same corner).
    // Replaces the old anime toggle.
    Q_PROPERTY(QString backgroundMode READ backgroundMode WRITE setBackgroundMode NOTIFY changed)
    Q_PROPERTY(QString userImage READ userImage WRITE setUserImage NOTIFY changed)
    // Themes of the user's own (themeName === "user"): each a whole theme
    // (base, corners, title font, six colours, a window background), not a
    // tweak of a built-in one. The active one drives Theme.qml.
    Q_PROPERTY(QVariantList customProfiles READ customProfiles NOTIFY profilesChanged)
    Q_PROPERTY(int activeProfile READ activeProfile WRITE setActiveProfile NOTIFY changed)
    Q_PROPERTY(QVariantMap userTheme READ activeMap NOTIFY changed)
    // True when the OS taskbar/tray is in light mode (for logo contrast).
    Q_PROPERTY(bool osLight READ osLight NOTIFY osSchemeChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    // App-icon picker: independent of the UI theme (issue #15). Swaps the live
    // Dock (macOS) / taskbar+window (Windows) icon; does NOT touch the signed
    // bundle's Finder/.exe icon.
    Q_PROPERTY(QString appIconChoice READ appIconChoice WRITE setAppIcon NOTIFY appIconChanged)
    // True when the process is on the Software scene-graph backend (or forced
    // via settings): QML can skip MultiEffect / heavy shaders.
    Q_PROPERTY(bool softwareRenderer READ softwareRenderer NOTIFY changed)
    // Dev-only: with BAT_QML_DIR set, icons load from that tree's ../icons
    // instead of the compiled resource, so an SVG edit shows up like a QML one.
    // Empty in a release build, where the qrc is the only source.
    Q_PROPERTY(QString devIconDir READ devIconDir CONSTANT)
    Q_PROPERTY(int iconEpoch READ iconEpoch NOTIFY iconEpochChanged)

public:
    explicit QmlThemeBridge(QObject *parent = nullptr);

    bool softwareRenderer() const { return m_softwareRenderer; }
    QString devIconDir() const;
    static int iconEpoch();
    static void bumpIconEpoch();
    void setSoftwareRenderer(bool on);

    QString themeName() const;
    void setThemeName(const QString &n);
    bool followSystem() const;
    void setFollowSystem(bool on);
    QString backgroundMode() const;
    void setBackgroundMode(const QString &mode);
    QString userImage() const;
    void setUserImage(const QString &path);

    // ---- the user's own themes ----
    // Each: {name, base(dark|light), corners(square|soft|round), font,
    //        bg, panel, text, primary, secondary, tertiary,
    //        image, fit(fill|fit|stretch|center|tile), zoom, dim, blur}
    QVariantList customProfiles() const;
    int activeProfile() const;
    void setActiveProfile(int i);
    QVariantMap activeMap() const;
    Q_INVOKABLE int addProfile();              // a new theme, returns its index
    Q_INVOKABLE int duplicateProfile(int i);   // a copy right after it, returns its index
    Q_INVOKABLE void removeProfile(int i);     // back to Default when it was the one in use
    Q_INVOKABLE void selectProfile(int i);     // switch to that theme
    Q_INVOKABLE void setProfileValue(int i, const QString &key, const QVariant &value);
    Q_INVOKABLE void renameProfile(int i, const QString &name);
    Q_INVOKABLE void setProfileColor(int i, const QString &role, const QString &hex);
    Q_INVOKABLE void setProfileImage(int i, const QString &path);
    // Light or dark, for the native title bar and for "follow system".
    Q_INVOKABLE bool isLightTheme(const QString &name) const;
    // The theme's interface face, so text drawn by Qt itself (menus, dialogs,
    // anything without font.family) follows the theme too.
    Q_INVOKABLE void setAppFontFamily(const QString &family);

    bool osLight() const;
    QString appVersion() const;
    Q_INVOKABLE QString releaseNotes() const;     // changelog HTML (shared with legacy dialog)
    Q_INVOKABLE void markBootHealthy() const;     // UI is up → clear the boot-crash sentinel
    Q_INVOKABLE QPointF cursorPos() const;        // global cursor: anchors the tray popup at the click
    // Where a notice can sit: the usable area (taskbar excluded) of the monitor
    // holding the window in front, a game while one runs, else the primary.
    // QML's Screen.desktopAvailableWidth spans every monitor, not one: with two
    // of them a corner computed from it lands off all screens.
    Q_INVOKABLE QRect noticeArea() const;
    // A Direct3D game holds the screen in exclusive fullscreen: no window, ours
    // included, can draw over it, so a notice has to go through the system.
    Q_INVOKABLE bool exclusiveFullscreenInFront() const;
    QIcon trayIcon() const;   // logo recolored for the current OS scheme

    // ---- app-icon picker ----
    QString appIconChoice() const;
    Q_INVOKABLE void setAppIcon(const QString &key);   // persist + apply live
    Q_INVOKABLE QVariantList appIcons() const;         // [{key,label,preview}]
    void applySavedAppIcon();                          // startup: apply a saved non-default pick
    static QIcon iconForKey(const QString &key);       // multi-size icon from a bundled png

    // SVG-swap logo renderer (shared with the image provider). darkBody=true
    // swaps the off-white body to dark text for light backgrounds.
    static QPixmap renderLogo(bool darkBody, int size, qreal dpr = 1.0);

signals:
    void iconEpochChanged();
    void changed();
    void profilesChanged();
    void osSchemeChanged();
    void appIconChanged();

protected:
    // Catch each top-level window's Show to paint its native title bar dark/light
    // to match the app theme (Windows leaves it white otherwise).
    bool eventFilter(QObject *o, QEvent *e) override;

private:
    void loadProfiles();
    void saveProfiles();
    void migrateThemes();                          // one-off: names and settings of the older themes
    static QVariantMap defaultProfile(const QString &name);
    bool darkTitleBar() const;                     // true unless a light-ish theme
    static void applyTitleBar(QWindow *w, bool dark);
    void refreshTitleBars();                       // re-apply to all open windows

    QString m_themeName;
    bool m_followSystem = false;
    QString m_backgroundMode;
    QString m_userImage;
    QVariantList m_profiles;
    int m_activeProfile = 0;
    bool m_osLight = false;
    QString m_appIconChoice;
    bool m_softwareRenderer = false;
    void applyAppIcon(const QString &key);   // sets QGuiApplication window icon
};

#endif // QMLTHEMEBRIDGE_H
