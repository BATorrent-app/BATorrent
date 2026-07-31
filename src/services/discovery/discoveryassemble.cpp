// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/discoveryassemble.h"

#include <QHash>
#include <QSet>
#include <QtGlobal>

namespace DiscoveryAssemble {

QVariantList mergeShelfByPoster(const QVariantList &existing,
                                const QVariantList &incoming)
{
    QVariantList merged = existing;
    QSet<QString> seen;
    for (const QVariant &v : existing)
        seen.insert(v.toMap().value(QStringLiteral("poster")).toString());
    for (const QVariant &v : incoming) {
        const QString key = v.toMap().value(QStringLiteral("poster")).toString();
        if (!seen.contains(key)) {
            merged.append(v);
            seen.insert(key);
        }
    }
    return merged;
}

QVariantList rowsFromAccum(const QMap<int, QVariantMap> &accum)
{
    // Stable canonical genre key per genre-shelf (by fetch order) — lets the HUB
    // match the user's taste to a shelf without depending on the translated label.
    static const QHash<int, QString> orderGenre = {
        {3, QStringLiteral("rpg")},      {4, QStringLiteral("shooter")},
        {5, QStringLiteral("strategy")}, {6, QStringLiteral("indie")},
        {14, QStringLiteral("action")},  {15, QStringLiteral("scifi")},
        {16, QStringLiteral("horror")}
    };

    QVariantList rows;
    QSet<QString> seenTitles;
    for (auto it = accum.constBegin(); it != accum.constEnd(); ++it) {
        QVariantMap row = it.value();
        row[QStringLiteral("genre")] = orderGenre.value(it.key());
        QVariantList kept;
        for (const QVariant &v : row.value(QStringLiteral("items")).toList()) {
            const QVariantMap m = v.toMap();
            const QString key = m.value(QStringLiteral("title")).toString().toLower()
                              + QLatin1Char('|')
                              + m.value(QStringLiteral("type")).toString();
            if (seenTitles.contains(key)) continue;
            seenTitles.insert(key);
            kept.append(v);
        }
        if (!kept.isEmpty()) {
            row[QStringLiteral("items")] = kept;
            rows.append(row);
        }
    }
    return rows;
}

QVariantList heroFromAccum(const QMap<int, QVariantMap> &accum, int limit)
{
    QVariantList hero;
    if (limit <= 0) return hero;

    QStringList seen;
    QList<QVariantList> rowItems;
    int maxItems = 0;
    for (auto it = accum.constBegin(); it != accum.constEnd(); ++it) {
        const QVariantList items = it.value().value(QStringLiteral("items")).toList();
        rowItems.append(items);
        maxItems = qMax(maxItems, int(items.size()));
    }
    for (int col = 0; col < maxItems && hero.size() < limit; ++col) {
        for (const QVariantList &items : rowItems) {
            if (hero.size() >= limit) break;
            if (col >= items.size()) continue;
            const QVariantMap m = items[col].toMap();
            if (m.value(QStringLiteral("backdrop")).toString().isEmpty()
                || m.value(QStringLiteral("overview")).toString().isEmpty())
                continue;
            const QString title = m.value(QStringLiteral("title")).toString();
            if (seen.contains(title)) continue;
            seen.append(title);
            hero.append(m);
        }
    }
    return hero;
}

} // namespace DiscoveryAssemble
