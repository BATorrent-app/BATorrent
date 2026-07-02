// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/searchranker.h"

#include <QRegularExpression>
#include <QSet>

namespace {
// Split on runs of non-alphanumerics, lowercased, dropping empty tokens.
QStringList wordsOf(const QString &s)
{
    static const QRegularExpression sep(QStringLiteral("[^a-z0-9]+"));
    QStringList out;
    const QStringList parts = s.toLower().split(sep);
    for (const QString &w : parts)
        if (!w.isEmpty())
            out << w;
    return out;
}
} // namespace

namespace SearchRanker {

QStringList significantWords(const QString &query)
{
    static const QSet<QString> stop = {
        QStringLiteral("the"), QStringLiteral("of"), QStringLiteral("a"),
        QStringLiteral("an"), QStringLiteral("and"), QStringLiteral("or"),
        QStringLiteral("to"), QStringLiteral("in"), QStringLiteral("on")
    };
    QStringList out;
    for (const QString &w : wordsOf(query))
        if (!stop.contains(w))
            out << w;
    return out;
}

int relevanceScore(const QString &name, const QStringList &queryWords)
{
    if (queryWords.isEmpty())
        return 0;
    const QStringList nw = wordsOf(name);
    const QSet<QString> nameWords(nw.cbegin(), nw.cend());
    int s = 0;
    for (const QString &q : queryWords)
        if (nameWords.contains(q))
            ++s;
    return s;
}

}
