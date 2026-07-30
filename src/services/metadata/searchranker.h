// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SEARCHRANKER_H
#define SEARCHRANKER_H

#include <QString>
#include <QStringList>
#include <QList>

// Pure text-relevance ranking for search results. Given a free-text query and a
// release/title name, score how well the name matches — used to sort results by
// relevance. Whole-word matching only ("blast" must not match "last") and common
// articles/prepositions are dropped so they don't inflate every score equally.
namespace SearchRanker {

// The query's significant words: lowercased, split on non-alphanumerics, with
// stopwords (the/of/a/an/and/or/to/in/on) removed. Empty words are dropped.
QStringList significantWords(const QString &query);

// Count of query words that appear as whole words in `name` (0..queryWords.size).
int relevanceScore(const QString &name, const QStringList &queryWords);

// Best match across several titles for the SAME work, as a percentage (0..100).
//
// A work is searched under more than one name (the user's language and the
// original), so a result may match either. Raw counts can't be compared across
// them: "Shang-Chi e a Lenda dos Dez Anéis" has more significant words than
// "Shang-Chi and the Legend of the Ten Rings", so counting alone would rank
// Portuguese releases above English ones by arithmetic rather than by relevance.
// Each set is scored against its own size and the best percentage wins.
int bestRelevance(const QString &name, const QList<QStringList> &titleWordSets);

}

#endif // SEARCHRANKER_H
