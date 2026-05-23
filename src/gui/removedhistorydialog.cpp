// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "removedhistorydialog.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include "../torrent/sessionmanager.h"
#include "thememanager.h"

#include <QDateTime>
#include <QLocale>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

RemovedHistoryDialog::RemovedHistoryDialog(SessionManager *session, QWidget *parent)
    : QDialog(parent), m_session(session)
{
    setWindowTitle(tr_("removed_history_title"));
    setMinimumSize(620, 480);

    const auto &tm = ThemeManager::instance();
    setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
        "QLabel { background: transparent; color: %2; }"
        "QListWidget { background: %3; color: %2; border: 1px solid %4;"
        "  border-radius: 6px; padding: 4px; font-size: 12px; }"
        "QListWidget::item { padding: 8px; }"
        "QListWidget::item:selected { background: %5; color: %2; }"
        "QPushButton { background: transparent; color: %2; border: 1px solid %4;"
        "  border-radius: 6px; padding: 7px 16px; font-size: 11px; font-weight: 500; }"
        "QPushButton:hover { background: %3; }"
        "#primaryBtn { background: %6; color: #ffffff; border-color: %6; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.surfaceColor(),
              tm.borderColor(), tm.accentTintColor(), tm.accentColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    auto *title = new QLabel(tr_("removed_history_title"));
    {
        QFont f; f.setPointSize(15); f.setWeight(QFont::Bold);
        title->setFont(f);
    }
    root->addWidget(title);

    auto *desc = new QLabel(tr_("removed_history_desc"));
    desc->setWordWrap(true);
    desc->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.mutedColor()));
    root->addWidget(desc);

    m_list = new QListWidget;
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(m_list, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) {
        restoreSelected();
    });
    root->addWidget(m_list, 1);

    auto *btnRow = new QHBoxLayout;
    auto *clearBtn = new QPushButton(tr_("removed_history_clear"));
    connect(clearBtn, &QPushButton::clicked, this, &RemovedHistoryDialog::clearAll);
    btnRow->addWidget(clearBtn);
    btnRow->addStretch();
    auto *restoreBtn = new QPushButton(tr_("removed_history_restore"));
    restoreBtn->setObjectName(QStringLiteral("primaryBtn"));
    connect(restoreBtn, &QPushButton::clicked, this, &RemovedHistoryDialog::restoreSelected);
    btnRow->addWidget(restoreBtn);
    auto *closeBtn = new QPushButton(tr_("welcome_close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);

    refresh();
}

void RemovedHistoryDialog::refresh()
{
    m_list->clear();
    const auto entries = m_session->recentlyRemoved();
    for (const auto &e : entries) {
        const QString when = QLocale::system().toString(
            QDateTime::fromSecsSinceEpoch(e.removedAt), QLocale::ShortFormat);
        const QString label = QStringLiteral("%1\n    %2 · %3")
            .arg(e.name, formatSize(e.totalSize), when);
        auto *item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, e.hash);
        m_list->addItem(item);
    }
    if (entries.isEmpty()) {
        auto *empty = new QListWidgetItem(tr_("removed_history_empty"));
        empty->setFlags(Qt::NoItemFlags);
        m_list->addItem(empty);
    }
}

void RemovedHistoryDialog::restoreSelected()
{
    auto *item = m_list->currentItem();
    if (!item) return;
    const QString hash = item->data(Qt::UserRole).toString();
    if (hash.isEmpty()) return;
    if (m_session->restoreRemoved(hash))
        refresh();
    else
        QMessageBox::warning(this, tr_("removed_history_title"),
                             tr_("removed_history_failed"));
}

void RemovedHistoryDialog::clearAll()
{
    auto reply = QMessageBox::question(this, tr_("removed_history_clear"),
        tr_("removed_history_clear_confirm"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;
    m_session->clearRemovedHistory();
    refresh();
}
