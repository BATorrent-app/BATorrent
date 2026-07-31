// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include "services/metadata/nameparser.h"

// Pure title-similarity / IGDB pick / cache type helpers peeled from
// MetadataResolver so characterisation tests can pin match behaviour.
namespace MetadataMatch {

inline constexpr double kMinConfidence = 0.34;
inline constexpr double kYearMatchBonus = 0.15;

QString foldTitle(const QString &s);
double titleSimilarity(const QString &a, const QString &b);
bool confidentTitle(const QString &query, const QString &title);

// Escape for Apicalypse `search "..."` literals (torrent names may carry " / \).
QString escapeApicalypse(const QString &title);

// Front-half token trim for IGDB retry; empty when query is already ≤3 tokens.
QString shortenedSearchTitle(const QString &queryTitle);

struct IgdbPick {
    QJsonObject item;
    double bestScore = 0.0;
    bool found = false;
};

// Score each IGDB hit by folded token overlap vs fullTitle (+ year bonus).
// found when bestScore ≥ kMinConfidence or folded titles equal.
IgdbPick pickBestIgdbResult(const QJsonArray &results,
                            const QString &fullTitle,
                            int year);

// File payload outranks the name for game-vs-movie; keep name-derived Series.
ContentType applyFileTypeOverride(ContentType nameType, const QStringList &fileNames);

QString contentTypeToString(ContentType ct);
ContentType contentTypeFromString(const QString &s);

QStringList genreNamesFromIds(const QJsonArray &genreIds);

} // namespace MetadataMatch
