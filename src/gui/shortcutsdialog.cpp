// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "shortcutsdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QHeaderView>
#include <QLabel>
#include <QFont>
#include <QAbstractItemView>

ShortcutsDialog::ShortcutsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("shortcuts_title"));
    setFixedSize(420, 380);

    const auto &tm = ThemeManager::instance();
    QString bg = tm.bgColor();
    QString sf = tm.surfaceColor();
    QString tx = tm.textColor();
    QString mt = tm.mutedColor();
    QString ac = tm.accentColor();
    QString bd = tm.borderColor();

    setStyleSheet(QString(R"(
        QDialog { background-color: %1; color: %2; }
        QLabel { color: %2; }
        QTableWidget {
            background-color: %3; color: %2;
            border: 1px solid %4; border-radius: 6px;
            gridline-color: %4;
        }
        QTableWidget::item { padding: 6px 12px; }
        QHeaderView::section {
            background-color: %1; color: %5;
            border: none; border-bottom: 1px solid %4;
            padding: 6px 12px; font-weight: 600;
        }
    )").arg(bg, tx, sf, bd, ac));

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(12);

    // Title
    auto *titleLabel = new QLabel(tr_("shortcuts_title"));
    titleLabel->setStyleSheet(QString("font-size: 18px; font-weight: 700; color: %1;").arg(tx));
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Table
    struct ShortcutEntry { QString key; QString action; };
    QList<ShortcutEntry> shortcuts = {
        {"Space",    tr_("action_pause") + " / " + tr_("action_resume")},
        {"Ctrl+O",   tr_("action_open")},
        {"Ctrl+M",   tr_("action_magnet")},
        {"Ctrl+A",   tr_("filter_all_active")},
        {"Delete",   tr_("action_remove")},
        {"Ctrl+,",   tr_("action_settings")},
        {"F5",       tr_("settings_refresh")},
        {"Escape",   tr_("btn_cancel")},
    };

    auto *table = new QTableWidget(shortcuts.size(), 2);
    table->setHorizontalHeaderLabels({tr_("shortcuts_key"), tr_("shortcuts_action")});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->verticalHeader()->hide();
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setFocusPolicy(Qt::NoFocus);

    for (int i = 0; i < shortcuts.size(); ++i) {
        auto *keyItem = new QTableWidgetItem(shortcuts[i].key);
        keyItem->setFont(QFont("monospace", 11, QFont::Bold));
        table->setItem(i, 0, keyItem);
        table->setItem(i, 1, new QTableWidgetItem(shortcuts[i].action));
    }

    mainLayout->addWidget(table);
}
