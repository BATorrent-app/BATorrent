// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef TORRENTFILTER_H
#define TORRENTFILTER_H

#include <QSortFilterProxyModel>

class TorrentFilter : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit TorrentFilter(QObject *parent = nullptr);

    void setStateFilter(const QString &state);
    void setNameFilter(const QString &text);
    void setCategoryFilter(const QString &category);

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_stateFilter;
    QString m_nameFilter;
    QString m_categoryFilter;
};

#endif
