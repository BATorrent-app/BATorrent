// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/hublogic.h"

#include <QHash>
#include <QVariantMap>
#include <QtGlobal>
#include <algorithm>

namespace HubLogic {

QString genreKey(const QString &name)
{
    const QString s = name.toLower();
    if (s.contains(QLatin1String("rpg")) || s.contains(QLatin1String("role")))
        return QStringLiteral("rpg");
    if (s.contains(QLatin1String("shoot")) || s.contains(QLatin1String("tiro"))
        || s.contains(QLatin1String("fps")))
        return QStringLiteral("shooter");
    if (s.contains(QLatin1String("strateg")) || s.contains(QLatin1String("estrat")))
        return QStringLiteral("strategy");
    if (s.contains(QLatin1String("indie")))
        return QStringLiteral("indie");
    if (s.contains(QLatin1String("sci")) || s.contains(QStringLiteral("ficç"))
        || s.contains(QLatin1String("cient")))
        return QStringLiteral("scifi");
    if (s.contains(QLatin1String("horror")) || s.contains(QLatin1String("terror")))
        return QStringLiteral("horror");
    if (s.contains(QLatin1String("action")) || s.contains(QStringLiteral("ação"))
        || s.contains(QLatin1String("acao")))
        return QStringLiteral("action");
    return {};
}

QString topGenre(const QStringList &genreNames)
{
    QHash<QString, int> counts;
    for (const QString &name : genreNames) {
        const QString k = genreKey(name);
        if (!k.isEmpty())
            ++counts[k];
    }
    QString best;
    int bestN = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > bestN) {
            bestN = it.value();
            best = it.key();
        }
    }
    return best;
}

QVariantList excludeOwnedTitles(const QVariantList &candidates,
                                const QSet<QString> &ownedLower,
                                int limit)
{
    QVariantList out;
    out.reserve(qMin(limit, candidates.size()));
    for (const QVariant &v : candidates) {
        if (out.size() >= limit)
            break;
        const QString title = v.toMap().value(QStringLiteral("title")).toString().toLower();
        if (title.isEmpty() || ownedLower.contains(title))
            continue;
        out << v;
    }
    return out;
}

QVariantList applyView(const QVariantList &list,
                       const QString &search,
                       const QString &sort)
{
    const QString q = search.trimmed().toLower();
    QVariantList arr;
    arr.reserve(list.size());
    for (const QVariant &v : list) {
        if (q.isEmpty()) {
            arr << v;
            continue;
        }
        const QString title = v.toMap().value(QStringLiteral("title")).toString().toLower();
        if (title.contains(q))
            arr << v;
    }
    if (sort == QLatin1String("name")) {
        std::sort(arr.begin(), arr.end(), [](const QVariant &a, const QVariant &b) {
            return a.toMap().value(QStringLiteral("title")).toString()
                .localeAwareCompare(b.toMap().value(QStringLiteral("title")).toString()) < 0;
        });
    }
    return arr;
}

} // namespace HubLogic
