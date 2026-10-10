// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details
//
// DiscoveryService: per-title TMDB lookups (trailer, recommendations, season
// episodes, artwork, details) for a title the user already picked.

#include "services/discovery/discoveryservice.h"
#include "services/discovery/discoveryservice_keys.h"
#include "services/discovery/tmdbparse.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>

using namespace DiscoveryKeys;

namespace {

QString tmdbKind(const QString &type)
{
    return type == QLatin1String("series") ? QStringLiteral("tv") : QStringLiteral("movie");
}

} // namespace

QNetworkReply *DiscoveryService::tmdbGet(const QString &path,
                                         const QList<QPair<QString, QString>> &extra,
                                         int timeoutMs)
{
    QUrl url(TmdbBaseUrl + path);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    for (const auto &kv : extra) q.addQueryItem(kv.first, kv.second);
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(timeoutMs);
    return m_nam->get(req);
}

void DiscoveryService::fetchTrailer(int tmdbId, const QString &type)
{
    if (tmdbId <= 0 || tmdbApiKey().isEmpty()) { emit trailerReady(tmdbId, QString()); return; }
    QNetworkReply *reply = tmdbGet(QStringLiteral("/%1/%2/videos").arg(tmdbKind(type)).arg(tmdbId));
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId]() {
        reply->deleteLater();
        const QString key = (reply->error() == QNetworkReply::NoError)
            ? TmdbParse::youtubeTrailerKey(reply->readAll())
            : QString{};
        emit trailerReady(tmdbId, key);
    });
}

void DiscoveryService::fetchRecommendations(int tmdbId, const QString &type)
{
    if (tmdbId <= 0 || tmdbApiKey().isEmpty()) { emit recommendationsReady(tmdbId, {}); return; }
    const bool isTv = (type == QLatin1String("series"));
    QNetworkReply *reply = tmdbGet(QStringLiteral("/%1/%2/recommendations").arg(tmdbKind(type)).arg(tmdbId),
                                   { { QStringLiteral("language"), tmdbLang() } });
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId, isTv]() {
        reply->deleteLater();
        const QVariantList items = (reply->error() == QNetworkReply::NoError)
            ? TmdbParse::recommendationRows(reply->readAll(), isTv, TmdbPosterBase)
            : QVariantList{};
        emit recommendationsReady(tmdbId, items);
    });
}

void DiscoveryService::fetchEpisodes(int tmdbId, int season)
{
    if (tmdbId <= 0 || season < 0 || tmdbApiKey().isEmpty()) { emit episodesReady(tmdbId, season, {}); return; }
    QNetworkReply *reply = tmdbGet(QStringLiteral("/tv/%1/season/%2").arg(tmdbId).arg(season),
                                   { { QStringLiteral("language"), tmdbLang() },
                                     { QStringLiteral("append_to_response"), QStringLiteral("credits") } });
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId, season]() {
        reply->deleteLater();
        const QByteArray body = (reply->error() == QNetworkReply::NoError)
            ? reply->readAll() : QByteArray{};
        emit episodesReady(tmdbId, season, TmdbParse::episodeRows(body, TmdbStillBase));
        // An anthology recasts every season; the show's own credits are the latest one's.
        emit seasonCastReady(tmdbId, season,
                             TmdbParse::workDetails(body).value(QStringLiteral("cast")).toStringList());
    });
}

void DiscoveryService::fetchBackdrops(int tmdbId, const QString &type)
{
    if (tmdbId <= 0 || tmdbApiKey().isEmpty()) { emit backdropsReady(tmdbId, {}); return; }
    QNetworkReply *reply = tmdbGet(QStringLiteral("/%1/%2/images").arg(tmdbKind(type)).arg(tmdbId));
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId]() {
        reply->deleteLater();
        // One read: readAll() empties the device, and the logos ride in the
        // same payload as the backdrops.
        const QByteArray body = (reply->error() == QNetworkReply::NoError)
            ? reply->readAll() : QByteArray{};
        emit backdropsReady(tmdbId, TmdbParse::backdropUrls(body, TmdbBackdrop));
        emit logoReady(tmdbId, TmdbParse::logoUrl(body, TmdbLogoBase));
    });
}

void DiscoveryService::fetchWorkDetails(int tmdbId, const QString &type)
{
    if (tmdbId <= 0 || tmdbApiKey().isEmpty()) { emit workDetailsReady(tmdbId, {}); return; }
    QNetworkReply *reply = tmdbGet(QStringLiteral("/%1/%2").arg(tmdbKind(type)).arg(tmdbId),
                                   { { QStringLiteral("language"), tmdbLang() },
                                     { QStringLiteral("append_to_response"), QStringLiteral("credits") } });
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId]() {
        reply->deleteLater();
        const QVariantMap details = (reply->error() == QNetworkReply::NoError)
            ? TmdbParse::workDetails(reply->readAll())
            : QVariantMap{};
        emit workDetailsReady(tmdbId, details);
    });
}

void DiscoveryService::fetchLogo(int tmdbId, const QString &type)
{
    if (tmdbId <= 0 || (type != QLatin1String("movie") && type != QLatin1String("series"))) return;
    const QString key = type + QLatin1Char(':') + QString::number(tmdbId);
    const auto hit = m_logoCache.constFind(key);
    if (hit != m_logoCache.constEnd()) {
        const QString url = *hit;
        // Queued, so a tile that asks while it is being created can still hear it.
        QTimer::singleShot(0, this, [this, tmdbId, type, url]() { emit titleLogoReady(tmdbId, type, url); });
        return;
    }
    if (m_logoPending.contains(key) || tmdbApiKey().isEmpty()) return;
    m_logoPending.insert(key);
    QNetworkReply *reply = tmdbGet(QStringLiteral("/%1/%2/images").arg(tmdbKind(type)).arg(tmdbId),
                                   { { QStringLiteral("include_image_language"), QStringLiteral("en,null") } });
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId, type, key]() {
        reply->deleteLater();
        m_logoPending.remove(key);
        if (reply->error() != QNetworkReply::NoError) return;   // not cached: a later tile retries
        const QString url = TmdbParse::logoUrl(reply->readAll(), TmdbLogoBase);
        m_logoCache.insert(key, url);
        emit titleLogoReady(tmdbId, type, url);
    });
}
