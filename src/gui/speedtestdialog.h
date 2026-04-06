// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SPEEDTESTDIALOG_H
#define SPEEDTESTDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QElapsedTimer>

class QLabel;
class QPushButton;
class QProgressBar;

class SpeedTestDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SpeedTestDialog(QWidget *parent = nullptr);

private slots:
    void startTest();

private:
    void testPing();
    void testDownload();
    void testUpload();
    void showResults();
    void setPhase(const QString &text);

    QNetworkAccessManager *m_nam;
    QElapsedTimer m_timer;

    QLabel *m_phaseLabel;
    QLabel *m_pingLabel;
    QLabel *m_downloadLabel;
    QLabel *m_uploadLabel;
    QProgressBar *m_progressBar;
    QPushButton *m_startBtn;

    double m_pingMs = 0.0;
    double m_downloadMbps = 0.0;
    double m_uploadMbps = 0.0;
};

#endif
