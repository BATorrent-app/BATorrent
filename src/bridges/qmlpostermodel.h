// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLPOSTERMODEL_H
#define QMLPOSTERMODEL_H

#include "bridges/bridgecommon.h"

class QmlPosterModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        StateKeyRole,
        InfoHashRole,
        ProgressRole,
        PosterPathRole,
        MetaTitleRole,
        StateStringRole,
        StateDetailRole,
        DownSpeedRole,
        UpSpeedRole,
        SizeRole,
        CategoryRole,
        NumPeersRole,
        DownRateRole,
        UpRateRole,
        SizeBytesRole,
        NumSeedsRole,       // classic-view power columns
        RatioRole,
        AvailabilityRole,
        EtaRole,
        DownloadedRole,     // formatted total_wanted_done ("107 MB of 6.4 GB" cards)
        PlayableRole        // a video torrent with no .exe → offer in-tile Play
    };

    explicit QmlPosterModel(IEngine *session, MetadataResolver *resolver,
                            QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    void refresh();                         // periodic tick: volatile roles only
    void refreshFull();                     // explicit edits: all roles
    void removeRow(int index);              // index-aware delete (no full reset)
    void posterResolved(const QString &hash); // one row's poster/title only
    void moveRow(int from, int to);

private:
    void syncCount();                       // insert/remove rows to match session
    void emitRows(bool fullRoles);          // dataChanged over the whole list
    IEngine *m_session;
    MetadataResolver *m_resolver;
    int m_lastCount = 0;
};
class QmlTorrentFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit QmlTorrentFilterProxy(QObject *parent = nullptr);

    Q_INVOKABLE void setFilterState(const QString &state);
    Q_INVOKABLE void setCategoryFilter(const QString &category);
    Q_INVOKABLE void setSearchText(const QString &text);
    Q_INVOKABLE void setSortColumn(const QString &column, bool ascending);
    Q_INVOKABLE void clearSort();
    Q_INVOKABLE int mapToSource(int proxyRow) const;
    Q_INVOKABLE int mapFromSource(int sourceRow) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &l, const QModelIndex &r) const override;

private:
    QString m_filterState = QStringLiteral("all");
    QString m_categoryFilter;
    QString m_searchText;
    QString m_sortColumn;
};

#endif // QMLPOSTERMODEL_H
