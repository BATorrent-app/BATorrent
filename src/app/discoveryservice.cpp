// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "discoveryservice.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QUrlQuery>
#include <QHash>
#include <QSet>
#include <QLocale>
#include <QPair>
#include <algorithm>
#include "translator.h"

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

QString tmdbLang()
{
    switch (Translator::instance().language()) {
    case Translator::Portuguese: return QStringLiteral("pt-BR");
    case Translator::Chinese:    return QStringLiteral("zh-CN");
    case Translator::Japanese:   return QStringLiteral("ja-JP");
    case Translator::Russian:    return QStringLiteral("ru-RU");
    case Translator::Spanish:    return QStringLiteral("es-ES");
    case Translator::German:     return QStringLiteral("de-DE");
    default:                     return QStringLiteral("en-US");
    }
}

QString cacheFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/discover/discover.json");
}

// The user's country (ISO 3166), for country-relative TMDB rows. From the system
// locale (e.g. "pt_BR" → BR), falling back to the app UI language's home country.
QString discoverRegion()
{
    const QString sys = QLocale::system().name();        // e.g. "pt_BR"
    const int us = sys.indexOf(QLatin1Char('_'));
    if (us >= 0 && sys.size() >= us + 3) {
        const QString cc = sys.mid(us + 1, 2).toUpper();
        if (cc[0].isLetter() && cc[1].isLetter()) return cc;
    }
    switch (Translator::instance().language()) {
    case Translator::Portuguese: return QStringLiteral("BR");
    case Translator::Russian:    return QStringLiteral("RU");
    case Translator::Japanese:   return QStringLiteral("JP");
    case Translator::German:     return QStringLiteral("DE");
    case Translator::Spanish:    return QStringLiteral("ES");
    case Translator::Ukrainian:  return QStringLiteral("UA");
    case Translator::Chinese:    return QStringLiteral("CN");
    default:                     return QStringLiteral("US");
    }
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
        // genre shelves (popular within a genre, region-aware)
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
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonArray results = QJsonDocument::fromJson(reply->readAll())
                                           .object().value(QLatin1String("results")).toArray();
            for (const QJsonValue &v : results) {
                const QJsonObject o = v.toObject();
                const QString mt = o.value(QLatin1String("media_type")).toString();
                if (mt != QLatin1String("movie") && mt != QLatin1String("tv")) continue;
                const QString poster = o.value(QLatin1String("poster_path")).toString();
                if (poster.isEmpty()) continue;
                const bool isTv = mt == QLatin1String("tv");
                const QString date = o.value(isTv ? QLatin1String("first_air_date")
                                                   : QLatin1String("release_date")).toString();
                QVariantMap m;
                m.insert(QStringLiteral("title"), o.value(isTv ? QLatin1String("name")
                                                               : QLatin1String("title")).toString());
                m.insert(QStringLiteral("poster"), TmdbPosterBase + poster);
                m.insert(QStringLiteral("year"), date.length() >= 4 ? date.left(4) : QString());
                m.insert(QStringLiteral("rating"), o.value(QLatin1String("vote_average")).toDouble());
                m.insert(QStringLiteral("overview"), o.value(QLatin1String("overview")).toString());
                m.insert(QStringLiteral("type"), isTv ? QStringLiteral("series") : QStringLiteral("movie"));
                m_searchWorks.append(m);
            }
        }
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
            "search \"%1\"; fields name,cover.image_id,first_release_date,total_rating,summary;"
            " where cover != null; limit 20;").arg(safe).toUtf8();

        QNetworkReply *reply = m_nam->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() == QNetworkReply::NoError) {
                const QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
                for (const QJsonValue &v : arr) {
                    const QJsonObject o = v.toObject();
                    const QString imageId = o.value(QLatin1String("cover")).toObject()
                                                .value(QLatin1String("image_id")).toString();
                    if (imageId.isEmpty()) continue;
                    const QString name = o.value(QLatin1String("name")).toString();
                    if (name.isEmpty()) continue;
                    const qint64 rel = qint64(o.value(QLatin1String("first_release_date")).toDouble());
                    QVariantMap m;
                    m.insert(QStringLiteral("title"), name);
                    m.insert(QStringLiteral("poster"),
                             QStringLiteral("https://images.igdb.com/igdb/image/upload/t_cover_big/%1.jpg").arg(imageId));
                    m.insert(QStringLiteral("year"), rel > 0
                             ? QString::number(QDateTime::fromSecsSinceEpoch(rel).date().year()) : QString());
                    m.insert(QStringLiteral("rating"), o.value(QLatin1String("total_rating")).toDouble() / 10.0);
                    m.insert(QStringLiteral("overview"), o.value(QLatin1String("summary")).toString());
                    m.insert(QStringLiteral("type"), QStringLiteral("game"));
                    m_searchWorks.append(m);
                }
            } else {
                qDebug() << "[search] IGDB title search error:" << reply->errorString();
            }
            maybeFinishSearch();
        });
    });
}

void DiscoveryService::maybeFinishSearch()
{
    if (--m_searchPending > 0) return;

    const QString ql = m_searchQuery.toLower();
    auto score = [&ql](const QVariant &v) {
        const QString t = v.toMap().value(QStringLiteral("title")).toString().toLower();
        if (t == ql) return 0;
        if (t.startsWith(ql)) return 1;
        if (t.contains(ql)) return 2;
        return 3;
    };
    auto byScore = [&score](const QVariant &a, const QVariant &b) { return score(a) < score(b); };

    // Split by kind, relevance-sort each, then interleave so movies and games
    // mix from the top instead of "all movies, then all games".
    QVariantList vids, games;
    for (const QVariant &v : std::as_const(m_searchWorks)) {
        if (v.toMap().value(QStringLiteral("type")).toString() == QLatin1String("game")) games.append(v);
        else vids.append(v);
    }
    std::stable_sort(vids.begin(), vids.end(), byScore);
    std::stable_sort(games.begin(), games.end(), byScore);

    QVariantList merged;
    for (int i = 0; i < vids.size() || i < games.size(); ++i) {
        if (i < games.size()) merged.append(games[i]);   // game first: queries are usually game-led
        if (i < vids.size())  merged.append(vids[i]);
    }

    QVariantList out;
    QSet<QString> seen;
    for (const QVariant &v : std::as_const(merged)) {
        const QVariantMap m = v.toMap();
        const QString key = m.value(QStringLiteral("title")).toString().toLower() + QLatin1Char('|')
                          + m.value(QStringLiteral("year")).toString() + QLatin1Char('|')
                          + m.value(QStringLiteral("type")).toString();
        if (seen.contains(key)) continue;
        seen.insert(key);
        out.append(v);
    }
    emit titleResults(m_searchQuery, out);
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
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());   // no language: trailers are usually en-tagged
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, tmdbId]() {
        reply->deleteLater();
        QString key, teaser;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonArray arr = QJsonDocument::fromJson(reply->readAll())
                                       .object().value(QLatin1String("results")).toArray();
            for (const QJsonValue &v : arr) {
                const QJsonObject o = v.toObject();
                if (o.value(QLatin1String("site")).toString() != QLatin1String("YouTube")) continue;
                const QString t = o.value(QLatin1String("type")).toString();
                const QString k = o.value(QLatin1String("key")).toString();
                if (t == QLatin1String("Trailer")) { key = k; break; }       // prefer an official trailer
                if (teaser.isEmpty() && t == QLatin1String("Teaser")) teaser = k;
            }
            if (key.isEmpty()) key = teaser;
        }
        emit trailerReady(tmdbId, key);
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

        QVariantList items;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonArray results = QJsonDocument::fromJson(reply->readAll())
                                           .object().value(QLatin1String("results")).toArray();
            for (const QJsonValue &v : results) {
                const QJsonObject o = v.toObject();
                const QString poster = o.value(QLatin1String("poster_path")).toString();
                if (poster.isEmpty()) continue;
                const bool isTv = type == QLatin1String("series");
                const QString date = o.value(isTv ? QLatin1String("first_air_date")
                                                   : QLatin1String("release_date")).toString();
                QVariantMap m;
                m.insert(QStringLiteral("title"), o.value(isTv ? QLatin1String("name")
                                                               : QLatin1String("title")).toString());
                m.insert(QStringLiteral("poster"), TmdbPosterBase + poster);
                const QString backdrop = o.value(QLatin1String("backdrop_path")).toString();
                m.insert(QStringLiteral("backdrop"), backdrop.isEmpty() ? QString() : TmdbBackdrop + backdrop);
                m.insert(QStringLiteral("year"), date.length() >= 4 ? date.left(4) : QString());
                m.insert(QStringLiteral("rating"), o.value(QLatin1String("vote_average")).toDouble());
                m.insert(QStringLiteral("overview"), o.value(QLatin1String("overview")).toString());
                m.insert(QStringLiteral("type"), type);
                m.insert(QStringLiteral("tmdbId"), o.value(QLatin1String("id")).toInt());
                items.append(m);
            }
        }
        // Merge-append: a shelf is fetched across multiple pages, all sharing
        // one `order`. Dedup by poster URL so a page overlap can't double a title.
        QVariantMap row = m_accum.value(order);
        QVariantList merged = row.value(QStringLiteral("items")).toList();
        QSet<QString> seen;
        for (const QVariant &v : std::as_const(merged))
            seen.insert(v.toMap().value(QStringLiteral("poster")).toString());
        for (const QVariant &v : std::as_const(items)) {
            const QString key = v.toMap().value(QStringLiteral("poster")).toString();
            if (!seen.contains(key)) { merged.append(v); seen.insert(key); }
        }
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

namespace {

// IGDB game object → poster card map (cover required, de-duped by title, capped).
QVariantList gamesToItems(const QList<QJsonObject> &objs, int cap)
{
    QVariantList items;
    QStringList seen;
    for (const QJsonObject &o : objs) {
        if (items.size() >= cap) break;
        const QString imageId = o.value(QLatin1String("cover")).toObject()
                                    .value(QLatin1String("image_id")).toString();
        if (imageId.isEmpty()) continue;
        const QString name = o.value(QLatin1String("name")).toString();
        if (name.isEmpty() || seen.contains(name)) continue;
        // IGDB has no "free" flag — drop the obvious free-to-play / live-service /
        // gacha / MMO titles. They dominate game popularity and ratings but make
        // no sense to download via torrent (you just install them for free, or
        // they're server-side). Substrings are specific enough not to catch paid
        // franchise entries (e.g. "warzone" not "call of duty").
        static const QStringList freeLiveService = {
            // arena / hero shooters / battle royale
            "counter-strike", "dota 2", "league of legends", "fortnite", "valorant",
            "apex legends", "warframe", "roblox", "marvel rivals", "the finals",
            "overwatch", "destiny 2", "team fortress", "warzone", "rocket league",
            "fall guys", "brawlhalla", "naraka", "smite", "paladins", "splitgate",
            "the first descendant", "xdefiant", "deadlock", "multiversus", "the day before",
            "pubg", "playerunknown", "delta force", "crossfire", "spellbreak", "hyper scape",
            // gacha / live-service RPG
            "genshin", "honkai", "wuthering waves", "zenless zone zero", "tower of fantasy",
            "blue archive", "arknights", "nikke", "goddess of victory", "punishing: gray raven",
            "epic seven", "summoners war", "raid: shadow legends", "diablo immortal",
            // MMO (server-side, not torrentable)
            "lost ark", "new world", "throne and liberty", "once human", "world of warcraft",
            "final fantasy xiv", "elder scrolls online", "star wars: the old republic",
            "guild wars", "neverwinter", "runescape", "eve online", "star trek online",
            "dc universe online", "phantasy star online", "blade and soul", "blade & soul",
            "tera online", "vindictus", "dauntless", "albion online", "lord of the rings online",
            // F2P military / vehicle / card
            "war thunder", "world of tanks", "world of warships", "crossout", "enlisted",
            "star conflict", "hearthstone", "legends of runeterra", "magic: the gathering arena",
            "yu-gi-oh! master duel", "yu-gi-oh master duel", "marvel snap",
            // F2P mobile/crossplay that show up by popularity
            "mobile legends", "honor of kings", "clash of", "pokemon unite", "pokemon go" };
        const QString lname = name.toLower();
        bool freeLs = false;
        for (const QString &d : freeLiveService) if (lname.contains(d)) { freeLs = true; break; }
        if (freeLs) continue;
        seen.append(name);
        const qint64 rel = qint64(o.value(QLatin1String("first_release_date")).toDouble());
        // games have no backdrop field — use an artwork (or a screenshot) so they
        // can headline the hero banner too.
        QString backdrop;
        const QJsonArray arts = o.value(QLatin1String("artworks")).toArray();
        if (!arts.isEmpty()) {
            const QString aid = arts.first().toObject().value(QLatin1String("image_id")).toString();
            if (!aid.isEmpty())
                backdrop = QStringLiteral("https://images.igdb.com/igdb/image/upload/t_1080p/%1.jpg").arg(aid);
        }
        if (backdrop.isEmpty()) {
            const QJsonArray shots = o.value(QLatin1String("screenshots")).toArray();
            if (!shots.isEmpty()) {
                const QString sid = shots.first().toObject().value(QLatin1String("image_id")).toString();
                if (!sid.isEmpty())
                    backdrop = QStringLiteral("https://images.igdb.com/igdb/image/upload/t_screenshot_huge/%1.jpg").arg(sid);
            }
        }
        QVariantMap m;
        m.insert(QStringLiteral("title"), name);
        m.insert(QStringLiteral("poster"),
                 QStringLiteral("https://images.igdb.com/igdb/image/upload/t_cover_big/%1.jpg").arg(imageId));
        m.insert(QStringLiteral("backdrop"), backdrop);
        m.insert(QStringLiteral("year"), rel > 0
                 ? QString::number(QDateTime::fromSecsSinceEpoch(rel).date().year()) : QString());
        m.insert(QStringLiteral("rating"), o.value(QLatin1String("rating")).toDouble() / 10.0);
        m.insert(QStringLiteral("overview"), o.value(QLatin1String("summary")).toString());
        m.insert(QStringLiteral("type"), QStringLiteral("game"));
        items.append(m);
    }
    return items;
}

QList<QJsonObject> toObjects(const QJsonArray &arr)
{
    QList<QJsonObject> objs;
    for (const QJsonValue &v : arr) objs.append(v.toObject());
    return objs;
}

} // namespace

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
            int sellers = 0, want = 0, playing = 0;
            if (reply->error() == QNetworkReply::NoError) {
                const QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
                for (const QJsonValue &v : arr) {
                    const QJsonObject o = v.toObject();
                    const QString name = o.value(QLatin1String("name")).toString().toLower();
                    const int id = o.value(QLatin1String("id")).toInt();
                    if (name.contains(QLatin1String("top seller"))) sellers = id;
                    else if (name.contains(QLatin1String("want to play"))) want = id;
                    else if (name.contains(QLatin1String("playing"))) playing = id;
                }
            }
            m_hypeTypeId = sellers ? sellers : (want ? want : (playing ? playing : 9));   // 9 = Global Top Sellers
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
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
            for (const QJsonValue &v : arr) {
                const qint64 gid = qint64(v.toObject().value(QLatin1String("game_id")).toDouble());
                if (gid > 0 && !ids.contains(gid)) ids.append(gid);
            }
        } else {
            qDebug() << "[discover] IGDB popularity error:" << reply->errorString();
        }
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
            QList<QJsonObject> objs = toObjects(QJsonDocument::fromJson(reply->readAll()).array());
            QHash<qint64, int> rank;                       // id → popularity position
            for (int i = 0; i < ids.size(); ++i) rank.insert(ids[i], i);
            std::sort(objs.begin(), objs.end(), [&rank](const QJsonObject &a, const QJsonObject &b) {
                return rank.value(qint64(a.value(QLatin1String("id")).toDouble()), 99999)
                     < rank.value(qint64(b.value(QLatin1String("id")).toDouble()), 99999);
            });
            items = gamesToItems(objs, 24);
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
                items = gamesToItems(toObjects(QJsonDocument::fromJson(reply->readAll()).array()), 24);
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
                items = gamesToItems(toObjects(QJsonDocument::fromJson(reply->readAll()).array()), 30);
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
    m_rows.clear();
    // De-dup across rows: a title in Trending shouldn't repeat in Popular, etc.
    // Keep the first occurrence (rows are in priority order).
    QSet<QString> seenTitles;
    for (auto it = m_accum.constBegin(); it != m_accum.constEnd(); ++it) {
        QVariantMap row = it.value();
        QVariantList kept;
        for (const QVariant &v : row.value(QStringLiteral("items")).toList()) {
            const QVariantMap m = v.toMap();
            const QString key = m.value(QStringLiteral("title")).toString().toLower()
                              + QLatin1Char('|') + m.value(QStringLiteral("type")).toString();
            if (seenTitles.contains(key)) continue;
            seenTitles.insert(key);
            kept.append(v);
        }
        if (!kept.isEmpty()) { row[QStringLiteral("items")] = kept; m_rows.append(row); }
    }

    // hero: round-robin one item per row per pass, so the banner mixes movies,
    // games and series (not 6 movies). Needs a backdrop + overview.
    m_hero.clear();
    QStringList seen;
    QList<QVariantList> rowItems;
    int maxItems = 0;
    for (auto it = m_accum.constBegin(); it != m_accum.constEnd(); ++it) {
        const QVariantList items = it.value().value(QStringLiteral("items")).toList();
        rowItems.append(items);
        maxItems = qMax(maxItems, int(items.size()));
    }
    for (int col = 0; col < maxItems && m_hero.size() < 6; ++col) {
        for (const QVariantList &items : rowItems) {
            if (m_hero.size() >= 6) break;
            if (col >= items.size()) continue;
            const QVariantMap m = items[col].toMap();
            if (m.value(QStringLiteral("backdrop")).toString().isEmpty()
                || m.value(QStringLiteral("overview")).toString().isEmpty()) continue;
            const QString title = m.value(QStringLiteral("title")).toString();
            if (seen.contains(title)) continue;
            seen.append(title);
            m_hero.append(m);
        }
    }

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
