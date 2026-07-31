// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/discoverysearch.h"

#include <QSet>
#include <QVariantMap>
#include <algorithm>

namespace DiscoverySearch {

QVariantList rankAndMerge(const QString &query, const QVariantList &works)
{
    const QString ql = query.toLower();
    auto score = [&ql](const QVariant &v) {
        const QString t = v.toMap().value(QStringLiteral("title")).toString().toLower();
        if (t == ql) return 0;
        if (t.startsWith(ql)) return 1;
        if (t.contains(ql)) return 2;
        return 3;
    };
    auto byScore = [&score](const QVariant &a, const QVariant &b) {
        return score(a) < score(b);
    };

    QVariantList vids, games;
    for (const QVariant &v : works) {
        if (v.toMap().value(QStringLiteral("type")).toString() == QLatin1String("game"))
            games.append(v);
        else
            vids.append(v);
    }
    std::stable_sort(vids.begin(), vids.end(), byScore);
    std::stable_sort(games.begin(), games.end(), byScore);

    // Merge by (name-match score, rating): when the game and the series share the
    // exact name ("Game of Thrones", "The Witcher") the better-rated work leads.
    auto rating = [](const QVariant &v) {
        return v.toMap().value(QStringLiteral("rating")).toDouble();
    };
    QVariantList merged;
    int gi = 0, vi = 0;
    while (gi < games.size() || vi < vids.size()) {
        if (vi >= vids.size()) { merged.append(games[gi++]); continue; }
        if (gi >= games.size()) { merged.append(vids[vi++]); continue; }
        const int sg = score(games[gi]), sv = score(vids[vi]);
        if (sg < sv || (sg == sv && rating(games[gi]) >= rating(vids[vi])))
            merged.append(games[gi++]);
        else
            merged.append(vids[vi++]);
    }

    QVariantList out;
    QSet<QString> seen;
    for (const QVariant &v : std::as_const(merged)) {
        const QVariantMap m = v.toMap();
        const QString key = m.value(QStringLiteral("title")).toString().toLower()
                          + QLatin1Char('|')
                          + m.value(QStringLiteral("year")).toString()
                          + QLatin1Char('|')
                          + m.value(QStringLiteral("type")).toString();
        if (seen.contains(key)) continue;
        seen.insert(key);
        out.append(v);
    }
    return out;
}

} // namespace DiscoverySearch
