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
    QString bgColor() const;
    QString surfaceColor() const;
    QString textColor() const;
    QString mutedColor() const;
    QString borderColor() const;
    QString accentDarkColor() const;
    QString accentLightColor() const;
    QString accentSurfaceColor() const;
    QString successColor() const;
    QString warningColor() const;
    QString errorColor() const;

    static QStringList themeNames();

private:
    ThemeManager() = default;
    Theme m_theme = Dark;
};

#endif
