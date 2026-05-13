// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef ADDTORRENTDIALOG_H
#define ADDTORRENTDIALOG_H

#include <QDialog>
#include <QString>
#include <vector>

class QLineEdit;
class QLabel;
class QCheckBox;
class QTreeWidget;
class QTreeWidgetItem;

// Pre-add confirmation dialog. Shown before a .torrent or magnet is actually
// handed to libtorrent so the user can:
//   - choose / change the save path (pre-filled with the user's last path),
//   - see (and uncheck) the files contained in the .torrent,
//   - decide whether the torrent starts paused.
class AddTorrentDialog : public QDialog
{
    Q_OBJECT
public:
    // Provide either torrentFilePath (path to a .torrent on disk) or
    // magnetUri (a "magnet:..." URI). At least one must be non-empty. For
    // magnets the file tree is empty because metadata isn't downloaded yet;
    // users adjust priorities later via the Files tab.
    explicit AddTorrentDialog(const QString &torrentFilePath,
                              const QString &magnetUri,
                              const QString &defaultSavePath,
                              QWidget *parent = nullptr);

    QString savePath() const;
    bool startImmediately() const;

    // One priority per file in the torrent, in file_index order. Empty when
    // the dialog was opened for a magnet (no metadata available, so the
    // tree wasn't populated). Skipped files get libtorrent's "dont_download"
    // priority (0); kept files get the default priority (4).
    std::vector<int> filePriorities() const;

private slots:
    void browseSavePath();

private:
    void populateFileTree(const QString &torrentFilePath);
    // Toggle propagation: ticking a folder ticks every descendant; ticking
    // a file updates its ancestor folder check states (tri-state).
    static void setSubtreeChecked(QTreeWidgetItem *item, bool checked);
    static void refreshAncestorCheckStates(QTreeWidgetItem *item);

    QLineEdit *m_savePathEdit;
    QCheckBox *m_startCheck;
    QLabel *m_summaryLabel;
    QTreeWidget *m_filesTree;        // null when the source is a magnet
    std::vector<int> m_fileIndices;  // tree leaf #N -> torrent file_index N
};

#endif
