// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLLIBRARYMODEL_H
#define QMLLIBRARYMODEL_H

#include "bridges/librarygroups.h"

#include <QAbstractListModel>
#include <QPersistentModelIndex>
#include <QSet>
#include <QVariantList>

class QmlTorrentFilterProxy;

// What the library grid and list draw: the filtered, sorted torrents, with the
// episodes of one series folded into a single row that opens to show them.
// Sits on top of QmlTorrentFilterProxy and answers to the same calls, so the
// QML that knew the proxy as torrentFilter keeps working. Rows here are not
// proxy rows: a group row has no torrent of its own, which is why selection
// asks sourceRowsAt() instead of mapToSource().
class QmlLibraryModel : public QAbstractListModel
{
    Q_OBJECT
public:
    // Past every role QmlPosterModel defines, which this model passes through.
    enum Roles {
        IsGroupRole = Qt::UserRole + 500,
        GroupCountRole,
        GroupExpandedRole,
        GroupChildRole,
        GroupKeyRole,
        RowKeyRole
    };

    // persistKey: the QSettings key that keeps which groups are open across
    // restarts. Empty keeps it in memory only.
    explicit QmlLibraryModel(QmlTorrentFilterProxy *filter,
                             const QString &persistKey = QString(),
                             QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Passed through to the filter proxy.
    Q_INVOKABLE void setFilterState(const QString &state);
    Q_INVOKABLE void setCategoryFilter(const QString &category);
    Q_INVOKABLE void setSearchText(const QString &text);
    Q_INVOKABLE void setSortColumn(const QString &column, bool ascending);
    Q_INVOKABLE void clearSort();
    Q_INVOKABLE QVariantMap categoryCounts() const;

    // The session row behind a torrent row; -1 for a group row.
    Q_INVOKABLE int mapToSource(int row) const;
    // The row that shows a session row: its own, or its group's while the
    // group is closed. -1 when the filter hides it.
    Q_INVOKABLE int mapFromSource(int sourceRow) const;
    // Every session row a row stands for: one for a torrent, all of its
    // episodes for a group. What an action on the row applies to.
    Q_INVOKABLE QVariantList sourceRowsAt(int row) const;
    Q_INVOKABLE bool isGroup(int row) const;
    Q_INVOKABLE bool isGroupExpanded(int row) const;
    Q_INVOKABLE void toggleGroup(int row);
    Q_INVOKABLE void setGroupExpanded(int row, bool expanded);
    // Opens the group a session row sits in, if it is closed, and returns the
    // row that now shows it.
    Q_INVOKABLE int revealSource(int sourceRow);
    // A name for a row that survives sorting, filtering and groups opening, so
    // a selection can follow its rows when they move.
    Q_INVOKABLE QString rowKey(int row) const;
    Q_INVOKABLE int rowForKey(const QString &key) const;

signals:
    // Rows were added, removed or moved. Row numbers held from before are stale.
    void structureChanged();

private:
    struct Row {
        QString key;
        QString groupKey;
        QList<QPersistentModelIndex> members;   // filter-proxy rows
        bool group = false;
        bool child = false;
    };

    void rebuild();
    QVector<Row> computeRows() const;
    void setExpanded(const QString &groupKey, bool expanded);
    QVariant groupData(const Row &row, int role) const;
    int groupRowOf(int proxyRow) const;

    QmlTorrentFilterProxy *m_filter;
    QString m_persistKey;
    QVector<Row> m_rows;
    QSet<QString> m_expanded;
};

#endif // QMLLIBRARYMODEL_H
