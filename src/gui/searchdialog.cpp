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
#include <QComboBox>
#include <QStackedWidget>

SearchDialog::SearchDialog(SessionManager *session, const QString &savePath, QWidget *parent)
    : QDialog(parent), m_session(session), m_savePath(savePath)
{
    setWindowTitle(tr_("search_title"));
    setMinimumSize(700, 500);
    setStyleSheet(ThemeManager::instance().dialogStyleSheet());

    auto &tm = ThemeManager::instance();
    auto *layout = new QVBoxLayout(this);

    // Source selector + category row
    auto *filterLayout = new QHBoxLayout;

    m_sourceCombo = new QComboBox;
    m_sourceCombo->addItem(tr_("search_source_stremio"));   // index 0
    if (AddonManager::instance().torrentSearchEnabled()) {
        m_sourceCombo->addItem(tr_("search_source_torrents")); // index 1
        m_sourceCombo->addItem(tr_("search_source_games"));    // index 2
    }
    m_sourceCombo->setStyleSheet(QString(
        "QComboBox { background: %1; color: %2; border: 1px solid %3;"
        "border-radius: 6px; padding: 6px 10px; }"
        "QComboBox::drop-down { border: none; }"
        "QComboBox QAbstractItemView { background: %1; color: %2; selection-background-color: %4; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    connect(m_sourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SearchDialog::onSourceChanged);
    filterLayout->addWidget(m_sourceCombo);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem(tr_("search_cat_all"), 0);
    m_categoryCombo->addItem(tr_("search_cat_audio"), 100);
    m_categoryCombo->addItem(tr_("search_cat_video"), 200);
    m_categoryCombo->addItem(tr_("search_cat_apps"), 300);
    m_categoryCombo->addItem(tr_("search_cat_games"), 400);
    m_categoryCombo->addItem(tr_("search_cat_other"), 500);
    m_categoryCombo->setStyleSheet(m_sourceCombo->styleSheet());
    m_categoryCombo->hide(); // only visible in torrent mode
    filterLayout->addWidget(m_categoryCombo);

    filterLayout->addStretch();
    layout->addLayout(filterLayout);

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

    // Table style shared by all tables
    QString tableStyle = QString(
        "QTableWidget { background: %1; color: %2; border: 1px solid %3; gridline-color: %3; }"
        "QTableWidget::item { padding: 6px; }"
        "QTableWidget::item:selected { background: %4; }"
        "QHeaderView::section { background: %1; color: %2; border: 1px solid %3; padding: 6px; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor());

    // Catalog results table
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

    // Torrent search results table (hidden initially)
    m_torrentTable = new QTableWidget;
    m_torrentTable->setColumnCount(4);
    m_torrentTable->setHorizontalHeaderLabels({
        tr_("search_col_name"), tr_("search_col_size"),
        tr_("search_col_seeds"), tr_("search_col_leechers")});
    m_torrentTable->horizontalHeader()->setStretchLastSection(true);
    m_torrentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_torrentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_torrentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_torrentTable->verticalHeader()->hide();
    m_torrentTable->setStyleSheet(tableStyle);
    connect(m_torrentTable, &QTableWidget::cellDoubleClicked, this, &SearchDialog::onTorrentDoubleClicked);
    m_torrentTable->hide();
    layout->addWidget(m_torrentTable);

    // Game search results table (hidden initially)
    m_gameTable = new QTableWidget;
    m_gameTable->setColumnCount(5);
    m_gameTable->setHorizontalHeaderLabels({
        tr_("search_col_name"), tr_("search_col_repacker"),
        tr_("search_col_size"), tr_("search_col_seeds"), tr_("search_col_leechers")});
    m_gameTable->horizontalHeader()->setStretchLastSection(true);
    m_gameTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_gameTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_gameTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gameTable->verticalHeader()->hide();
    m_gameTable->setStyleSheet(tableStyle);
    connect(m_gameTable, &QTableWidget::cellDoubleClicked, this, &SearchDialog::onGameDoubleClicked);
    m_gameTable->hide();
    layout->addWidget(m_gameTable);

    // Back button (for stream view)
    auto *btnLayout = new QHBoxLayout;
    m_backBtn = new QPushButton(QString::fromUtf8("\xe2\x86\x90 ") + tr_("search_back"));
    m_backBtn->setFixedWidth(120);
    m_backBtn->setStyleSheet(QString(
        "QPushButton { background: %1; color: %2; border: 1px solid %3;"
        "border-radius: 6px; padding: 8px 14px; font-weight: bold; }"
        "QPushButton:hover { background: %4; }")
        .arg(tm.surfaceColor(), tm.textColor(), tm.borderColor(), tm.accentColor()));
    m_backBtn->hide();
    connect(m_backBtn, &QPushButton::clicked, this, &SearchDialog::switchToCatalog);
    btnLayout->addWidget(m_backBtn);
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
    connect(&AddonManager::instance(), &AddonManager::torrentSearchResults,
            this, [this](const QList<TorrentSearchResult> &results) {
                if (m_isGameSearch)
                    showGameResults(results);
                else
                    showTorrentResults(results);
            });
    connect(&AddonManager::instance(), &AddonManager::torrentSearchFinished,
            this, [this]() {
                int count = m_isGameSearch ? m_gameResults.size() : m_torrentResults.size();
                m_statusLabel->setText(tr_("search_done").arg(count));
            });
    connect(&AddonManager::instance(), &AddonManager::torrentSearchError,
            this, [this](const QString &err) { m_statusLabel->setText(err); });
}

void SearchDialog::onSourceChanged(int index)
{
    if (index == 2) {
        // Games mode
        m_categoryCombo->hide();
        m_searchEdit->setPlaceholderText(tr_("search_placeholder_games"));
        switchToGameResults();
    } else if (index == 1) {
        // Torrent search mode
        m_categoryCombo->show();
        m_searchEdit->setPlaceholderText(tr_("search_placeholder_torrent"));
        switchToTorrentResults();
    } else {
        // Stremio mode
        m_categoryCombo->hide();
        m_searchEdit->setPlaceholderText(tr_("search_placeholder"));
        switchToCatalog();
    }
}

void SearchDialog::performSearch()
{
    QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty()) return;

    int sourceIndex = m_sourceCombo->currentIndex();

    if (sourceIndex == 2) {
        // Games mode — fixed category 400
        m_gameTable->setRowCount(0);
        m_gameResults.clear();
        m_isGameSearch = true;
        switchToGameResults();
        m_statusLabel->setText(tr_("search_searching"));

        AddonManager::instance().searchTorrents(query, 400);
    } else if (sourceIndex == 1) {
        m_torrentTable->setRowCount(0);
        m_torrentResults.clear();
        m_isGameSearch = false;
        switchToTorrentResults();
        m_statusLabel->setText(tr_("search_searching"));

        int cat = m_categoryCombo->currentData().toInt();
        AddonManager::instance().searchTorrents(query, cat);
    } else {
        if (!AddonManager::instance().hasCatalogAddon()) {
            m_statusLabel->setText(tr_("search_no_catalog"));
            return;
        }

        m_catalogTable->setRowCount(0);
        m_currentItems.clear();
        switchToCatalog();
        m_statusLabel->setText(tr_("search_searching"));

        AddonManager::instance().searchCatalog(query);
    }
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

    if (!AddonManager::instance().hasStreamAddon()) {
        m_statusLabel->setText(tr_("search_no_stream"));
        return;
    }

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

void SearchDialog::showTorrentResults(const QList<TorrentSearchResult> &results)
{
    m_torrentResults = results;
    m_torrentTable->setRowCount(results.size());

    for (int i = 0; i < results.size(); ++i) {
        m_torrentTable->setItem(i, 0, new QTableWidgetItem(results[i].name));
        m_torrentTable->setItem(i, 1, new QTableWidgetItem(
            results[i].size > 0 ? formatSize(results[i].size) : ""));

        auto *seedItem = new QTableWidgetItem(QString::number(results[i].seeders));
        seedItem->setForeground(results[i].seeders > 0 ? QColor("#4CAF50") : QColor("#888"));
        m_torrentTable->setItem(i, 2, seedItem);

        auto *leechItem = new QTableWidgetItem(QString::number(results[i].leechers));
        leechItem->setForeground(QColor("#FF9800"));
        m_torrentTable->setItem(i, 3, leechItem);
    }
}

void SearchDialog::onTorrentDoubleClicked(int row, int)
{
    if (row < 0 || row >= m_torrentResults.size()) return;

    const auto &result = m_torrentResults[row];
    m_session->addMagnet(result.magnet, m_savePath);
    m_statusLabel->setText(tr_("search_added").arg(result.name));
}

void SearchDialog::switchToStreams()
{
    m_catalogTable->hide();
    m_torrentTable->hide();
    m_gameTable->hide();
    m_streamTable->show();
    m_backBtn->show();
}

void SearchDialog::switchToCatalog()
{
    m_streamTable->hide();
    m_torrentTable->hide();
    m_gameTable->hide();
    m_catalogTable->show();
    m_backBtn->hide();
}

void SearchDialog::switchToTorrentResults()
{
    m_catalogTable->hide();
    m_streamTable->hide();
    m_gameTable->hide();
    m_torrentTable->show();
    m_backBtn->hide();
}

void SearchDialog::switchToGameResults()
{
    m_catalogTable->hide();
    m_streamTable->hide();
    m_torrentTable->hide();
    m_gameTable->show();
    m_backBtn->hide();
}

QString SearchDialog::detectRepacker(const QString &name)
{
    QString lower = name.toLower();
    if (lower.contains("fitgirl")) return "FitGirl";
    if (lower.contains("dodi")) return "DODI";
    if (lower.contains("online-fix") || lower.contains("onlinefix")) return "Online-Fix";
    if (lower.contains("elamigos")) return "ElAmigos";
    if (lower.contains("xatab")) return "Xatab";
    if (lower.contains("r.g. mechanics") || lower.contains("rg mechanics")) return "R.G. Mechanics";
    if (lower.contains("gog")) return "GOG";
    if (lower.contains("codex")) return "CODEX";
    if (lower.contains("plaza")) return "PLAZA";
    if (lower.contains("skidrow")) return "SKIDROW";
    return "";
}

void SearchDialog::showGameResults(const QList<TorrentSearchResult> &results)
{
    m_gameResults = results;
    m_gameTable->setRowCount(results.size());

    for (int i = 0; i < results.size(); ++i) {
        m_gameTable->setItem(i, 0, new QTableWidgetItem(results[i].name));

        QString repacker = detectRepacker(results[i].name);
        auto *repackerItem = new QTableWidgetItem(repacker);
        if (repacker == "FitGirl")
            repackerItem->setForeground(QColor("#E91E63"));
        else if (repacker == "DODI")
            repackerItem->setForeground(QColor("#2196F3"));
        else if (repacker == "Online-Fix")
            repackerItem->setForeground(QColor("#4CAF50"));
        else if (repacker == "GOG")
            repackerItem->setForeground(QColor("#9C27B0"));
        else if (!repacker.isEmpty())
            repackerItem->setForeground(QColor("#888"));
        m_gameTable->setItem(i, 1, repackerItem);

        m_gameTable->setItem(i, 2, new QTableWidgetItem(
            results[i].size > 0 ? formatSize(results[i].size) : ""));

        auto *seedItem = new QTableWidgetItem(QString::number(results[i].seeders));
        seedItem->setForeground(results[i].seeders > 0 ? QColor("#4CAF50") : QColor("#888"));
        m_gameTable->setItem(i, 3, seedItem);

        auto *leechItem = new QTableWidgetItem(QString::number(results[i].leechers));
        leechItem->setForeground(QColor("#FF9800"));
        m_gameTable->setItem(i, 4, leechItem);
    }
}

void SearchDialog::onGameDoubleClicked(int row, int)
{
    if (row < 0 || row >= m_gameResults.size()) return;

    const auto &result = m_gameResults[row];
    m_session->addMagnet(result.magnet, m_savePath);
    m_statusLabel->setText(tr_("search_added").arg(result.name));
}
