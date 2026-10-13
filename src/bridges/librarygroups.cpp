// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "bridges/librarygroups.h"

#include <QHash>
#include <algorithm>

namespace LibraryGroups {

QString nameKey(const QString &title)
{
    QString out;
    out.reserve(title.size());
    bool gap = false;
    for (const QChar c : title) {
        if (c.isLetterOrNumber()) {
            if (gap && !out.isEmpty()) out += QLatin1Char(' ');
            out += c.toLower();
            gap = false;
        } else {
            gap = true;
        }
    }
    return out;
}

QStringList keys(const QVector<Member> &members)
{
    // A name the resolver has already tied to a show points every other
    // episode with that name at the same show.
    QHash<QString, int> nameToTmdb;
    for (const Member &m : members)
        if (m.tmdbId > 0 && !m.nameKey.isEmpty() && !nameToTmdb.contains(m.nameKey))
            nameToTmdb.insert(m.nameKey, m.tmdbId);

    QStringList out;
    out.reserve(members.size());
    for (const Member &m : members) {
        int id = m.tmdbId;
        if (id <= 0 && !m.nameKey.isEmpty())
            id = nameToTmdb.value(m.nameKey, 0);
        if (id > 0)
            out << QStringLiteral("tmdb:") + QString::number(id);
        else if (!m.nameKey.isEmpty())
            out << QStringLiteral("name:") + m.nameKey;
        else
            out << QString();
    }
    return out;
}

QVector<Row> layout(const QVector<Member> &members, const QSet<QString> &expanded)
{
    const QStringList k = keys(members);

    QHash<QString, QVector<int>> byKey;
    for (int i = 0; i < members.size(); ++i)
        if (!k[i].isEmpty())
            byKey[k[i]].append(i);

    // Packs sort ahead of single episodes: -1 is smaller than any number, so a
    // complete pack opens the list and a season pack opens its season.
    for (auto it = byKey.begin(); it != byKey.end(); ++it) {
        std::stable_sort(it->begin(), it->end(), [&](int a, int b) {
            const Member &x = members[a];
            const Member &y = members[b];
            if (x.season != y.season) return x.season < y.season;
            if (x.episode != y.episode) return x.episode < y.episode;
            return x.name.compare(y.name, Qt::CaseInsensitive) < 0;
        });
    }

    QVector<Row> rows;
    rows.reserve(members.size());
    QSet<QString> placed;
    for (int i = 0; i < members.size(); ++i) {
        const QString &key = k[i];
        const auto it = key.isEmpty() ? byKey.constEnd() : byKey.constFind(key);
        if (it == byKey.constEnd() || it->size() < 2) {
            Row r;
            r.members = {i};
            rows << r;
            continue;
        }
        if (placed.contains(key))
            continue;
        placed.insert(key);

        Row head;
        head.group = true;
        head.groupKey = key;
        head.members = *it;
        rows << head;
        if (!expanded.contains(key))
            continue;
        for (int m : *it) {
            Row c;
            c.groupKey = key;
            c.child = true;
            c.members = {m};
            rows << c;
        }
    }
    return rows;
}

} // namespace LibraryGroups
