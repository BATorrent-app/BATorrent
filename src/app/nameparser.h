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
};

class NameParser
{
public:
    static ParsedName parse(const QString &rawName);
};

#endif
