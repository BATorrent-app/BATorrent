// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "thememanager.h"
#include <QColor>
#include <QPainter>
#include <QStringList>

ThemeManager &ThemeManager::instance()
{
    static ThemeManager tm;
    return tm;
}

void ThemeManager::setTheme(Theme theme)
{
    m_theme = theme;
}

QStringList ThemeManager::themeNames()
{
    return {"Dark", "Light", "Midnight"};
}

QString ThemeManager::accentColor() const
{
    switch (m_theme) {
    case Light:    return "#dc2626";
    case Midnight: return "#dc2626";
    default:       return "#dc2626";
    }
}

QString ThemeManager::accentDarkColor() const
{
    switch (m_theme) {
    case Light:    return "#991b1b";
    case Midnight: return "#991b1b";
    default:       return "#991b1b";
    }
}

QString ThemeManager::accentLightColor() const
{
    switch (m_theme) {
    case Light:    return "#ef4444";
    case Midnight: return "#ef4444";
    default:       return "#ef4444";
    }
}

QString ThemeManager::accentSurfaceColor() const
{
    switch (m_theme) {
    case Light:    return "#f5e2dc";  // warm red tint on cream
    case Midnight: return "#1a0a10";
    default:       return "#1f1012";
    }
}

QString ThemeManager::bgColor() const
{
    switch (m_theme) {
    case Light:    return "#ece4d2";  // warm cream, matches JSX "Comfortable" palette
    case Midnight: return "#08070d";
    default:       return "#0e0a0a";
    }
}

QString ThemeManager::surfaceColor() const
{
    switch (m_theme) {
    case Light:    return "#f5eed9";  // light cream cards / inputs
    case Midnight: return "#12121c";
    default:       return "#15110f";  // was #1a1a20 (blue tint) — match warm dark
    }
}

QString ThemeManager::panelColor() const
{
    switch (m_theme) {
    case Light:    return "#f1e9d4";  // slightly above bg for details panel
    case Midnight: return "#181425";
    default:       return "#14100f";  // was #201a1a (too brown) — closer to bg
    }
}

QString ThemeManager::surfaceAltColor() const
{
    switch (m_theme) {
    case Light:    return "#e2d9bf";  // alt rows / progress track
    case Midnight: return "#0f0e18";
    default:       return "#100c0b";
    }
}

QString ThemeManager::textColor() const
{
    switch (m_theme) {
    case Light:    return "#1a1614";
    case Midnight: return "#eceafb";
    default:       return "#f0f0f0";
    }
}

QString ThemeManager::mutedColor() const
{
    switch (m_theme) {
    case Light:    return "#6d5f4d";  // warm muted, balances cream bg
    case Midnight: return "#9a95c8";
    default:       return "#b0b0b8";
    }
}

QString ThemeManager::dimColor() const
{
    switch (m_theme) {
    case Light:    return "#94897c";
    case Midnight: return "#605c82";
    default:       return "#71717a";
    }
}

QString ThemeManager::borderColor() const
{
    switch (m_theme) {
    case Light:    return "#d9cfb8";  // warm border tone
    case Midnight: return "#22203a";
    default:       return "#2a2018";  // less blue, warm dark
    }
}

QString ThemeManager::borderStrongColor() const
{
    switch (m_theme) {
    case Light:    return "#c2b59b";
    case Midnight: return "#322e50";
    default:       return "#3a2e22";
    }
}

QString ThemeManager::hairlineColor() const
{
    return m_theme == Light ? "rgba(0,0,0,0.06)" : "rgba(255,255,255,0.05)";
}

QString ThemeManager::accentTintColor() const
{
    switch (m_theme) {
    case Light:    return "rgba(220,38,38,0.08)";
    case Midnight: return "rgba(220,38,38,0.10)";
    default:       return "rgba(220,38,38,0.10)";
    }
}

QString ThemeManager::accentTintStrongColor() const
{
    switch (m_theme) {
    case Light:    return "rgba(220,38,38,0.14)";
    case Midnight: return "rgba(220,38,38,0.18)";
    default:       return "rgba(220,38,38,0.18)";
    }
}

QPixmap ThemeManager::themedLogo(int size, qreal dpr) const
{
    if (dpr <= 0) dpr = 1.0;
    QPixmap raw(":/images/logo1.png");
    QPixmap scaled = raw.scaled(int(size * dpr), int(size * dpr),
                                Qt::KeepAspectRatio, Qt::SmoothTransformation);
    scaled.setDevicePixelRatio(dpr);
    if (m_theme != Light)
        return scaled;
    // Light theme: re-paint the white alpha mask with textColor so the logo
    // stays visible on the off-white bg.
    QPixmap tinted(scaled.size());
    tinted.setDevicePixelRatio(dpr);
    tinted.fill(Qt::transparent);
    QPainter p(&tinted);
    p.drawPixmap(0, 0, scaled);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(tinted.rect(), QColor(textColor()));
    p.end();
    return tinted;
}

QString ThemeManager::accentTintForGradient(int alphaPercent) const
{
    const QColor base(bgColor());
    const QColor accent(220, 38, 38);
    const float a = qBound(0, alphaPercent, 100) / 100.0f;
    const int r = qRound(accent.red()   * a + base.red()   * (1.0f - a));
    const int g = qRound(accent.green() * a + base.green() * (1.0f - a));
    const int b = qRound(accent.blue()  * a + base.blue()  * (1.0f - a));
    return QColor(r, g, b).name();
}

QString ThemeManager::stateDownloadingColor() const
{
    return accentColor(); // red
}

QString ThemeManager::stateSeedingColor() const
{
    switch (m_theme) {
    case Light:    return "#b45309";
    case Midnight: return "#f59e0b";
    default:       return "#f59e0b";
    }
}

QString ThemeManager::stateFinishedColor() const
{
    switch (m_theme) {
    case Light:    return "#94897c";
    case Midnight: return "#9a95c8";
    default:       return "#b0b0b8";
    }
}

QString ThemeManager::stateCompletedColor() const
{
    // Greens are otherwise absent from the palette by design, but the user
    // asked for the universal "download done" cue here — kept slightly
    // desaturated so it sits next to the red/amber accents without fighting.
    switch (m_theme) {
    case Light:    return "#16a34a";
    case Midnight: return "#22c55e";
    default:       return "#22c55e";
    }
}

QString ThemeManager::statePausedColor() const
{
    switch (m_theme) {
    case Light:    return "#cfcac0";
    case Midnight: return "#2d2a42";
    default:       return "#3a3035";
    }
}

QString ThemeManager::stateErrorColor() const
{
    switch (m_theme) {
    case Light:    return "#7f1d1d";
    default:       return "#991b1b";
    }
}

QString ThemeManager::successColor() const
{
    // Seeding-equivalent (amber) — used for success states in dialogs.
    return stateSeedingColor();
}

QString ThemeManager::warningColor() const
{
    return accentColor();
}

QString ThemeManager::errorColor() const
{
    return stateErrorColor();
}

QString ThemeManager::formLabelStyle() const
{
    return QString("color: %1; font-weight: 600;").arg(textColor());
}

static QString applyColors(QString css,
                           const QString &bg, const QString &sf,
                           const QString &tx, const QString &mt,
                           const QString &ac, const QString &acDk,
                           const QString &acLt, const QString &acSf,
                           const QString &bd)
{
    css.replace("@acDk", acDk);
    css.replace("@acLt", acLt);
    css.replace("@acSf", acSf);
    css.replace("@ac", ac);
    css.replace("@bg", bg);
    css.replace("@sf", sf);
    css.replace("@tx", tx);
    css.replace("@mt", mt);
    css.replace("@bd", bd);
    return css;
}

QString ThemeManager::dialogStyleSheet() const
{
    QString css = R"(
        QDialog { background-color: @bg; color: @tx; }
        QTabWidget::pane { background-color: @bg; border: none; }
        QTabBar::tab {
            background-color: transparent; color: @mt;
            padding: 10px 22px; border: none;
            border-bottom: 2px solid transparent;
            font-weight: 600; font-size: 12px;
        }
        QTabBar::tab:selected { color: @tx; border-bottom: 2px solid @ac; }
        QTabBar::tab:hover { color: @tx; }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QTextEdit {
            background-color: @sf; color: @tx;
            border: 1px solid @bd; border-radius: 4px;
            padding: 7px 10px; font-size: 13px;
        }
        QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus,
        QComboBox:focus, QTextEdit:focus { border-color: @ac; }
        QComboBox::drop-down { border: none; background-color: @sf; padding-right: 8px; }
        QComboBox QAbstractItemView {
            background-color: @sf; color: @tx;
            selection-background-color: @acSf; border: 1px solid @bd;
        }
        QPushButton {
            background-color: transparent; color: @tx;
            border: 1px solid @bd; border-radius: 4px;
            padding: 8px 18px; font-weight: 500; font-size: 12px;
        }
        QPushButton:hover { background-color: @sf; }
        QPushButton[default="true"], QPushButton:default {
            background-color: @ac; color: #ffffff; border-color: @ac;
        }
        QPushButton[default="true"]:hover, QPushButton:default:hover {
            background-color: @acLt; border-color: @acLt;
        }
        QLabel { color: @tx; }
        QGroupBox {
            color: @tx; font-weight: 600;
            border: none; margin-top: 16px; padding-top: 8px;
        }
        QRadioButton, QCheckBox { color: @tx; spacing: 8px; font-size: 13px; }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border: 1px solid @bd; border-radius: 4px;
            background-color: @sf;
        }
        QCheckBox::indicator:checked {
            background-color: @ac; border-color: @ac;
        }
        QScrollArea { background: transparent; border: none; }
        QScrollArea > QWidget { background: transparent; }
        QProgressBar {
            background-color: @sf; border: none;
            border-radius: 3px; text-align: center;
            color: @tx; font-size: 11px; height: 6px;
        }
        QProgressBar::chunk {
            background-color: @ac; border-radius: 3px;
        }
    )";
    return applyColors(css, bgColor(), surfaceColor(), textColor(),
                       mutedColor(), accentColor(), accentDarkColor(),
                       accentLightColor(), accentSurfaceColor(), borderColor());
}

QString ThemeManager::appPopupStyleSheet() const
{
    return QString(
        "QMessageBox, QInputDialog {"
        "  background: %1; color: %2;"
        "}"
        "QMessageBox QLabel, QInputDialog QLabel {"
        "  background: transparent; color: %2; font-size: 11px;"
        "}"
        "QMessageBox QPushButton, QInputDialog QPushButton {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 6px 18px; font-size: 11px; font-weight: 500;"
        "  min-width: 80px;"
        "}"
        "QMessageBox QPushButton:hover, QInputDialog QPushButton:hover {"
        "  background: %5; border-color: %4;"
        "}"
        "QMessageBox QPushButton:default, QInputDialog QPushButton:default {"
        "  background: %6; color: #ffffff; border-color: %6;"
        "}"
        "QMessageBox QPushButton:default:hover, QInputDialog QPushButton:default:hover {"
        "  background: %7; border-color: %7;"
        "}"
        "QInputDialog QLineEdit, QInputDialog QComboBox, QInputDialog QSpinBox {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 6px 10px; font-size: 11px;"
        "  selection-background-color: %6;"
        "}"
        "QInputDialog QLineEdit:focus { border-color: %6; }"
        "QToolTip {"
        "  background: %5; color: %2;"
        "  border: 1px solid %4; padding: 4px 8px;"
        "  font-size: 11px;"
        "}"
        ).arg(panelColor(), textColor(), surfaceColor(),
              borderColor(), surfaceAltColor(), accentColor(),
              accentLightColor());
}

QString ThemeManager::styleSheet() const
{
    QString css = R"(
        QMainWindow { background-color: @bg; }

        QMenuBar {
            background-color: @bg; color: @tx;
            border: none; font-weight: 500;
        }
        QMenuBar::item:selected { background-color: @sf; color: @tx; }
        QMenu {
            background-color: @sf; color: @tx; border: 1px solid @bd;
        }
        QMenu::item:selected { background-color: @acSf; }
        QMenu::separator { background-color: @bd; height: 1px; margin: 4px 8px; }

        QToolBar {
            background-color: @bg; border: none;
            spacing: 4px; padding: 6px 10px;
        }
        QToolButton {
            color: @mt; background-color: transparent;
            border: none; border-radius: 6px;
            padding: 6px 12px; font-weight: 500; font-size: 11px;
        }
        QToolButton:hover { background-color: @sf; color: @tx; }
        QToolButton:pressed { background-color: @acSf; color: #ffffff; }

        QTableView {
            background-color: @bg;
            alternate-background-color: @bg;
            color: @tx; gridline-color: transparent; border: none;
            selection-background-color: rgba(220, 38, 38, 0.12);
            selection-color: @tx;
            font-size: 12px;
        }
        QTableView::item { padding: 6px 8px; min-height: 36px; }

        QHeaderView::section {
            background-color: @bg; color: @mt;
            border: none; border-bottom: 1px solid @bd;
            padding: 10px 8px; font-weight: 900; font-size: 9px;
        }

        QTabWidget::pane { background-color: @bg; border: none; }
        QTabBar::tab {
            background-color: transparent; color: @mt;
            padding: 10px 22px; border: none;
            border-bottom: 2px solid transparent;
            font-weight: 600;
        }
        QTabBar::tab:selected { color: @tx; border-bottom: 2px solid @ac; }
        QTabBar::tab:hover { color: @tx; }

        QLabel { color: @tx; }

        QStatusBar {
            background-color: @bg; color: @mt;
            border: none; font-size: 11px;
        }

        QSplitter::handle:vertical {
            background-color: @bd;
            height: 1px;
            margin: 3px 0;
        }
        QSplitter::handle:vertical:hover { background-color: @ac; }
        QSplitter::handle:horizontal {
            background-color: @bd;
            width: 1px;
            margin: 0 3px;
        }
        QSplitter::handle:horizontal:hover { background-color: @ac; }

        QProgressBar {
            background-color: @sf; border: none;
            border-radius: 3px; text-align: center;
            color: @tx; font-size: 11px;
        }
        QProgressBar::chunk { background-color: @ac; border-radius: 3px; }

        QScrollBar:vertical {
            background-color: @bg; width: 8px; border: none;
        }
        QScrollBar::handle:vertical {
            background-color: @bd; border-radius: 4px; min-height: 20px;
        }
        QScrollBar::handle:vertical:hover { background-color: @mt; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal {
            background-color: @bg; height: 8px; border: none;
        }
        QScrollBar::handle:horizontal {
            background-color: @bd; border-radius: 4px; min-width: 20px;
        }
        QScrollBar::handle:horizontal:hover { background-color: @mt; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }

        QTableWidget {
            background-color: @bg; color: @tx; border: none;
            selection-background-color: rgba(220, 38, 38, 0.12);
            selection-color: @tx; font-size: 12px;
        }

        QComboBox {
            background-color: @sf; color: @tx;
            border: 1px solid @bd; border-radius: 4px;
            padding: 3px 6px; font-size: 12px;
        }
        QComboBox::drop-down { border: none; background-color: @sf; }
        QComboBox QAbstractItemView {
            background-color: @sf; color: @tx;
            selection-background-color: @acSf; border: 1px solid @bd;
        }
    )";
    return applyColors(css, bgColor(), surfaceColor(), textColor(),
                       mutedColor(), accentColor(), accentDarkColor(),
                       accentLightColor(), accentSurfaceColor(), borderColor());
}
