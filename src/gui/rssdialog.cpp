// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "rssdialog.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QListWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>

RssDialog::RssDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("rss_title"));
    setMinimumSize(800, 550);
    setStyleSheet(ThemeManager::instance().dialogStyleSheet());

    auto &tm = ThemeManager::instance();
    auto *mainLayout = new QVBoxLayout(this);

    // Add feed bar
    auto *addLayout = new QHBoxLayout;
    m_urlEdit = new QLineEdit;
    m_urlEdit->setPlaceholderText(tr_("rss_url_hint"));
    m_urlEdit->setStyleSheet(QString(
        "QLineEdit { background: %1; color: %2; border: 1px solid %3;"
        "border-radius: 6px; padding: 8px 12px; font-size: 14px; }"
        "QLineEdit:focus { border-color: %4; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    addLayout->addWidget(m_urlEdit);

    auto *addBtn = new QPushButton(tr_("rss_add"));
    addBtn->setFixedWidth(100);
    connect(addBtn, &QPushButton::clicked, this, &RssDialog::addFeed);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &RssDialog::addFeed);
    addLayout->addWidget(addBtn);
    mainLayout->addLayout(addLayout);

    // Splitter: feed list (left) + items/settings (right)
    auto *splitter = new QSplitter(Qt::Horizontal);

    // Left: feed list
    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto *feedLabel = new QLabel(tr_("rss_feeds"));
    feedLabel->setStyleSheet(QString("color: %1; font-weight: bold; padding: 4px;").arg(tm.textColor()));
    leftLayout->addWidget(feedLabel);

    m_feedList = new QListWidget;
    m_feedList->setStyleSheet(QString(
        "QListWidget { background: %1; color: %2; border: 1px solid %3; border-radius: 6px; }"
        "QListWidget::item { padding: 8px; }"
        "QListWidget::item:selected { background: %4; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    connect(m_feedList, &QListWidget::currentRowChanged, this, &RssDialog::onFeedSelected);
    leftLayout->addWidget(m_feedList);

    auto *feedBtnLayout = new QHBoxLayout;
    auto *removeBtn = new QPushButton(tr_("rss_remove"));
    removeBtn->setFixedWidth(100);
    connect(removeBtn, &QPushButton::clicked, this, &RssDialog::removeFeed);
    feedBtnLayout->addWidget(removeBtn);

    auto *refreshBtn = new QPushButton(tr_("rss_refresh_all"));
    refreshBtn->setFixedWidth(100);
    connect(refreshBtn, &QPushButton::clicked, this, &RssDialog::refreshAllFeeds);
    feedBtnLayout->addWidget(refreshBtn);
    feedBtnLayout->addStretch();
    leftLayout->addLayout(feedBtnLayout);

    splitter->addWidget(leftWidget);

    // Right: items table + feed settings
    auto *rightWidget = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    // Feed settings group
    auto *settingsGroup = new QGroupBox(tr_("rss_feed_settings"));
    auto *settingsLayout = new QFormLayout(settingsGroup);

    QString inputStyle = QString(
        "QLineEdit, QSpinBox { background: %1; color: %2; border: 1px solid %3;"
        "border-radius: 4px; padding: 4px 8px; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor());

    m_enabledCheck = new QCheckBox(tr_("rss_enabled"));
    settingsLayout->addRow(m_enabledCheck);

    m_autoDownloadCheck = new QCheckBox(tr_("rss_auto_download"));
    settingsLayout->addRow(m_autoDownloadCheck);

    m_filterEdit = new QLineEdit;
    m_filterEdit->setPlaceholderText(tr_("rss_filter_hint"));
    m_filterEdit->setStyleSheet(inputStyle);
    settingsLayout->addRow(tr_("rss_filter"), m_filterEdit);

    m_savePathEdit = new QLineEdit;
    m_savePathEdit->setPlaceholderText(tr_("rss_save_path_hint"));
    m_savePathEdit->setStyleSheet(inputStyle);
    auto *pathLayout = new QHBoxLayout;
    pathLayout->addWidget(m_savePathEdit);
    auto *browseBtn = new QPushButton(tr_("settings_browse"));
    browseBtn->setFixedWidth(80);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr_("dlg_save_to"));
        if (!dir.isEmpty()) m_savePathEdit->setText(dir);
    });
    pathLayout->addWidget(browseBtn);
    settingsLayout->addRow(tr_("rss_save_path"), pathLayout);

    m_intervalSpin = new QSpinBox;
    m_intervalSpin->setRange(5, 1440);
    m_intervalSpin->setValue(30);
    m_intervalSpin->setSuffix(" min");
    m_intervalSpin->setStyleSheet(inputStyle);
    settingsLayout->addRow(tr_("rss_interval"), m_intervalSpin);

    m_lastCheckedLabel = new QLabel;
    m_lastCheckedLabel->setStyleSheet("color: #888;");
    settingsLayout->addRow(m_lastCheckedLabel);

    m_saveBtn = new QPushButton(tr_("rss_save_settings"));
    m_saveBtn->setFixedWidth(140);
    connect(m_saveBtn, &QPushButton::clicked, this, &RssDialog::saveCurrentFeedSettings);
    settingsLayout->addRow(m_saveBtn);

    rightLayout->addWidget(settingsGroup);

    // Items table
    auto *itemsLabel = new QLabel(tr_("rss_items"));
    itemsLabel->setStyleSheet(QString("color: %1; font-weight: bold; padding: 4px;").arg(tm.textColor()));
    rightLayout->addWidget(itemsLabel);

    m_itemsTable = new QTableWidget;
    m_itemsTable->setColumnCount(3);
    m_itemsTable->setHorizontalHeaderLabels({
        tr_("rss_col_title"), tr_("rss_col_size"), tr_("rss_col_date")
    });
    m_itemsTable->horizontalHeader()->setStretchLastSection(true);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_itemsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_itemsTable->verticalHeader()->hide();
    m_itemsTable->setStyleSheet(QString(
        "QTableWidget { background: %1; color: %2; border: 1px solid %3; gridline-color: %3; }"
        "QTableWidget::item { padding: 6px; }"
        "QTableWidget::item:selected { background: %4; }"
        "QHeaderView::section { background: %1; color: %2; border: 1px solid %3; padding: 6px; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    connect(m_itemsTable, &QTableWidget::cellDoubleClicked, this, &RssDialog::onItemDoubleClicked);
    rightLayout->addWidget(m_itemsTable);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter, 1);

    // Status bar
    m_statusLabel = new QLabel;
    m_statusLabel->setStyleSheet("color: #888; padding: 4px;");
    mainLayout->addWidget(m_statusLabel);

    // Bottom buttons
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *closeBtn = new QPushButton(tr_("btn_ok"));
    closeBtn->setFixedWidth(100);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnLayout->addWidget(closeBtn);
    mainLayout->addLayout(btnLayout);

    // Connect RSS manager signals
    connect(&RssManager::instance(), &RssManager::feedAdded, this, [this](const RssFeed &) {
        refreshFeedList();
    });
    connect(&RssManager::instance(), &RssManager::feedUpdated, this, &RssDialog::onFeedUpdated);
    connect(&RssManager::instance(), &RssManager::feedError, this, [this](const QString &err) {
        m_statusLabel->setText(err);
    });

    refreshFeedList();

    // Disable settings when no feed selected
    settingsGroup->setEnabled(false);
    connect(m_feedList, &QListWidget::currentRowChanged, settingsGroup, [settingsGroup](int row) {
        settingsGroup->setEnabled(row >= 0);
    });
}

void RssDialog::addFeed()
{
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) return;
    RssManager::instance().addFeed(url);
    m_urlEdit->clear();
    m_statusLabel->setText(tr_("rss_adding"));
}

void RssDialog::removeFeed()
{
    int row = m_feedList->currentRow();
    if (row < 0) return;
    RssManager::instance().removeFeed(row);
    m_selectedFeed = -1;
    refreshFeedList();
    m_itemsTable->setRowCount(0);
    m_statusLabel->setText(tr_("rss_removed"));
}

void RssDialog::onFeedSelected(int row)
{
    m_selectedFeed = row;
    if (row < 0) return;
    updateFeedSettingsUI(row);
    showFeedItems(row);
}

void RssDialog::updateFeedSettingsUI(int feedIndex)
{
    auto feeds = RssManager::instance().feeds();
    if (feedIndex < 0 || feedIndex >= feeds.size()) return;

    const auto &feed = feeds[feedIndex];
    m_enabledCheck->setChecked(feed.enabled);
    m_autoDownloadCheck->setChecked(feed.autoDownload);
    m_filterEdit->setText(feed.filterPattern);
    m_savePathEdit->setText(feed.savePath);
    m_intervalSpin->setValue(feed.checkIntervalMin);

    if (feed.lastChecked.isValid())
        m_lastCheckedLabel->setText(tr_("rss_last_checked").arg(
            feed.lastChecked.toString("yyyy-MM-dd hh:mm")));
    else
        m_lastCheckedLabel->setText(tr_("rss_never_checked"));
}

void RssDialog::saveCurrentFeedSettings()
{
    if (m_selectedFeed < 0) return;

    auto feeds = RssManager::instance().feeds();
    if (m_selectedFeed >= feeds.size()) return;

    RssFeed feed = feeds[m_selectedFeed];
    feed.enabled = m_enabledCheck->isChecked();
    feed.autoDownload = m_autoDownloadCheck->isChecked();
    feed.filterPattern = m_filterEdit->text();
    feed.savePath = m_savePathEdit->text();
    feed.checkIntervalMin = m_intervalSpin->value();

    RssManager::instance().updateFeed(m_selectedFeed, feed);
    m_statusLabel->setText(tr_("rss_settings_saved"));
    refreshFeedList();
}

void RssDialog::showFeedItems(int feedIndex)
{
    auto items = RssManager::instance().itemsForFeed(feedIndex);
    m_itemsTable->setRowCount(items.size());

    for (int i = 0; i < items.size(); ++i) {
        auto *titleItem = new QTableWidgetItem(items[i].title);
        if (items[i].downloaded)
            titleItem->setForeground(QColor("#888888"));
        m_itemsTable->setItem(i, 0, titleItem);

        m_itemsTable->setItem(i, 1, new QTableWidgetItem(
            items[i].size > 0 ? formatSize(items[i].size) : ""));

        m_itemsTable->setItem(i, 2, new QTableWidgetItem(
            items[i].pubDate.isValid() ? items[i].pubDate.toString("yyyy-MM-dd hh:mm") : ""));
    }

    m_statusLabel->setText(tr_("rss_items_count").arg(items.size()));
}

void RssDialog::onFeedUpdated(int index, const QList<RssItem> &)
{
    refreshFeedList();
    if (index == m_selectedFeed)
        showFeedItems(index);
}

void RssDialog::onItemDoubleClicked(int row, int)
{
    if (m_selectedFeed < 0) return;
    RssManager::instance().downloadItem(m_selectedFeed, row);
    showFeedItems(m_selectedFeed); // refresh to show downloaded state
    m_statusLabel->setText(tr_("rss_downloading"));
}

void RssDialog::refreshAllFeeds()
{
    RssManager::instance().checkAllFeeds();
    m_statusLabel->setText(tr_("rss_refreshing"));
}

void RssDialog::refreshFeedList()
{
    m_feedList->clear();
    auto feeds = RssManager::instance().feeds();
    for (const auto &f : feeds) {
        QString text = f.name;
        if (!f.enabled) text += QString(" [%1]").arg(tr_("rss_disabled"));
        if (f.autoDownload) text += QString(" [%1]").arg(tr_("rss_auto"));
        m_feedList->addItem(text);
    }
}
