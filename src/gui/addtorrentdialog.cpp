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
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/magnet_uri.hpp>

AddTorrentDialog::AddTorrentDialog(const QString &torrentFilePath,
                                    const QString &magnetUri,
                                    const QString &defaultSavePath,
                                    QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("add_torrent_title"));
    setMinimumWidth(520);
    setStyleSheet(ThemeManager::instance().dialogStyleSheet());

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ---- Summary section (name / size / file count) ----
    // Built best-effort from libtorrent's parser — for a magnet without
    // metadata we can only show the display name from the URI.
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
    m_summaryLabel->setStyleSheet(QString("color: %1; padding: 8px;")
        .arg(ThemeManager::instance().textColor()));
    mainLayout->addWidget(m_summaryLabel);

    // ---- Save path row ----
    QString labelStyle = ThemeManager::instance().formLabelStyle();
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

    // ---- Options ----
    m_startCheck = new QCheckBox(tr_("add_torrent_start_now"));
    m_startCheck->setChecked(true);
    mainLayout->addWidget(m_startCheck);

    mainLayout->addStretch();

    // ---- Buttons ----
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

QString AddTorrentDialog::savePath() const
{
    return m_savePathEdit->text().trimmed();
}

bool AddTorrentDialog::startImmediately() const
{
    return m_startCheck->isChecked();
}

void AddTorrentDialog::browseSavePath()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr_("dlg_choose_folder"), m_savePathEdit->text());
    if (!dir.isEmpty())
        m_savePathEdit->setText(dir);
}
