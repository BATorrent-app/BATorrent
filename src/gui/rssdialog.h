// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef RSSDIALOG_H
#define RSSDIALOG_H

#include <QDialog>
#include "../app/rssmanager.h"

class QListWidget;
class QTableWidget;
class QLineEdit;
class QCheckBox;
class QSpinBox;
class QLabel;
class QPushButton;

class RssDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RssDialog(QWidget *parent = nullptr);

private slots:
    void addFeed();
    void removeFeed();
    void onFeedSelected(int row);
    void onFeedUpdated(int index, const QList<RssItem> &items);
    void onItemDoubleClicked(int row, int col);
    void saveCurrentFeedSettings();
    void refreshAllFeeds();

private:
    void refreshFeedList();
    void showFeedItems(int feedIndex);
    void updateFeedSettingsUI(int feedIndex);

    QListWidget *m_feedList;
    QTableWidget *m_itemsTable;
    QLineEdit *m_urlEdit;
    QLineEdit *m_filterEdit;
    QLineEdit *m_savePathEdit;
    QCheckBox *m_autoDownloadCheck;
    QCheckBox *m_enabledCheck;
    QSpinBox *m_intervalSpin;
    QLabel *m_statusLabel;
    QLabel *m_lastCheckedLabel;
    QPushButton *m_saveBtn;

    int m_selectedFeed = -1;
};

#endif
