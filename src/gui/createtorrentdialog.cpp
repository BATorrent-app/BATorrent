#include "createtorrentdialog.h"
#include "../app/translator.h"
#include "../gui/thememanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QProgressBar>
#include <QMessageBox>
#include <QApplication>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/file_storage.hpp>
#include <fstream>

CreateTorrentDialog::CreateTorrentDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("create_title"));
    setMinimumSize(500, 350);
    setStyleSheet(ThemeManager::instance().dialogStyleSheet());

    QString labelStyle = ThemeManager::instance().formLabelStyle();

    auto *layout = new QFormLayout;
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // Source file/folder
    auto *sourceLayout = new QHBoxLayout;
    m_sourceEdit = new QLineEdit;
    auto *sourceBrowseBtn = new QPushButton(tr_("settings_browse"));
    sourceBrowseBtn->setFixedWidth(100);
    connect(sourceBrowseBtn, &QPushButton::clicked, this, &CreateTorrentDialog::browseSource);
    sourceLayout->addWidget(m_sourceEdit);
    sourceLayout->addWidget(sourceBrowseBtn);

    auto *sourceLabel = new QLabel(tr_("create_source"));
    sourceLabel->setStyleSheet(labelStyle);
    layout->addRow(sourceLabel, sourceLayout);

    // Output .torrent file
    auto *outputLayout = new QHBoxLayout;
    m_outputEdit = new QLineEdit;
    auto *outputBrowseBtn = new QPushButton(tr_("settings_browse"));
    outputBrowseBtn->setFixedWidth(100);
    connect(outputBrowseBtn, &QPushButton::clicked, this, &CreateTorrentDialog::browseOutput);
    outputLayout->addWidget(m_outputEdit);
    outputLayout->addWidget(outputBrowseBtn);

    auto *outputLabel = new QLabel(tr_("create_output"));
    outputLabel->setStyleSheet(labelStyle);
    layout->addRow(outputLabel, outputLayout);

    // Trackers (one per line)
    m_trackerEdit = new QTextEdit;
    m_trackerEdit->setMaximumHeight(60);
    m_trackerEdit->setPlaceholderText("https://tracker.example.com/announce");
    auto *trackerLabel = new QLabel(tr_("create_trackers"));
    trackerLabel->setStyleSheet(labelStyle);
    layout->addRow(trackerLabel, m_trackerEdit);

    // Comment
    m_commentEdit = new QLineEdit;
    auto *commentLabel = new QLabel(tr_("create_comment"));
    commentLabel->setStyleSheet(labelStyle);
    layout->addRow(commentLabel, m_commentEdit);

    // Piece size
    m_pieceSizeSpin = new QSpinBox;
    m_pieceSizeSpin->setRange(0, 16384);
    m_pieceSizeSpin->setSuffix(" KB");
    m_pieceSizeSpin->setSpecialValueText(tr_("create_auto"));
    auto *pieceLabel = new QLabel(tr_("create_piece_size"));
    pieceLabel->setStyleSheet(labelStyle);
    layout->addRow(pieceLabel, m_pieceSizeSpin);

    // Progress bar
    m_progressBar = new QProgressBar;
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setVisible(false);
    layout->addRow("", m_progressBar);

    // Buttons
    auto *btnLayout = new QHBoxLayout;
    btnLayout->addStretch();
    auto *createBtn = new QPushButton(tr_("create_btn"));
    auto *cancelBtn = new QPushButton(tr_("btn_cancel"));
    createBtn->setFixedWidth(120);
    cancelBtn->setFixedWidth(100);
    connect(createBtn, &QPushButton::clicked, this, &CreateTorrentDialog::createTorrent);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    btnLayout->addWidget(createBtn);
    btnLayout->addWidget(cancelBtn);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(layout);
    mainLayout->addLayout(btnLayout);
}

void CreateTorrentDialog::browseSource()
{
    // Let user pick file or folder
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
                                                 "Torrent Files (*.torrent)");
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

        // Add trackers
        QString trackers = m_trackerEdit->toPlainText();
        int tier = 0;
        for (const auto &line : trackers.split('\n', Qt::SkipEmptyParts)) {
            QString url = line.trimmed();
            if (!url.isEmpty()) {
                ct.add_tracker(url.toStdString(), tier++);
            }
        }

        // Comment
        if (!m_commentEdit->text().isEmpty()) {
            std::string comment = m_commentEdit->text().toStdString();
            ct.set_comment(comment.c_str());
        }

        ct.set_creator("BATorrent");

        m_progressBar->setValue(30);
        QApplication::processEvents();

        // Set piece hashes
        QString parentDir = QFileInfo(source).absolutePath();
        lt::set_piece_hashes(ct, parentDir.toStdString());

        m_progressBar->setValue(90);
        QApplication::processEvents();

        // Write to file
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
