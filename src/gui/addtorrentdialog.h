// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ADDTORRENTDIALOG_H
#define ADDTORRENTDIALOG_H

#include <QDialog>
#include <QString>

class QLineEdit;
class QLabel;
class QCheckBox;

// Pre-add confirmation dialog. Shown before a .torrent or magnet is actually
// handed to libtorrent so the user can choose / change the save path, see
// what the torrent contains (when metadata is available), and decide
// whether to start it immediately.
class AddTorrentDialog : public QDialog
{
    Q_OBJECT
public:
    // Provide either torrentFilePath (a path to a .torrent file) or
    // magnetUri (a "magnet:..." URI). At least one must be non-empty.
    explicit AddTorrentDialog(const QString &torrentFilePath,
                              const QString &magnetUri,
                              const QString &defaultSavePath,
                              QWidget *parent = nullptr);

    QString savePath() const;
    bool startImmediately() const;

private slots:
    void browseSavePath();

private:
    QLineEdit *m_savePathEdit;
    QCheckBox *m_startCheck;
    QLabel *m_summaryLabel;
};

#endif
