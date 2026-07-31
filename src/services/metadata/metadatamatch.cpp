// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/metadatamatch.h"

#include <QDateTime>
#include <QHash>
#include <QRegularExpression>
#include <QSet>

namespace MetadataMatch {
namespace {

const QHash<int, QString> &tmdbGenres()
{
    static const QHash<int, QString> map = {
        {28, QStringLiteral("Action")},
        {12, QStringLiteral("Adventure")},
        {16, QStringLiteral("Animation")},
        {35, QStringLiteral("Comedy")},
        {80, QStringLiteral("Crime")},
        {99, QStringLiteral("Documentary")},
        {18, QStringLiteral("Drama")},
        {10751, QStringLiteral("Family")},
        {14, QStringLiteral("Fantasy")},
        {36, QStringLiteral("History")},
        {27, QStringLiteral("Horror")},
        {10402, QStringLiteral("Music")},
        {9648, QStringLiteral("Mystery")},
        {10749, QStringLiteral("Romance")},
        {878, QStringLiteral("Sci-Fi")},
        {10770, QStringLiteral("TV Movie")},
        {53, QStringLiteral("Thriller")},
        {10752, QStringLiteral("War")},
        {37, QStringLiteral("Western")}
    };
    return map;
}

QSet<QString> titleTokens(const QString &s)
{
    QSet<QString> out;
    const auto parts = foldTitle(s).split(QRegularExpression(QStringLiteral("[^a-z0-9]+")),
                                          Qt::SkipEmptyParts);
    static const QSet<QString> stop = {QStringLiteral("the"), QStringLiteral("a"),
                                       QStringLiteral("of"), QStringLiteral("and"),
                                       QStringLiteral("edition")};
    static const QHash<QString, QString> roman = {
        {QStringLiteral("ii"), "2"},   {QStringLiteral("iii"), "3"},
        {QStringLiteral("iv"), "4"},   {QStringLiteral("v"), "5"},
        {QStringLiteral("vi"), "6"},   {QStringLiteral("vii"), "7"},
        {QStringLiteral("viii"), "8"}, {QStringLiteral("ix"), "9"},
        {QStringLiteral("x"), "10"},   {QStringLiteral("xi"), "11"},
        {QStringLiteral("xii"), "12"}, {QStringLiteral("xiii"), "13"},
        {QStringLiteral("xiv"), "14"}, {QStringLiteral("xv"), "15"}};
    for (const auto &p : parts) {
        if (stop.contains(p))
            continue;
        const auto it = roman.constFind(p);
        out.insert(it != roman.constEnd() ? it.value() : p);
    }
    return out;
}

} // namespace

QString foldTitle(const QString &s)
{
    QString n = s.normalized(QString::NormalizationForm_KD);
    n.remove(QRegularExpression(QStringLiteral("[\\x{0300}-\\x{036F}]")));
    n.remove(QRegularExpression(QStringLiteral("['\\x{2019}\\x{2018}`]")));
    return n.toLower();
}

double titleSimilarity(const QString &a, const QString &b)
{
    const QSet<QString> ta = titleTokens(a), tb = titleTokens(b);
    if (ta.isEmpty() || tb.isEmpty())
        return 0.0;
    int inter = 0;
    for (const auto &t : ta)
        if (tb.contains(t))
            ++inter;
    const int uni = ta.size() + tb.size() - inter;
    return uni ? double(inter) / double(uni) : 0.0;
}

bool confidentTitle(const QString &query, const QString &title)
{
    return titleSimilarity(query, title) >= kMinConfidence
           || foldTitle(title) == foldTitle(query);
}

QString escapeApicalypse(const QString &title)
{
    QString safe = title;
    safe.replace('\\', QStringLiteral("\\\\")).replace('"', QStringLiteral("\\\""));
    return safe;
}

QString shortenedSearchTitle(const QString &queryTitle)
{
    const QStringList toks = queryTitle.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (toks.size() <= 3)
        return {};
    const int keep = qMax(3, int((toks.size() + 1) / 2));
    return QStringList(toks.mid(0, keep)).join(QLatin1Char(' '));
}

IgdbPick pickBestIgdbResult(const QJsonArray &results,
                            const QString &fullTitle,
                            int year)
{
    IgdbPick pick;
    for (const auto &v : results) {
        const QJsonObject obj = v.toObject();
        double score = titleSimilarity(fullTitle, obj.value(QLatin1String("name")).toString());
        if (year > 0) {
            const qint64 rel = qint64(obj.value(QLatin1String("first_release_date")).toDouble());
            if (rel > 0) {
                const int ry = QDateTime::fromSecsSinceEpoch(rel).date().year();
                if (qAbs(ry - year) <= 1)
                    score += kYearMatchBonus;
            }
        }
        if (score > pick.bestScore) {
            pick.bestScore = score;
            pick.item = obj;
        }
    }
    pick.found = pick.bestScore >= kMinConfidence
                 || foldTitle(pick.item.value(QLatin1String("name")).toString())
                        == foldTitle(fullTitle);
    return pick;
}

ContentType applyFileTypeOverride(ContentType nameType, const QStringList &fileNames)
{
    if (fileNames.isEmpty() || nameType == ContentType::Series)
        return nameType;
    const ContentType byFiles = NameParser::classifyByFiles(fileNames);
    if (byFiles != ContentType::Unknown)
        return byFiles;
    return nameType;
}

QString contentTypeToString(ContentType ct)
{
    switch (ct) {
    case ContentType::Movie:
        return QStringLiteral("movie");
    case ContentType::Series:
        return QStringLiteral("series");
    case ContentType::Game:
        return QStringLiteral("game");
    case ContentType::Unknown:
        return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

ContentType contentTypeFromString(const QString &s)
{
    if (s == QLatin1String("movie"))
        return ContentType::Movie;
    if (s == QLatin1String("series"))
        return ContentType::Series;
    if (s == QLatin1String("game"))
        return ContentType::Game;
    return ContentType::Unknown;
}

QStringList genreNamesFromIds(const QJsonArray &genreIds)
{
    QStringList out;
    const auto &genreMap = tmdbGenres();
    for (const QJsonValue &gv : genreIds) {
        const QString name = genreMap.value(gv.toInt());
        if (!name.isEmpty())
            out.append(name);
    }
    return out;
}

} // namespace MetadataMatch
