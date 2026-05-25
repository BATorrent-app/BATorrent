// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "logviewerdialog.h"
#include "../app/logger.h"
#include "../app/translator.h"
#include "thememanager.h"

#include <QComboBox>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

LogViewerDialog::LogViewerDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr_("logviewer_title"));
    resize(900, 600);

    const auto &tm = ThemeManager::instance();
    setStyleSheet(QString(
        "QDialog { background: %1; color: %2; }"
        "QLabel { background: transparent; color: %2; }"
        "QPlainTextEdit { background: %3; color: %2; border: 1px solid %4;"
        "  font-family: Menlo, Consolas, monospace; font-size: 11px; padding: 8px; }"
        "QComboBox { background: %3; color: %2; border: 1px solid %4;"
        "  border-radius: 6px; padding: 5px 10px; font-size: 11px; }"
        "QPushButton { background: transparent; color: %2; border: 1px solid %4;"
        "  border-radius: 6px; padding: 7px 16px; font-size: 11px; font-weight: 500; }"
        "QPushButton:hover { background: %3; }"
        "#primaryBtn { background: %5; color: #ffffff; border-color: %5; }"
        "#primaryBtn:hover { background: %6; border-color: %6; }"
        "#dangerBtn { color: %5; }"
        "#dangerBtn:hover { background: %3; color: %6; }"
        ).arg(tm.bgColor(), tm.textColor(), tm.surfaceColor(),
              tm.borderColor(), tm.accentColor(), tm.accentLightColor()));

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 20);
    root->setSpacing(12);

    // Header row: title + level filter
    auto *header = new QHBoxLayout;
    auto *title = new QLabel(tr_("logviewer_title"));
    {
        QFont f; f.setPointSize(13); f.setWeight(QFont::Bold);
        title->setFont(f);
    }
    header->addWidget(title);
    header->addStretch();
    header->addWidget(new QLabel(tr_("logviewer_level") + ":"));
    m_levelFilter = new QComboBox;
    m_levelFilter->addItem(QStringLiteral("Trace"),   int(Logger::Trace));
    m_levelFilter->addItem(QStringLiteral("Debug"),   int(Logger::Debug));
    m_levelFilter->addItem(QStringLiteral("Info"),    int(Logger::Info));
    m_levelFilter->addItem(QStringLiteral("Warning"), int(Logger::Warning));
    m_levelFilter->addItem(QStringLiteral("Error"),   int(Logger::Error));
    m_levelFilter->setCurrentIndex(m_levelFilter->findData(int(Logger::instance().level())));
    connect(m_levelFilter, &QComboBox::currentIndexChanged, this, [this]() {
        Logger::instance().setLevel(static_cast<Logger::Level>(
            m_levelFilter->currentData().toInt()));
        m_lastSize = 0;
        refresh();
    });
    header->addWidget(m_levelFilter);
    root->addLayout(header);

    m_view = new QPlainTextEdit;
    m_view->setReadOnly(true);
    m_view->setMaximumBlockCount(20000); // show more history (includes previous sessions)
    root->addWidget(m_view, 1);

    // Bottom row: actions
    auto *bottom = new QHBoxLayout;
    auto *folderBtn = new QPushButton(tr_("logviewer_open_folder"));
    connect(folderBtn, &QPushButton::clicked, this, &LogViewerDialog::openLogsFolder);
    bottom->addWidget(folderBtn);
    auto *saveBtn = new QPushButton(tr_("logviewer_save"));
    saveBtn->setObjectName(QStringLiteral("primaryBtn"));
    connect(saveBtn, &QPushButton::clicked, this, &LogViewerDialog::saveLogs);
    bottom->addWidget(saveBtn);
    auto *clearBtn = new QPushButton(tr_("logviewer_clear"));
    clearBtn->setObjectName(QStringLiteral("dangerBtn"));
    connect(clearBtn, &QPushButton::clicked, this, &LogViewerDialog::clearLogs);
    bottom->addWidget(clearBtn);
    bottom->addStretch();
    auto *closeBtn = new QPushButton(tr_("welcome_close"));
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::accept);
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);

    refresh();

    // Live tail: re-read every 1 s. Cheap because we only read the delta.
    m_pollTimer = new QTimer(this);
    connect(m_pollTimer, &QTimer::timeout, this, &LogViewerDialog::refresh);
    m_pollTimer->start(1000);
}

void LogViewerDialog::refresh()
{
    const QString path = Logger::instance().currentLogPath();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    const qint64 size = f.size();
    if (size == m_lastSize) return;
    if (size < m_lastSize) {
        // File shrunk (rotation / clear) — reread from scratch.
        m_view->clear();
        m_lastSize = 0;
    }
    f.seek(m_lastSize);
    const QString delta = QString::fromUtf8(f.readAll());
    m_lastSize = size;

    // Append line by line so QPlainTextEdit's max-block cap kicks in.
    QScrollBar *bar = m_view->verticalScrollBar();
    const bool atBottom = bar->value() == bar->maximum();
    for (const QString &line : delta.split('\n', Qt::SkipEmptyParts))
        m_view->appendPlainText(line);
    if (atBottom)
        bar->setValue(bar->maximum());
}

void LogViewerDialog::openLogsFolder()
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(Logger::instance().logsDir()));
}

void LogViewerDialog::saveLogs()
{
    const QString defaultName =
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)
        + "/batorrent-logs-"
        + QDateTime::currentDateTime().toString("yyyy-MM-dd-HHmmss") + ".txt";
    const QString path = QFileDialog::getSaveFileName(this,
        tr_("logviewer_save"), defaultName, QStringLiteral("Text (*.txt)"));
    if (path.isEmpty()) return;
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr_("logviewer_save"),
            tr_("logviewer_save_failed"));
        return;
    }
    QTextStream out(&f);
    out << Logger::instance().readAllLogs();
}

void LogViewerDialog::clearLogs()
{
    auto reply = QMessageBox::question(this, tr_("logviewer_clear"),
        tr_("logviewer_clear_confirm"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;
    Logger::instance().clear();
    m_view->clear();
    m_lastSize = 0;
    refresh();
}
