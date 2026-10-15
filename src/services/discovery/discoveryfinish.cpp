// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/discoveryfinish.h"

#include "services/discovery/discoveryassemble.h"
#include "services/discovery/discoverysearch.h"
#include "services/discovery/igdbparse.h"
#include "services/discovery/tmdbparse.h"

#include <algorithm>

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
                      const QString &posterBase,
                      const QString &backdropBase)
{
    if (ok)
        works += TmdbParse::multiSearchRows(body, posterBase, backdropBase);
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

static QString foldTitle(const QString &s)
{
    QString out;
    for (const QChar c : s.toLower()) {
        if (c.isLetterOrNumber()) out += c;
        else if (!out.endsWith(QLatin1Char(' '))) out += QLatin1Char(' ');
    }
    return out.trimmed();
}

static int editDistance(const QString &a, const QString &b)
{
    QList<int> prev(b.size() + 1), cur(b.size() + 1);
    for (int j = 0; j <= b.size(); ++j) prev[j] = j;
    for (int i = 1; i <= a.size(); ++i) {
        cur[0] = i;
        for (int j = 1; j <= b.size(); ++j)
            cur[j] = std::min({ prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1) });
        std::swap(prev, cur);
    }
    return prev[b.size()];
}

QStringList suggestionQueries(const QString &query)
{
    const QStringList words = foldTitle(query).split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.isEmpty() || foldTitle(query).size() < 4) return {};
    QStringList out;
    if (words.size() > 1 && words.first().size() >= 3) out << words.first();
    QStringList cut;
    for (const QString &w : words) cut << w.left(3);
    const QString c = cut.join(QLatin1Char(' '));
    if (!out.contains(c)) out << c;
    return out;
}

QVariantMap nearestTitle(const QString &query, const QVariantList &works)
{
    const QString q = foldTitle(query);
    if (q.isEmpty()) return {};
    // a typo or two per word, never a different title
    const int allowed = std::max(2, int(q.size()) / 5);
    QVariantMap best;
    int bestD = allowed + 1;
    for (const QVariant &v : works) {
        const QVariantMap w = v.toMap();
        for (const QString key : { QStringLiteral("title"), QStringLiteral("originalTitle") }) {
            const QString t = foldTitle(w.value(key).toString());
            if (t.isEmpty() || t == q) continue;
            const int d = editDistance(q, t);
            if (d < bestD) { bestD = d; best = w; }
        }
    }
    return best;
}

} // namespace DiscoveryFinish
