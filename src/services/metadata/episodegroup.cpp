// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/episodegroup.h"
#include "services/metadata/nameparser.h"

#include <QRegularExpression>

EpisodeTag EpisodeGroup::classify(const QString &releaseName)
{
    EpisodeTag tag;

    const ParsedName p = NameParser::parse(releaseName);
    if (p.episode > 0) {
        // anime absolute numbering has no on-screen season — NameParser pins it to 1
        tag.season = p.season > 0 ? p.season : 1;
        tag.episode = p.episode;
        return tag;
    }

    // "S01 E05" / "S01.E05" — separated forms NameParser's strict SxxExx misses
    static const QRegularExpression spacedSeRe(
        QStringLiteral("\\bS(\\d{1,2})[ ._-]+E(\\d{1,3})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch m = spacedSeRe.match(releaseName);
    if (m.hasMatch()) {
        tag.season = m.captured(1).toInt();
        tag.episode = m.captured(2).toInt();
        return tag;
    }

    // multi-season range → whole-series pack: "S01-S05", "Season 1-3"
    static const QRegularExpression rangeRe(
        QStringLiteral("\\bS(?:easons?)?[ ._]?(\\d{1,2})[ ._]?[-~][ ._]?S?(?:easons?[ ._]?)?(\\d{1,2})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    if (rangeRe.match(releaseName).hasMatch()) {
        tag.pack = true;
        return tag;
    }

    // season-only marker → season pack. Checked in priority order — a single
    // alternation would let "Round 6 Temporada 2" resolve to 6 ("6 Temporada"
    // ordinal form) because the leftmost match wins across alternatives.
    static const QRegularExpression wordSeasonRe(
        QStringLiteral("\\b(?:Season|Temporada|Saison|Staffel)[ ._]?(\\d{1,2})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression ordinalSeasonRe(
        QStringLiteral("\\b(\\d{1,2})\\s*[ªº]?\\s*Temporada\\b"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression bareSRe(
        QStringLiteral("\\bS(\\d{1,2})\\b"),
        QRegularExpression::CaseInsensitiveOption);
    for (const auto *re : { &wordSeasonRe, &ordinalSeasonRe, &bareSRe }) {
        m = re->match(releaseName);
        if (m.hasMatch()) {
            tag.season = m.captured(1).toInt();
            tag.pack = true;
            return tag;
        }
    }

    // complete/batch keywords → whole-series pack
    static const QRegularExpression completeRe(
        QStringLiteral("\\b(?:complete|completa|completo|batch|full[ ._]?series|all[ ._]?seasons|"
                       "todas[ ._]as[ ._]temporadas|cole[cç][aã]o|collection|int[eé]grale)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    if (completeRe.match(releaseName).hasMatch()) {
        tag.pack = true;
        return tag;
    }

    return tag;
}
