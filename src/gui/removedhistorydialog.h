// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_REMOVEDHISTORYDIALOG_H
#define BATORRENT_REMOVEDHISTORYDIALOG_H

#include <QDialog>

class SessionManager;
class QListWidget;

// Lists the last 50 removed torrents and offers one-click re-add. Backed by
// SessionManager::recentlyRemoved() (resume files stashed under <AppData>/
// removed/ when removeTorrent() runs).
class RemovedHistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RemovedHistoryDialog(SessionManager *session, QWidget *parent = nullptr);

private slots:
    void refresh();
    void restoreSelected();
    void clearAll();

private:
    SessionManager *m_session;
    QListWidget *m_list;
};

#endif
