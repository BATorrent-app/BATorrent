// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

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
    QString formLabelStyle() const;

    QString accentColor() const;
    QString accentDarkColor() const;
    QString accentLightColor() const;
    QString accentSurfaceColor() const;
    QString accentTintColor() const;
    QString accentTintStrongColor() const;

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
    QString statePausedColor() const;
    QString stateErrorColor() const;

    QString successColor() const;
    QString warningColor() const;
    QString errorColor() const;

    static QStringList themeNames();

private:
    ThemeManager() = default;
    Theme m_theme = Dark;
};

#endif
