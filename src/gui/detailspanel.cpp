// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "detailspanel.h"
#include "piecemapwidget.h"
#include "../torrent/sessionmanager.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include "../app/geoip.h"
#include "../app/metadataresolver.h"
#include "thememanager.h"
#include <QLabel>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QComboBox>
#include <QPushButton>
#include <QLineEdit>
#include <QInputDialog>
#include <QMenu>
#include <QScrollArea>
#include <QFrame>

DetailsPanel::DetailsPanel(SessionManager *session, QWidget *parent)
    : QTabWidget(parent), m_session(session)
{
    m_geoIp = new GeoIpResolver(this);

    // Match canvas/primitives.jsx Tabs: paddingLeft 16 on the tab strip. The
    // global QSS positions tabs flush-left; setting it on this widget here
    // overrides for this panel only.
    const auto &tm = ThemeManager::instance();
    setStyleSheet(QString(
        "QTabWidget::pane { background: %1; border: none; }"
        "QTabBar { background: %1; alignment: left; }"
        "QTabBar::tab {"
        "  background: transparent; color: %2;"
        "  padding: 10px 16px; margin: 0;"
        "  border: none; border-bottom: 2px solid transparent;"
        "  font-size: 12px; font-weight: 500;"
        "}"
        "QTabBar::tab:first { margin-left: 16px; }"
        "QTabBar::tab:selected {"
        "  color: %3; border-bottom: 2px solid %4;"
        "  font-weight: 600;"
        "}"
        "QTabBar::tab:hover:!selected { color: %3; }"

        "QTableWidget {"
        "  background: %1; color: %3;"
        "  border: none; outline: none;"
        "  alternate-background-color: %5;"
        "  gridline-color: transparent;"
        "}"
        "QTableWidget::item { padding: 4px 8px; border: none; }"
        "QTableWidget::item:selected { background: %6; color: %3; }"
        "QHeaderView { background: transparent; border: none; }"
        "QHeaderView::section {"
        "  background: %1; color: %7;"
        "  border: none; border-bottom: 1px solid %8;"
        "  padding: 8px 10px; font-weight: 700;"
        "  font-size: 9px; letter-spacing: 1.2px;"
        "}"

        "QPushButton {"
        "  background: transparent; color: %3;"
        "  border: 1px solid %8; border-radius: 6px;"
        "  padding: 6px 16px; font-size: 11px; font-weight: 500;"
        "}"
        "QPushButton:hover { background: %5; }"
        ).arg(tm.panelColor(), tm.mutedColor(), tm.textColor(),
              tm.accentColor(), tm.surfaceColor(), tm.accentTintColor(),
              tm.dimColor(), tm.borderColor()));

    auto wrapScroll = [](QWidget *content) -> QScrollArea * {
        auto *scroll = new QScrollArea;
        scroll->setWidget(content);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        return scroll;
    };

    addTab(wrapScroll(createGeneralTab()), tr_("detail_general"));
    addTab(createPeersTab(), tr_("detail_peers"));
    addTab(createFilesTab(), tr_("detail_files"));
    addTab(createTrackersTab(), tr_("detail_trackers"));
    addTab(wrapScroll(createPiecesTab()), tr_("detail_pieces"));

    setMinimumHeight(140);

    connect(m_session, &SessionManager::torrentsUpdated, this, &DetailsPanel::refresh);

    // When a GeoIP lookup completes, update the peers table
    connect(m_geoIp, &GeoIpResolver::resolved, this, [this](const QString &ip, const QString &countryCode) {
        if (currentIndex() != 1) return;
        for (int row = 0; row < m_peersTable->rowCount(); ++row) {
            auto *ipItem = m_peersTable->item(row, 1); // IP column is 1
            if (ipItem && ipItem->text() == ip) {
                auto *flagItem = m_peersTable->item(row, 0);
                if (flagItem)
                    flagItem->setText(countryCodeToFlag(countryCode));
            }
        }
    });
}

void DetailsPanel::showTorrent(int index)
{
    m_currentIndex = index;
    refresh();
}

void DetailsPanel::refresh()
{
    const QString dash = QStringLiteral("—");
    if (m_currentIndex < 0 || m_currentIndex >= m_session->torrentCount()) {
        for (QLabel *l : {m_nameLabel, m_savePathLabel, m_sizeLabel, m_hashLabel,
                          m_downloadedLabel, m_uploadedLabel, m_speedLabel,
                          m_etaLabel, m_seedsLabel, m_peersLabel,
                          m_ratioLabel, m_stateLabel}) {
            l->setText(dash);
        }
        m_peersTable->setRowCount(0);
        m_filesTable->setRowCount(0);
        m_trackersTable->setRowCount(0);
        m_pieceMap->setPieces({});
        return;
    }

    TorrentInfo info = m_session->torrentAt(m_currentIndex);
    m_nameLabel->setText(info.name);
    m_savePathLabel->setText(info.savePath);
    m_sizeLabel->setText(formatSize(info.totalSize));
    const QString fullHash = m_session->torrentHashAt(m_currentIndex);
    m_hashLabel->setText(fullHash.size() > 14
        ? fullHash.left(8) + QStringLiteral("…") + fullHash.right(4)
        : fullHash.isEmpty() ? dash : fullHash);

    m_downloadedLabel->setText(QStringLiteral("%1 (%2%)")
        .arg(formatSize(info.totalDone))
        .arg(info.progress * 100.0, 0, 'f', 1));
    const qint64 uploaded = static_cast<qint64>(info.totalDone * info.ratio);
    m_uploadedLabel->setText(formatSize(uploaded));
    m_speedLabel->setText(QStringLiteral("↓ %1   ↑ %2")
        .arg(formatSpeed(info.downloadRate), formatSpeed(info.uploadRate)));
    if (info.downloadRate > 0 && info.totalSize > info.totalDone) {
        const qint64 remaining = info.totalSize - info.totalDone;
        const qint64 secs = remaining / info.downloadRate;
        if (secs < 60)
            m_etaLabel->setText(QStringLiteral("%1s").arg(secs));
        else if (secs < 3600)
            m_etaLabel->setText(QStringLiteral("%1m %2s").arg(secs / 60).arg(secs % 60));
        else
            m_etaLabel->setText(QStringLiteral("%1h %2m").arg(secs / 3600).arg((secs % 3600) / 60));
    } else {
        m_etaLabel->setText(dash);
    }

    m_seedsLabel->setText(QStringLiteral("%1 %2").arg(info.numSeeds).arg(tr_("detail_connected")));
    m_peersLabel->setText(QStringLiteral("%1 %2").arg(info.numPeers).arg(tr_("detail_connected")));
    m_ratioLabel->setText(QString::number(info.ratio, 'f', 2));
    m_stateLabel->setText(info.stateString);

    if (m_resolver && !fullHash.isEmpty() && m_resolver->hasCached(fullHash)) {
        auto meta = m_resolver->cached(fullHash);
        if (meta.valid) {
            m_metadataWidget->show();
            if (!meta.posterPath.isEmpty()) {
                QPixmap poster(meta.posterPath);
                if (!poster.isNull())
                    m_posterLabel->setPixmap(poster.scaled(120, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            m_metaTitleLabel->setText(meta.title);
            QString infoLine;
            if (meta.year > 0) infoLine = QString::number(meta.year);
            if (!meta.genres.isEmpty()) {
                if (!infoLine.isEmpty()) infoLine += QStringLiteral(" · ");
                infoLine += meta.genres.mid(0, 3).join(QStringLiteral(", "));
            }
            if (meta.rating > 0) {
                if (!infoLine.isEmpty()) infoLine += QStringLiteral(" · ");
                infoLine += QStringLiteral("%1/10").arg(meta.rating, 0, 'f', 1);
            }
            m_metaInfoLabel->setText(infoLine);
            m_metaSynopsisLabel->setText(meta.description.left(300));
        } else {
            m_metadataWidget->hide();
        }
    } else {
        m_metadataWidget->hide();
    }

    // Peers tab
    if (currentIndex() == 1) {
        auto peers = m_session->peersAt(m_currentIndex);
        m_peersTable->setRowCount(static_cast<int>(peers.size()));
        for (int i = 0; i < static_cast<int>(peers.size()); ++i) {
            // Country flag column
            QString cached = m_geoIp->cachedCountry(peers[i].ip);
            m_peersTable->setItem(i, 0, new QTableWidgetItem(
                cached.isEmpty() ? QString() : countryCodeToFlag(cached)));
            m_peersTable->setItem(i, 1, new QTableWidgetItem(peers[i].ip));
            m_peersTable->setItem(i, 2, new QTableWidgetItem(QString::number(peers[i].port)));
            m_peersTable->setItem(i, 3, new QTableWidgetItem(peers[i].client));
            m_peersTable->setItem(i, 4, new QTableWidgetItem(formatSpeed(peers[i].downloadRate)));
            m_peersTable->setItem(i, 5, new QTableWidgetItem(formatSpeed(peers[i].uploadRate)));
            m_peersTable->setItem(i, 6, new QTableWidgetItem(
                QString::number(peers[i].progress * 100.0, 'f', 1) + "%"));

            // Request GeoIP resolution if not cached
            if (cached.isEmpty())
                m_geoIp->resolve(peers[i].ip);
        }
    }

    // Files tab
    if (currentIndex() == 2) {
        auto files = m_session->filesAt(m_currentIndex);
        m_filesTable->setRowCount(static_cast<int>(files.size()));
        for (int i = 0; i < static_cast<int>(files.size()); ++i) {
            m_filesTable->setItem(i, 0, new QTableWidgetItem(files[i].path));
            m_filesTable->setItem(i, 1, new QTableWidgetItem(formatSize(files[i].size)));
            m_filesTable->setItem(i, 2, new QTableWidgetItem(
                QString::number(files[i].progress * 100.0, 'f', 1) + "%"));

            // Priority combo
            auto *combo = new QComboBox;
            combo->addItem(tr_("priority_skip"), 0);
            combo->addItem(tr_("priority_low"), 1);
            combo->addItem(tr_("priority_normal"), 4);
            combo->addItem(tr_("priority_high"), 7);

            // Set current based on priority value
            int prio = files[i].priority;
            if (prio == 0) combo->setCurrentIndex(0);
            else if (prio <= 2) combo->setCurrentIndex(1);
            else if (prio <= 5) combo->setCurrentIndex(2);
            else combo->setCurrentIndex(3);

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this, i, combo](int) {
                int prio = combo->currentData().toInt();
                onFilePriorityChanged(i, prio);
            });

            m_filesTable->setCellWidget(i, 3, combo);
        }
    }

    // Trackers tab
    if (currentIndex() == 3) {
        auto trackers = m_session->trackersAt(m_currentIndex);
        m_trackersTable->setRowCount(static_cast<int>(trackers.size()));
        for (int i = 0; i < static_cast<int>(trackers.size()); ++i) {
            m_trackersTable->setItem(i, 0, new QTableWidgetItem(trackers[i].url));
            m_trackersTable->setItem(i, 1, new QTableWidgetItem(QString::number(trackers[i].tier)));
            m_trackersTable->setItem(i, 2, new QTableWidgetItem(trackers[i].status));
        }
    }

    // Pieces tab
    if (currentIndex() == 4) {
        auto pieces = m_session->piecesAt(m_currentIndex);
        m_pieceMap->setPieces(pieces);
    }
}

void DetailsPanel::onFilePriorityChanged(int row, int priority)
{
    if (m_currentIndex >= 0)
        m_session->setFilePriority(m_currentIndex, row, priority);
}

void DetailsPanel::setMetadataResolver(MetadataResolver *resolver)
{
    m_resolver = resolver;
}

void DetailsPanel::onAddTracker()
{
    if (m_currentIndex < 0) return;
    QString url = QInputDialog::getText(this, tr_("tracker_add"), tr_("tracker_url"));
    if (!url.isEmpty())
        m_session->addTracker(m_currentIndex, url);
}

QWidget *DetailsPanel::createGeneralTab()
{
    // Mirrors canvas/main.jsx MiniDetails: 3 columns (Info / Transfer / Peers),
    // each with an Eyebrow CAPS header and KV rows below.
    const auto &tm = ThemeManager::instance();
    auto *widget = new QWidget;
    widget->setAutoFillBackground(true);
    widget->setStyleSheet(QString("QWidget { background: %1; }").arg(tm.panelColor()));

    auto *row = new QHBoxLayout(widget);
    row->setContentsMargins(16, 16, 16, 16);
    row->setSpacing(24);

    // Eyebrow: 8pt Bold, letterSpacing 1.2, color textDim (matches primitives.jsx).
    auto makeEyebrow = [&](const QString &text) {
        auto *lbl = new QLabel(text.toUpper());
        QFont f;
        f.setPointSize(8);
        f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        lbl->setFont(f);
        lbl->setStyleSheet(QString("color: %1;").arg(tm.dimColor()));
        return lbl;
    };

    // KV row: padding 6 vertical, fixed-width label, value sits right after
    // the label at its natural width; trailing stretch consumes the rest of
    // the column. Previous version stretched the value, leaving a huge gap
    // between value and the next column's label.
    auto makeKV = [&](const QString &k, QLabel *value, bool mono = false) {
        auto *r = new QWidget;
        auto *rl = new QHBoxLayout(r);
        rl->setContentsMargins(0, 6, 0, 6);
        rl->setSpacing(12);
        auto *key = new QLabel(k);
        QFont kf;
        kf.setPointSize(9);
        kf.setWeight(QFont::DemiBold);
        key->setFont(kf);
        key->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
        key->setFixedWidth(90);
        rl->addWidget(key);
        QFont vf;
        vf.setPointSize(9);
        if (mono) {
            vf.setFamily(QStringLiteral("Menlo"));
            if (!QFontInfo(vf).fixedPitch())
                vf.setFamily(QStringLiteral("Consolas"));
        }
        value->setFont(vf);
        value->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
        value->setText(QStringLiteral("—"));
        value->setTextInteractionFlags(Qt::TextSelectableByMouse);
        rl->addWidget(value);
        rl->addStretch();
        return r;
    };

    auto makeColumn = [&](const QString &eyebrow,
                          std::initializer_list<QWidget *> rows) {
        auto *col = new QWidget;
        auto *cl = new QVBoxLayout(col);
        cl->setContentsMargins(0, 0, 0, 0);
        cl->setSpacing(0);
        cl->addWidget(makeEyebrow(eyebrow));
        cl->addSpacing(8);
        for (QWidget *r : rows)
            cl->addWidget(r);
        cl->addStretch();
        return col;
    };

    m_metadataWidget = new QWidget;
    auto *metaCol = new QVBoxLayout(m_metadataWidget);
    metaCol->setContentsMargins(0, 0, 16, 0);
    metaCol->setSpacing(8);
    m_posterLabel = new QLabel;
    m_posterLabel->setFixedSize(120, 180);
    m_posterLabel->setScaledContents(true);
    m_posterLabel->setStyleSheet(QStringLiteral("border-radius: 6px; background: transparent;"));
    metaCol->addWidget(m_posterLabel);
    m_metaTitleLabel = new QLabel;
    m_metaTitleLabel->setWordWrap(true);
    {
        QFont f; f.setPointSize(10); f.setWeight(QFont::Bold);
        m_metaTitleLabel->setFont(f);
        m_metaTitleLabel->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    metaCol->addWidget(m_metaTitleLabel);
    m_metaInfoLabel = new QLabel;
    m_metaInfoLabel->setWordWrap(true);
    m_metaInfoLabel->setStyleSheet(QString("color: %1; font-size: 9px;").arg(tm.dimColor()));
    metaCol->addWidget(m_metaInfoLabel);
    m_metaSynopsisLabel = new QLabel;
    m_metaSynopsisLabel->setWordWrap(true);
    m_metaSynopsisLabel->setMaximumHeight(100);
    m_metaSynopsisLabel->setStyleSheet(QString("color: %1; font-size: 9px;").arg(tm.mutedColor()));
    metaCol->addWidget(m_metaSynopsisLabel);
    metaCol->addStretch();
    m_metadataWidget->setFixedWidth(140);
    m_metadataWidget->hide();
    row->addWidget(m_metadataWidget);

    m_nameLabel = new QLabel;
    m_savePathLabel = new QLabel;
    m_sizeLabel = new QLabel;
    m_hashLabel = new QLabel;
    m_downloadedLabel = new QLabel;
    m_uploadedLabel = new QLabel;
    m_speedLabel = new QLabel;
    m_etaLabel = new QLabel;
    m_seedsLabel = new QLabel;
    m_peersLabel = new QLabel;
    m_ratioLabel = new QLabel;
    m_stateLabel = new QLabel;

    row->addWidget(makeColumn(tr_("detail_section_info"), {
        makeKV(tr_("detail_kv_name"),    m_nameLabel),
        makeKV(tr_("detail_kv_save_to"), m_savePathLabel, /*mono=*/true),
        makeKV(tr_("detail_kv_size"),    m_sizeLabel),
        makeKV(tr_("detail_kv_hash"),    m_hashLabel,     /*mono=*/true),
    }), /*stretch=*/1);

    row->addWidget(makeColumn(tr_("detail_section_transfer"), {
        makeKV(tr_("detail_kv_downloaded"), m_downloadedLabel),
        makeKV(tr_("detail_kv_uploaded"),   m_uploadedLabel),
        makeKV(tr_("detail_kv_speed"),      m_speedLabel),
        makeKV(tr_("detail_kv_eta"),        m_etaLabel),
    }), /*stretch=*/1);

    row->addWidget(makeColumn(tr_("detail_section_peers"), {
        makeKV(tr_("detail_kv_seeds"), m_seedsLabel),
        makeKV(tr_("detail_kv_peers"), m_peersLabel),
        makeKV(tr_("detail_kv_ratio"), m_ratioLabel),
        makeKV(tr_("detail_kv_state"), m_stateLabel),
    }), /*stretch=*/1);

    return widget;
}

QWidget *DetailsPanel::createPeersTab()
{
    const auto &tm = ThemeManager::instance();
    auto *widget = new QWidget;
    widget->setAutoFillBackground(true);
    widget->setStyleSheet(QString("QWidget { background: %1; }").arg(tm.panelColor()));
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_peersTable = new QTableWidget(0, 7);
    m_peersTable->setHorizontalHeaderLabels({
        tr_("peer_country").toUpper(), tr_("peer_ip").toUpper(),
        tr_("peer_port").toUpper(), tr_("peer_client").toUpper(),
        tr_("peer_down").toUpper(), tr_("peer_up").toUpper(),
        tr_("peer_progress").toUpper()
    });
    m_peersTable->horizontalHeader()->setStretchLastSection(true);
    m_peersTable->setColumnWidth(0, 40); // narrow flag column
    m_peersTable->verticalHeader()->hide();
    m_peersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_peersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_peersTable->setShowGrid(false);
    m_peersTable->setAlternatingRowColors(true);

    layout->addWidget(m_peersTable);
    return widget;
}

QWidget *DetailsPanel::createFilesTab()
{
    const auto &tm = ThemeManager::instance();
    auto *widget = new QWidget;
    widget->setAutoFillBackground(true);
    widget->setStyleSheet(QString("QWidget { background: %1; }").arg(tm.panelColor()));
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_filesTable = new QTableWidget(0, 4);
    m_filesTable->setHorizontalHeaderLabels({
        tr_("file_name").toUpper(), tr_("file_size").toUpper(),
        tr_("file_progress").toUpper(), tr_("file_priority").toUpper()});
    m_filesTable->horizontalHeader()->setStretchLastSection(true);
    m_filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_filesTable->verticalHeader()->hide();
    m_filesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_filesTable->setShowGrid(false);
    m_filesTable->setAlternatingRowColors(true);

    // Right-click on a file row → Reveal in Finder/Explorer. The file may
    // still be in-progress (`.!bt` suffix) so utils::revealInFileManager
    // tries both the real name and the suffix variant.
    m_filesTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_filesTable, &QTableWidget::customContextMenuRequested,
            this, [this](const QPoint &pos) {
        const int row = m_filesTable->rowAt(pos.y());
        if (row < 0 || m_currentIndex < 0) return;
        auto files = m_session->filesAt(m_currentIndex);
        if (row >= static_cast<int>(files.size())) return;
        TorrentInfo info = m_session->torrentAt(m_currentIndex);
        const QString fullPath = info.savePath + "/" + files[row].path;
        QMenu menu(m_filesTable);
        menu.addAction(tr_("ctx_reveal_file"), this, [fullPath]() {
            revealInFileManager(fullPath);
        });
        menu.exec(m_filesTable->viewport()->mapToGlobal(pos));
    });

    layout->addWidget(m_filesTable);
    return widget;
}

QWidget *DetailsPanel::createTrackersTab()
{
    const auto &tm = ThemeManager::instance();
    auto *widget = new QWidget;
    widget->setAutoFillBackground(true);
    widget->setStyleSheet(QString("QWidget { background: %1; }").arg(tm.panelColor()));
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_trackersTable = new QTableWidget(0, 3);
    m_trackersTable->setHorizontalHeaderLabels({
        tr_("tracker_url_col").toUpper(), tr_("tracker_tier").toUpper(),
        tr_("tracker_status").toUpper()});
    m_trackersTable->horizontalHeader()->setStretchLastSection(true);
    m_trackersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_trackersTable->verticalHeader()->hide();
    m_trackersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_trackersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_trackersTable->setShowGrid(false);
    m_trackersTable->setAlternatingRowColors(true);
    m_trackersTable->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_trackersTable, &QWidget::customContextMenuRequested,
            this, [this](const QPoint &p) {
        if (m_currentIndex < 0) return;
        auto selected = m_trackersTable->selectionModel()->selectedRows();
        if (selected.isEmpty()) return;
        QMenu menu(this);
        menu.addAction(tr_("tracker_remove"), this, [this, selected]() {
            // Collect the URLs the user wants to keep and call replace.
            // libtorrent has no single-tracker delete; replace_trackers
            // with the survivors is the canonical way.
            QSet<int> toRemove;
            for (const auto &idx : selected) toRemove.insert(idx.row());
            QStringList kept;
            for (int r = 0; r < m_trackersTable->rowCount(); ++r) {
                if (toRemove.contains(r)) continue;
                auto *item = m_trackersTable->item(r, 0);
                if (item) kept << item->text();
            }
            m_session->replaceTrackers(m_currentIndex, kept);
        });
        menu.exec(m_trackersTable->viewport()->mapToGlobal(p));
    });

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *addBtn = new QPushButton(tr_("tracker_add"));
    connect(addBtn, &QPushButton::clicked, this, &DetailsPanel::onAddTracker);
    btnLayout->addWidget(addBtn);

    layout->addWidget(m_trackersTable);
    layout->addLayout(btnLayout);
    return widget;
}

QWidget *DetailsPanel::createPiecesTab()
{
    const auto &tm = ThemeManager::instance();
    auto *widget = new QWidget;
    widget->setAutoFillBackground(true);
    widget->setStyleSheet(QString("QWidget { background: %1; }").arg(tm.panelColor()));
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(8, 8, 8, 8);

    m_pieceMap = new PieceMapWidget;
    layout->addWidget(m_pieceMap);
    layout->addStretch();
    return widget;
}

void DetailsPanel::restyle()
{
    const auto &tm = ThemeManager::instance();
    setStyleSheet(QString(
        "QTabWidget::pane { background: %1; border: none; }"
        "QTabBar { background: %1; alignment: left; }"
        "QTabBar::tab {"
        "  background: transparent; color: %2;"
        "  padding: 10px 16px; margin: 0;"
        "  border: none; border-bottom: 2px solid transparent;"
        "  font-size: 12px; font-weight: 500;"
        "}"
        "QTabBar::tab:first { margin-left: 16px; }"
        "QTabBar::tab:selected {"
        "  color: %3; border-bottom: 2px solid %4;"
        "  font-weight: 600;"
        "}"
        "QTabBar::tab:hover:!selected { color: %3; }"
        "QTableWidget {"
        "  background: %1; color: %3;"
        "  border: none; outline: none;"
        "  alternate-background-color: %5;"
        "  gridline-color: transparent;"
        "}"
        "QTableWidget::item { padding: 4px 8px; border: none; }"
        "QTableWidget::item:selected { background: %6; color: %3; }"
        "QHeaderView { background: transparent; border: none; }"
        "QHeaderView::section {"
        "  background: %1; color: %7;"
        "  border: none; border-bottom: 1px solid %8;"
        "  padding: 8px 10px; font-weight: 700;"
        "  font-size: 9px; letter-spacing: 1.2px;"
        "}"
        "QPushButton {"
        "  background: transparent; color: %3;"
        "  border: 1px solid %8; border-radius: 6px;"
        "  padding: 6px 16px; font-size: 11px; font-weight: 500;"
        "}"
        "QPushButton:hover { background: %5; }"
        ).arg(tm.panelColor(), tm.mutedColor(), tm.textColor(),
              tm.accentColor(), tm.surfaceColor(), tm.accentTintColor(),
              tm.dimColor(), tm.borderColor()));

    const QString tabBg = QString("QWidget { background: %1; }").arg(tm.panelColor());
    for (int i = 0; i < count(); ++i) {
        QWidget *w = widget(i);
        if (auto *scroll = qobject_cast<QScrollArea *>(w))
            w = scroll->widget();
        if (w) w->setStyleSheet(tabBg);
    }
}

void DetailsPanel::retranslate()
{
    setTabText(0, tr_("detail_general"));
    setTabText(1, tr_("detail_peers"));
    setTabText(2, tr_("detail_files"));
    setTabText(3, tr_("detail_trackers"));
    setTabText(4, tr_("detail_pieces"));

    m_peersTable->setHorizontalHeaderLabels({
        tr_("peer_country").toUpper(), tr_("peer_ip").toUpper(),
        tr_("peer_port").toUpper(), tr_("peer_client").toUpper(),
        tr_("peer_down").toUpper(), tr_("peer_up").toUpper(),
        tr_("peer_progress").toUpper()
    });
    m_filesTable->setHorizontalHeaderLabels({
        tr_("file_name").toUpper(), tr_("file_size").toUpper(),
        tr_("file_progress").toUpper(), tr_("file_priority").toUpper()
    });
    m_trackersTable->setHorizontalHeaderLabels({
        tr_("tracker_url_col").toUpper(), tr_("tracker_tier").toUpper(),
        tr_("tracker_status").toUpper()
    });
}
