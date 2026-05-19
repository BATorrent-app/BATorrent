// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "createtorrentdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include <QApplication>
#include <QCheckBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QStringList>
#include <QTextEdit>
#include <QVBoxLayout>
#include <fstream>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/torrent_info.hpp>

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

CreateTorrentDialog::CreateTorrentDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("create_title"));
    setMinimumSize(620, 540);
    resize(680, 600);

    const auto &tm = ThemeManager::instance();

    setStyleSheet(QString(
        "QDialog {"
        "  background: qradialgradient(cx:0.5, cy:0, radius:0.7,"
        "      stop:0 rgba(220,38,38,0.10),"
        "      stop:1 %1);"
        "  color: %2;"
        "}"
        "QLabel { background: transparent; color: %2; }"
        "QLineEdit, QPlainTextEdit, QTextEdit, QComboBox, QSpinBox {"
        "  background: %3; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 7px 10px; font-size: 11px;"
        "  selection-background-color: %5;"
        "}"
        "QLineEdit:focus, QPlainTextEdit:focus, QTextEdit:focus, QComboBox:focus, QSpinBox:focus {"
        "  border-color: %5;"
        "}"
        "QCheckBox { color: %2; spacing: 8px; font-size: 11px; }"
        "QCheckBox::indicator {"
        "  width: 14px; height: 14px;"
        "  border: 1px solid %4; border-radius: 4px;"
        "  background: %3;"
        "}"
        "QCheckBox::indicator:checked { background: %5; border-color: %5; }"
        "QProgressBar {"
        "  background: %3; border: none; border-radius: 4px;"
        "  height: 6px; text-align: center;"
        "}"
        "QProgressBar::chunk { background: %5; border-radius: 4px; }"
        "#primaryBtn {"
        "  background: %5; color: #ffffff;"
        "  border: none; border-radius: 6px;"
        "  padding: 8px 22px; font-size: 11px; font-weight: 600;"
        "}"
        "#primaryBtn:hover { background: %6; }"
        "#ghostBtn {"
        "  background: transparent; color: %2;"
        "  border: 1px solid %4; border-radius: 6px;"
        "  padding: 8px 18px; font-size: 11px; font-weight: 500;"
        "}"
        "#ghostBtn:hover { background: %3; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.surfaceColor(),
              tm.borderColor(), tm.accentColor(), tm.accentLightColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(32, 28, 32, 24);
    root->setSpacing(0);

    auto *eyebrow = new QLabel(tr_("create_title").toUpper());
    {
        QFont f; f.setPointSize(8); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, 1.2);
        eyebrow->setFont(f);
        eyebrow->setStyleSheet(QString("color: %1;").arg(tm.accentColor()));
    }
    root->addWidget(eyebrow);
    root->addSpacing(6);

    auto *heading = new QLabel(tr_("create_heading"));
    {
        QFont f; f.setPointSize(18); f.setWeight(QFont::Bold);
        f.setLetterSpacing(QFont::AbsoluteSpacing, -0.3);
        heading->setFont(f);
        heading->setStyleSheet(QString("color: %1;").arg(tm.textColor()));
    }
    root->addWidget(heading);
    root->addSpacing(20);

    QFont mono("Menlo");
    mono.setStyleHint(QFont::Monospace);
    mono.setPointSize(10);

    root->addWidget(makeEyebrow(tr_("create_source"), this));
    root->addSpacing(6);

    m_sourceEdit = new QLineEdit;
    m_sourceEdit->setFont(mono);
    m_sourceEdit->setPlaceholderText(tr_("create_source_placeholder"));

    auto *sourceBrowseBtn = new QPushButton(tr_("settings_browse"));
    sourceBrowseBtn->setObjectName(QStringLiteral("ghostBtn"));
    sourceBrowseBtn->setCursor(Qt::PointingHandCursor);
    connect(sourceBrowseBtn, &QPushButton::clicked, this, &CreateTorrentDialog::browseSource);

    auto *sourceRow = new QHBoxLayout;
    sourceRow->setSpacing(8);
    sourceRow->addWidget(m_sourceEdit, 1);
    sourceRow->addWidget(sourceBrowseBtn);
    root->addLayout(sourceRow);
    root->addSpacing(16);

    root->addWidget(makeEyebrow(tr_("create_output"), this));
    root->addSpacing(6);

    m_outputEdit = new QLineEdit;
    m_outputEdit->setFont(mono);
    m_outputEdit->setPlaceholderText(tr_("create_output_placeholder"));

    auto *outputBrowseBtn = new QPushButton(tr_("settings_browse"));
    outputBrowseBtn->setObjectName(QStringLiteral("ghostBtn"));
    outputBrowseBtn->setCursor(Qt::PointingHandCursor);
    connect(outputBrowseBtn, &QPushButton::clicked, this, &CreateTorrentDialog::browseOutput);

    auto *outputRow = new QHBoxLayout;
    outputRow->setSpacing(8);
    outputRow->addWidget(m_outputEdit, 1);
    outputRow->addWidget(outputBrowseBtn);
    root->addLayout(outputRow);
    root->addSpacing(16);

    root->addWidget(makeEyebrow(tr_("create_trackers"), this));
    root->addSpacing(6);

    m_trackerEdit = new QTextEdit;
    m_trackerEdit->setFont(mono);
    m_trackerEdit->setPlaceholderText(QStringLiteral("https://tracker.example.com/announce"));
    m_trackerEdit->setMinimumHeight(80);
    m_trackerEdit->setMaximumHeight(110);
    root->addWidget(m_trackerEdit);
    root->addSpacing(16);

    root->addWidget(makeEyebrow(tr_("create_options"), this));
    root->addSpacing(8);

    m_commentEdit = new QLineEdit;
    m_commentEdit->setPlaceholderText(tr_("create_comment"));
    root->addWidget(m_commentEdit);
    root->addSpacing(10);

    auto *pieceRow = new QHBoxLayout;
    pieceRow->setSpacing(10);
    auto *pieceLbl = new QLabel(tr_("create_piece_size"));
    pieceLbl->setStyleSheet(QString("color: %1; font-size: 11px;").arg(tm.mutedColor()));
    m_pieceSizeSpin = new QSpinBox;
    m_pieceSizeSpin->setRange(0, 16384);
    m_pieceSizeSpin->setSuffix(QStringLiteral(" KB"));
    m_pieceSizeSpin->setSpecialValueText(tr_("create_auto"));
    m_pieceSizeSpin->setMinimumWidth(140);
    pieceRow->addWidget(pieceLbl);
    pieceRow->addWidget(m_pieceSizeSpin);
    pieceRow->addStretch();
    root->addLayout(pieceRow);
    root->addSpacing(14);

    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(false);
    m_progressBar->setVisible(false);
    root->addWidget(m_progressBar);

    root->addStretch(1);
    root->addSpacing(14);

    auto *footer = new QHBoxLayout;
    footer->setSpacing(8);
    footer->addStretch();

    auto *cancelBtn = new QPushButton(tr_("btn_cancel"));
    cancelBtn->setObjectName(QStringLiteral("ghostBtn"));
    cancelBtn->setCursor(Qt::PointingHandCursor);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    footer->addWidget(cancelBtn);

    auto *createBtn = new QPushButton(tr_("create_btn"));
    createBtn->setObjectName(QStringLiteral("primaryBtn"));
    createBtn->setCursor(Qt::PointingHandCursor);
    createBtn->setDefault(true);
    connect(createBtn, &QPushButton::clicked, this, &CreateTorrentDialog::createTorrent);
    footer->addWidget(createBtn);

    root->addLayout(footer);
}

void CreateTorrentDialog::browseSource()
{
    QString path = QFileDialog::getExistingDirectory(this, tr_("create_select_source"));
    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(this, tr_("create_select_source"));
    }
    if (!path.isEmpty()) {
        m_sourceEdit->setText(path);
        if (m_outputEdit->text().isEmpty()) {
            m_outputEdit->setText(path + ".torrent");
        }
    }
}

void CreateTorrentDialog::browseOutput()
{
    QString path = QFileDialog::getSaveFileName(this, tr_("create_output"),
                                                 m_outputEdit->text(),
                                                 QStringLiteral("Torrent Files (*.torrent)"));
    if (!path.isEmpty())
        m_outputEdit->setText(path);
}

void CreateTorrentDialog::createTorrent()
{
    QString source = m_sourceEdit->text();
    QString output = m_outputEdit->text();

    if (source.isEmpty() || output.isEmpty()) {
        QMessageBox::warning(this, tr_("dlg_error"), tr_("create_err_empty"));
        return;
    }

    m_progressBar->setVisible(true);
    m_progressBar->setValue(10);
    QApplication::processEvents();

    try {
        lt::file_storage fs;
        lt::add_files(fs, source.toStdString());

        if (fs.num_files() == 0) {
            QMessageBox::warning(this, tr_("dlg_error"), tr_("create_err_no_files"));
            m_progressBar->setVisible(false);
            return;
        }

        int pieceSize = m_pieceSizeSpin->value() * 1024;
        lt::create_torrent ct(fs, pieceSize > 0 ? pieceSize : 0);

        QString trackers = m_trackerEdit->toPlainText();
        int tier = 0;
        for (const auto &line : trackers.split('\n', Qt::SkipEmptyParts)) {
            QString url = line.trimmed();
            if (!url.isEmpty()) {
                ct.add_tracker(url.toStdString(), tier++);
            }
        }

        if (!m_commentEdit->text().isEmpty()) {
            std::string comment = m_commentEdit->text().toStdString();
            ct.set_comment(comment.c_str());
        }

        ct.set_creator("BATorrent");

        m_progressBar->setValue(30);
        QApplication::processEvents();

        QString parentDir = QFileInfo(source).absolutePath();
        lt::set_piece_hashes(ct, parentDir.toStdString());

        m_progressBar->setValue(90);
        QApplication::processEvents();

        auto entry = ct.generate();
        std::vector<char> buf;
        lt::bencode(std::back_inserter(buf), entry);

        std::ofstream out(output.toStdString(), std::ios::binary);
        out.write(buf.data(), static_cast<std::streamsize>(buf.size()));
        out.close();

        m_progressBar->setValue(100);

        QMessageBox::information(this, tr_("create_title"), tr_("create_success"));
        accept();

    } catch (const std::exception &e) {
        m_progressBar->setVisible(false);
        QMessageBox::critical(this, tr_("dlg_error"), QString::fromStdString(e.what()));
    }
}

QString CreateTorrentDialog::sourcePath() const { return m_sourceEdit->text(); }
QString CreateTorrentDialog::outputPath() const { return m_outputEdit->text(); }
QString CreateTorrentDialog::trackerUrl() const { return m_trackerEdit->toPlainText(); }
QString CreateTorrentDialog::comment() const { return m_commentEdit->text(); }
int CreateTorrentDialog::pieceSize() const { return m_pieceSizeSpin->value(); }
