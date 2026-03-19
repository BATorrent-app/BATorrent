// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "searchdialog.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include "../torrent/sessionmanager.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>

SearchDialog::SearchDialog(SessionManager *session, const QString &savePath, QWidget *parent)
    : QDialog(parent), m_session(session), m_savePath(savePath)
{
    setWindowTitle(tr_("search_title"));
    setMinimumSize(700, 500);
    setStyleSheet(ThemeManager::instance().dialogStyleSheet());

    auto &tm = ThemeManager::instance();
    auto *layout = new QVBoxLayout(this);

    // Search bar
    auto *searchLayout = new QHBoxLayout;
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(tr_("search_placeholder"));
    m_searchEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; border: 1px solid %3;"
        "border-radius: 6px; padding: 8px 12px; font-size: 14px; }"
        "QLineEdit:focus { border-color: %4; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    searchLayout->addWidget(m_searchEdit);

    auto *searchBtn = new QPushButton(tr_("search_btn"));
    searchBtn->setFixedWidth(100);
    connect(searchBtn, &QPushButton::clicked, this, &SearchDialog::performSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SearchDialog::performSearch);
    searchLayout->addWidget(searchBtn);
    layout->addLayout(searchLayout);

    // Status
    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #888; padding: 4px;");
    layout->addWidget(m_statusLabel);

    // Catalog results table
    QString tableStyle = QString(
        "QTableWidget { background: %1; color: %2; border: 1px solid %3; gridline-color: %3; }"
        "QTableWidget::item { padding: 6px; }"
        "QTableWidget::item:selected { background: %4; }"
        "QHeaderView::section { background: %1; color: %2; border: 1px solid %3; padding: 6px; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor());

    m_catalogTable = new QTableWidget;
    m_catalogTable->setColumnCount(3);
    m_catalogTable->setHorizontalHeaderLabels({tr_("search_col_name"), tr_("search_col_type"), tr_("search_col_year")});
    m_catalogTable->horizontalHeader()->setStretchLastSection(true);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_catalogTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_catalogTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_catalogTable->verticalHeader()->hide();
    m_catalogTable->setStyleSheet(tableStyle);
    connect(m_catalogTable, &QTableWidget::cellDoubleClicked, this, &SearchDialog::onItemDoubleClicked);
    layout->addWidget(m_catalogTable);

    // Stream results table (hidden initially)
    m_streamTable = new QTableWidget;
    m_streamTable->setColumnCount(3);
    m_streamTable->setHorizontalHeaderLabels({tr_("search_col_quality"), tr_("search_col_size"), tr_("search_col_addon")});
    m_streamTable->horizontalHeader()->setStretchLastSection(true);
    m_streamTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_streamTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_streamTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_streamTable->verticalHeader()->hide();
    m_streamTable->setStyleSheet(tableStyle);
    connect(m_streamTable, &QTableWidget::cellDoubleClicked, this, &SearchDialog::onStreamDoubleClicked);
    m_streamTable->hide();
    layout->addWidget(m_streamTable);

    // Back button (for stream view)
    auto *btnLayout = new QHBoxLayout;
    auto *backBtn = new QPushButton(tr_("search_back"));
    backBtn->setFixedWidth(100);
    connect(backBtn, &QPushButton::clicked, this, &SearchDialog::switchToCatalog);
    btnLayout->addWidget(backBtn);
    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    // Connect addon signals
    connect(&AddonManager::instance(), &AddonManager::catalogResults,
            this, &SearchDialog::showCatalogResults);
    connect(&AddonManager::instance(), &AddonManager::catalogFinished,
            this, [this]() { m_statusLabel->setText(tr_("search_done").arg(m_currentItems.size())); });
    connect(&AddonManager::instance(), &AddonManager::streamResults,
            this, &SearchDialog::showStreamResults);
    connect(&AddonManager::instance(), &AddonManager::streamFinished,
            this, [this]() { m_statusLabel->setText(tr_("search_streams_done").arg(m_currentStreams.size())); });
}

void SearchDialog::performSearch()
{
    QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty()) return;

    m_catalogTable->setRowCount(0);
    m_currentItems.clear();
    switchToCatalog();
    m_statusLabel->setText(tr_("search_searching"));

    AddonManager::instance().searchCatalog(query);
}

void SearchDialog::showCatalogResults(const QList<CatalogItem> &items)
{
    m_currentItems = items;
    m_catalogTable->setRowCount(items.size());

    for (int i = 0; i < items.size(); ++i) {
        m_catalogTable->setItem(i, 0, new QTableWidgetItem(items[i].name));
        m_catalogTable->setItem(i, 1, new QTableWidgetItem(items[i].type));
        m_catalogTable->setItem(i, 2, new QTableWidgetItem(
            items[i].year > 0 ? QString::number(items[i].year) : ""));
    }
}

void SearchDialog::onItemDoubleClicked(int row, int)
{
    if (row < 0 || row >= m_currentItems.size()) return;

    const auto &item = m_currentItems[row];
    m_statusLabel->setText(tr_("search_loading_streams").arg(item.name));
    m_streamTable->setRowCount(0);
    m_currentStreams.clear();
    switchToStreams();

    AddonManager::instance().getStreams(item.type, item.id);
}

void SearchDialog::showStreamResults(const QList<StreamResult> &streams)
{
    m_currentStreams = streams;
    m_streamTable->setRowCount(streams.size());

    for (int i = 0; i < streams.size(); ++i) {
        m_streamTable->setItem(i, 0, new QTableWidgetItem(streams[i].title));
        m_streamTable->setItem(i, 1, new QTableWidgetItem(
            streams[i].size > 0 ? formatSize(streams[i].size) : ""));
        m_streamTable->setItem(i, 2, new QTableWidgetItem(streams[i].addonName));
    }
}

void SearchDialog::onStreamDoubleClicked(int row, int)
{
    if (row < 0 || row >= m_currentStreams.size()) return;

    const auto &stream = m_currentStreams[row];
    if (stream.magnet.startsWith("magnet:"))
        m_session->addMagnet(stream.magnet, m_savePath);

    m_statusLabel->setText(tr_("search_added").arg(stream.title));
}

void SearchDialog::switchToStreams()
{
    m_catalogTable->hide();
    m_streamTable->show();
}

void SearchDialog::switchToCatalog()
{
    m_streamTable->hide();
    m_catalogTable->show();
}
