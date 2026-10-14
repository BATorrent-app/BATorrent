// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/cinemeta.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>

namespace Cinemeta {

QUrl metaUrl(const QString &imdbId)
{
    return QUrl(QStringLiteral("https://v3-cinemeta.strem.io/meta/series/%1.json").arg(imdbId));
}

QList<Video> videos(const QByteArray &json)
{
    QList<Video> out;
    const QJsonArray list = QJsonDocument::fromJson(json).object().value(QStringLiteral("meta")).toObject()
                                .value(QStringLiteral("videos")).toArray();
    for (const QJsonValue &v : list) {
        const QJsonObject o = v.toObject();
        Video video;
        video.season = o.value(QStringLiteral("season")).toInt();
        video.episode = o.value(QStringLiteral("episode")).toInt(o.value(QStringLiteral("number")).toInt());
        // "2004-10-06T00:00:00.000Z": the day is what both sides agree on
        video.released = QDate::fromString(o.value(QStringLiteral("released")).toString().left(10), Qt::ISODate);
        if (video.episode > 0) out << video;
    }
    return out;
}

Number mapByAirDate(const QList<Video> &videos, const QDate &airDate, int tmdbSeason, int tmdbEpisode)
{
    if (!airDate.isValid()) return { tmdbSeason, tmdbEpisode };
    for (int slack : { 0, 1 }) {
        const Video *first = nullptr;
        for (const Video &v : videos) {
            if (!v.released.isValid() || qAbs(v.released.daysTo(airDate)) > slack) continue;
            // specials on one side are rarely specials on the other
            if ((v.season == 0) != (tmdbSeason == 0)) continue;
            if (v.episode == tmdbEpisode) return { v.season, v.episode };
            if (!first) first = &v;
        }
        if (first) return { first->season, first->episode };
    }
    return { tmdbSeason, tmdbEpisode };
}

Number nextAfter(const QList<Video> &videos, int season, int episode)
{
    QList<Video> list;
    for (const Video &v : videos)
        if (v.season > 0) list << v;
    std::sort(list.begin(), list.end(), [](const Video &a, const Video &b) {
        return a.season != b.season ? a.season < b.season : a.episode < b.episode;
    });
    int at = -1;
    for (int i = 0; i < list.size(); ++i)
        if (list[i].season == qMax(1, season) && list[i].episode == episode) { at = i; break; }
    if (at < 0 && season <= 1 && episode > 0 && episode <= list.size()) at = episode - 1;
    if (at < 0 || at + 1 >= list.size()) return { -1, -1 };
    return { list[at + 1].season, list[at + 1].episode };
}

} // namespace Cinemeta
