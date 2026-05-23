// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_LOGVIEWERDIALOG_H
#define BATORRENT_LOGVIEWERDIALOG_H

#include <QDialog>

class QPlainTextEdit;
class QComboBox;
class QTimer;

class LogViewerDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LogViewerDialog(QWidget *parent = nullptr);

private slots:
    void refresh();
    void openLogsFolder();
    void saveLogs();
    void clearLogs();

private:
    QPlainTextEdit *m_view;
    QComboBox *m_levelFilter;
    QTimer *m_pollTimer;
    qint64 m_lastSize = 0;
};

#endif
