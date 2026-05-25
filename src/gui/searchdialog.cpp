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
#include <QFont>

namespace {

QLabel *makeEyebrow(const QString &text, const QString &color, QWidget *parent)
{
    auto *lbl = new QLabel(text.toUpper(), parent);
    QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
    f.setLetterSpacing(QFont::AbsoluteSpacing, 1.4);
    lbl->setFont(f);
    lbl->setStyleSheet(QString("color: %1; background: transparent;").arg(color));
    return lbl;
}

} // namespace

SearchDialog::SearchDialog(SessionManager *session, const QString &savePath, QWidget *parent)
    : QDialog(parent), m_session(session), m_savePath(savePath)
{
    setWindowTitle(tr_("search_title"));
    setMinimumSize(720, 560);
    resize(820, 640);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 %10,"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; color: %2; }"
        "QLineEdit, QComboBox {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 7px 10px; font-size: 11px;"
        "  selection-background-color: %5;"
        "}"
        "QLineEdit:focus, QComboBox:focus { border-color: %5; }"
        "QComboBox::drop-down { border: none; width: 22px; }"
        "QComboBox QAbstractItemView {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; selection-background-color: %6;"
        "  outline: none;"
        "}"
        "QTableWidget {"
        "  background: %7; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  alternate-background-color: %3; outline: none;"
        "  gridline-color: transparent;"
        "}"
        "QTableWidget::item { padding: 4px 2px; }"
        "QTableWidget::item:selected {"
        "  background: %6; color: %2;"
        "}"
        "QHeaderView::section {"
        "  background: %1; color: %8;"
        "  border: none; border-bottom: 1px solid %4;"
        "  padding: 6px 10px; font-weight: 600;"
        "  text-transform: uppercase; font-size: 9px; letter-spacing: 1px;"
        "}"
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
              tm.panelColor(), tm.dimColor(), tm.accentLightColor(),
              tm.accentTintForGradient(10)));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 24);
    root->setSpacing(0);

    auto *eyebrow = new QLabel(tr_("search_eyebrow").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);
    root->addSpacing(6);

    auto *heading = new QLabel(tr_("search_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    root->addWidget(heading);
    root->addSpacing(2);

    auto *subtitle = new QLabel(tr_("search_subtitle"));
    {
        QFont f; f.setPointSize(11);
        subtitle->setFont(f);
        subtitle->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
    }
    root->addWidget(subtitle);
    root->addSpacing(20);

    root->addWidget(makeEyebrow(tr_("search_section_source"), tm.dimColor(), this));
    root->addSpacing(6);

    auto *filterRow = new QHBoxLayout;
    filterRow->setSpacing(8);

    m_sourceCombo = new QComboBox;
    m_sourceCombo->addItem(tr_("search_source_stremio"), QStringLiteral("stremio"));
    if (AddonManager::instance().torrentSearchEnabled()) {
        m_sourceCombo->addItem(tr_("search_source_torrents"), QStringLiteral("legacy"));
        m_sourceCombo->addItem(tr_("search_source_games"), QStringLiteral("games"));
    }
    const auto providers = AddonManager::instance().searchProviders();
    for (int i = 0; i < providers.size(); ++i) {
        if (providers[i].enabled)
            m_sourceCombo->addItem(providers[i].name, QStringLiteral("provider:%1").arg(i));
    }
    m_sourceCombo->setCursor(Qt::PointingHandCursor);
    connect(m_sourceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SearchDialog::onSourceChanged);
    filterRow->addWidget(m_sourceCombo, 1);

    m_categoryCombo = new QComboBox;
    m_categoryCombo->addItem(tr_("search_cat_all"), 0);
    m_categoryCombo->addItem(tr_("search_cat_audio"), 100);
    m_categoryCombo->addItem(tr_("search_cat_video"), 200);
    m_categoryCombo->addItem(tr_("search_cat_apps"), 300);
    m_categoryCombo->addItem(tr_("search_cat_games"), 400);
    m_categoryCombo->addItem(tr_("search_cat_other"), 500);
    m_categoryCombo->setCursor(Qt::PointingHandCursor);
    m_categoryCombo->hide();
    filterRow->addWidget(m_categoryCombo, 1);

    root->addLayout(filterRow);
    root->addSpacing(14);

    root->addWidget(makeEyebrow(tr_("search_section_query"), tm.dimColor(), this));
    root->addSpacing(6);

    auto *searchRow = new QHBoxLayout;
    searchRow->setSpacing(8);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(tr_("search_placeholder"));
    m_searchEdit->setClearButtonEnabled(true);
    searchRow->addWidget(m_searchEdit, 1);

    auto *searchBtn = new QPushButton(tr_("search_btn"));
    searchBtn->setObjectName(QStringLiteral("primaryBtn"));
    searchBtn->setCursor(Qt::PointingHandCursor);
    connect(searchBtn, &QPushButton::clicked, this, &SearchDialog::performSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &SearchDialog::performSearch);
    searchRow->addWidget(searchBtn);
    root->addLayout(searchRow);

    root->addSpacing(16);
    root->addWidget(makeEyebrow(tr_("search_section_results"), tm.dimColor(), this));
    root->addSpacing(6);

    m_catalogTable = new QTableWidget;
    m_catalogTable->setColumnCount(3);
    m_catalogTable->setHorizontalHeaderLabels({tr_("search_col_name"), tr_("search_col_type"), tr_("search_col_year")});
    m_catalogTable->horizontalHeader()->setStretchLastSection(true);
    m_catalogTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_catalogTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_catalogTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_catalogTable->setAlternatingRowColors(true);
    m_catalogTable->setShowGrid(false);
    m_catalogTable->verticalHeader()->hide();
    connect(m_catalogTable, &QTableWidget::cellDoubleClicked, this, &SearchDialog::onItemDoubleClicked);
    root->addWidget(m_catalogTable, 1);

    m_streamTable = new QTableWidget;
    m_streamTable->setColumnCount(3);
    m_streamTable->setHorizontalHeaderLabels({tr_("search_col_quality"), tr_("search_col_size"), tr_("search_col_addon")});
    m_streamTable->horizontalHeader()->setStretchLastSection(true);
    m_streamTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_streamTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_streamTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_streamTable->setAlternatingRowColors(true);
    m_streamTable->setShowGrid(false);
    m_streamTable->verticalHeader()->hide();
    connect(m_streamTable, &QTableWidget::cellDoubleClicked, this, &SearchDialog::onStreamDoubleClicked);
    m_streamTable->hide();
    root->addWidget(m_streamTable, 1);

    m_torrentTable = new QTableWidget;
    m_torrentTable->setColumnCount(4);
    m_torrentTable->setHorizontalHeaderLabels({
        tr_("search_col_name"), tr_("search_col_size"),
        tr_("search_col_seeds"), tr_("search_col_leechers")});
    m_torrentTable->horizontalHeader()->setStretchLastSection(true);
    m_torrentTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_torrentTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_torrentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_torrentTable->setAlternatingRowColors(true);
    m_torrentTable->setShowGrid(false);
    m_torrentTable->verticalHeader()->hide();
    connect(m_torrentTable, &QTableWidget::cellDoubleClicked, this, &SearchDialog::onTorrentDoubleClicked);
    m_torrentTable->hide();
    root->addWidget(m_torrentTable, 1);

    m_gameTable = new QTableWidget;
    m_gameTable->setColumnCount(5);
    m_gameTable->setHorizontalHeaderLabels({
        tr_("search_col_name"), tr_("search_col_repacker"),
        tr_("search_col_size"), tr_("search_col_seeds"), tr_("search_col_leechers")});
    m_gameTable->horizontalHeader()->setStretchLastSection(true);
    m_gameTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_gameTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_gameTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_gameTable->setAlternatingRowColors(true);
    m_gameTable->setShowGrid(false);
    m_gameTable->verticalHeader()->hide();
    connect(m_gameTable, &QTableWidget::cellDoubleClicked, this, &SearchDialog::onGameDoubleClicked);
    m_gameTable->hide();
    root->addWidget(m_gameTable, 1);

    root->addSpacing(10);

    m_statusLabel = new QLabel;
    {
        QFont f("Menlo");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(9);
        m_statusLabel->setFont(f);
        m_statusLabel->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
    }
    root->addWidget(m_statusLabel);
    root->addSpacing(14);

    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);

    m_backBtn = new QPushButton(QString::fromUtf8("\xe2\x86\x90  ") + tr_("search_back"));
    m_backBtn->setObjectName(QStringLiteral("ghostBtn"));
    m_backBtn->setCursor(Qt::PointingHandCursor);
    m_backBtn->hide();
    connect(m_backBtn, &QPushButton::clicked, this, &SearchDialog::switchToCatalog);
    footer->addWidget(m_backBtn);

    footer->addStretch();

    auto *closeBtn = new QPushButton(tr_("btn_cancel"));
    closeBtn->setObjectName(QStringLiteral("ghostBtn"));
    closeBtn->setCursor(Qt::PointingHandCursor);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    footer->addWidget(closeBtn);

    auto *primaryBtn = new QPushButton(tr_("search_btn"));
    primaryBtn->setObjectName(QStringLiteral("primaryBtn"));
    primaryBtn->setCursor(Qt::PointingHandCursor);
    primaryBtn->setDefault(true);
    connect(primaryBtn, &QPushButton::clicked, this, &SearchDialog::performSearch);
    footer->addWidget(primaryBtn);

    root->addLayout(footer);

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
    QString data = m_sourceCombo->currentData().toString();
    if (data == "games") {
        m_categoryCombo->hide();
        m_searchEdit->setPlaceholderText(tr_("search_placeholder_games"));
        switchToGameResults();
    } else if (data == "legacy" || data.startsWith("provider:")) {
        m_categoryCombo->setVisible(data == "legacy");
        m_searchEdit->setPlaceholderText(tr_("search_placeholder_torrent"));
        switchToTorrentResults();
    } else {
        m_categoryCombo->hide();
        m_searchEdit->setPlaceholderText(tr_("search_placeholder"));
        switchToCatalog();
    }
}

void SearchDialog::performSearch()
{
    QString query = m_searchEdit->text().trimmed();
    if (query.isEmpty()) return;

    QString data = m_sourceCombo->currentData().toString();

    if (data == "games") {
        m_gameTable->setRowCount(0);
        m_gameResults.clear();
        m_isGameSearch = true;
        switchToGameResults();
        m_statusLabel->setText(tr_("search_searching"));
        AddonManager::instance().searchTorrents(query, 400);
    } else if (data.startsWith("provider:")) {
        int idx = data.mid(9).toInt();
        m_torrentTable->setRowCount(0);
        m_torrentResults.clear();
        m_isGameSearch = false;
        switchToTorrentResults();
        m_statusLabel->setText(tr_("search_searching"));
        AddonManager::instance().searchWithProvider(idx, query);
    } else if (data == "legacy") {
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

    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);

    for (int i = 0; i < streams.size(); ++i) {
        m_streamTable->setItem(i, 0, new QTableWidgetItem(streams[i].title));

        auto *sizeItem = new QTableWidgetItem(
            streams[i].size > 0 ? formatSize(streams[i].size) : "");
        sizeItem->setFont(mono);
        m_streamTable->setItem(i, 1, sizeItem);

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
    const auto &tm = ThemeManager::instance();
    m_torrentResults = results;
    m_torrentTable->setRowCount(results.size());

    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);

    for (int i = 0; i < results.size(); ++i) {
        m_torrentTable->setItem(i, 0, new QTableWidgetItem(results[i].name));

        auto *sizeItem = new QTableWidgetItem(
            results[i].size > 0 ? formatSize(results[i].size) : "");
        sizeItem->setFont(mono);
        m_torrentTable->setItem(i, 1, sizeItem);

        auto *seedItem = new QTableWidgetItem(QString::number(results[i].seeders));
        seedItem->setFont(mono);
        seedItem->setForeground(results[i].seeders > 0
            ? QColor(tm.stateSeedingColor()) : QColor(tm.dimColor()));
        m_torrentTable->setItem(i, 2, seedItem);

        auto *leechItem = new QTableWidgetItem(QString::number(results[i].leechers));
        leechItem->setFont(mono);
        leechItem->setForeground(QColor(tm.warningColor()));
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
    const auto &tm = ThemeManager::instance();
    m_gameResults = results;
    m_gameTable->setRowCount(results.size());

    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);

    for (int i = 0; i < results.size(); ++i) {
        m_gameTable->setItem(i, 0, new QTableWidgetItem(results[i].name));

        QString repacker = detectRepacker(results[i].name);
        auto *repackerItem = new QTableWidgetItem(repacker);
        if (repacker == "FitGirl")
            repackerItem->setForeground(QColor("#E91E63"));
        else if (repacker == "DODI")
            repackerItem->setForeground(QColor(tm.stateDownloadingColor()));
        else if (repacker == "Online-Fix")
            repackerItem->setForeground(QColor(tm.stateSeedingColor()));
        else if (repacker == "GOG")
            repackerItem->setForeground(QColor("#9C27B0"));
        else if (!repacker.isEmpty())
            repackerItem->setForeground(QColor(tm.dimColor()));
        m_gameTable->setItem(i, 1, repackerItem);

        auto *sizeItem = new QTableWidgetItem(
            results[i].size > 0 ? formatSize(results[i].size) : "");
        sizeItem->setFont(mono);
        m_gameTable->setItem(i, 2, sizeItem);

        auto *seedItem = new QTableWidgetItem(QString::number(results[i].seeders));
        seedItem->setFont(mono);
        seedItem->setForeground(results[i].seeders > 0
            ? QColor(tm.stateSeedingColor()) : QColor(tm.dimColor()));
        m_gameTable->setItem(i, 3, seedItem);

        auto *leechItem = new QTableWidgetItem(QString::number(results[i].leechers));
        leechItem->setFont(mono);
        leechItem->setForeground(QColor(tm.warningColor()));
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
