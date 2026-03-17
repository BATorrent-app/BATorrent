#include "detailspanel.h"
#include "../core/sessionmanager.h"
#include "../core/translator.h"
#include "../core/utils.h"
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

DetailsPanel::DetailsPanel(SessionManager *session, QWidget *parent)
    : QTabWidget(parent), m_session(session)
{
    addTab(createGeneralTab(), tr_("detail_general"));
    addTab(createPeersTab(), tr_("detail_peers"));
    addTab(createFilesTab(), tr_("detail_files"));
    addTab(createTrackersTab(), tr_("detail_trackers"));

    setMaximumHeight(200);

    connect(m_session, &SessionManager::torrentsUpdated, this, &DetailsPanel::refresh);
}

void DetailsPanel::showTorrent(int index)
{
    m_currentIndex = index;
    refresh();
}

void DetailsPanel::refresh()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_session->torrentCount()) {
        m_nameLabel->setText("-");
        m_sizeLabel->setText("-");
        m_progressLabel->setText("-");
        m_downSpeedLabel->setText("-");
        m_upSpeedLabel->setText("-");
        m_stateLabel->setText("-");
        m_peersLabel->setText("-");
        m_downloadedLabel->setText("-");
        m_savePathLabel->setText("-");
        m_ratioLabel->setText("-");
        m_peersTable->setRowCount(0);
        m_filesTable->setRowCount(0);
        m_trackersTable->setRowCount(0);
        return;
    }

    TorrentInfo info = m_session->torrentAt(m_currentIndex);
    m_nameLabel->setText(info.name);
    m_stateLabel->setText(info.stateString);
    m_peersLabel->setText(QString("%1 (%2 seeds)").arg(info.numPeers).arg(info.numSeeds));
    m_progressLabel->setText(QString::number(info.progress * 100.0, 'f', 1) + "%");
    m_sizeLabel->setText(formatSize(info.totalSize));
    m_downloadedLabel->setText(formatSize(info.totalDone));
    m_downSpeedLabel->setText(formatSpeed(info.downloadRate));
    m_upSpeedLabel->setText(formatSpeed(info.uploadRate));
    m_savePathLabel->setText(info.savePath);
    m_ratioLabel->setText(QString::number(info.ratio, 'f', 2));

    // Peers tab
    if (currentIndex() == 1) {
        auto peers = m_session->peersAt(m_currentIndex);
        m_peersTable->setRowCount(static_cast<int>(peers.size()));
        for (int i = 0; i < static_cast<int>(peers.size()); ++i) {
            m_peersTable->setItem(i, 0, new QTableWidgetItem(peers[i].ip));
            m_peersTable->setItem(i, 1, new QTableWidgetItem(QString::number(peers[i].port)));
            m_peersTable->setItem(i, 2, new QTableWidgetItem(peers[i].client));
            m_peersTable->setItem(i, 3, new QTableWidgetItem(formatSpeed(peers[i].downloadRate)));
            m_peersTable->setItem(i, 4, new QTableWidgetItem(formatSpeed(peers[i].uploadRate)));
            m_peersTable->setItem(i, 5, new QTableWidgetItem(
                QString::number(peers[i].progress * 100.0, 'f', 1) + "%"));
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
}

void DetailsPanel::onFilePriorityChanged(int row, int priority)
{
    if (m_currentIndex >= 0)
        m_session->setFilePriority(m_currentIndex, row, priority);
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
    auto *widget = new QWidget;
    auto *layout = new QFormLayout(widget);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(4);

    m_nameLabel = new QLabel("-");
    m_sizeLabel = new QLabel("-");
    m_progressLabel = new QLabel("-");
    m_downSpeedLabel = new QLabel("-");
    m_upSpeedLabel = new QLabel("-");
    m_stateLabel = new QLabel("-");
    m_peersLabel = new QLabel("-");
    m_downloadedLabel = new QLabel("-");
    m_savePathLabel = new QLabel("-");
    m_ratioLabel = new QLabel("-");

    auto addRow = [&](const QString &labelKey, QLabel *value) {
        auto *lbl = new QLabel(tr_(labelKey));
        lbl->setStyleSheet("color: #c43030; font-weight: bold;");
        layout->addRow(lbl, value);
    };

    addRow("detail_name", m_nameLabel);
    addRow("detail_save_path", m_savePathLabel);
    addRow("detail_size", m_sizeLabel);
    addRow("detail_downloaded", m_downloadedLabel);
    addRow("detail_progress", m_progressLabel);
    addRow("detail_down_speed", m_downSpeedLabel);
    addRow("detail_up_speed", m_upSpeedLabel);
    addRow("detail_state", m_stateLabel);
    addRow("detail_peers_count", m_peersLabel);
    addRow("detail_ratio", m_ratioLabel);

    return widget;
}

QWidget *DetailsPanel::createPeersTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_peersTable = new QTableWidget(0, 6);
    m_peersTable->setHorizontalHeaderLabels({"IP", "Port", "Client", "Down", "Up", "Progress"});
    m_peersTable->horizontalHeader()->setStretchLastSection(true);
    m_peersTable->verticalHeader()->hide();
    m_peersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_peersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_peersTable->setShowGrid(false);
    m_peersTable->setAlternatingRowColors(true);
    m_peersTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #141414;
            alternate-background-color: #181818;
            color: #b0b0b0;
            border: none;
        }
        QHeaderView::section {
            background-color: #1a1a1a;
            color: #c43030;
            border: none;
            border-bottom: 1px solid #c43030;
            padding: 4px;
            font-weight: bold;
        }
    )");

    layout->addWidget(m_peersTable);
    return widget;
}

QWidget *DetailsPanel::createFilesTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_filesTable = new QTableWidget(0, 4);
    m_filesTable->setHorizontalHeaderLabels({"File", "Size", "Progress", "Priority"});
    m_filesTable->horizontalHeader()->setStretchLastSection(true);
    m_filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_filesTable->verticalHeader()->hide();
    m_filesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_filesTable->setShowGrid(false);
    m_filesTable->setAlternatingRowColors(true);
    m_filesTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #141414;
            alternate-background-color: #181818;
            color: #b0b0b0;
            border: none;
        }
        QHeaderView::section {
            background-color: #1a1a1a;
            color: #c43030;
            border: none;
            border-bottom: 1px solid #c43030;
            padding: 4px;
            font-weight: bold;
        }
    )");

    layout->addWidget(m_filesTable);
    return widget;
}

QWidget *DetailsPanel::createTrackersTab()
{
    auto *widget = new QWidget;
    auto *layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    m_trackersTable = new QTableWidget(0, 3);
    m_trackersTable->setHorizontalHeaderLabels({"URL", "Tier", "Status"});
    m_trackersTable->horizontalHeader()->setStretchLastSection(true);
    m_trackersTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    m_trackersTable->verticalHeader()->hide();
    m_trackersTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_trackersTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_trackersTable->setShowGrid(false);
    m_trackersTable->setAlternatingRowColors(true);
    m_trackersTable->setStyleSheet(R"(
        QTableWidget {
            background-color: #141414;
            alternate-background-color: #181818;
            color: #b0b0b0;
            border: none;
        }
        QHeaderView::section {
            background-color: #1a1a1a;
            color: #c43030;
            border: none;
            border-bottom: 1px solid #c43030;
            padding: 4px;
            font-weight: bold;
        }
    )");

    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *addBtn = new QPushButton(tr_("tracker_add"));
    addBtn->setStyleSheet(
        "QPushButton { background-color: #2a1015; color: #d0d0d0; border: 1px solid #3d1520;"
        "border-radius: 4px; padding: 4px 12px; font-weight: bold; }"
        "QPushButton:hover { background-color: #6b2020; color: #ffffff; }");
    connect(addBtn, &QPushButton::clicked, this, &DetailsPanel::onAddTracker);
    btnLayout->addWidget(addBtn);

    layout->addWidget(m_trackersTable);
    layout->addLayout(btnLayout);
    return widget;
}

void DetailsPanel::retranslate()
{
    setTabText(0, tr_("detail_general"));
    setTabText(1, tr_("detail_peers"));
    setTabText(2, tr_("detail_files"));
    setTabText(3, tr_("detail_trackers"));

    m_peersTable->setHorizontalHeaderLabels({
        tr_("peer_ip"), tr_("peer_port"), tr_("peer_client"),
        tr_("peer_down"), tr_("peer_up"), tr_("peer_progress")
    });
    m_filesTable->setHorizontalHeaderLabels({
        tr_("file_name"), tr_("file_size"), tr_("file_progress"), tr_("file_priority")
    });
    m_trackersTable->setHorizontalHeaderLabels({
        tr_("tracker_url_col"), tr_("tracker_tier"), tr_("tracker_status")
    });
}
