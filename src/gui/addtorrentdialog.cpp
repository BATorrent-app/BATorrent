// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "addtorrentdialog.h"
#include "thememanager.h"
#include "../app/translator.h"
#include "../app/utils.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QFrame>
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

} // namespace

AddTorrentDialog::AddTorrentDialog(const QString &torrentFilePath,
                                    const QString &magnetUri,
                                    const QString &defaultSavePath,
                                    QWidget *parent)
    : QDialog(parent), m_filesTree(nullptr)
{
    setWindowTitle(tr_("add_torrent_title"));
    setMinimumSize(640, 560);
    resize(640, 560);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 %10,"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; color: %2; }"
        "QLineEdit {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 7px 10px; font-size: 11px;"
        "  selection-background-color: %5;"
        "}"
        "QLineEdit:focus { border-color: %5; }"
        "QTreeWidget {"
        "  background: %6; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  alternate-background-color: %3;"
        "  outline: none;"
        "}"
        "QTreeWidget::item { padding: 4px 2px; }"
        "QTreeWidget::item:selected { background: %7; color: %2; }"
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
        "  background-color: %3;"
        "}"
        "QCheckBox::indicator:checked { background-color: %5; border-color: %5; }"
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
              tm.borderColor(), tm.accentColor(), tm.panelColor(),
              tm.accentTintColor(), tm.dimColor(), tm.accentLightColor(),
              tm.accentTintForGradient(10)));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 24);
    root->setSpacing(0);

    // Header eyebrow + heading
    auto *eyebrow = new QLabel(tr_("add_torrent_title").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);
    root->addSpacing(6);

    auto *heading = new QLabel(tr_("add_torrent_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    root->addWidget(heading);
    root->addSpacing(18);

    // Summary card: torrent name + meta line (size, files)
    auto *summaryCard = new QFrame;
    summaryCard->setObjectName(QStringLiteral("summaryCard"));
    summaryCard->setStyleSheet(QString(
        "QFrame#summaryCard {"
        "  background: %1; border: none; border-radius: 8px;"
        "}"
        ).arg(tm.panelColor()));
    auto *summaryCol = new QVBoxLayout(summaryCard);
    summaryCol->setContentsMargins(16, 14, 16, 14);
    summaryCol->setSpacing(4);

    QString torrentName;
    QString metaLine;
    bool valid = true;

    if (!torrentFilePath.isEmpty()) {
        try {
            lt::torrent_info ti(torrentFilePath.toStdString());
            torrentName = QString::fromStdString(ti.name());
            metaLine = QString("%1 · %2 %3")
                .arg(formatSize(ti.total_size()))
                .arg(ti.num_files())
                .arg(tr_("add_torrent_files"));
        } catch (...) {
            valid = false;
            torrentName = tr_("add_torrent_invalid");
        }
    } else if (!magnetUri.isEmpty()) {
        try {
            lt::error_code ec;
            lt::add_torrent_params atp = lt::parse_magnet_uri(magnetUri.toStdString(), ec);
            torrentName = QString::fromStdString(atp.name);
            if (torrentName.isEmpty()) torrentName = tr_("add_torrent_magnet_label");
            metaLine = tr_("add_torrent_magnet_hint");
        } catch (...) {
            valid = false;
            torrentName = tr_("add_torrent_invalid");
        }
    }

    auto *nameLbl = new QLabel(torrentName);
    nameLbl->setWordWrap(true);
    {
        QFont f; f.setPointSize(12); f.setWeight(QFont::DemiBold);
        nameLbl->setFont(f);
        nameLbl->setStyleSheet(QString("color: %1;")
            .arg(valid ? tm.textColor() : tm.stateErrorColor()));
    }
    summaryCol->addWidget(nameLbl);

    if (!metaLine.isEmpty()) {
        auto *metaLbl = new QLabel(metaLine);
        QFont f("Menlo");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(9);
        metaLbl->setFont(f);
        metaLbl->setStyleSheet(QString("color: %1;").arg(tm.mutedColor()));
        summaryCol->addWidget(metaLbl);
    }

    m_summaryLabel = nameLbl;
    root->addWidget(summaryCard);
    root->addSpacing(18);

    // Save path section
    root->addWidget(makeEyebrow(tr_("add_torrent_save_to"), this));
    root->addSpacing(6);

    QString initial = defaultSavePath;
    if (initial.isEmpty())
        initial = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    m_savePathEdit = new QLineEdit(initial);
    {
        QFont f("Menlo");
        f.setStyleHint(QFont::Monospace);
        f.setPointSize(10);
        m_savePathEdit->setFont(f);
    }

    auto *browseBtn = new QPushButton(tr_("settings_browse"));
    browseBtn->setObjectName(QStringLiteral("ghostBtn"));
    browseBtn->setCursor(Qt::PointingHandCursor);
    connect(browseBtn, &QPushButton::clicked, this, &AddTorrentDialog::browseSavePath);

    auto *pathRow = new QHBoxLayout;
    pathRow->setSpacing(8);
    pathRow->addWidget(m_savePathEdit, 1);
    pathRow->addWidget(browseBtn);
    root->addLayout(pathRow);
    root->addSpacing(16);

    // File tree (only for .torrent files with bundled metadata)
    if (!torrentFilePath.isEmpty()) {
        root->addWidget(makeEyebrow(tr_("add_torrent_col_name"), this));
        root->addSpacing(6);

        m_filesTree = new QTreeWidget;
        m_filesTree->setHeaderLabels({tr_("add_torrent_col_name"),
                                       tr_("add_torrent_col_size")});
        m_filesTree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_filesTree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_filesTree->setAlternatingRowColors(true);
        m_filesTree->setRootIsDecorated(true);
        m_filesTree->setUniformRowHeights(true);
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
        root->addWidget(m_filesTree, 1);
        populateFileTree(torrentFilePath);
    } else {
        root->addStretch(1);
    }

    root->addSpacing(14);

    m_startCheck = new QCheckBox(tr_("add_torrent_start_now"));
    m_startCheck->setChecked(true);
    root->addWidget(m_startCheck);
    root->addSpacing(14);

    // Footer
    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->addStretch();

    auto *cancelBtn = new QPushButton(tr_("btn_cancel"));
    cancelBtn->setObjectName(QStringLiteral("ghostBtn"));
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footer->addWidget(cancelBtn);

    auto *okBtn = new QPushButton(tr_("add_torrent_add_btn"));
    okBtn->setObjectName(QStringLiteral("primaryBtn"));
    okBtn->setCursor(Qt::PointingHandCursor);
    okBtn->setDefault(true);
    okBtn->setEnabled(valid);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    footer->addWidget(okBtn);

    root->addLayout(footer);
}

void AddTorrentDialog::populateFileTree(const QString &torrentFilePath)
{
    try {
        lt::torrent_info ti(torrentFilePath.toStdString());
        const auto &fs = ti.files();

        QMap<QString, QTreeWidgetItem *> folders;
        QSignalBlocker blocker(m_filesTree);

        std::function<QTreeWidgetItem *(const QString &)> getFolder =
            [&](const QString &path) -> QTreeWidgetItem * {
            if (path.isEmpty()) return nullptr;
            auto it = folders.find(path);
            if (it != folders.end()) return it.value();
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
            int slash = fullPath.lastIndexOf('/');
            QString parentPath = slash >= 0 ? fullPath.left(slash) : QString();
            QString fileName = slash >= 0 ? fullPath.mid(slash + 1) : fullPath;

            QTreeWidgetItem *parent = getFolder(parentPath);
            auto *node = new QTreeWidgetItem;
            node->setText(0, fileName);
            node->setText(1, formatSize(fs.file_size(i)));
            node->setFlags(node->flags() | Qt::ItemIsUserCheckable);
            node->setCheckState(0, Qt::Checked);
            node->setData(0, Qt::UserRole, static_cast<int>(i));
            if (parent) parent->addChild(node);
            else m_filesTree->addTopLevelItem(node);

            m_fileIndices.push_back(static_cast<int>(i));
        }

        m_filesTree->expandAll();
    } catch (...) {
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
            else if (s == Qt::PartiallyChecked) checked = -1;
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

    int maxIndex = 0;
    for (int idx : m_fileIndices) if (idx > maxIndex) maxIndex = idx;
    priorities.assign(static_cast<size_t>(maxIndex + 1), 4);

    std::function<void(QTreeWidgetItem *)> walk = [&](QTreeWidgetItem *node) {
        if (node->childCount() == 0) {
            int idx = node->data(0, Qt::UserRole).toInt();
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
