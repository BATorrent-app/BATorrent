// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_DIAGNOSTICSDIALOG_H
#define BATORRENT_DIAGNOSTICSDIALOG_H

#include <QDialog>

class SessionManager;
class QNetworkAccessManager;
class QLabel;
class QPushButton;

class DiagnosticsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit DiagnosticsDialog(SessionManager *session, QWidget *parent = nullptr);

private slots:
    void runHealthCheck();
    void runIpLeakTest();

private:
    SessionManager *m_session;
    QNetworkAccessManager *m_nam;
    QLabel *m_healthLabel;
    QLabel *m_ipLabel;
    QPushButton *m_ipBtn;
};

#endif
