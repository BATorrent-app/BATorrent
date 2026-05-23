// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "torrentinspectordialog.h"
#include "../app/translator.h"
#include "../app/utils.h"
#include "../torrent/sessionmanager.h"
#include "thememanager.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

#include <libtorrent/torrent_info.hpp>
#include <sstream>

TorrentInspectorDialog::TorrentInspectorDialog(const QString &filePath,
                                               SessionManager *session,
                                               QWidget *parent)
    : QDialog(parent), m_session(session), m_filePath(filePath)
{
    setWindowTitle(tr_("inspector_title"));
    setMinimumSize(640, 520);

    const auto &tm = ThemeManager::instance();
    setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
        "QLabel { background: transparent; color: %2; }"
        "QListWidget, QTabWidget::pane { background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 6px; }"
        "QListWidget::item { padding: 4px 8px; }"
        "QTabBar::tab { background: transparent; color: %5; padding: 8px 16px;"
        "  border: none; border-bottom: 2px solid transparent; font-weight: 600; }"
        "QTabBar::tab:selected { color: %2; border-bottom-color: %6; }"
        "QPushButton { background: transparent; color: %2; border: 1px solid %4;"
        "  border-radius: 6px; padding: 7px 16px; font-size: 11px; font-weight: 500; }"
        "QPushButton:hover { background: %3; }"
        "#primaryBtn { background: %6; color: #ffffff; border-color: %6; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.surfaceColor(),
              tm.borderColor(), tm.mutedColor(), tm.accentColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    // Try to parse — if it fails, show the error and bail with a close-only
    // dialog so the user knows the file is bad without crashing the app.
    std::shared_ptr<lt::torrent_info> ti;
    try {
        ti = std::make_shared<lt::torrent_info>(filePath.toStdString());
    } catch (const std::exception &e) {
        auto *err = new QLabel(tr_("inspector_parse_failed").arg(QString::fromUtf8(e.what())));
        err->setWordWrap(true);
        root->addWidget(err);
        auto *closeBtn = new QPushButton(tr_("welcome_close"));
        connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
        root->addWidget(closeBtn);
        return;
    }

    auto *title = new QLabel(QString::fromStdString(ti->name()));
    {
        QFont f; f.setPointSize(15); f.setWeight(QFont::Bold);
        title->setFont(f);
        title->setWordWrap(true);
    }
    root->addWidget(title);

    // Hash + size + piece info + creation date + comment in a small KV block.
    QString hash = QString::fromStdString(
        (std::ostringstream() << ti->info_hashes().get_best()).str());
    QString comment = QString::fromStdString(ti->comment());
    QString creator = QString::fromStdString(ti->creator());
    QDateTime created = QDateTime::fromSecsSinceEpoch(ti->creation_date());

    QString info;
    info += "<table cellspacing=4>";
    auto row = [&](const QString &k, const QString &v) {
        info += QStringLiteral("<tr><td><b>%1</b></td><td>%2</td></tr>")
                    .arg(k, v.toHtmlEscaped());
    };
    row(tr_("inspector_size"), formatSize(ti->total_size()));
    row(tr_("inspector_files"), QString::number(ti->num_files()));
    row(tr_("inspector_pieces"),
        QStringLiteral("%1 × %2").arg(ti->num_pieces()).arg(formatSize(ti->piece_length())));
    row(tr_("inspector_hash"), hash);
    if (!creator.isEmpty()) row(tr_("inspector_creator"), creator);
    if (ti->creation_date() > 0)
        row(tr_("inspector_created"), QLocale::system().toString(created, QLocale::ShortFormat));
    if (!comment.isEmpty()) row(tr_("inspector_comment"), comment);
    if (ti->priv()) row(tr_("inspector_private"), tr_("inspector_private_yes"));
    info += "</table>";

    auto *infoLabel = new QLabel(info);
    infoLabel->setTextFormat(Qt::RichText);
    infoLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(infoLabel);

    // Files + Trackers tabs
    auto *tabs = new QTabWidget;
    auto *filesList = new QListWidget;
    const auto &files = ti->files();
    for (int i = 0; i < ti->num_files(); ++i) {
        const auto fi = lt::file_index_t(i);
        filesList->addItem(QStringLiteral("%1   %2")
            .arg(QString::fromStdString(files.file_path(fi)),
                 formatSize(files.file_size(fi))));
    }
    tabs->addTab(filesList, tr_("inspector_tab_files"));

    auto *trackList = new QListWidget;
    for (const auto &t : ti->trackers())
        trackList->addItem(QString::fromStdString(t.url));
    if (trackList->count() == 0)
        trackList->addItem(tr_("inspector_no_trackers"));
    tabs->addTab(trackList, tr_("inspector_tab_trackers"));
    root->addWidget(tabs, 1);

    // Footer: Copy hash, Add now, Close
    auto *btnRow = new QHBoxLayout;
    auto *copyBtn = new QPushButton(tr_("ctx_copy_hash"));
    connect(copyBtn, &QPushButton::clicked, this, [hash, copyBtn]() {
        QApplication::clipboard()->setText(hash);
        copyBtn->setText(tr_("pairing_copied"));
    });
    btnRow->addWidget(copyBtn);
    btnRow->addStretch();
    auto *addBtn = new QPushButton(tr_("inspector_add_now"));
    addBtn->setObjectName(QStringLiteral("primaryBtn"));
    connect(addBtn, &QPushButton::clicked, this, [this]() {
        // Caller's MainWindow already handles the save-path UX via the
        // existing add path, so we just flag the request and accept.
        m_addRequested = true;
        accept();
    });
    btnRow->addWidget(addBtn);
    auto *closeBtn = new QPushButton(tr_("welcome_close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    btnRow->addWidget(closeBtn);
    root->addLayout(btnRow);
}
