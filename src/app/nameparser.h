// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef NAMEPARSER_H
#define NAMEPARSER_H

#include <QString>
#include <QStringList>

enum class ContentType { Movie, Series, Game, Unknown };

struct ParsedName {
    QString cleanTitle;
    int year = 0;
    int season = -1;
    int episode = -1;
    ContentType contentType = ContentType::Unknown;
    QString repackerTag;
    // 0..1 — how confident the type guess is, from the margin between the
    // game/video token scores. Low confidence → caller should lean on stronger
    // signals (file list, source) or fall back to a placeholder.
    double typeConfidence = 0.0;
};

class NameParser
{
public:
    static ParsedName parse(const QString &rawName);

    // Type inferred purely from a torrent's file list — the strongest signal
    // (a name can lie, the payload can't). Returns Unknown when it's ambiguous.
    static ContentType classifyByFiles(const QStringList &fileNames);
};

#endif
