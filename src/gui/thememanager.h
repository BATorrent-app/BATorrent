// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QPixmap>
#include <QString>

class ThemeManager
{
public:
    enum Theme { Dark, Light, Midnight };

    static ThemeManager &instance();

    void setTheme(Theme theme);
    Theme theme() const { return m_theme; }
    QString styleSheet() const;
    QString dialogStyleSheet() const;
    // App-wide QSS for popups Qt creates on-the-fly (QMessageBox, QInputDialog,
    // QToolTip). Has to be re-applied via qApp->setStyleSheet on each theme
    // change so QMessageBox::about() etc. don't keep startup-time colors.
    QString appPopupStyleSheet() const;
    QString formLabelStyle() const;

    QString accentColor() const;
    QString accentDarkColor() const;
    QString accentLightColor() const;
    QString accentSurfaceColor() const;
    QString accentTintColor() const;
    QString accentTintStrongColor() const;
    // Opaque rgb produced by blending the accent red into bgColor() at
    // `alphaPercent` %. Used for dialog headers that need a soft red tint at
    // the top — qradialgradient with a translucent stop interpolates alpha
    // separately and shows the window-manager background through the
    // gradient, which on macOS reads as a black streak in light mode.
    QString accentTintForGradient(int alphaPercent) const;

    QString bgColor() const;
    QString surfaceColor() const;
    QString surfaceAltColor() const;
    QString panelColor() const;

    QString textColor() const;
    QString mutedColor() const;
    QString dimColor() const;

    QString borderColor() const;
    QString borderStrongColor() const;
    QString hairlineColor() const;

    QString stateDownloadingColor() const;
    QString stateSeedingColor() const;
    QString stateFinishedColor() const;
    QString stateCompletedColor() const;
    QString statePausedColor() const;
    QString stateErrorColor() const;

    QString successColor() const;
    QString warningColor() const;
    QString errorColor() const;

    // Logo tinted to stay legible against the current theme's bg/surface.
    // The source PNG is pure-white-on-transparent, which disappears on
    // light-mode surfaces; in light theme this re-paints the alpha mask in
    // textColor() instead. `dpr` controls the device-pixel ratio for retina
    // rendering (use the widget's devicePixelRatioF()).
    QPixmap themedLogo(int size, qreal dpr = 1.0) const;

    static QStringList themeNames();

private:
    ThemeManager() = default;
    Theme m_theme = Dark;
};

#endif
