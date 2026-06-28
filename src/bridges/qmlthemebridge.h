// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLTHEMEBRIDGE_H
#define QMLTHEMEBRIDGE_H

#include "bridges/bridgecommon.h"

class QmlThemeBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString themeName READ themeName WRITE setThemeName NOTIFY changed)
    Q_PROPERTY(bool followSystem READ followSystem WRITE setFollowSystem NOTIFY changed)
    Q_PROPERTY(bool anime READ anime WRITE setAnime NOTIFY changed)
    // Custom theme (name === "custom"): a list of user profiles, each a full
    // palette (6 colors + bg image + opacity). The active one drives Theme.qml.
    Q_PROPERTY(QVariantList customProfiles READ customProfiles NOTIFY profilesChanged)
    Q_PROPERTY(int activeProfile READ activeProfile WRITE setActiveProfile NOTIFY changed)
    // Active-profile colors, read by Theme.qml (no per-token signal needed —
    // changed() re-evaluates all of them).
    Q_PROPERTY(QString cBg        READ cBg        NOTIFY changed)
    Q_PROPERTY(QString cPanel     READ cPanel     NOTIFY changed)
    Q_PROPERTY(QString cText      READ cText      NOTIFY changed)
    Q_PROPERTY(QString cPrimary   READ cPrimary   NOTIFY changed)
    Q_PROPERTY(QString cSecondary READ cSecondary NOTIFY changed)
    Q_PROPERTY(QString cTertiary  READ cTertiary  NOTIFY changed)
    Q_PROPERTY(QString cBgImage   READ cBgImage   NOTIFY changed)
    Q_PROPERTY(int     cBgOpacity READ cBgOpacity NOTIFY changed)
    // True when the OS taskbar/tray is in light mode (for logo contrast).
    Q_PROPERTY(bool osLight READ osLight NOTIFY osSchemeChanged)
    Q_PROPERTY(QString appVersion READ appVersion CONSTANT)
    // App-icon picker — independent of the UI theme (issue #15). Swaps the live
    // Dock (macOS) / taskbar+window (Windows) icon; does NOT touch the signed
    // bundle's Finder/.exe icon.
    Q_PROPERTY(QString appIconChoice READ appIconChoice WRITE setAppIcon NOTIFY appIconChanged)

public:
    explicit QmlThemeBridge(QObject *parent = nullptr);

    QString themeName() const;
    void setThemeName(const QString &n);
    bool followSystem() const;
    void setFollowSystem(bool on);
    bool anime() const;
    void setAnime(bool on);

    // ---- custom profiles ----
    QVariantList customProfiles() const;       // list of {name,bg,panel,text,primary,secondary,tertiary,image,opacity}
    int activeProfile() const;
    void setActiveProfile(int i);
    Q_INVOKABLE int addProfile();              // appends a default profile, returns its index
    Q_INVOKABLE void removeProfile(int i);     // keeps at least one profile
    Q_INVOKABLE void renameProfile(int i, const QString &name);
    Q_INVOKABLE void setProfileColor(int i, const QString &role, const QString &hex);  // role: bg/panel/text/primary/secondary/tertiary
    Q_INVOKABLE void setProfileImage(int i, const QString &path);
    Q_INVOKABLE void setProfileOpacity(int i, int pct);

    // active-profile accessors (Theme.qml)
    QString cBg() const;
    QString cPanel() const;
    QString cText() const;
    QString cPrimary() const;
    QString cSecondary() const;
    QString cTertiary() const;
    QString cBgImage() const;
    int cBgOpacity() const;

    bool osLight() const;
    QString appVersion() const;
    Q_INVOKABLE QString releaseNotes() const;     // changelog HTML (shared with legacy dialog)
    Q_INVOKABLE QVariantList libraries() const;   // [{nm,v}] real linked-library versions
    Q_INVOKABLE void markBootHealthy() const;     // UI is up → clear the boot-crash sentinel
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
    QVariantMap activeMap() const;
    static QVariantMap defaultProfile(const QString &name);
    bool darkTitleBar() const;                     // true unless a light-ish theme
    static void applyTitleBar(QWindow *w, bool dark);
    void refreshTitleBars();                       // re-apply to all open windows

    QString m_themeName;
    bool m_followSystem = false;
    bool m_anime = true;
    QVariantList m_profiles;
    int m_activeProfile = 0;
    bool m_osLight = false;
    QString m_appIconChoice;
    void applyAppIcon(const QString &key);   // sets QGuiApplication window icon
};

#endif // QMLTHEMEBRIDGE_H
