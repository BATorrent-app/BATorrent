// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "rssdialog.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include "thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
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

namespace {

QLabel *makeEyebrow(const QString &text, QWidget *parent)
{
    const auto &tm = ThemeManager::instance();
    auto *lbl = new QLabel(text.toUpper(), parent);
    QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    lbl->setFont(f);
    lbl->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.dimColor()));
    return lbl;
}

QLabel *makeFieldLabel(const QString &text, QWidget *parent)
{
    const auto &tm = ThemeManager::instance();
    auto *lbl = new QLabel(text, parent);
    QFont f; f.setPointSize(10); f.setWeight(QFont::Medium);
    lbl->setFont(f);
    lbl->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.mutedColor()));
    return lbl;
}

} // namespace

RssDialog::RssDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("rss_title"));
    setMinimumSize(720, 540);
    resize(820, 620);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 rgba(220,38,38,0.10),"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; color: %2; }"
        "QLineEdit, QComboBox, QSpinBox {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 7px 10px; font-size: 11px;"
        "  selection-background-color: %5;"
        "}"
        "QLineEdit:focus, QComboBox:focus, QSpinBox:focus { border-color: %5; }"
        "QListWidget, QTableWidget, QTreeWidget {"
        "  background: %7; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  alternate-background-color: %3; outline: none;"
        "}"
        "QListWidget::item, QTableWidget::item, QTreeWidget::item { padding: 6px 8px; }"
        "QListWidget::item:selected, QTableWidget::item:selected, QTreeWidget::item:selected {"
        "  background: %6; color: %2;"
        "}"
        "QHeaderView::section {"
        "  background: %1; color: %8;"
        "  border: none; border-bottom: 1px solid %4;"
        "  padding: 6px 10px; font-weight: 600;"
        "  text-transform: uppercase; font-size: 9px; letter-spacing: 1px;"
        "}"
        "QCheckBox { color: %2; spacing: 8px; font-size: 11px; }"
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid %4; border-radius: 4px;"
        "  background: %3;"
        "}"
        "QCheckBox::indicator:checked { background: %5; border-color: %5; }"
        "QSplitter::handle { background: transparent; }"
        "QSplitter::handle:horizontal { width: 12px; }"
        "#primaryBtn {"
        "  background: %5; color: #ffffff;"
        "  border: none; border-radius: 6px;"
        "  padding: 8px 22px; font-size: 11px; font-weight: 600;"
        "}"
        "#primaryBtn:hover { background: %9; }"
        "#ghostBtn {"
        "  background: transparent; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 8px 18px; font-size: 11px; font-weight: 500;"
        "}"
        "#ghostBtn:hover { background: %3; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.surfaceColor(),
              tm.borderColor(), tm.accentColor(), tm.accentTintColor(),
              tm.panelColor(), tm.dimColor(), tm.accentLightColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 24);
    root->setSpacing(0);

    auto *eyebrow = new QLabel(tr_("rss_eyebrow").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);
    root->addSpacing(6);

    auto *heading = new QLabel(tr_("rss_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    root->addWidget(heading);
    root->addSpacing(4);

    auto *subtitle = new QLabel(tr_("rss_subtitle"));
    {
        QFont f; f.setPointSize(11);
        subtitle->setFont(f);
        subtitle->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
        subtitle->setWordWrap(true);
    }
    root->addWidget(subtitle);
    root->addSpacing(18);

    auto *addRow = new QHBoxLayout;
    addRow->setSpacing(8);
    m_urlEdit = new QLineEdit;
    m_urlEdit->setPlaceholderText(tr_("rss_url_hint"));
    {
        QFont f("Menlo");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(10);
        m_urlEdit->setFont(f);
    }
    addRow->addWidget(m_urlEdit, 1);

    auto *addBtn = new QPushButton(tr_("rss_add"));
    addBtn->setObjectName(QStringLiteral("primaryBtn"));
    addBtn->setCursor(Qt::PointingHandCursor);
    connect(addBtn, &QPushButton::clicked, this, &RssDialog::addFeed);
    connect(m_urlEdit, &QLineEdit::returnPressed, this, &RssDialog::addFeed);
    addRow->addWidget(addBtn);
    root->addLayout(addRow);
    root->addSpacing(20);

    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->setHandleWidth(12);
    splitter->setChildrenCollapsible(false);

    auto *leftWidget = new QWidget;
    auto *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);

    leftLayout->addWidget(makeEyebrow(tr_("rss_feeds"), leftWidget));
    leftLayout->addSpacing(8);

    m_feedList = new QListWidget;
    m_feedList->setAlternatingRowColors(true);
    connect(m_feedList, &QListWidget::currentRowChanged, this, &RssDialog::onFeedSelected);
    leftLayout->addWidget(m_feedList, 1);
    leftLayout->addSpacing(10);

    auto *feedBtnLayout = new QHBoxLayout;
    feedBtnLayout->setSpacing(8);
    auto *removeBtn = new QPushButton(tr_("rss_remove"));
    removeBtn->setObjectName(QStringLiteral("ghostBtn"));
    removeBtn->setCursor(Qt::PointingHandCursor);
    connect(removeBtn, &QPushButton::clicked, this, &RssDialog::removeFeed);
    feedBtnLayout->addWidget(removeBtn);

    auto *refreshBtn = new QPushButton(tr_("rss_refresh_all"));
    refreshBtn->setObjectName(QStringLiteral("ghostBtn"));
    refreshBtn->setCursor(Qt::PointingHandCursor);
    connect(refreshBtn, &QPushButton::clicked, this, &RssDialog::refreshAllFeeds);
    feedBtnLayout->addWidget(refreshBtn);
    feedBtnLayout->addStretch();
    leftLayout->addLayout(feedBtnLayout);

    splitter->addWidget(leftWidget);

    auto *rightWidget = new QWidget;
    auto *rightLayout = new QVBoxLayout(rightWidget);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);

    auto *settingsCard = new QFrame;
    settingsCard->setObjectName(QStringLiteral("settingsCard"));
    settingsCard->setStyleSheet(QString(
        "QFrame#settingsCard {"
        "  background: %1; border: none; border-radius: 8px;"
        "}"
        ).arg(tm.panelColor()));
    auto *settingsCol = new QVBoxLayout(settingsCard);
    settingsCol->setContentsMargins(16, 14, 16, 16);
    settingsCol->setSpacing(10);

    auto *settingsHeader = makeEyebrow(tr_("rss_feed_settings"), settingsCard);
    settingsCol->addWidget(settingsHeader);

    auto *togglesRow = new QHBoxLayout;
    togglesRow->setSpacing(18);
    m_enabledCheck = new QCheckBox(tr_("rss_enabled"));
    m_autoDownloadCheck = new QCheckBox(tr_("rss_auto_download"));
    togglesRow->addWidget(m_enabledCheck);
    togglesRow->addWidget(m_autoDownloadCheck);
    togglesRow->addStretch();
    settingsCol->addLayout(togglesRow);

    auto *filterCol = new QVBoxLayout;
    filterCol->setSpacing(4);
    filterCol->addWidget(makeFieldLabel(tr_("rss_filter"), settingsCard));
    m_filterEdit = new QLineEdit;
    m_filterEdit->setPlaceholderText(tr_("rss_filter_hint"));
    {
        QFont f("Menlo");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(10);
        m_filterEdit->setFont(f);
    }
    filterCol->addWidget(m_filterEdit);
    settingsCol->addLayout(filterCol);

    auto *pathCol = new QVBoxLayout;
    pathCol->setSpacing(4);
    pathCol->addWidget(makeFieldLabel(tr_("rss_save_path"), settingsCard));
    auto *pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    m_savePathEdit = new QLineEdit;
    m_savePathEdit->setPlaceholderText(tr_("rss_save_path_hint"));
    {
        QFont f("Menlo");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(10);
        m_savePathEdit->setFont(f);
    }
    pathRow->addWidget(m_savePathEdit, 1);
    auto *browseBtn = new QPushButton(tr_("settings_browse"));
    browseBtn->setObjectName(QStringLiteral("ghostBtn"));
    browseBtn->setCursor(Qt::PointingHandCursor);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr_("dlg_save_to"));
        if (!dir.isEmpty()) m_savePathEdit->setText(dir);
    });
    pathRow->addWidget(browseBtn);
    pathCol->addLayout(pathRow);
    settingsCol->addLayout(pathCol);

    auto *intervalRow = new QHBoxLayout;
    intervalRow->setSpacing(10);
    auto *intervalCol = new QVBoxLayout;
    intervalCol->setSpacing(4);
    intervalCol->addWidget(makeFieldLabel(tr_("rss_interval"), settingsCard));
    m_intervalSpin = new QSpinBox;
    m_intervalSpin->setRange(5, 1440);
    m_intervalSpin->setValue(30);
    m_intervalSpin->setSuffix(QString(" %1").arg(tr_("rss_minutes_suffix")));
    m_intervalSpin->setFixedWidth(140);
    intervalCol->addWidget(m_intervalSpin);
    intervalRow->addLayout(intervalCol);
    intervalRow->addStretch();

    m_saveBtn = new QPushButton(tr_("rss_save_settings"));
    m_saveBtn->setObjectName(QStringLiteral("primaryBtn"));
    m_saveBtn->setCursor(Qt::PointingHandCursor);
    connect(m_saveBtn, &QPushButton::clicked, this, &RssDialog::saveCurrentFeedSettings);
    intervalRow->addWidget(m_saveBtn, 0, Qt::AlignBottom);
    settingsCol->addLayout(intervalRow);

    m_lastCheckedLabel = new QLabel;
    {
        QFont f; f.setPointSize(10);
        m_lastCheckedLabel->setFont(f);
        m_lastCheckedLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.dimColor()));
    }
    settingsCol->addWidget(m_lastCheckedLabel);

    rightLayout->addWidget(settingsCard);
    rightLayout->addSpacing(16);

    rightLayout->addWidget(makeEyebrow(tr_("rss_items"), rightWidget));
    rightLayout->addSpacing(8);

    m_itemsTable = new QTableWidget;
    m_itemsTable->setColumnCount(3);
    m_itemsTable->setHorizontalHeaderLabels({
        tr_("rss_col_title"), tr_("rss_col_size"), tr_("rss_col_date")
    });
    m_itemsTable->horizontalHeader()->setStretchLastSection(false);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    m_itemsTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    m_itemsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_itemsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_itemsTable->setAlternatingRowColors(true);
    m_itemsTable->verticalHeader()->hide();
    m_itemsTable->setShowGrid(false);
    connect(m_itemsTable, &QTableWidget::cellDoubleClicked, this, &RssDialog::onItemDoubleClicked);
    rightLayout->addWidget(m_itemsTable, 1);

    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    root->addWidget(splitter, 1);
    root->addSpacing(14);

    m_statusLabel = new QLabel;
    {
        QFont f; f.setPointSize(10);
        m_statusLabel->setFont(f);
        m_statusLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(tm.mutedColor()));
    }

    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->addWidget(m_statusLabel);
    footer->addStretch();

    auto *closeBtn = new QPushButton(tr_("btn_ok"));
    closeBtn->setObjectName(QStringLiteral("primaryBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    closeBtn->setDefault(true);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(closeBtn);

    root->addLayout(footer);

    connect(&RssManager::instance(), &RssManager::feedAdded, this, [this](const RssFeed &) {
        refreshFeedList();
    });
    connect(&RssManager::instance(), &RssManager::feedUpdated, this, &RssDialog::onFeedUpdated);
    connect(&RssManager::instance(), &RssManager::feedError, this, [this](const QString &err) {
        m_statusLabel->setText(err);
    });

    refreshFeedList();

    settingsCard->setEnabled(false);
    connect(m_feedList, &QListWidget::currentRowChanged, settingsCard, [settingsCard](int row) {
        settingsCard->setEnabled(row >= 0);
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

    const auto &tm = ThemeManager::instance();
    for (int i = 0; i < items.size(); ++i) {
        auto *titleItem = new QTableWidgetItem(items[i].title);
        if (items[i].downloaded)
            titleItem->setForeground(QColor(tm.dimColor()));
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
    showFeedItems(m_selectedFeed);
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
        if (!f.enabled) text += QString("  ·  %1").arg(tr_("rss_disabled"));
        if (f.autoDownload) text += QString("  ·  %1").arg(tr_("rss_auto"));
        m_feedList->addItem(text);
    }
}
