// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "thememanager.h"
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
    case Light:    return "#fef2f2";
    case Midnight: return "#1a0a10";
    default:       return "#1f1012";
    }
}

QString ThemeManager::bgColor() const
{
    switch (m_theme) {
    case Light:    return "#f7f6f4";
    case Midnight: return "#08070d";
    default:       return "#0e0a0a";
    }
}

QString ThemeManager::surfaceColor() const
{
    switch (m_theme) {
    case Light:    return "#ffffff";
    case Midnight: return "#12121c";
    default:       return "#1a1a20";
    }
}

QString ThemeManager::panelColor() const
{
    switch (m_theme) {
    case Light:    return "#fbfaf8";
    case Midnight: return "#181425";
    default:       return "#201a1a";
    }
}

QString ThemeManager::surfaceAltColor() const
{
    switch (m_theme) {
    case Light:    return "#f1efeb";
    case Midnight: return "#0f0e18";
    default:       return "#16161c";
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
    case Light:    return "#5e574f";
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
    case Light:    return "#e5e1da";
    case Midnight: return "#22203a";
    default:       return "#2a2a35";
    }
}

QString ThemeManager::borderStrongColor() const
{
    switch (m_theme) {
    case Light:    return "#cfcac0";
    case Midnight: return "#322e50";
    default:       return "#3a3a48";
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
