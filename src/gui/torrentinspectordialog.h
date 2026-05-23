// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef BATORRENT_TORRENTINSPECTORDIALOG_H
#define BATORRENT_TORRENTINSPECTORDIALOG_H

#include <QDialog>
#include <QString>

class SessionManager;

// Parses a .torrent file with libtorrent and shows its metadata without
// adding the torrent. Useful when the user wants to peek at name, file
// count, total size, hash, and tracker list before committing to download.
// Includes an "Add now" shortcut that promotes the inspection to a real
// add via SessionManager.
class TorrentInspectorDialog : public QDialog
{
    Q_OBJECT
public:
    TorrentInspectorDialog(const QString &filePath, SessionManager *session,
                           QWidget *parent = nullptr);

    // True if the user clicked "Add now" — caller picks the save path and
    // forwards to the existing add flow.
    bool addRequested() const { return m_addRequested; }

private:
    SessionManager *m_session;
    QString m_filePath;
    bool m_addRequested = false;
};

#endif
