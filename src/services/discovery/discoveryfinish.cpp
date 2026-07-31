// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/discoveryfinish.h"

#include "services/discovery/discoveryassemble.h"
#include "services/discovery/discoverysearch.h"
#include "services/discovery/igdbparse.h"
#include "services/discovery/tmdbparse.h"

namespace DiscoveryFinish {

void ingestTmdbShelf(QMap<int, QVariantMap> &accum,
                     int order,
                     const QString &label,
                     bool isTv,
                     bool ok,
                     const QByteArray &body,
                     const QString &posterBase,
                     const QString &backdropBase)
{
    const QVariantList items = ok
        ? TmdbParse::shelfRows(body, isTv, posterBase, backdropBase)
        : QVariantList{};
    QVariantMap row = accum.value(order);
    const QVariantList merged = DiscoveryAssemble::mergeShelfByPoster(
        row.value(QStringLiteral("items")).toList(), items);
    row.insert(QStringLiteral("label"), label);
    row.insert(QStringLiteral("items"), merged);
    if (!merged.isEmpty())
        accum.insert(order, row);
}

void ingestTmdbSearch(QVariantList &works,
                      bool ok,
                      const QByteArray &body,
                      const QString &posterBase)
{
    if (ok)
        works += TmdbParse::multiSearchRows(body, posterBase);
}

void ingestIgdbSearch(QVariantList &works, bool ok, const QByteArray &body)
{
    if (ok)
        works += IgdbParse::titleSearchRows(body);
}

bool tryFinishShelves(int &pending,
                      const QMap<int, QVariantMap> &accum,
                      QVariantList *rowsOut,
                      QVariantList *heroOut)
{
    if (--pending > 0)
        return false;
    if (rowsOut)
        *rowsOut = DiscoveryAssemble::rowsFromAccum(accum);
    if (heroOut)
        *heroOut = DiscoveryAssemble::heroFromAccum(accum);
    return true;
}

bool tryFinishSearch(int &pending,
                     const QString &query,
                     const QVariantList &works,
                     QVariantList *rankedOut)
{
    if (--pending > 0)
        return false;
    if (rankedOut)
        *rankedOut = DiscoverySearch::rankAndMerge(query, works);
    return true;
}

} // namespace DiscoveryFinish
