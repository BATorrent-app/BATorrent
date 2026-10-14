// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/aniskip.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

namespace AniSkip {

QUrl malSearchUrl()
{
    return QUrl(QStringLiteral("https://graphql.anilist.co"));
}

QByteArray malSearchBody(const QString &title)
{
    const QJsonObject body{
        { QStringLiteral("query"), QStringLiteral(
              "query($q:String){Page(perPage:5){media(search:$q,type:ANIME,format:TV)"
              "{idMal title{romaji english native} synonyms}}}") },
        { QStringLiteral("variables"), QJsonObject{ { QStringLiteral("q"), title } } } };
    return QJsonDocument(body).toJson(QJsonDocument::Compact);
}

namespace {
QString squash(const QString &s)
{
    QString out;
    for (const QChar c : s.toCaseFolded())
        if (c.isLetterOrNumber()) out += c;
    return out;
}
}

int malIdFromSearch(const QByteArray &json, const QStringList &names)
{
    QStringList want;
    for (const QString &n : names)
        if (!squash(n).isEmpty()) want << squash(n);
    if (want.isEmpty()) return 0;
    const QJsonArray media = QJsonDocument::fromJson(json).object().value(QStringLiteral("data")).toObject()
                                 .value(QStringLiteral("Page")).toObject().value(QStringLiteral("media")).toArray();
    for (const QJsonValue &v : media) {
        const QJsonObject e = v.toObject();
        const int malId = e.value(QStringLiteral("idMal")).toInt();
        if (malId <= 0) continue;
        const QJsonObject t = e.value(QStringLiteral("title")).toObject();
        QStringList titles = { t.value(QStringLiteral("romaji")).toString(), t.value(QStringLiteral("english")).toString(),
                               t.value(QStringLiteral("native")).toString() };
        for (const QJsonValue &s : e.value(QStringLiteral("synonyms")).toArray())
            titles << s.toString();
        for (const QString &name : titles)
            if (want.contains(squash(name))) return malId;
    }
    return 0;
}

QUrl skipTimesUrl(int malId, int episode)
{
    // episodeLength=0 asks for every submission regardless of the cut's length
    QUrl url(QStringLiteral("https://api.aniskip.com/v2/skip-times/%1/%2").arg(malId).arg(episode));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("types"), QStringLiteral("op"));
    q.addQueryItem(QStringLiteral("types"), QStringLiteral("ed"));
    q.addQueryItem(QStringLiteral("episodeLength"), QStringLiteral("0"));
    url.setQuery(q);
    return url;
}

QList<MkvChapter> chaptersFromSkipTimes(const QByteArray &json)
{
    QList<MkvChapter> out;
    const QJsonObject root = QJsonDocument::fromJson(json).object();
    if (!root.value(QStringLiteral("found")).toBool()) return out;
    for (const QJsonValue &v : root.value(QStringLiteral("results")).toArray()) {
        const QJsonObject r = v.toObject();
        const QString type = r.value(QStringLiteral("skipType")).toString();
        if (type != QLatin1String("op") && type != QLatin1String("ed")) continue;
        const QJsonObject iv = r.value(QStringLiteral("interval")).toObject();
        const double start = iv.value(QStringLiteral("startTime")).toDouble(-1);
        const double end = iv.value(QStringLiteral("endTime")).toDouble(-1);
        if (start < 0 || end <= start) continue;
        MkvChapter c;
        c.startMs = qint64(start * 1000);
        c.endMs = qint64(end * 1000);
        c.kind = type == QLatin1String("op") ? QStringLiteral("intro") : QStringLiteral("credits");
        c.name = type == QLatin1String("op") ? QStringLiteral("Opening") : QStringLiteral("Ending");
        out << c;
    }
    return out;
}

} // namespace AniSkip
