// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "addtorrentdialog.h"
#include "thememanager.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QPushButton>
#include <QStandardPaths>
#include <QStringList>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <functional>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/magnet_uri.hpp>

AddTorrentDialog::AddTorrentDialog(const QString &torrentFilePath,
                                    const QString &magnetUri,
                                    const QString &defaultSavePath,
                                    QWidget *parent)
    : QDialog(parent), m_filesTree(nullptr)
{
    setWindowTitle(tr_("add_torrent_title"));
    setMinimumSize(620, 460);
    setStyleSheet(ThemeManager::instance().dialogStyleSheet());

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    QString labelStyle = ThemeManager::instance().formLabelStyle();

    // ---- Save path row (always visible, at the top) ----
    auto *pathLayout = new QHBoxLayout;
    auto *pathLabel = new QLabel(tr_("add_torrent_save_to"));
    pathLabel->setStyleSheet(labelStyle);

    // Pre-fill with the caller's default save path; if that's empty, fall
    // back to the user's Downloads folder so the input is never blank.
    QString initial = defaultSavePath;
    if (initial.isEmpty())
        initial = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    m_savePathEdit = new QLineEdit(initial);

    auto *browseBtn = new QPushButton(tr_("settings_browse"));
    browseBtn->setFixedWidth(100);
    connect(browseBtn, &QPushButton::clicked, this, &AddTorrentDialog::browseSavePath);

    pathLayout->addWidget(m_savePathEdit);
    pathLayout->addWidget(browseBtn);

    auto *pathForm = new QFormLayout;
    pathForm->setSpacing(8);
    pathForm->addRow(pathLabel, pathLayout);
    mainLayout->addLayout(pathForm);

    // ---- Summary section (name / size / file count) ----
    QString summary;
    if (!torrentFilePath.isEmpty()) {
        try {
            lt::torrent_info ti(torrentFilePath.toStdString());
            qint64 size = ti.total_size();
            int files = ti.num_files();
            summary = QString("<b>%1</b><br>%2 — %3 %4")
                .arg(QString::fromStdString(ti.name()),
                     formatSize(size),
                     QString::number(files),
                     tr_("add_torrent_files"));
        } catch (...) {
            summary = tr_("add_torrent_invalid");
        }
    } else if (!magnetUri.isEmpty()) {
        try {
            lt::error_code ec;
            lt::add_torrent_params atp = lt::parse_magnet_uri(magnetUri.toStdString(), ec);
            QString name = QString::fromStdString(atp.name);
            if (name.isEmpty()) name = tr_("add_torrent_magnet_label");
            summary = QString("<b>%1</b><br>%2").arg(name, tr_("add_torrent_magnet_hint"));
        } catch (...) {
            summary = tr_("add_torrent_invalid");
        }
    }

    m_summaryLabel = new QLabel(summary);
    m_summaryLabel->setWordWrap(true);
    m_summaryLabel->setStyleSheet(QString("color: %1; padding: 4px 0;")
        .arg(ThemeManager::instance().textColor()));
    mainLayout->addWidget(m_summaryLabel);

    // ---- File tree (only when we have metadata, i.e. .torrent files) ----
    if (!torrentFilePath.isEmpty()) {
        m_filesTree = new QTreeWidget;
        m_filesTree->setHeaderLabels({tr_("add_torrent_col_name"),
                                       tr_("add_torrent_col_size")});
        m_filesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_filesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_filesTree->setAlternatingRowColors(true);
        // React to user toggling files / folders. We have to disconnect
        // ourselves during programmatic state changes to avoid infinite
        // recursion through itemChanged → setSubtreeChecked → itemChanged.
        connect(m_filesTree, &QTreeWidget::itemChanged, this,
            [this](QTreeWidgetItem *item, int column) {
                if (column != 0) return;
                QSignalBlocker blocker(m_filesTree);
                Qt::CheckState s = item->checkState(0);
                if (item->childCount() > 0 && s != Qt::PartiallyChecked)
                    setSubtreeChecked(item, s == Qt::Checked);
                if (item->parent())
                    refreshAncestorCheckStates(item->parent());
            });
        mainLayout->addWidget(m_filesTree, 1);
        populateFileTree(torrentFilePath);
    }

    // ---- Start-immediately checkbox ----
    m_startCheck = new QCheckBox(tr_("add_torrent_start_now"));
    m_startCheck->setChecked(true);
    mainLayout->addWidget(m_startCheck);

    // ---- Buttons ----
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

void AddTorrentDialog::populateFileTree(const QString &torrentFilePath)
{
    try {
        lt::torrent_info ti(torrentFilePath.toStdString());
        const auto &fs = ti.files();

        // Build a path → node map so siblings group under shared parents.
        // The root has no item; children of root sit at the top level of
        // the tree widget.
        QMap<QString, QTreeWidgetItem *> folders;
        QSignalBlocker blocker(m_filesTree);

        std::function<QTreeWidgetItem *(const QString &)> getFolder =
            [&](const QString &path) -> QTreeWidgetItem * {
            if (path.isEmpty()) return nullptr;
            auto it = folders.find(path);
            if (it != folders.end()) return it.value();
            // Recursively create the parent chain.
            int slash = path.lastIndexOf('/');
            QString parentPath = slash >= 0 ? path.left(slash) : QString();
            QString name = slash >= 0 ? path.mid(slash + 1) : path;
            QTreeWidgetItem *parent = getFolder(parentPath);
            auto *node = new QTreeWidgetItem;
            node->setText(0, name);
            node->setFlags(node->flags() | Qt::ItemIsUserCheckable
                           | Qt::ItemIsAutoTristate);
            node->setCheckState(0, Qt::Checked);
            if (parent) parent->addChild(node);
            else m_filesTree->addTopLevelItem(node);
            folders[path] = node;
            return node;
        };

        m_fileIndices.clear();
        m_fileIndices.reserve(static_cast<size_t>(fs.num_files()));

        for (lt::file_index_t i(0); i < fs.end_file(); ++i) {
            QString fullPath = QString::fromStdString(fs.file_path(i));
            // libtorrent gives the path including the torrent root folder
            // for multi-file torrents — that's what we want, so the tree
            // mirrors what the user will see on disk.
            int slash = fullPath.lastIndexOf('/');
            QString parentPath = slash >= 0 ? fullPath.left(slash) : QString();
            QString fileName = slash >= 0 ? fullPath.mid(slash + 1) : fullPath;

            QTreeWidgetItem *parent = getFolder(parentPath);
            auto *node = new QTreeWidgetItem;
            node->setText(0, fileName);
            node->setText(1, formatSize(fs.file_size(i)));
            node->setFlags(node->flags() | Qt::ItemIsUserCheckable);
            node->setCheckState(0, Qt::Checked);
            // Remember which torrent file_index this leaf corresponds to so
            // we can reconstruct the priority list later.
            node->setData(0, Qt::UserRole, static_cast<int>(i));
            if (parent) parent->addChild(node);
            else m_filesTree->addTopLevelItem(node);

            m_fileIndices.push_back(static_cast<int>(i));
        }

        m_filesTree->expandAll();
    } catch (...) {
        // The summary label already reported the parse failure; just leave
        // the tree empty so the dialog still works as a save-path picker.
    }
}

void AddTorrentDialog::setSubtreeChecked(QTreeWidgetItem *item, bool checked)
{
    item->setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
    for (int i = 0; i < item->childCount(); ++i)
        setSubtreeChecked(item->child(i), checked);
}

void AddTorrentDialog::refreshAncestorCheckStates(QTreeWidgetItem *item)
{
    while (item) {
        int checked = 0, total = 0;
        for (int i = 0; i < item->childCount(); ++i) {
            Qt::CheckState s = item->child(i)->checkState(0);
            ++total;
            if (s == Qt::Checked) ++checked;
            else if (s == Qt::PartiallyChecked) checked = -1; // partial wins
        }
        if (checked == -1 || (checked > 0 && checked < total))
            item->setCheckState(0, Qt::PartiallyChecked);
        else if (checked == total)
            item->setCheckState(0, Qt::Checked);
        else
            item->setCheckState(0, Qt::Unchecked);
        item = item->parent();
    }
}

QString AddTorrentDialog::savePath() const
{
    return m_savePathEdit->text().trimmed();
}

bool AddTorrentDialog::startImmediately() const
{
    return m_startCheck->isChecked();
}

std::vector<int> AddTorrentDialog::filePriorities() const
{
    std::vector<int> priorities;
    if (!m_filesTree) return priorities;

    // Walk the tree to find every leaf and read its check state. Build a
    // dense vector indexed by file_index so the caller can pass it straight
    // into add_torrent_params.file_priorities.
    int maxIndex = 0;
    for (int idx : m_fileIndices) if (idx > maxIndex) maxIndex = idx;
    priorities.assign(static_cast<size_t>(maxIndex + 1), 4); // 4 = default

    std::function<void(QTreeWidgetItem *)> walk = [&](QTreeWidgetItem *node) {
        if (node->childCount() == 0) {
            int idx = node->data(0, Qt::UserRole).toInt();
            // 0 = don't_download, 4 = default. We keep the distinction
            // intentionally minimal here; per-file low/high tweaking lives
            // in the Files tab post-add.
            priorities[static_cast<size_t>(idx)] =
                node->checkState(0) == Qt::Checked ? 4 : 0;
            return;
        }
        for (int i = 0; i < node->childCount(); ++i)
            walk(node->child(i));
    };
    for (int i = 0; i < m_filesTree->topLevelItemCount(); ++i)
        walk(m_filesTree->topLevelItem(i));
    return priorities;
}

void AddTorrentDialog::browseSavePath()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr_("dlg_choose_folder"), m_savePathEdit->text());
    if (!dir.isEmpty())
        m_savePathEdit->setText(dir);
}
