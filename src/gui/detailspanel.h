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
class GeoIpResolver;
class PieceMapWidget;

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
    QWidget *createPiecesTab();

    SessionManager *m_session;
    int m_currentIndex = -1;

    // General tab. Layout matches canvas/main.jsx MiniDetails: three columns
    // (Info / Transfer / Peers), each headed by a CAPS eyebrow, with KV rows
    // below. Speed is a single combined "↓ X  ↑ Y" string; Uploaded and ETA
    // are computed from existing TorrentInfo fields.
    QLabel *m_nameLabel;
    QLabel *m_savePathLabel;
    QLabel *m_sizeLabel;
    QLabel *m_hashLabel;
    QLabel *m_downloadedLabel;
    QLabel *m_uploadedLabel;
    QLabel *m_speedLabel;
    QLabel *m_etaLabel;
    QLabel *m_seedsLabel;
    QLabel *m_peersLabel;
    QLabel *m_ratioLabel;
    QLabel *m_stateLabel;

    // Peers tab
    QTableWidget *m_peersTable;
    GeoIpResolver *m_geoIp;

    // Files tab
    QTableWidget *m_filesTable;

    // Trackers tab
    QTableWidget *m_trackersTable;

    // Pieces tab
    PieceMapWidget *m_pieceMap;
};

#endif
