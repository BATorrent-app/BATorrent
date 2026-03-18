// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef DETAILSPANEL_H
#define DETAILSPANEL_H

#include <QTabWidget>

class QLabel;
class QTableWidget;
class QComboBox;
class SessionManager;

class DetailsPanel : public QTabWidget
{
    Q_OBJECT
public:
    explicit DetailsPanel(SessionManager *session, QWidget *parent = nullptr);

public slots:
    void showTorrent(int index);
    void refresh();
    void retranslate();

private slots:
    void onFilePriorityChanged(int row, int priority);
    void onAddTracker();

private:
    QWidget *createGeneralTab();
    QWidget *createPeersTab();
    QWidget *createFilesTab();
    QWidget *createTrackersTab();

    SessionManager *m_session;
    int m_currentIndex = -1;

    // General tab
    QLabel *m_nameLabel;
    QLabel *m_sizeLabel;
    QLabel *m_progressLabel;
    QLabel *m_downSpeedLabel;
    QLabel *m_upSpeedLabel;
    QLabel *m_stateLabel;
    QLabel *m_peersLabel;
    QLabel *m_downloadedLabel;
    QLabel *m_savePathLabel;
    QLabel *m_ratioLabel;

    // Peers tab
    QTableWidget *m_peersTable;

    // Files tab
    QTableWidget *m_filesTable;

    // Trackers tab
    QTableWidget *m_trackersTable;
};

#endif
