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
    void onItemDoubleClicked(int row, int col);
    void onStreamDoubleClicked(int row, int col);

private:
    void switchToStreams();
    void switchToCatalog();

    QLineEdit *m_searchEdit;
    QTableWidget *m_catalogTable;
    QTableWidget *m_streamTable;
    QLabel *m_statusLabel;
    SessionManager *m_session;
    QString m_savePath;
    QList<CatalogItem> m_currentItems;
    QList<StreamResult> m_currentStreams;
};

#endif
