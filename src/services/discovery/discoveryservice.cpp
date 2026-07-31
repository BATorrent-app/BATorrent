// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/discoveryservice.h"
#include "services/discovery/discoveryassemble.h"
#include "services/discovery/discoverysearch.h"
#include "services/discovery/hublogic.h"
#include "services/discovery/igdbparse.h"
#include "services/discovery/tmdbparse.h"
#include "services/platform/contentlanguage.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPair>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QUrlQuery>

namespace {

QString tmdbApiKey()
{
    QString key = QSettings("BATorrent", "BATorrent").value("tmdbApiKey").toString();
#ifdef BAT_TMDB_KEY
    if (key.isEmpty()) key = QStringLiteral(BAT_TMDB_KEY);
#endif
    return key;
}
QString igdbClientId()
{
    QString id = QSettings("BATorrent", "BATorrent").value("igdbClientId").toString();
#ifdef BAT_IGDB_CLIENT_ID
    if (id.isEmpty()) id = QStringLiteral(BAT_IGDB_CLIENT_ID);
#endif
    return id;
}
QString igdbClientSecret()
{
    QString s = QSettings("BATorrent", "BATorrent").value("igdbClientSecret").toString();
#ifdef BAT_IGDB_CLIENT_SECRET
    if (s.isEmpty()) s = QStringLiteral(BAT_IGDB_CLIENT_SECRET);
#endif
    return s;
}

const QString TmdbBaseUrl    = QStringLiteral("https://api.themoviedb.org/3");
const QString TmdbPosterBase = QStringLiteral("https://image.tmdb.org/t/p/w342");
const QString TmdbBackdrop   = QStringLiteral("https://image.tmdb.org/t/p/w1280");

QString tmdbLang() { return ContentLanguage::tmdb(); }

QString cacheFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/discover/discover.json");
}

QString discoverRegion()
{
    return ContentLanguage::region();
}

const qint64 CacheTtlSecs = 12 * 60 * 60;
const int CacheVersion = 8;   // bump when the row schema/order/source changes (invalidates stale cache)

} // namespace

DiscoveryService::DiscoveryService(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
}

QString DiscoveryService::genreKey(const QString &name) const
{
    return HubLogic::genreKey(name);
}

QString DiscoveryService::topGenreFromNames(const QStringList &genreNames) const
{
    return HubLogic::topGenre(genreNames);
}

QVariantList DiscoveryService::applyLibraryView(const QVariantList &list,
                                               const QString &search,
                                               const QString &sort) const
{
    return HubLogic::applyView(list, search, sort);
}

QVariantList DiscoveryService::excludeOwned(const QVariantList &candidates,
                                            const QStringList &ownedTitles,
                                            int limit) const
{
    QSet<QString> owned;
    owned.reserve(ownedTitles.size());
    for (const QString &t : ownedTitles)
        owned.insert(t.toLower());
    return HubLogic::excludeOwnedTitles(candidates, owned, limit);
}

void DiscoveryService::load()
{
    if (!m_rows.isEmpty()) return;        // already populated this session
    if (loadFromCache()) return;          // fresh disk cache
    refresh();
}

void DiscoveryService::refresh()
{
    if (m_loading) return;

    const QString tmdb = tmdbApiKey();
    const bool haveTmdb = !tmdb.isEmpty();
    const bool haveIgdb = !igdbClientId().isEmpty() && !igdbClientSecret().isEmpty();

    if (!haveTmdb && !haveIgdb) {
        setStatus(tr_("discover_no_keys"));
        return;
    }

    m_accum.clear();
    m_pending = 0;
    setLoading(true);
    setStatus(QString());

    // Games lead (orders 0–6): BATorrent's audience is game-first, so its shelves
    // come before movies/series. Movies fill in below (orders 10+).
    if (haveIgdb) {
        fetchIgdbTrending(0, tr_("discover_trending_games"));   // hot & new
        fetchIgdbRecent(1, tr_("discover_new_games"));
        fetchIgdbGames(2, tr_("discover_top_games"),
                       QStringLiteral("rating != null & rating_count >= 100"), QStringLiteral("rating desc"));
        auto gameGenre = [this](int order, const QString &label, int gid) {
            fetchIgdbGames(order, label,
                           QStringLiteral("genres = (%1) & rating_count >= 5").arg(gid),
                           QStringLiteral("rating desc"));
        };
        gameGenre(3, tr_("discover_game_rpg"),      12);
        gameGenre(4, tr_("discover_game_shooter"),   5);
        gameGenre(5, tr_("discover_game_strategy"), 15);
        gameGenre(6, tr_("discover_game_indie"),    32);
    }
    if (haveTmdb) {
        // Country-relative "trending": popularity-sorted titles available to stream/
        // rent/buy in the user's region (TMDB /trending has no region param).
        const QString region = discoverRegion();
        const QList<QPair<QString, QString>> regionDiscover = {
            { QStringLiteral("sort_by"), QStringLiteral("popularity.desc") },
            { QStringLiteral("watch_region"), region },
            { QStringLiteral("with_watch_monetization_types"),
              QStringLiteral("flatrate|free|ads|rent|buy") }
        };
        const QList<QPair<QString, QString>> regionOnly = { { QStringLiteral("region"), region } };
        // Each shelf pulls two pages (~40 titles) so there's plenty to scroll.
        auto shelf = [this](int order, const QString &path, const QString &label,
                            const QString &type, const QList<QPair<QString, QString>> &extra) {
            fetchTmdb(order, path, label, type, extra, 1);
            fetchTmdb(order, path, label, type, extra, 2);
        };
        shelf(10, QStringLiteral("/discover/movie"), tr_("discover_trending_movies"), QStringLiteral("movie"),  regionDiscover);
        shelf(11, QStringLiteral("/movie/popular"),  tr_("discover_popular_movies"),  QStringLiteral("movie"),  regionOnly);
        shelf(12, QStringLiteral("/discover/tv"),    tr_("discover_trending_series"), QStringLiteral("series"), regionDiscover);
        shelf(13, QStringLiteral("/tv/popular"),     tr_("discover_popular_series"),  QStringLiteral("series"), {});
        auto genre = [regionDiscover](int id) {
            QList<QPair<QString, QString>> e = regionDiscover;
            e.append({ QStringLiteral("with_genres"), QString::number(id) });
            return e;
        };
        shelf(14, QStringLiteral("/discover/movie"), tr_("discover_genre_action"), QStringLiteral("movie"), genre(28));
        shelf(15, QStringLiteral("/discover/movie"), tr_("discover_genre_scifi"),  QStringLiteral("movie"), genre(878));
        shelf(16, QStringLiteral("/discover/movie"), tr_("discover_genre_horror"), QStringLiteral("movie"), genre(27));
        shelf(17, QStringLiteral("/movie/top_rated"), tr_("discover_top_movies"),  QStringLiteral("movie"), {});
    }
}

void DiscoveryService::searchTitles(const QString &query)
{
    const QString q = query.trimmed();
    if (q.isEmpty()) return;
    m_searchQuery = q;
    m_searchWorks.clear();
    m_searchPending = 0;

    const bool haveTmdb = !tmdbApiKey().isEmpty();
    const bool haveIgdb = !igdbClientId().isEmpty() && !igdbClientSecret().isEmpty();
    if (!haveTmdb && !haveIgdb) {
        emit titleResults(q, QVariantList());   // no keys → caller falls back to raw
        return;
    }
    if (haveTmdb) searchTmdbTitles(q);
    if (haveIgdb) searchIgdbTitles(q);
}

void DiscoveryService::searchTmdbTitles(const QString &query)
{
    ++m_searchPending;

    QUrl url(TmdbBaseUrl + QStringLiteral("/search/multi"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    q.addQueryItem(QStringLiteral("language"), tmdbLang());
    q.addQueryItem(QStringLiteral("query"), query);
    q.addQueryItem(QStringLiteral("include_adult"), QStringLiteral("false"));
    q.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(12000);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError)
            m_searchWorks += TmdbParse::multiSearchRows(reply->readAll(), TmdbPosterBase);
        maybeFinishSearch();
    });
}

void DiscoveryService::searchIgdbTitles(const QString &query)
{
    ++m_searchPending;
    ensureIgdbToken([this, query]() {
        if (m_igdbToken.isEmpty()) { maybeFinishSearch(); return; }

        QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
        setIgdbHeaders(req);
        QString safe = query;
        safe.replace(QLatin1Char('"'), QLatin1Char(' '));
        // No category filter here: it's unreliable across IGDB game records and
        // was hiding legitimate matches. Relevance from `search` is enough.
        const QByteArray body = QStringLiteral(
            "search \"%1\"; fields name,cover.image_id,first_release_date,total_rating,summary,"
            "screenshots.image_id; where cover != null; limit 20;").arg(safe).toUtf8();

        QNetworkReply *reply = m_nam->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError)
                m_searchWorks += IgdbParse::titleSearchRows(reply->readAll());
            else
                qDebug() << "[search] IGDB title search error:" << reply->errorString();
            maybeFinishSearch();
        });
    });
}

void DiscoveryService::maybeFinishSearch()
{
    if (--m_searchPending > 0) return;
    emit titleResults(m_searchQuery, DiscoverySearch::rankAndMerge(m_searchQuery, m_searchWorks));
}

bool DiscoveryService::hasMetadataKeys() const
{
    const bool haveTmdb = !tmdbApiKey().isEmpty();
    const bool haveIgdb = !igdbClientId().isEmpty() && !igdbClientSecret().isEmpty();
    return haveTmdb || haveIgdb;
}

void DiscoveryService::fetchTrailer(int tmdbId, const QString &type)
{
    if (tmdbId <= 0 || tmdbApiKey().isEmpty()) { emit trailerReady(tmdbId, QString()); return; }
    const QString kind = (type == QLatin1String("series")) ? QStringLiteral("tv") : QStringLiteral("movie");
    QUrl url(TmdbBaseUrl + QStringLiteral("/%1/%2/videos").arg(kind).arg(tmdbId));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);
    QNetworkReply *reply = m_nam->get(req);
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
    const QString kind = isTv ? QStringLiteral("tv") : QStringLiteral("movie");
    QUrl url(TmdbBaseUrl + QStringLiteral("/%1/%2/recommendations").arg(kind).arg(tmdbId));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    q.addQueryItem(QStringLiteral("language"), tmdbLang());
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId, isTv]() {
        reply->deleteLater();
        const QVariantList items = (reply->error() == QNetworkReply::NoError)
            ? TmdbParse::recommendationRows(reply->readAll(), isTv, TmdbPosterBase)
            : QVariantList{};
        emit recommendationsReady(tmdbId, items);
    });
}

void DiscoveryService::fetchGameRecommendations(const QString &gameName)
{
    if (gameName.trimmed().isEmpty()) { emit gameRecommendationsReady(gameName, {}); return; }
    ensureIgdbToken([this, gameName]() {
        if (m_igdbToken.isEmpty()) { emit gameRecommendationsReady(gameName, {}); return; }
        QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
        setIgdbHeaders(req);
        QString safe = gameName; safe.replace(QLatin1Char('"'), QLatin1Char(' '));
        const QByteArray body = QStringLiteral(
            "search \"%1\"; fields similar_games.name, similar_games.cover.image_id,"
            " similar_games.first_release_date, similar_games.rating, similar_games.summary; limit 1;")
            .arg(safe).toUtf8();
        QNetworkReply *reply = m_nam->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [this, reply, gameName]() {
            reply->deleteLater();
            QVariantList items;
            if (reply->error() == QNetworkReply::NoError) {
                const QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
                if (!arr.isEmpty()) {
                    const QJsonArray sims = arr.first().toObject()
                                                .value(QLatin1String("similar_games")).toArray();
                    items = IgdbParse::gameCards(IgdbParse::objectsFromArray(sims), 16);
                }
            } else {
                qDebug() << "[discover] IGDB similar-games error:" << reply->errorString();
            }
            emit gameRecommendationsReady(gameName, items);
        });
    });
}

void DiscoveryService::fetchEpisodes(int tmdbId, int season)
{
    if (tmdbId <= 0 || season < 0 || tmdbApiKey().isEmpty()) { emit episodesReady(tmdbId, season, {}); return; }
    QUrl url(TmdbBaseUrl + QStringLiteral("/tv/%1/season/%2").arg(tmdbId).arg(season));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    q.addQueryItem(QStringLiteral("language"), tmdbLang());
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId, season]() {
        reply->deleteLater();
        const QVariantList eps = (reply->error() == QNetworkReply::NoError)
            ? TmdbParse::episodeRows(reply->readAll())
            : QVariantList{};
        emit episodesReady(tmdbId, season, eps);
    });
}

void DiscoveryService::fetchBackdrops(int tmdbId, const QString &type)
{
    if (tmdbId <= 0 || tmdbApiKey().isEmpty()) { emit backdropsReady(tmdbId, {}); return; }
    const QString kind = (type == QLatin1String("series")) ? QStringLiteral("tv") : QStringLiteral("movie");
    QUrl url(TmdbBaseUrl + QStringLiteral("/%1/%2/images").arg(kind).arg(tmdbId));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId]() {
        reply->deleteLater();
        const QStringList urls = (reply->error() == QNetworkReply::NoError)
            ? TmdbParse::backdropUrls(reply->readAll(), TmdbBackdrop)
            : QStringList{};
        emit backdropsReady(tmdbId, urls);
    });
}

void DiscoveryService::fetchTmdb(int order, const QString &path, const QString &label, const QString &type,
                                 const QList<QPair<QString, QString>> &extra, int page)
{
    ++m_pending;

    QUrl url(TmdbBaseUrl + path);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    q.addQueryItem(QStringLiteral("language"), tmdbLang());
    q.addQueryItem(QStringLiteral("page"), QString::number(page));
    for (const auto &kv : extra) q.addQueryItem(kv.first, kv.second);
    url.setQuery(q);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(12000);

    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, order, label, type]() {
        reply->deleteLater();

        const bool isTv = type == QLatin1String("series");
        const QVariantList items = (reply->error() == QNetworkReply::NoError)
            ? TmdbParse::shelfRows(reply->readAll(), isTv, TmdbPosterBase, TmdbBackdrop)
            : QVariantList{};
        // Merge-append: a shelf is fetched across multiple pages, all sharing
        // one `order`. Dedup by poster URL so a page overlap can't double a title.
        QVariantMap row = m_accum.value(order);
        const QVariantList merged = DiscoveryAssemble::mergeShelfByPoster(
            row.value(QStringLiteral("items")).toList(), items);
        row.insert(QStringLiteral("label"), label);
        row.insert(QStringLiteral("items"), merged);
        if (!merged.isEmpty()) m_accum.insert(order, row);
        maybeFinish();
    });
}

void DiscoveryService::ensureIgdbToken(std::function<void()> then)
{
    if (!m_igdbToken.isEmpty() && QDateTime::currentSecsSinceEpoch() < m_igdbTokenExpiry) {
        then();
        return;
    }
    QUrl url(QStringLiteral("https://id.twitch.tv/oauth2/token"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), igdbClientId());
    q.addQueryItem(QStringLiteral("client_secret"), igdbClientSecret());
    q.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("client_credentials"));
    url.setQuery(q);

    QNetworkRequest req{url};
    req.setTransferTimeout(12000);
    QNetworkReply *reply = m_nam->post(req, QByteArray());
    connect(reply, &QNetworkReply::finished, this, [this, reply, then]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
            m_igdbToken = o.value(QLatin1String("access_token")).toString();
            const int exp = o.value(QLatin1String("expires_in")).toInt(3600);
            m_igdbTokenExpiry = QDateTime::currentSecsSinceEpoch() + exp - 60;
        }
        then();
    });
}

void DiscoveryService::setIgdbHeaders(QNetworkRequest &req) const
{
    req.setRawHeader("Client-ID", igdbClientId().toUtf8());
    req.setRawHeader("Authorization", ("Bearer " + m_igdbToken).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    req.setTransferTimeout(12000);
}

// Trending = IGDB popularity primitives (currently most-active games), then the
// game records fetched by id and re-sorted into that popularity order.
void DiscoveryService::fetchIgdbTrending(int order, const QString &label)
{
    ++m_pending;
    ensureIgdbToken([this, order, label]() {
        if (m_igdbToken.isEmpty()) {
            qDebug() << "[discover] IGDB token empty — skipping trending games";
            maybeFinish();
            return;
        }
        if (m_hypeTypeId > 0) { fetchIgdbHypeIds(order, label); return; }
        // Resolve a torrent-relevant popularity type once. The default primitive
        // (Visits) just ranks perennial free games (LoL/CS/GTA V). Prefer "Global
        // Top Sellers" (paid games selling now), then "Want to Play" (anticipation)
        // — combined with the recent-release filter below, that's "hot & new".
        QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/popularity_types"))};
        setIgdbHeaders(req);
        QNetworkReply *reply = m_nam->post(req, QByteArray("fields id,name; limit 50;"));
        connect(reply, &QNetworkReply::finished, this, [this, reply, order, label]() {
            reply->deleteLater();
            const QByteArray body = (reply->error() == QNetworkReply::NoError)
                ? reply->readAll() : QByteArray{};
            m_hypeTypeId = IgdbParse::pickHypeTypeId(body);
            fetchIgdbHypeIds(order, label);
        });
    });
}

void DiscoveryService::fetchIgdbHypeIds(int order, const QString &label)
{
    QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/popularity_primitives"))};
    setIgdbHeaders(req);
    // Big pool: most of these get filtered out by the recent-release window in
    // fetchIgdbGamesByIds, so we over-fetch to still land ~24 recent hyped games.
    const QByteArray body = QStringLiteral(
        "fields game_id,value; where popularity_type = %1; sort value desc; limit 120;")
        .arg(m_hypeTypeId).toUtf8();
    QNetworkReply *reply = m_nam->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply, order, label]() {
        reply->deleteLater();
        QList<qint64> ids;
        if (reply->error() == QNetworkReply::NoError)
            ids = IgdbParse::orderedGameIds(reply->readAll());
        else
            qDebug() << "[discover] IGDB popularity error:" << reply->errorString();
        if (ids.isEmpty()) { maybeFinish(); return; }
        fetchIgdbGamesByIds(order, label, ids);
    });
}

void DiscoveryService::fetchIgdbGamesByIds(int order, const QString &label, const QList<qint64> &ids)
{
    QStringList idStrs;
    for (qint64 id : ids) idStrs << QString::number(id);

    // Only keep ones released in the last ~10 months (and already out — torrentable),
    // so the hype list becomes "hot & new", not perennial anticipated/old titles.
    const qint64 now = QDateTime::currentSecsSinceEpoch();
    const qint64 from = now - qint64(300) * 86400;

    QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
    setIgdbHeaders(req);
    const QByteArray body = QStringLiteral(
        "fields id,name,summary,rating,first_release_date,cover.image_id,artworks.image_id,screenshots.image_id;"
        " where id = (%1) & cover != null & first_release_date >= %3 & first_release_date <= %4; limit %2;")
        .arg(idStrs.join(QLatin1Char(','))).arg(ids.size()).arg(from).arg(now).toUtf8();

    QNetworkReply *reply = m_nam->post(req, body);
    connect(reply, &QNetworkReply::finished, this, [this, reply, order, label, ids]() {
        reply->deleteLater();
        QVariantList items;
        if (reply->error() == QNetworkReply::NoError) {
            const auto objs = IgdbParse::sortObjectsByIdRank(
                IgdbParse::objectsFromJson(reply->readAll()), ids);
            items = IgdbParse::gameCards(objs, 24);
        } else {
            qDebug() << "[discover] IGDB games-by-id error:" << reply->errorString();
        }
        QVariantMap row;
        row.insert(QStringLiteral("label"), label);
        row.insert(QStringLiteral("items"), items);
        if (!items.isEmpty()) m_accum.insert(order, row);
        maybeFinish();
    });
}

void DiscoveryService::fetchIgdbRecent(int order, const QString &label)
{
    ++m_pending;
    ensureIgdbToken([this, order, label]() {
        if (m_igdbToken.isEmpty()) { maybeFinish(); return; }
        const qint64 now = QDateTime::currentSecsSinceEpoch();
        const qint64 from = now - qint64(182) * 86400;     // ~6 months

        QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
        setIgdbHeaders(req);
        const QByteArray body = QStringLiteral(
            "fields name,summary,rating,first_release_date,cover.image_id,artworks.image_id,screenshots.image_id;"
            " where cover != null & first_release_date > %1 & first_release_date <= %2 & rating_count >= 3;"
            " sort first_release_date desc; limit 40;").arg(from).arg(now).toUtf8();

        QNetworkReply *reply = m_nam->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [this, reply, order, label]() {
            reply->deleteLater();
            QVariantList items;
            if (reply->error() == QNetworkReply::NoError)
                items = IgdbParse::gameCardsFromJson(reply->readAll(), 24);
            else
                qDebug() << "[discover] IGDB recent error:" << reply->errorString();
            QVariantMap row;
            row.insert(QStringLiteral("label"), label);
            row.insert(QStringLiteral("items"), items);
            if (!items.isEmpty()) m_accum.insert(order, row);
            maybeFinish();
        });
    });
}

// Generic games shelf: any apicalypse where-clause + sort (e.g. a genre, or
// best-rated). `cover != null` is always enforced so every card has art.
void DiscoveryService::fetchIgdbGames(int order, const QString &label,
                                      const QString &whereClause, const QString &sort)
{
    ++m_pending;
    ensureIgdbToken([this, order, label, whereClause, sort]() {
        if (m_igdbToken.isEmpty()) { maybeFinish(); return; }
        QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
        setIgdbHeaders(req);
        const QByteArray body = QStringLiteral(
            "fields name,summary,rating,first_release_date,cover.image_id,artworks.image_id,screenshots.image_id;"
            " where cover != null & %1; sort %2; limit 40;").arg(whereClause, sort).toUtf8();
        QNetworkReply *reply = m_nam->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [this, reply, order, label]() {
            reply->deleteLater();
            QVariantList items;
            if (reply->error() == QNetworkReply::NoError)
                items = IgdbParse::gameCardsFromJson(reply->readAll(), 30);
            else
                qDebug() << "[discover] IGDB games shelf error:" << reply->errorString();
            QVariantMap row;
            row.insert(QStringLiteral("label"), label);
            row.insert(QStringLiteral("items"), items);
            if (!items.isEmpty()) m_accum.insert(order, row);
            maybeFinish();
        });
    });
}

void DiscoveryService::maybeFinish()
{
    if (--m_pending > 0) return;
    assembleAndEmit();
    saveToCache();
    setLoading(false);
    if (m_rows.isEmpty()) setStatus(tr_("discover_empty"));
}

void DiscoveryService::assembleAndEmit()
{
    m_rows = DiscoveryAssemble::rowsFromAccum(m_accum);
    m_hero = DiscoveryAssemble::heroFromAccum(m_accum);
    emit rowsChanged();
    emit heroChanged();
}

bool DiscoveryService::loadFromCache()
{
    QFile f(cacheFile());
    if (!f.open(QIODevice::ReadOnly)) return false;
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    if (o.value(QLatin1String("v")).toInt() != CacheVersion) return false;
    if (o.value(QLatin1String("region")).toString() != discoverRegion()) return false;  // region changed → refetch
    const QDateTime savedAt = QDateTime::fromString(o.value(QLatin1String("savedAt")).toString(), Qt::ISODate);
    if (!savedAt.isValid()) return false;
    if (savedAt.secsTo(QDateTime::currentDateTimeUtc()) > CacheTtlSecs) return false;

    m_rows = o.value(QLatin1String("rows")).toArray().toVariantList();
    m_hero = o.value(QLatin1String("hero")).toArray().toVariantList();
    if (m_rows.isEmpty()) return false;

    emit rowsChanged();
    emit heroChanged();
    return true;
}

void DiscoveryService::saveToCache()
{
    if (m_rows.isEmpty()) return;
    QDir().mkpath(QFileInfo(cacheFile()).absolutePath());
    QJsonObject o;
    o.insert(QStringLiteral("v"), CacheVersion);
    o.insert(QStringLiteral("region"), discoverRegion());
    o.insert(QStringLiteral("savedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    o.insert(QStringLiteral("rows"), QJsonArray::fromVariantList(m_rows));
    o.insert(QStringLiteral("hero"), QJsonArray::fromVariantList(m_hero));
    QFile f(cacheFile());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

void DiscoveryService::setLoading(bool on)
{
    if (m_loading == on) return;
    m_loading = on;
    emit loadingChanged();
}

void DiscoveryService::setStatus(const QString &s)
{
    if (m_status == s) return;
    m_status = s;
    emit statusChanged();
}
