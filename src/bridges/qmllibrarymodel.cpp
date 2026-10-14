// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/qmllibrarymodel.h"
#include "bridges/qmlpostermodel.h"
#include "services/platform/utils.h"

#include <QSettings>
#include <algorithm>

namespace {

using PM = QmlPosterModel;

// Roles that decide which group a torrent falls in. A change to any of them
// can move rows between groups; anything else only redraws.
const QList<int> &groupingRoles()
{
    static const QList<int> roles = {PM::NameRole, PM::ShowTmdbRole, PM::ShowKeyRole,
                                     PM::SeasonRole, PM::EpisodeRole};
    return roles;
}

// The one state a group shows for all of its episodes: whatever needs the
// user most. A broken episode outranks a downloading one, and a paused one
// outranks the finished ones, so nothing that needs a hand hides behind the rest.
int stateRank(const QString &key)
{
    static const QStringList order = {
        QStringLiteral("missing"), QStringLiteral("error"), QStringLiteral("downloading"),
        QStringLiteral("fetching"), QStringLiteral("queued"), QStringLiteral("seeding"),
        QStringLiteral("paused"), QStringLiteral("completed")};
    const int i = order.indexOf(key);
    return i < 0 ? order.size() : i;
}

// Same rules as the per-torrent ETA in QmlPosterModel, over the whole group.
QString formatGroupEta(qint64 remaining, qint64 rate)
{
    if (remaining <= 0) return {};
    if (rate <= 0) return QStringLiteral("∞");
    qint64 secs = remaining / rate;
    if (secs >= 100 * 24 * 3600) return QStringLiteral("∞");
    const qint64 d = secs / 86400; secs %= 86400;
    const qint64 h = secs / 3600;  secs %= 3600;
    const qint64 m = secs / 60;    secs %= 60;
    if (d > 0) return QStringLiteral("%1d %2h").arg(d).arg(h);
    if (h > 0) return QStringLiteral("%1h %2m").arg(h).arg(m);
    if (m > 0) return QStringLiteral("%1m %2s").arg(m).arg(secs);
    return QStringLiteral("%1s").arg(secs);
}

} // namespace

QmlLibraryModel::QmlLibraryModel(QmlTorrentFilterProxy *filter, const QString &persistKey,
                                 QObject *parent)
    : QAbstractListModel(parent), m_filter(filter), m_persistKey(persistKey)
{
    if (!m_persistKey.isEmpty()) {
        const QStringList open = QSettings("BATorrent", "BATorrent").value(m_persistKey).toStringList();
        m_expanded = QSet<QString>(open.begin(), open.end());
    }

    auto regroup = [this] { rebuild(); };
    connect(m_filter, &QAbstractItemModel::rowsInserted, this, regroup);
    connect(m_filter, &QAbstractItemModel::rowsRemoved, this, regroup);
    connect(m_filter, &QAbstractItemModel::rowsMoved, this, regroup);
    connect(m_filter, &QAbstractItemModel::layoutChanged, this, regroup);
    connect(m_filter, &QAbstractItemModel::modelReset, this, regroup);
    connect(m_filter, &QAbstractItemModel::dataChanged, this,
            [this](const QModelIndex &tl, const QModelIndex &br, const QList<int> &roles) {
        bool regroupNeeded = roles.isEmpty();
        for (int r : roles)
            if (groupingRoles().contains(r)) { regroupNeeded = true; break; }
        if (regroupNeeded)
            rebuild();
        // Redraw the rows that show the changed torrents, and the groups that
        // add them up.
        int lo = -1, hi = -1;
        for (int i = 0; i < m_rows.size(); ++i) {
            for (const QPersistentModelIndex &m : m_rows[i].members) {
                if (m.isValid() && m.row() >= tl.row() && m.row() <= br.row()) {
                    if (lo < 0) lo = i;
                    hi = i;
                    break;
                }
            }
        }
        if (lo >= 0)
            emit dataChanged(index(lo), index(hi), roles);
    });

    rebuild();
}

int QmlLibraryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QHash<int, QByteArray> QmlLibraryModel::roleNames() const
{
    QHash<int, QByteArray> names = m_filter->roleNames();
    names.insert(IsGroupRole, "isGroup");
    names.insert(GroupCountRole, "groupCount");
    names.insert(GroupExpandedRole, "groupExpanded");
    names.insert(GroupChildRole, "groupChild");
    names.insert(GroupKeyRole, "groupKey");
    names.insert(RowKeyRole, "rowKey");
    return names;
}

QVariant QmlLibraryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows[index.row()];
    switch (role) {
    case IsGroupRole:       return r.group;
    case GroupCountRole:    return r.group ? int(r.members.size()) : 0;
    case GroupExpandedRole: return r.group && m_expanded.contains(r.groupKey);
    case GroupChildRole:    return r.child;
    case GroupKeyRole:      return r.groupKey;
    case RowKeyRole:        return r.key;
    default: break;
    }
    if (r.group)
        return groupData(r, role);
    const QPersistentModelIndex &p = r.members.value(0);
    return p.isValid() ? m_filter->data(p, role) : QVariant();
}

QVariant QmlLibraryModel::groupData(const Row &row, int role) const
{
    QList<QModelIndex> live;
    for (const QPersistentModelIndex &p : row.members)
        if (p.isValid()) live << QModelIndex(p);
    if (live.isEmpty())
        return {};
    auto val = [this](const QModelIndex &i, int r) { return m_filter->data(i, r); };

    // The episode whose state the group shows, and the one whose art it wears.
    auto stateMember = [&]() {
        QModelIndex best = live.first();
        int bestRank = stateRank(val(best, PM::StateKeyRole).toString());
        for (const QModelIndex &i : live) {
            const int rank = stateRank(val(i, PM::StateKeyRole).toString());
            if (rank < bestRank) { best = i; bestRank = rank; }
        }
        return best;
    };
    auto artMember = [&]() {
        for (const QModelIndex &i : live)
            if (!val(i, PM::PosterPathRole).toString().isEmpty())
                return i;
        return live.first();
    };
    auto sum = [&](int r) {
        qint64 total = 0;
        for (const QModelIndex &i : live) total += val(i, r).toLongLong();
        return total;
    };
    auto doneBytes = [&]() {
        qint64 done = 0;
        for (const QModelIndex &i : live)
            done += qint64(val(i, PM::SizeBytesRole).toLongLong() * val(i, PM::ProgressRole).toReal());
        return done;
    };

    switch (role) {
    case PM::NameRole:
    case PM::MetaTitleRole:
    case PM::ShowTitleRole: {
        // The resolver's title where an episode has one: it is the show's
        // real name, where the parsed one is whatever the release spelled.
        QString parsed;
        for (const QModelIndex &i : live) {
            const QString t = val(i, PM::ShowTitleRole).toString();
            if (t.isEmpty()) continue;
            if (val(i, PM::ShowTmdbRole).toInt() > 0) return t;
            if (parsed.isEmpty()) parsed = t;
        }
        return parsed.isEmpty() ? val(live.first(), PM::NameRole) : QVariant(parsed);
    }
    case PM::InfoHashRole:
        // The first episode's: the series page opens from any of them.
        return val(live.first(), PM::InfoHashRole);
    case PM::ProgressRole: {
        const qint64 size = sum(PM::SizeBytesRole);
        if (size <= 0) {
            qreal p = 0;
            for (const QModelIndex &i : live) p += val(i, PM::ProgressRole).toReal();
            return p / live.size();
        }
        return qreal(doneBytes()) / qreal(size);
    }
    case PM::SizeBytesRole:  return sum(PM::SizeBytesRole);
    case PM::SizeRole:       return formatSize(sum(PM::SizeBytesRole));
    case PM::DownloadedRole: return formatSize(doneBytes());
    case PM::DownRateRole:   return int(sum(PM::DownRateRole));
    case PM::UpRateRole:     return int(sum(PM::UpRateRole));
    case PM::DownSpeedRole:  return formatSpeed(int(sum(PM::DownRateRole)));
    case PM::UpSpeedRole:    return formatSpeed(int(sum(PM::UpRateRole)));
    case PM::NumPeersRole:   return int(sum(PM::NumPeersRole));
    case PM::NumSeedsRole:   return int(sum(PM::NumSeedsRole));
    case PM::EtaRole: {
        bool allDone = true;
        for (const QModelIndex &i : live)
            if (!val(i, PM::EtaRole).toString().isEmpty()) { allDone = false; break; }
        if (allDone) return QString();
        return formatGroupEta(sum(PM::SizeBytesRole) - doneBytes(), sum(PM::DownRateRole));
    }
    case PM::RatioRole: {
        // Weighted by size, so one tiny episode with a huge ratio does not
        // speak for a season. Any infinite ratio (-1) keeps the group infinite.
        qreal weighted = 0, weight = 0;
        for (const QModelIndex &i : live) {
            const qreal r = val(i, PM::RatioRole).toReal();
            if (r < 0) return -1.0;
            const qreal w = qMax<qint64>(1, val(i, PM::SizeBytesRole).toLongLong());
            weighted += r * w;
            weight += w;
        }
        return weight > 0 ? weighted / weight : 0.0;
    }
    case PM::AvailabilityRole: {
        qreal lowest = val(live.first(), PM::AvailabilityRole).toReal();
        for (const QModelIndex &i : live)
            lowest = qMin(lowest, val(i, PM::AvailabilityRole).toReal());
        return lowest;
    }
    case PM::StateKeyRole:
    case PM::StateStringRole:
    case PM::StateDetailRole:
    case PM::StateDetailShortRole:
        return val(stateMember(), role);
    case PM::CategoryRole: {
        // A category the episodes share, else none: showing one episode's
        // would claim it for the rest.
        const QString first = val(live.first(), role).toString();
        for (const QModelIndex &i : live)
            if (val(i, role).toString() != first) return QString();
        return first;
    }
    case PM::AutoCategoryRole: return QStringLiteral("Series");
    case PM::FileKindRole:     return QStringLiteral("SERIES");
    case PM::PlayableRole:     return false;   // which episode? the row opens instead
    case PM::QueuePosRole:     return 0;
    case PM::PosterPathRole:
    case PM::YearRole:
    case PM::GenresRole:
        return val(artMember(), role);
    case PM::SeasonRole:
    case PM::EpisodeRole:
        return -1;
    default:
        return val(live.first(), role);
    }
}

QVector<QmlLibraryModel::Row> QmlLibraryModel::computeRows() const
{
    const int n = m_filter->rowCount();
    QVector<LibraryGroups::Member> members(n);
    QStringList hashes;
    hashes.reserve(n);
    for (int i = 0; i < n; ++i) {
        const QModelIndex mi = m_filter->index(i, 0);
        LibraryGroups::Member &m = members[i];
        m.tmdbId = mi.data(PM::ShowTmdbRole).toInt();
        m.nameKey = mi.data(PM::ShowKeyRole).toString();
        m.season = mi.data(PM::SeasonRole).isValid() ? mi.data(PM::SeasonRole).toInt() : -1;
        m.episode = mi.data(PM::EpisodeRole).isValid() ? mi.data(PM::EpisodeRole).toInt() : -1;
        m.name = mi.data(PM::NameRole).toString();
        hashes << mi.data(PM::InfoHashRole).toString();
    }

    QVector<Row> rows;
    QHash<QString, int> seen;
    for (const LibraryGroups::Row &lr : LibraryGroups::layout(members, m_expanded)) {
        Row r;
        r.group = lr.group;
        r.child = lr.child;
        r.groupKey = lr.groupKey;
        for (int m : lr.members)
            r.members << QPersistentModelIndex(m_filter->index(m, 0));
        if (r.group) {
            r.key = QStringLiteral("g:") + r.groupKey;
        } else {
            // A hash can back two rows (see QmlPosterModel::posterResolved);
            // the keys must still be unique.
            const QString base = QStringLiteral("t:") + hashes.value(lr.members.value(0));
            const int dup = seen[base]++;
            r.key = dup ? base + QLatin1Char('#') + QString::number(dup) : base;
        }
        rows << r;
    }
    return rows;
}

// Brings m_rows to the new layout in steps a view can animate: removals, then
// a reorder of what is left, then insertions. A reset would be simpler and
// would drop the scroll position and every delegate on each regroup.
void QmlLibraryModel::rebuild()
{
    const QVector<Row> next = computeRows();
    QHash<QString, int> target;
    for (int i = 0; i < next.size(); ++i)
        target.insert(next[i].key, i);

    bool structural = false;

    for (int i = m_rows.size() - 1; i >= 0; --i) {
        if (target.contains(m_rows[i].key)) continue;
        int first = i;
        while (first > 0 && !target.contains(m_rows[first - 1].key)) --first;
        beginRemoveRows(QModelIndex(), first, i);
        m_rows.remove(first, i - first + 1);
        endRemoveRows();
        i = first;
        structural = true;
    }

    auto byTarget = [&](const Row &a, const Row &b) { return target.value(a.key) < target.value(b.key); };
    if (!std::is_sorted(m_rows.begin(), m_rows.end(), byTarget)) {
        emit layoutAboutToBeChanged({}, QAbstractItemModel::VerticalSortHint);
        QVector<Row> sorted = m_rows;
        std::sort(sorted.begin(), sorted.end(), byTarget);
        QHash<QString, int> newPos;
        for (int i = 0; i < sorted.size(); ++i) newPos.insert(sorted[i].key, i);
        const QModelIndexList from = persistentIndexList();
        QModelIndexList to;
        for (const QModelIndex &f : from)
            to << (f.row() < m_rows.size() ? index(newPos.value(m_rows[f.row()].key)) : QModelIndex());
        m_rows = sorted;
        changePersistentIndexList(from, to);
        emit layoutChanged({}, QAbstractItemModel::VerticalSortHint);
        structural = true;
    }

    QSet<QString> present;
    for (const Row &r : m_rows) present.insert(r.key);
    QVector<int> regrouped;
    for (int j = 0; j < next.size();) {
        if (present.contains(next[j].key)) {
            Row &cur = m_rows[j];
            if (cur.group && cur.members != next[j].members) regrouped << j;
            cur = next[j];
            ++j;
            continue;
        }
        int end = j;
        while (end < next.size() && !present.contains(next[end].key)) ++end;
        beginInsertRows(QModelIndex(), j, end - 1);
        for (int k = j; k < end; ++k) m_rows.insert(k, next[k]);
        endInsertRows();
        j = end;
        structural = true;
    }

    if (!m_rows.isEmpty())
        emit dataChanged(index(0), index(m_rows.size() - 1),
                         {IsGroupRole, GroupCountRole, GroupExpandedRole, GroupChildRole, GroupKeyRole});
    // A group that gained or lost an episode adds up to something else now.
    for (int g : regrouped)
        emit dataChanged(index(g), index(g));
    if (structural)
        emit structureChanged();
}

void QmlLibraryModel::setFilterState(const QString &state)       { m_filter->setFilterState(state); }
void QmlLibraryModel::setCategoryFilter(const QString &category) { m_filter->setCategoryFilter(category); }
void QmlLibraryModel::setSearchText(const QString &text)         { m_filter->setSearchText(text); }
void QmlLibraryModel::setSortColumn(const QString &column, bool ascending) { m_filter->setSortColumn(column, ascending); }
void QmlLibraryModel::clearSort()                                { m_filter->clearSort(); }
QVariantMap QmlLibraryModel::categoryCounts() const              { return m_filter->categoryCounts(); }

int QmlLibraryModel::mapToSource(int row) const
{
    if (row < 0 || row >= m_rows.size() || m_rows[row].group) return -1;
    const QPersistentModelIndex &p = m_rows[row].members.value(0);
    return p.isValid() ? m_filter->mapToSource(p.row()) : -1;
}

int QmlLibraryModel::mapFromSource(int sourceRow) const
{
    const int proxyRow = m_filter->mapFromSource(sourceRow);
    if (proxyRow < 0) return -1;
    for (int i = 0; i < m_rows.size(); ++i)
        if (!m_rows[i].group && m_rows[i].members.value(0).row() == proxyRow)
            return i;
    return groupRowOf(proxyRow);
}

int QmlLibraryModel::groupRowOf(int proxyRow) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (!m_rows[i].group) continue;
        for (const QPersistentModelIndex &m : m_rows[i].members)
            if (m.row() == proxyRow) return i;
    }
    return -1;
}

QVariantList QmlLibraryModel::sourceRowsAt(int row) const
{
    QVariantList out;
    if (row < 0 || row >= m_rows.size()) return out;
    for (const QPersistentModelIndex &p : m_rows[row].members) {
        if (!p.isValid()) continue;
        const int s = m_filter->mapToSource(p.row());
        if (s >= 0) out << s;
    }
    return out;
}

bool QmlLibraryModel::isGroup(int row) const
{
    return row >= 0 && row < m_rows.size() && m_rows[row].group;
}

bool QmlLibraryModel::isGroupExpanded(int row) const
{
    return isGroup(row) && m_expanded.contains(m_rows[row].groupKey);
}

void QmlLibraryModel::toggleGroup(int row)
{
    if (!isGroup(row)) return;
    setExpanded(m_rows[row].groupKey, !m_expanded.contains(m_rows[row].groupKey));
}

void QmlLibraryModel::setGroupExpanded(int row, bool expanded)
{
    if (!isGroup(row)) return;
    setExpanded(m_rows[row].groupKey, expanded);
}

int QmlLibraryModel::revealSource(int sourceRow)
{
    const int proxyRow = m_filter->mapFromSource(sourceRow);
    if (proxyRow < 0) return -1;
    const int g = groupRowOf(proxyRow);
    if (g >= 0 && !m_expanded.contains(m_rows[g].groupKey))
        setExpanded(m_rows[g].groupKey, true);
    return mapFromSource(sourceRow);
}

void QmlLibraryModel::setExpanded(const QString &groupKey, bool expanded)
{
    if (groupKey.isEmpty() || m_expanded.contains(groupKey) == expanded) return;
    if (expanded) m_expanded.insert(groupKey);
    else m_expanded.remove(groupKey);
    if (!m_persistKey.isEmpty()) {
        QStringList open(m_expanded.begin(), m_expanded.end());
        open.sort();
        QSettings("BATorrent", "BATorrent").setValue(m_persistKey, open);
    }
    rebuild();
}

QString QmlLibraryModel::rowKey(int row) const
{
    return row >= 0 && row < m_rows.size() ? m_rows[row].key : QString();
}

int QmlLibraryModel::rowForKey(const QString &key) const
{
    for (int i = 0; i < m_rows.size(); ++i)
        if (m_rows[i].key == key) return i;
    return -1;
}
