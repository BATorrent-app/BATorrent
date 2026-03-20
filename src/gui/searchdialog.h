// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SEARCHDIALOG_H
#define SEARCHDIALOG_H

#include <QDialog>
#include "../app/addonmanager.h"

class QLineEdit;
class QTableWidget;
class QLabel;
class QComboBox;
class SessionManager;

class SearchDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SearchDialog(SessionManager *session, const QString &savePath, QWidget *parent = nullptr);

private slots:
    void performSearch();
    void showCatalogResults(const QList<CatalogItem> &items);
    void showStreamResults(const QList<StreamResult> &streams);
    void showTorrentResults(const QList<TorrentSearchResult> &results);
    void onItemDoubleClicked(int row, int col);
    void onStreamDoubleClicked(int row, int col);
    void onTorrentDoubleClicked(int row, int col);
    void onSourceChanged(int index);

private:
    void switchToStreams();
    void switchToCatalog();
    void switchToTorrentResults();

    QComboBox *m_sourceCombo;
    QComboBox *m_categoryCombo;
    QLineEdit *m_searchEdit;
    QTableWidget *m_catalogTable;
    QTableWidget *m_streamTable;
    QTableWidget *m_torrentTable;
    QLabel *m_statusLabel;
    SessionManager *m_session;
    QString m_savePath;
    QPushButton *m_backBtn;
    QList<CatalogItem> m_currentItems;
    QList<StreamResult> m_currentStreams;
    QList<TorrentSearchResult> m_torrentResults;
};

#endif
