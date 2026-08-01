// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/metadata/metadataresolver.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QStandardPaths>
#include <QUrlQuery>

#include "services/metadata/metadatamatch.h"
#include "services/platform/contentlanguage.h"

namespace {

QString tmdbApiKey()
{
    QString key = QSettings("BATorrent", "BATorrent").value("tmdbApiKey").toString();
#ifdef BAT_TMDB_KEY
    if (key.isEmpty()) key = QStringLiteral(BAT_TMDB_KEY);
#endif
    return key;
}
const QString TmdbBaseUrl = QStringLiteral("https://api.themoviedb.org/3");
const QString TmdbPosterBase = QStringLiteral("https://image.tmdb.org/t/p/w342");

QString tmdbLang() { return ContentLanguage::tmdb(); }

// Pre-3.0 AppData lived one level up (…/BATorrent vs …/BATorrent/BATorrent).
// Resume already migrates; covers lived in metadata/ + posters/ and were left
// behind — library tiles then resolved titles from names but never found art.
void migrateLegacyMetadataDirs(const QString &appData)
{
    QSettings s;
    if (s.value(QStringLiteral("metadataMigrated"), false).toBool())
        return;
    s.setValue(QStringLiteral("metadataMigrated"), true);

    const QString legacyRoot = MetadataMatch::legacyAppDataSibling(appData);
    if (legacyRoot.isEmpty())
        return;

    const QString newMeta = appData + QStringLiteral("/metadata");
    const QString newPosters = appData + QStringLiteral("/posters");
    QDir().mkpath(newMeta);
    QDir().mkpath(newPosters);

    auto copyMissing = [](const QString &fromDir, const QString &toDir,
                          const QStringList &filters) -> int {
        QDir from(fromDir);
        if (!from.exists())
            return 0;
        int n = 0;
        for (const QString &f : from.entryList(filters, QDir::Files)) {
            const QString dest = toDir + QLatin1Char('/') + f;
            if (QFile::exists(dest))
                continue;
            if (QFile::copy(from.filePath(f), dest))
                ++n;
        }
        return n;
    };

    // Prefer legacy JSON when the nested entry has no usable poster (title-only
    // stubs from a failed download after the AppData nest).
    auto refreshPosterlessMeta = [&](const QString &fromDir, const QString &toDir) -> int {
        QDir from(fromDir);
        if (!from.exists())
            return 0;
        int n = 0;
        for (const QString &f : from.entryList({QStringLiteral("*.json")}, QDir::Files)) {
            const QString dest = toDir + QLatin1Char('/') + f;
            if (!QFile::exists(dest))
                continue;
            QFile destFile(dest);
            if (!destFile.open(QIODevice::ReadOnly))
                continue;
            const QJsonObject destObj = QJsonDocument::fromJson(destFile.readAll()).object();
            destFile.close();
            const QString hash = f.chopped(5);
            const QString destPoster = MetadataMatch::locatePosterFile(
                destObj.value(QLatin1String("posterFile")).toString(), newPosters, hash);
            if (!destPoster.isEmpty())
                continue;
            const QString src = from.filePath(f);
            QFile::remove(dest);
            if (QFile::copy(src, dest))
                ++n;
        }
        return n;
    };

    auto rewritePosterPaths = [&](const QString &metaDir) -> int {
        QDir dir(metaDir);
        if (!dir.exists())
            return 0;
        int n = 0;
        for (const QString &f : dir.entryList({QStringLiteral("*.json")}, QDir::Files)) {
            const QString path = dir.filePath(f);
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly))
                continue;
            QJsonObject obj = QJsonDocument::fromJson(file.readAll()).object();
            file.close();
            const QString hash = f.chopped(5);
            const QString stored = obj.value(QLatin1String("posterFile")).toString();
            const QString located = MetadataMatch::locatePosterFile(stored, newPosters, hash);
            if (located.isEmpty() || located == stored)
                continue;
            obj.insert(QLatin1String("posterFile"), located);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
                continue;
            file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
            ++n;
        }
        return n;
    };

    const int posterN = copyMissing(legacyRoot + QStringLiteral("/posters"), newPosters,
                                    {QStringLiteral("*.jpg"), QStringLiteral("*.jpeg"),
                                     QStringLiteral("*.png"), QStringLiteral("*.webp")});
    const int metaN = copyMissing(legacyRoot + QStringLiteral("/metadata"), newMeta,
                                  {QStringLiteral("*.json")});
    const int refreshed = refreshPosterlessMeta(legacyRoot + QStringLiteral("/metadata"), newMeta);
    const int rewritten = rewritePosterPaths(newMeta);
    if (metaN > 0 || posterN > 0 || refreshed > 0 || rewritten > 0)
        qInfo() << "[metadata] migrated" << metaN << "cache files and" << posterN
                << "posters from legacy AppData"
                << "(refreshed" << refreshed << ", rewrote" << rewritten << "paths)";
}

} // namespace

MetadataResolver::MetadataResolver(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
    m_nam->setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);

    m_rateLimiter.setInterval(300);
    m_rateLimiter.setSingleShot(true);
    connect(&m_rateLimiter, &QTimer::timeout, this, &MetadataResolver::processQueue);

    // A hit clears the pending manual entry, so finishLookup() — which every
    // lookup ends on — only reports the misses. Wiring it here keeps the
    // invariant true for any future success path without touching it.
    connect(this, &MetadataResolver::metadataReady, this,
            [this](const QString &infoHash, const MetadataResult &) {
        m_manualQueries.remove(infoHash);
    });

    migrateLegacyMetadataDirs(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    QDir dir(cacheDir());
    if (dir.exists()) {
        const auto entries = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
        for (const QString &file : entries) {
            const QString hash = file.chopped(5);
            loadFromDisk(hash);
        }
    }
}

void MetadataResolver::resolve(const QString &infoHash, const QString &torrentName,
                               const QStringList &fileNames)
{
    const QString key = MetadataMatch::canonicalInfoHash(infoHash);
    if (key.isEmpty())
        return;
    if (m_cache.contains(key)) {
        const MetadataResult c = m_cache.value(key);
        // Have art, or an intentional clearMetadata blank — don't re-hit TMDB/IGDB.
        if (!c.posterPath.isEmpty() || (c.valid && c.title.isEmpty()))
            return;
        // Title cached but poster file gone/stale — fall through to refill art.
    }

    ParsedName parsed = NameParser::parse(torrentName);
    // The file payload outranks the name for game-vs-movie (a name can lie). Keep
    // a name-derived Series though — episode markers there are reliable, and a
    // single-episode torrent looks like a movie by file count alone.
    parsed.contentType = MetadataMatch::applyFileTypeOverride(parsed.contentType, fileNames);
    qDebug() << "[metadata] resolve:" << torrentName << "->" << parsed.cleanTitle
             << "type:" << static_cast<int>(parsed.contentType);
    m_queue.enqueue({key, parsed});

    if (!m_requestInFlight && !m_rateLimiter.isActive())
        processQueue();
}

MetadataResult MetadataResolver::cached(const QString &infoHash) const
{
    return m_cache.value(MetadataMatch::canonicalInfoHash(infoHash));
}

bool MetadataResolver::hasCached(const QString &infoHash) const
{
    return m_cache.contains(MetadataMatch::canonicalInfoHash(infoHash));
}

void MetadataResolver::batchResolve(const QStringList &infoHashes, const QStringList &torrentNames)
{
    const int count = qMin(infoHashes.size(), torrentNames.size());
    for (int i = 0; i < count; ++i) {
        const QString key = MetadataMatch::canonicalInfoHash(infoHashes[i]);
        if (key.isEmpty())
            continue;
        if (m_cache.contains(key)) {
            const MetadataResult c = m_cache.value(key);
            if (!c.posterPath.isEmpty() || (c.valid && c.title.isEmpty()))
                continue;
        }
        ParsedName parsed = NameParser::parse(torrentNames[i]);
        m_queue.enqueue({key, parsed});
    }

    if (!m_requestInFlight && !m_rateLimiter.isActive() && !m_queue.isEmpty())
        processQueue();
}

void MetadataResolver::resolveManual(const QString &infoHash, const QString &query, ContentType type)
{
    // User-driven re-link when the auto match was wrong. Parse the typed query
    // (so "Love 2015" still yields a year), but force the user's chosen type,
    // and drop any cached entry so the cache-skip in resolve() doesn't block the
    // re-query. The new result is cached + saved, so it persists and auto-resolve
    // never clobbers it.
    const QString key = MetadataMatch::canonicalInfoHash(infoHash);
    if (key.isEmpty())
        return;
    ParsedName parsed = NameParser::parse(query);
    if (parsed.cleanTitle.trimmed().isEmpty())
        parsed.cleanTitle = query.trimmed();
    parsed.contentType = type;
    m_cache.remove(key);
    m_manualQueries.insert(key, query.trimmed());
    m_queue.enqueue({key, parsed});
    if (!m_requestInFlight && !m_rateLimiter.isActive())
        processQueue();
}

void MetadataResolver::finishLookup(const QString &infoHash)
{
    auto it = m_manualQueries.find(infoHash);
    if (it != m_manualQueries.end()) {
        const QString query = it.value();
        m_manualQueries.erase(it);
        emit manualResolveFailed(infoHash, query);
    }
    m_rateLimiter.start();
}

void MetadataResolver::clearMetadata(const QString &infoHash)
{
    // "No cover" drops the artwork, not everything we know. A blank entry also
    // wiped contentType, and the tile then had no way to say GAME or MOVIE in
    // the poster's place — asking for no art is not asking the app to forget
    // what the torrent is. Being cached, it is still never auto-resolved again.
    const QString key = MetadataMatch::canonicalInfoHash(infoHash);
    if (key.isEmpty())
        return;
    MetadataResult r;
    const MetadataResult prev = m_cache.value(key);
    if (prev.valid) {
        r.contentType = prev.contentType;
        r.year        = prev.year;
        r.genres      = prev.genres;
        r.tmdbId      = prev.tmdbId;
    }
    r.valid = true;
    m_cache.insert(key, r);
    saveToDisk(key, r);
    emit metadataReady(key, r);
}

void MetadataResolver::processQueue()
{
    if (m_queue.isEmpty() || m_requestInFlight)
        return;

    auto [infoHash, parsed] = m_queue.dequeue();

    switch (parsed.contentType) {
    case ContentType::Series:
        queryTmdbTv(infoHash, parsed);
        break;
    case ContentType::Movie:
        queryTmdbMovie(infoHash, parsed);
        break;
    case ContentType::Game:
    case ContentType::Unknown:
        queryIgdb(infoHash, parsed);
        break;
    }
}

void MetadataResolver::queryTmdbMovie(const QString &infoHash, const ParsedName &parsed)
{
    qDebug() << "[metadata] queryTmdbMovie:" << parsed.cleanTitle;
    QUrl url(TmdbBaseUrl + QStringLiteral("/search/movie"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    query.addQueryItem(QStringLiteral("query"), parsed.cleanTitle);
    query.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("language"), tmdbLang());
    if (parsed.year > 0)
        query.addQueryItem(QStringLiteral("year"), QString::number(parsed.year));
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);

    m_requestInFlight = true;
    QNetworkReply *reply = m_nam->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, infoHash, parsed]() {
        reply->deleteLater();
        m_requestInFlight = false;

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[metadata] TMDB error:" << reply->errorString();
            finishLookup(infoHash);
            return;
        }

        const QByteArray body = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(body);
        const QJsonArray results = doc.object().value(QLatin1String("results")).toArray();
        qDebug() << "[metadata] TMDB movie results:" << results.size() << "for" << parsed.cleanTitle;

        if (results.isEmpty()) {
            if (parsed.contentType == ContentType::Unknown) {
                queryTmdbTv(infoHash, parsed);
                return;
            }
            finishLookup(infoHash);
            return;
        }

        const QJsonObject item = results[0].toObject();

        // A generic/unknown torrent must not adopt a fuzzy movie match ("debian"
        // → some film called "Debian"). Only a confident title match sticks; else
        // try TV, and failing that stay coverless.
        if (parsed.contentType == ContentType::Unknown
            && !MetadataMatch::confidentTitle(parsed.cleanTitle,
                                              item.value(QLatin1String("title")).toString())) {
            queryTmdbTv(infoHash, parsed);
            return;
        }

        MetadataResult result;
        result.valid = true;
        result.title = item.value(QLatin1String("title")).toString();
        result.description = item.value(QLatin1String("overview")).toString();
        result.rating = item.value(QLatin1String("vote_average")).toDouble();
        result.contentType = (parsed.contentType == ContentType::Game) ? ContentType::Game : ContentType::Movie;

        const QString releaseDate = item.value(QLatin1String("release_date")).toString();
        if (releaseDate.length() >= 4)
            result.year = releaseDate.left(4).toInt();

        result.genres = MetadataMatch::genreNamesFromIds(
            item.value(QLatin1String("genre_ids")).toArray());

        const QString posterPath = item.value(QLatin1String("poster_path")).toString();
        const int tmdbId = item.value(QLatin1String("id")).toInt();
        result.tmdbId = tmdbId;

        auto finish = [this, infoHash, posterPath](MetadataResult r) {
            if (!posterPath.isEmpty()) {
                downloadPoster(infoHash, TmdbPosterBase + posterPath, r);
            } else {
                m_cache.insert(infoHash, r);
                saveToDisk(infoHash, r);
                emit metadataReady(infoHash, r);
                finishLookup(infoHash);
            }
        };
        // empty overview = no translation for this language → fall back to EN
        if (result.description.isEmpty() && tmdbId > 0 && tmdbLang() != QStringLiteral("en-US"))
            fetchTmdbOverviewEn(QStringLiteral("movie"), tmdbId, result, finish);
        else
            finish(result);
    });
}

void MetadataResolver::queryTmdbTv(const QString &infoHash, const ParsedName &parsed)
{
    QUrl url(TmdbBaseUrl + QStringLiteral("/search/tv"));
    QUrlQuery query;
    query.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    query.addQueryItem(QStringLiteral("query"), parsed.cleanTitle);
    query.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
    query.addQueryItem(QStringLiteral("language"), tmdbLang());
    if (parsed.year > 0)
        query.addQueryItem(QStringLiteral("first_air_date_year"), QString::number(parsed.year));
    url.setQuery(query);

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);

    m_requestInFlight = true;
    QNetworkReply *reply = m_nam->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, infoHash, parsed]() {
        reply->deleteLater();
        m_requestInFlight = false;

        if (reply->error() != QNetworkReply::NoError) {
            finishLookup(infoHash);
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray results = doc.object().value(QLatin1String("results")).toArray();

        if (results.isEmpty()) {
            finishLookup(infoHash);
            return;
        }

        const QJsonObject item = results[0].toObject();

        // last stop in the Unknown chain (IGDB → movie → here) — without a
        // confident title match, leave it coverless rather than guess a show.
        if (parsed.contentType == ContentType::Unknown
            && !MetadataMatch::confidentTitle(parsed.cleanTitle,
                                              item.value(QLatin1String("name")).toString())) {
            finishLookup(infoHash);
            return;
        }

        MetadataResult result;
        result.valid = true;
        result.title = item.value(QLatin1String("name")).toString();
        result.description = item.value(QLatin1String("overview")).toString();
        result.rating = item.value(QLatin1String("vote_average")).toDouble();
        result.contentType = ContentType::Series;

        const QString firstAirDate = item.value(QLatin1String("first_air_date")).toString();
        if (firstAirDate.length() >= 4)
            result.year = firstAirDate.left(4).toInt();

        result.genres = MetadataMatch::genreNamesFromIds(
            item.value(QLatin1String("genre_ids")).toArray());

        const QString posterPath = item.value(QLatin1String("poster_path")).toString();
        const int tmdbId = item.value(QLatin1String("id")).toInt();
        result.tmdbId = tmdbId;

        auto finish = [this, infoHash, posterPath](MetadataResult r) {
            if (!posterPath.isEmpty()) {
                downloadPoster(infoHash, TmdbPosterBase + posterPath, r);
            } else {
                m_cache.insert(infoHash, r);
                saveToDisk(infoHash, r);
                emit metadataReady(infoHash, r);
                finishLookup(infoHash);
            }
        };
        if (result.description.isEmpty() && tmdbId > 0 && tmdbLang() != QStringLiteral("en-US"))
            fetchTmdbOverviewEn(QStringLiteral("tv"), tmdbId, result, finish);
        else
            finish(result);
    });
}

void MetadataResolver::fetchTmdbOverviewEn(const QString &kind, int id,
                                           MetadataResult result,
                                           std::function<void(MetadataResult)> done)
{
    QUrl url(TmdbBaseUrl + QStringLiteral("/%1/%2").arg(kind).arg(id));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    q.addQueryItem(QStringLiteral("language"), QStringLiteral("en-US"));
    url.setQuery(q);
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);
    // keep the queue serialized while this fallback request is in flight,
    // otherwise processQueue() could fire a concurrent request (→ TMDB 429)
    m_requestInFlight = true;
    QNetworkReply *reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, result, done]() mutable {
        reply->deleteLater();
        m_requestInFlight = false;
        if (reply->error() == QNetworkReply::NoError) {
            const QJsonObject o = QJsonDocument::fromJson(reply->readAll()).object();
            const QString en = o.value(QLatin1String("overview")).toString();
            if (!en.isEmpty()) result.description = en;
        }
        done(result);
    });
}

static QString igdbClientId()
{
    QString id = QSettings("BATorrent", "BATorrent").value("igdbClientId").toString();
#ifdef BAT_IGDB_CLIENT_ID
    if (id.isEmpty()) id = QStringLiteral(BAT_IGDB_CLIENT_ID);
#endif
    return id;
}
static QString igdbClientSecret()
{
    QString secret = QSettings("BATorrent", "BATorrent").value("igdbClientSecret").toString();
#ifdef BAT_IGDB_CLIENT_SECRET
    if (secret.isEmpty()) secret = QStringLiteral(BAT_IGDB_CLIENT_SECRET);
#endif
    return secret;
}

void MetadataResolver::ensureIgdbToken()
{
    if (!m_igdbAccessToken.isEmpty() && QDateTime::currentSecsSinceEpoch() < m_igdbTokenExpiry)
        return;

    QString clientId = igdbClientId();
    QString clientSecret = igdbClientSecret();
    if (clientId.isEmpty() || clientSecret.isEmpty()) return;

    QUrl url(QStringLiteral("https://id.twitch.tv/oauth2/token"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("client_id"), clientId);
    q.addQueryItem(QStringLiteral("client_secret"), clientSecret);
    q.addQueryItem(QStringLiteral("grant_type"), QStringLiteral("client_credentials"));
    url.setQuery(q);

    QNetworkRequest req{url};
    req.setTransferTimeout(10000);
    auto *reply = m_nam->post(req, QByteArray());

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[metadata] IGDB token error:" << reply->errorString();
            return;
        }
        QJsonObject obj = QJsonDocument::fromJson(reply->readAll()).object();
        m_igdbAccessToken = obj.value(QLatin1String("access_token")).toString();
        int expiresIn = obj.value(QLatin1String("expires_in")).toInt(3600);
        m_igdbTokenExpiry = QDateTime::currentSecsSinceEpoch() + expiresIn - 60;
        qDebug() << "[metadata] IGDB token acquired, expires in" << expiresIn << "s";

        while (!m_igdbPending.isEmpty()) {
            auto [hash, parsed] = m_igdbPending.dequeue();
            queryIgdb(hash, parsed);
        }
    });
}

void MetadataResolver::queryIgdb(const QString &infoHash, const ParsedName &parsed,
                                 const QString &searchOverride)
{
    const QString queryTitle = searchOverride.isEmpty() ? parsed.cleanTitle
                                                        : searchOverride;
    QString clientId = igdbClientId();
    if (clientId.isEmpty() || igdbClientSecret().isEmpty()) {
        if (parsed.contentType == ContentType::Unknown)
            queryTmdbMovie(infoHash, parsed);
        else
            finishLookup(infoHash);
        return;
    }

    if (m_igdbAccessToken.isEmpty() || QDateTime::currentSecsSinceEpoch() >= m_igdbTokenExpiry) {
        m_igdbPending.enqueue({infoHash, parsed});
        m_requestInFlight = false;
        ensureIgdbToken();
        return;
    }

    qDebug() << "[metadata] queryIgdb:" << queryTitle;

    QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
    req.setRawHeader("Client-ID", clientId.toUtf8());
    req.setRawHeader("Authorization", ("Bearer " + m_igdbAccessToken).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    req.setTransferTimeout(10000);

    // escape the quoted search term — cleanTitle comes from the torrent name,
    // so a stray " or \ would break out of the Apicalypse string literal.
    const QString safeTitle = MetadataMatch::escapeApicalypse(queryTitle);
    QString body = QStringLiteral("search \"%1\"; fields name,summary,rating,first_release_date,genres.name,platforms.name,cover.image_id; limit 5;")
        .arg(safeTitle);

    m_requestInFlight = true;
    auto *reply = m_nam->post(req, body.toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply, infoHash, parsed, queryTitle]() {
        reply->deleteLater();
        m_requestInFlight = false;

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[metadata] IGDB error:" << reply->errorString();
            if (parsed.contentType == ContentType::Unknown)
                queryTmdbMovie(infoHash, parsed);
            else
                finishLookup(infoHash);
            return;
        }

        QJsonArray results = QJsonDocument::fromJson(reply->readAll()).array();
        qDebug() << "[metadata] IGDB results:" << results.size() << "for" << queryTitle;

        // Pick the result whose title is most similar to the query (folded token
        // overlap), with a small bonus when the release year matches. This beats
        // taking results[0], which is how a loose IGDB search returned the wrong
        // game. Below the threshold we trust nothing (placeholder > wrong cover).
        const auto pick = MetadataMatch::pickBestIgdbResult(
            results, parsed.cleanTitle, parsed.year);
        const QJsonObject item = pick.item;

        if (!pick.found) {
            // IGDB's search chokes on long subtitled names ("Garfield Kart 2
            // All You Can Drift" finds nothing; "Garfield Kart 2" does). Retry
            // with the front half of the tokens, down to 3 — scoring above
            // still compares against the full title, so a wrong-franchise hit
            // can't sneak in just because the query got shorter.
            const QString shorter = MetadataMatch::shortenedSearchTitle(queryTitle);
            if (!shorter.isEmpty()) {
                qDebug() << "[metadata] IGDB: retrying with shortened title" << shorter;
                queryIgdb(infoHash, parsed, shorter);
                return;
            }
            qDebug() << "[metadata] IGDB: no confident match for" << parsed.cleanTitle;
            if (parsed.contentType == ContentType::Unknown)
                queryTmdbMovie(infoHash, parsed);
            else
                finishLookup(infoHash);
            return;
        }

        MetadataResult result;
        result.valid = true;
        result.title = item.value(QLatin1String("name")).toString();
        result.description = item.value(QLatin1String("summary")).toString();
        result.rating = item.value(QLatin1String("rating")).toDouble() / 10.0;
        result.contentType = ContentType::Game;

        qint64 releaseDate = item.value(QLatin1String("first_release_date")).toVariant().toLongLong();
        if (releaseDate > 0)
            result.year = QDateTime::fromSecsSinceEpoch(releaseDate).date().year();

        QJsonArray genres = item.value(QLatin1String("genres")).toArray();
        for (const auto &g : genres)
            result.genres.append(g.toObject().value(QLatin1String("name")).toString());

        QJsonArray platforms = item.value(QLatin1String("platforms")).toArray();
        for (const auto &p : platforms)
            result.platforms.append(p.toObject().value(QLatin1String("name")).toString());

        QJsonObject cover = item.value(QLatin1String("cover")).toObject();
        QString imageId = cover.value(QLatin1String("image_id")).toString();
        if (!imageId.isEmpty()) {
            QString coverUrl = QStringLiteral("https://images.igdb.com/igdb/image/upload/t_cover_big/%1.jpg").arg(imageId);
            downloadPoster(infoHash, coverUrl, result);
        } else {
            m_cache.insert(infoHash, result);
            saveToDisk(infoHash, result);
            emit metadataReady(infoHash, result);
            finishLookup(infoHash);
        }
    });
}

void MetadataResolver::downloadPoster(const QString &infoHash, const QString &url,
                                       const MetadataResult &partial)
{
    QDir().mkpath(posterDir());
    const QString key = MetadataMatch::canonicalInfoHash(infoHash);

    QUrl posterUrl(url);
    QNetworkRequest req{posterUrl};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);

    QNetworkReply *reply = m_nam->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, key, partial]() {
        reply->deleteLater();

        MetadataResult result = partial;

        if (reply->error() == QNetworkReply::NoError) {
            const QString filePath = posterDir() + QLatin1Char('/') + key + QStringLiteral(".jpg");
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                result.posterPath = filePath;
                qDebug() << "[metadata] poster saved:" << filePath;
            }
        } else {
            qDebug() << "[metadata] poster download failed:" << reply->errorString();
        }

        m_cache.insert(key, result);
        saveToDisk(key, result);
        emit metadataReady(key, result);
        finishLookup(key);
    });
}

void MetadataResolver::loadFromDisk(const QString &infoHash)
{
    const QString key = MetadataMatch::canonicalInfoHash(infoHash);
    QString jsonPath = cacheDir() + QLatin1Char('/') + infoHash + QStringLiteral(".json");
    if (!QFile::exists(jsonPath) && infoHash != key)
        jsonPath = cacheDir() + QLatin1Char('/') + key + QStringLiteral(".json");
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly))
        return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject obj = doc.object();

    MetadataResult result;
    result.valid = true;
    result.title = obj.value(QLatin1String("title")).toString();
    result.description = obj.value(QLatin1String("description")).toString();
    result.rating = obj.value(QLatin1String("rating")).toDouble();
    result.year = obj.value(QLatin1String("year")).toInt();
    result.tmdbId = obj.value(QLatin1String("tmdbId")).toInt();
    result.contentType = MetadataMatch::contentTypeFromString(
        obj.value(QLatin1String("contentType")).toString());

    const QJsonArray genresArr = obj.value(QLatin1String("genres")).toArray();
    for (const QJsonValue &v : genresArr)
        result.genres.append(v.toString());

    const QJsonArray platformsArr = obj.value(QLatin1String("platforms")).toArray();
    for (const QJsonValue &v : platformsArr)
        result.platforms.append(v.toString());

    const QString posterFile = obj.value(QLatin1String("posterFile")).toString();
    result.posterPath = MetadataMatch::locatePosterFile(posterFile, posterDir(), key);

    m_cache.insert(key, result);
}

void MetadataResolver::saveToDisk(const QString &infoHash, const MetadataResult &result)
{
    QDir().mkpath(cacheDir());
    const QString key = MetadataMatch::canonicalInfoHash(infoHash);

    QJsonObject obj;
    obj.insert(QLatin1String("title"), result.title);
    obj.insert(QLatin1String("description"), result.description);
    obj.insert(QLatin1String("rating"), result.rating);
    obj.insert(QLatin1String("year"), result.year);
    obj.insert(QLatin1String("tmdbId"), result.tmdbId);
    obj.insert(QLatin1String("genres"), QJsonArray::fromStringList(result.genres));
    obj.insert(QLatin1String("platforms"), QJsonArray::fromStringList(result.platforms));
    obj.insert(QLatin1String("posterFile"), result.posterPath);
    obj.insert(QLatin1String("contentType"),
               MetadataMatch::contentTypeToString(result.contentType));
    obj.insert(QLatin1String("resolvedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    QString path = cacheDir() + QLatin1Char('/') + key + QStringLiteral(".json");
    QFile file(path);
    if (file.open(QIODevice::WriteOnly))
        file.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    else
        qDebug() << "[metadata] saveToDisk failed:" << path;
}

QString MetadataResolver::cacheDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/metadata");
}

QString MetadataResolver::posterDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
           + QStringLiteral("/posters");
}
