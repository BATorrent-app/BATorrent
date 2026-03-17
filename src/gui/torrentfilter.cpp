#include "torrentfilter.h"
#include "torrentmodel.h"
#include "../core/translator.h"

TorrentFilter::TorrentFilter(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setSortRole(TorrentModel::SortRole);
}

void TorrentFilter::setStateFilter(const QString &state)
{
    beginFilterChange();
    m_stateFilter = state;
    endFilterChange();
}

void TorrentFilter::setNameFilter(const QString &text)
{
    beginFilterChange();
    m_nameFilter = text.toLower();
    endFilterChange();
}

bool TorrentFilter::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_stateFilter.isEmpty()) {
        QModelIndex idx = sourceModel()->index(sourceRow, 0, sourceParent);
        QString state = idx.data(TorrentModel::StateFilterRole).toString();
        if (m_stateFilter == "__active__") {
            // Active = downloading or seeding
            if (state != tr_("state_downloading") && state != tr_("state_seeding"))
                return false;
        } else if (state != m_stateFilter) {
            return false;
        }
    }

    if (!m_nameFilter.isEmpty()) {
        QModelIndex idx = sourceModel()->index(sourceRow, TorrentModel::Name, sourceParent);
        QString name = idx.data(Qt::DisplayRole).toString().toLower();
        if (!name.contains(m_nameFilter))
            return false;
    }

    return true;
}

bool TorrentFilter::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QVariant l = left.data(TorrentModel::SortRole);
    QVariant r = right.data(TorrentModel::SortRole);

    if (l.typeId() == QMetaType::LongLong)
        return l.toLongLong() < r.toLongLong();
    if (l.typeId() == QMetaType::Int)
        return l.toInt() < r.toInt();
    if (l.typeId() == QMetaType::Float || l.typeId() == QMetaType::Double)
        return l.toDouble() < r.toDouble();

    return l.toString() < r.toString();
}
