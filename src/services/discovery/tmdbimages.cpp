// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/tmdbimages.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace TmdbImages {

static void appendBackdrops(QStringList &urls,
                            const QJsonArray &arr,
                            const QString &imageBaseUrl,
                            bool untaggedOnly,
                            int limit)
{
    for (const QJsonValue &v : arr) {
        if (urls.size() >= limit)
            return;
        const QJsonObject o = v.toObject();
        const bool untagged = o.value(QLatin1String("iso_639_1")).isNull();
        if (untaggedOnly != untagged)
            continue;
        const QString path = o.value(QLatin1String("file_path")).toString();
        if (path.isEmpty())
            continue;
        const QString url = imageBaseUrl + path;
        if (!urls.contains(url))
            urls << url;
    }
}

QStringList backdropUrls(const QByteArray &imagesJson,
                         const QString &imageBaseUrl,
                         int limit)
{
    QStringList urls;
    if (imagesJson.isEmpty() || limit <= 0)
        return urls;

    const QJsonArray arr = QJsonDocument::fromJson(imagesJson)
                               .object()
                               .value(QLatin1String("backdrops"))
                               .toArray();
    appendBackdrops(urls, arr, imageBaseUrl, /*untaggedOnly=*/true, limit);
    appendBackdrops(urls, arr, imageBaseUrl, /*untaggedOnly=*/false, limit);
    return urls;
}

QString youtubeTrailerKey(const QByteArray &videosJson)
{
    if (videosJson.isEmpty())
        return {};

    QString trailer;
    QString teaser;
    const QJsonArray arr = QJsonDocument::fromJson(videosJson)
                               .object()
                               .value(QLatin1String("results"))
                               .toArray();
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        if (o.value(QLatin1String("site")).toString() != QLatin1String("YouTube"))
            continue;
        const QString type = o.value(QLatin1String("type")).toString();
        const QString key = o.value(QLatin1String("key")).toString();
        if (key.isEmpty())
            continue;
        if (type == QLatin1String("Trailer"))
            return key;
        if (teaser.isEmpty() && type == QLatin1String("Teaser"))
            teaser = key;
    }
    return teaser;
}

} // namespace TmdbImages
