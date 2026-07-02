// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef SEARCHRANKER_H
#define SEARCHRANKER_H

#include <QString>
#include <QStringList>

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

}

#endif // SEARCHRANKER_H
