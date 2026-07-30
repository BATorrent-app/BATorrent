// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/searchranker.h"

#include <QRegularExpression>
#include <QSet>

namespace {
// Strip diacritics: decompose, then drop the combining marks. Without this the
// [^a-z0-9] split treats an accent as a separator, so "Anéis" becomes "an"+"is"
// and never matches a release named "Aneis" — releases are routinely typed
// without accents. Breaks every accented language, i.e. most of the ones this
// ranking exists to serve.
QString foldAccents(const QString &s)
{
    const QString d = s.normalized(QString::NormalizationForm_D);
    QString out;
    out.reserve(d.size());
    for (const QChar &c : d)
        if (c.category() != QChar::Mark_NonSpacing)
            out.append(c);
    return out;
}

// Split on runs of non-alphanumerics, lowercased, dropping empty tokens.
QStringList wordsOf(const QString &s)
{
    static const QRegularExpression sep(QStringLiteral("[^a-z0-9]+"));
    QStringList out;
    const QStringList parts = foldAccents(s.toLower()).split(sep);
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

int bestRelevance(const QString &name, const QList<QStringList> &titleWordSets)
{
    int best = 0;
    for (const QStringList &words : titleWordSets) {
        if (words.isEmpty())
            continue;
        const int hits = relevanceScore(name, words);
        const int pct = hits * 100 / words.size();
        if (pct > best)
            best = pct;
    }
    return best;
}

}
