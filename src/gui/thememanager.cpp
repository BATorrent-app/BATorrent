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
    case Light: return "#b91c1c";
    case Midnight: return "#f43f5e";
    default: return "#dc2626";
    }
}

QString ThemeManager::bgColor() const
{
    switch (m_theme) {
    case Light: return "#f5f5f7";
    case Midnight: return "#08080e";
    default: return "#111115";
    }
}

QString ThemeManager::surfaceColor() const
{
    switch (m_theme) {
    case Light: return "#ffffff";
    case Midnight: return "#0e0e18";
    default: return "#1c1c22";
    }
}

QString ThemeManager::textColor() const
{
    switch (m_theme) {
    case Light: return "#1a1a1a";
    case Midnight: return "#d8d8e0";
    default: return "#e0e0e6";
    }
}

QString ThemeManager::mutedColor() const
{
    switch (m_theme) {
    case Light: return "#6b7280";
    case Midnight: return "#6b6b80";
    default: return "#78788a";
    }
}

QString ThemeManager::borderColor() const
{
    switch (m_theme) {
    case Light: return "#d1d5db";
    case Midnight: return "#1a1a2a";
    default: return "#2a2a32";
    }
}

QString ThemeManager::accentDarkColor() const
{
    switch (m_theme) {
    case Light: return "#991b1b";
    case Midnight: return "#be123c";
    default: return "#2a1015";
    }
}

QString ThemeManager::accentLightColor() const
{
    switch (m_theme) {
    case Light: return "#dc2626";
    case Midnight: return "#fb7185";
    default: return "#ef4444";
    }
}

QString ThemeManager::accentSurfaceColor() const
{
    switch (m_theme) {
    case Light: return "#fef2f2";
    case Midnight: return "#1a0a10";
    default: return "#1f1012";
    }
}

QString ThemeManager::successColor() const
{
    switch (m_theme) {
    case Light: return "#16a34a";
    default: return "#22c55e";
    }
}

QString ThemeManager::warningColor() const
{
    switch (m_theme) {
    case Light: return "#ca8a04";
    default: return "#eab308";
    }
}

QString ThemeManager::errorColor() const
{
    switch (m_theme) {
    case Light: return "#dc2626";
    default: return "#ef4444";
    }
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
        QTabWidget::pane {
            background-color: @bg; border: 1px solid @bd;
            border-top: none;
        }
        QTabBar::tab {
            background-color: transparent; color: @mt;
            padding: 10px 22px; border: none;
            border-bottom: 2px solid transparent;
            font-weight: 600; font-size: 12px;
        }
        QTabBar::tab:selected {
            color: @tx; border-bottom: 2px solid @ac;
            background-color: @sf;
        }
        QTabBar::tab:hover { color: @tx; background-color: @acSf; }
        QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox, QTextEdit {
            background-color: @sf; color: @tx;
            border: 1px solid @bd; border-radius: 6px;
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
            background-color: @sf; color: @tx;
            border: 1px solid @bd; border-radius: 6px;
            padding: 8px 18px; font-weight: 600; font-size: 13px;
        }
        QPushButton:hover { background-color: @acSf; border-color: @ac; color: #ffffff; }
        QPushButton[default="true"], QPushButton:default {
            background-color: @ac; color: #ffffff; border-color: @ac;
        }
        QPushButton[default="true"]:hover, QPushButton:default:hover {
            background-color: @acLt; border-color: @acLt;
        }
        QCheckBox { color: @tx; spacing: 8px; font-size: 13px; }
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
        QLabel { color: @tx; }
        QGroupBox {
            color: @tx; font-weight: 600;
            border: 1px solid @bd; border-radius: 6px;
            margin-top: 8px; padding-top: 16px;
        }
        QProgressBar {
            background-color: @bd; border: 1px solid @bd;
            border-radius: 5px; text-align: center;
            color: @tx; font-weight: 600; font-size: 11px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 @acDk, stop:1 @ac);
            border-radius: 4px;
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

        /* ---- Menu bar ---- */
        QMenuBar {
            background-color: @sf; color: @tx;
            border-bottom: 1px solid @bd; font-weight: 600;
        }
        QMenuBar::item:selected { background-color: @acSf; color: @tx; }
        QMenu {
            background-color: @sf; color: @tx; border: 1px solid @bd;
        }
        QMenu::item:selected { background-color: @acSf; }
        QMenu::separator { background-color: @bd; height: 1px; margin: 4px 8px; }

        /* ---- Toolbar ---- */
        QToolBar {
            background-color: @sf; border-bottom: 1px solid @bd;
            spacing: 8px; padding: 8px 12px;
        }
        QToolButton {
            color: @tx; background-color: transparent;
            border: 1px solid transparent; border-radius: 6px;
            padding: 6px 14px 6px 10px; font-weight: 600; font-size: 12px;
        }
        QToolButton:hover {
            background-color: @acSf; color: #ffffff; border-color: @bd;
        }
        QToolButton:pressed { background-color: @acDk; color: #ffffff; }

        /* ---- Table ---- */
        QTableView {
            background-color: @bg; alternate-background-color: @sf;
            color: @tx; gridline-color: transparent; border: none;
            selection-background-color: rgba(220, 38, 38, 0.12);
            selection-color: @tx;
            font-size: 13px;
        }
        QTableView::item { padding: 4px 6px; min-height: 36px; }

        /* ---- Table headers ---- */
        QHeaderView::section {
            background-color: @sf; color: @tx;
            border: none; border-bottom: 1px solid @bd;
            padding: 8px 6px; font-weight: 600; font-size: 11px;
            text-transform: uppercase;
        }

        /* ---- Tab widget (details panel) ---- */
        QTabWidget::pane {
            background-color: @bg; border: 1px solid @bd;
            border-top: none;
        }
        QTabBar::tab {
            background-color: transparent; color: @mt;
            padding: 10px 22px; border: none;
            border-bottom: 2px solid transparent;
            font-weight: 600;
        }
        QTabBar::tab:selected {
            color: @tx; border-bottom: 2px solid @ac;
            background-color: @sf;
        }
        QTabBar::tab:hover { color: @tx; background-color: @acSf; }

        QLabel { color: @tx; }

        /* ---- Status bar ---- */
        QStatusBar {
            background-color: @sf; color: @mt;
            border-top: 1px solid @bd; font-size: 11px;
        }

        /* ---- Splitter ---- */
        QSplitter::handle { background-color: @bd; height: 1px; }

        /* ---- Progress bar ---- */
        QProgressBar {
            background-color: @bd; border: 1px solid @bd;
            border-radius: 5px; text-align: center;
            color: @tx; font-weight: 600; font-size: 11px;
        }
        QProgressBar::chunk {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 @acDk, stop:1 @acLt);
            border-radius: 4px;
        }

        /* ---- Scrollbars ---- */
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

        /* ---- Details panel inner tables ---- */
        QTableWidget {
            background-color: @bg; alternate-background-color: @sf;
            color: @tx; border: none;
            selection-background-color: rgba(220, 38, 38, 0.12);
            selection-color: @tx; font-size: 13px;
        }

        /* ---- Combo box in tables ---- */
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
