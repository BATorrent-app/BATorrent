// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/tmdbparse.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QVariantMap>

namespace TmdbParse {

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

static QString yearFromDate(const QString &date)
{
    return date.length() >= 4 ? date.left(4) : QString();
}

static QVariantMap catalogCard(const QJsonObject &o,
                               bool isTv,
                               const QString &posterBase,
                               const QString &backdropBase,
                               bool withTmdbId,
                               bool withOriginalTitle)
{
    const QString poster = o.value(QLatin1String("poster_path")).toString();
    if (poster.isEmpty())
        return {};

    const QString date = o.value(isTv ? QLatin1String("first_air_date")
                                      : QLatin1String("release_date")).toString();
    QVariantMap m;
    m.insert(QStringLiteral("title"),
             o.value(isTv ? QLatin1String("name") : QLatin1String("title")).toString());
    if (withOriginalTitle) {
        m.insert(QStringLiteral("originalTitle"),
                 o.value(isTv ? QLatin1String("original_name")
                              : QLatin1String("original_title")).toString());
    }
    m.insert(QStringLiteral("poster"), posterBase + poster);
    if (!backdropBase.isEmpty()) {
        const QString backdrop = o.value(QLatin1String("backdrop_path")).toString();
        m.insert(QStringLiteral("backdrop"),
                 backdrop.isEmpty() ? QString() : backdropBase + backdrop);
    }
    m.insert(QStringLiteral("year"), yearFromDate(date));
    m.insert(QStringLiteral("rating"), o.value(QLatin1String("vote_average")).toDouble());
    m.insert(QStringLiteral("overview"), o.value(QLatin1String("overview")).toString());
    m.insert(QStringLiteral("type"), isTv ? QStringLiteral("series") : QStringLiteral("movie"));
    if (withTmdbId)
        m.insert(QStringLiteral("tmdbId"), o.value(QLatin1String("id")).toInt());
    return m;
}

static QJsonArray resultsArray(const QByteArray &json)
{
    if (json.isEmpty())
        return {};
    return QJsonDocument::fromJson(json).object().value(QLatin1String("results")).toArray();
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

QVariantList episodeRows(const QByteArray &seasonJson)
{
    QVariantList eps;
    if (seasonJson.isEmpty())
        return eps;

    const QJsonArray arr = QJsonDocument::fromJson(seasonJson)
                               .object()
                               .value(QLatin1String("episodes"))
                               .toArray();
    eps.reserve(arr.size());
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        QVariantMap m;
        m.insert(QStringLiteral("episode"), o.value(QLatin1String("episode_number")).toInt());
        m.insert(QStringLiteral("name"), o.value(QLatin1String("name")).toString());
        m.insert(QStringLiteral("air_date"), o.value(QLatin1String("air_date")).toString());
        eps << m;
    }
    return eps;
}

QVariantList recommendationRows(const QByteArray &json,
                                bool isTv,
                                const QString &posterBase,
                                int limit)
{
    QVariantList items;
    if (limit <= 0)
        return items;
    for (const QJsonValue &v : resultsArray(json)) {
        const QVariantMap m = catalogCard(v.toObject(), isTv, posterBase, {}, false, false);
        if (m.isEmpty())
            continue;
        items.append(m);
        if (items.size() >= limit)
            break;
    }
    return items;
}

QVariantList shelfRows(const QByteArray &json,
                       bool isTv,
                       const QString &posterBase,
                       const QString &backdropBase)
{
    QVariantList items;
    for (const QJsonValue &v : resultsArray(json)) {
        const QVariantMap m = catalogCard(v.toObject(), isTv, posterBase, backdropBase, true, false);
        if (!m.isEmpty())
            items.append(m);
    }
    return items;
}

QVariantList multiSearchRows(const QByteArray &json, const QString &posterBase)
{
    QVariantList items;
    for (const QJsonValue &v : resultsArray(json)) {
        const QJsonObject o = v.toObject();
        const QString mt = o.value(QLatin1String("media_type")).toString();
        if (mt != QLatin1String("movie") && mt != QLatin1String("tv"))
            continue;
        const bool isTv = mt == QLatin1String("tv");
        const QVariantMap m = catalogCard(o, isTv, posterBase, {}, true, true);
        if (!m.isEmpty())
            items.append(m);
    }
    return items;
}

} // namespace TmdbParse
