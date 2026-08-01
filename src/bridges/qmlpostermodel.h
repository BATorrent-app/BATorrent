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
        PlayableRole,       // a video torrent with no .exe → offer in-tile Play
        YearRole,           // TMDB release year (0 if unknown) — poster subtitle
        GenresRole,         // top genres, ", "-joined (empty if unknown)
        QueuePosRole,       // 1-based position among queued torrents (0 if not queued)
        // The preset category a torrent falls into on its own ("Movies",
        // "Series", "Games", "Apps", or empty when unknown). The manual
        // category still wins where the user set one; this is what makes the
        // filter useful before anyone has tagged anything by hand.
        AutoCategoryRole,
        // Uppercase extension of the biggest file ("ISO", "MKV", "ZIP"), for the
        // typographic cover a torrent with no artwork gets. Empty while a magnet
        // still has no file list.
        FileKindRole
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
    // PlayableRole needs the file list, and filesAt() is a blocking sync_call
    // into the libtorrent thread. Asking once per row per refresh froze the UI
    // for seconds whenever that thread was busy (peer crypto saturates it). The
    // file list is fixed once metadata lands, so answer from here after the
    // first look. Keyed by info-hash, not row: rows move.
    mutable QHash<QString, bool> m_playableCache;
    // Same reason as above: the file list costs a blocking hop into the
    // libtorrent thread, and it stops changing once metadata lands.
    mutable QHash<QString, QString> m_fileKindCache;
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
