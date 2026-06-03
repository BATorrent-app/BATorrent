// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "metadataresolver.h"

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
#include "translator.h"
#include <QStandardPaths>
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
const QString TmdbBaseUrl = QStringLiteral("https://api.themoviedb.org/3");
const QString TmdbPosterBase = QStringLiteral("https://image.tmdb.org/t/p/w342");

// App language → TMDB locale, so titles/overviews/genres come localized.
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

QString contentTypeToString(ContentType ct)
{
    switch (ct) {
    case ContentType::Movie:   return QStringLiteral("movie");
    case ContentType::Series:  return QStringLiteral("series");
    case ContentType::Game:    return QStringLiteral("game");
    case ContentType::Unknown: return QStringLiteral("unknown");
    }
    return QStringLiteral("unknown");
}

ContentType contentTypeFromString(const QString &s)
{
    if (s == QLatin1String("movie"))  return ContentType::Movie;
    if (s == QLatin1String("series")) return ContentType::Series;
    if (s == QLatin1String("game"))   return ContentType::Game;
    return ContentType::Unknown;
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

    QDir dir(cacheDir());
    if (dir.exists()) {
        const auto entries = dir.entryList({QStringLiteral("*.json")}, QDir::Files);
        for (const QString &file : entries) {
            const QString hash = file.chopped(5);
            loadFromDisk(hash);
        }
    }
}

void MetadataResolver::resolve(const QString &infoHash, const QString &torrentName)
{
    if (m_cache.contains(infoHash))
        return;

    ParsedName parsed = NameParser::parse(torrentName);
    qDebug() << "[metadata] resolve:" << torrentName << "->" << parsed.cleanTitle
             << "type:" << static_cast<int>(parsed.contentType);
    m_queue.enqueue({infoHash, parsed});

    if (!m_requestInFlight && !m_rateLimiter.isActive())
        processQueue();
}

MetadataResult MetadataResolver::cached(const QString &infoHash) const
{
    return m_cache.value(infoHash);
}

bool MetadataResolver::hasCached(const QString &infoHash) const
{
    return m_cache.contains(infoHash);
}

void MetadataResolver::batchResolve(const QStringList &infoHashes, const QStringList &torrentNames)
{
    const int count = qMin(infoHashes.size(), torrentNames.size());
    for (int i = 0; i < count; ++i) {
        if (!m_cache.contains(infoHashes[i])) {
            ParsedName parsed = NameParser::parse(torrentNames[i]);
            m_queue.enqueue({infoHashes[i], parsed});
        }
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
    ParsedName parsed = NameParser::parse(query);
    if (parsed.cleanTitle.trimmed().isEmpty())
        parsed.cleanTitle = query.trimmed();
    parsed.contentType = type;
    m_cache.remove(infoHash);
    m_queue.enqueue({infoHash, parsed});
    if (!m_requestInFlight && !m_rateLimiter.isActive())
        processQueue();
}

void MetadataResolver::clearMetadata(const QString &infoHash)
{
    // "No cover" — a valid-but-empty entry shows the placeholder + the parsed/raw
    // title, and (being cached) is never auto-resolved again.
    MetadataResult r;
    r.valid = true;
    m_cache.insert(infoHash, r);
    saveToDisk(infoHash, r);
    emit metadataReady(infoHash, r);
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
            m_rateLimiter.start();
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
            m_rateLimiter.start();
            return;
        }

        const QJsonObject item = results[0].toObject();

        MetadataResult result;
        result.valid = true;
        result.title = item.value(QLatin1String("title")).toString();
        result.description = item.value(QLatin1String("overview")).toString();
        result.rating = item.value(QLatin1String("vote_average")).toDouble();
        result.contentType = (parsed.contentType == ContentType::Game) ? ContentType::Game : ContentType::Movie;

        const QString releaseDate = item.value(QLatin1String("release_date")).toString();
        if (releaseDate.length() >= 4)
            result.year = releaseDate.left(4).toInt();

        const QJsonArray genreIds = item.value(QLatin1String("genre_ids")).toArray();
        const auto &genreMap = tmdbGenres();
        for (const QJsonValue &gv : genreIds) {
            const QString name = genreMap.value(gv.toInt());
            if (!name.isEmpty())
                result.genres.append(name);
        }

        const QString posterPath = item.value(QLatin1String("poster_path")).toString();
        const int tmdbId = item.value(QLatin1String("id")).toInt();

        auto finish = [this, infoHash, posterPath](MetadataResult r) {
            if (!posterPath.isEmpty()) {
                downloadPoster(infoHash, TmdbPosterBase + posterPath, r);
            } else {
                m_cache.insert(infoHash, r);
                saveToDisk(infoHash, r);
                emit metadataReady(infoHash, r);
                m_rateLimiter.start();
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
            m_rateLimiter.start();
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        const QJsonArray results = doc.object().value(QLatin1String("results")).toArray();

        if (results.isEmpty()) {
            m_rateLimiter.start();
            return;
        }

        const QJsonObject item = results[0].toObject();

        MetadataResult result;
        result.valid = true;
        result.title = item.value(QLatin1String("name")).toString();
        result.description = item.value(QLatin1String("overview")).toString();
        result.rating = item.value(QLatin1String("vote_average")).toDouble();
        result.contentType = ContentType::Series;

        const QString firstAirDate = item.value(QLatin1String("first_air_date")).toString();
        if (firstAirDate.length() >= 4)
            result.year = firstAirDate.left(4).toInt();

        const QJsonArray genreIds = item.value(QLatin1String("genre_ids")).toArray();
        const auto &genreMap = tmdbGenres();
        for (const QJsonValue &gv : genreIds) {
            const QString name = genreMap.value(gv.toInt());
            if (!name.isEmpty())
                result.genres.append(name);
        }

        const QString posterPath = item.value(QLatin1String("poster_path")).toString();
        const int tmdbId = item.value(QLatin1String("id")).toInt();

        auto finish = [this, infoHash, posterPath](MetadataResult r) {
            if (!posterPath.isEmpty()) {
                downloadPoster(infoHash, TmdbPosterBase + posterPath, r);
            } else {
                m_cache.insert(infoHash, r);
                saveToDisk(infoHash, r);
                emit metadataReady(infoHash, r);
                m_rateLimiter.start();
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

void MetadataResolver::queryIgdb(const QString &infoHash, const ParsedName &parsed)
{
    QString clientId = igdbClientId();
    if (clientId.isEmpty() || igdbClientSecret().isEmpty()) {
        if (parsed.contentType == ContentType::Unknown)
            queryTmdbMovie(infoHash, parsed);
        else
            m_rateLimiter.start();
        return;
    }

    if (m_igdbAccessToken.isEmpty() || QDateTime::currentSecsSinceEpoch() >= m_igdbTokenExpiry) {
        m_igdbPending.enqueue({infoHash, parsed});
        m_requestInFlight = false;
        ensureIgdbToken();
        return;
    }

    qDebug() << "[metadata] queryIgdb:" << parsed.cleanTitle;

    QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
    req.setRawHeader("Client-ID", clientId.toUtf8());
    req.setRawHeader("Authorization", ("Bearer " + m_igdbAccessToken).toUtf8());
    req.setHeader(QNetworkRequest::ContentTypeHeader, "text/plain");
    req.setTransferTimeout(10000);

    // escape the quoted search term — cleanTitle comes from the torrent name,
    // so a stray " or \ would break out of the Apicalypse string literal.
    QString safeTitle = parsed.cleanTitle;
    safeTitle.replace('\\', QStringLiteral("\\\\")).replace('"', QStringLiteral("\\\""));
    QString body = QStringLiteral("search \"%1\"; fields name,summary,rating,first_release_date,genres.name,platforms.name,cover.image_id; limit 5;")
        .arg(safeTitle);

    m_requestInFlight = true;
    auto *reply = m_nam->post(req, body.toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply, infoHash, parsed]() {
        reply->deleteLater();
        m_requestInFlight = false;

        if (reply->error() != QNetworkReply::NoError) {
            qDebug() << "[metadata] IGDB error:" << reply->errorString();
            if (parsed.contentType == ContentType::Unknown)
                queryTmdbMovie(infoHash, parsed);
            else
                m_rateLimiter.start();
            return;
        }

        QJsonArray results = QJsonDocument::fromJson(reply->readAll()).array();
        qDebug() << "[metadata] IGDB results:" << results.size() << "for" << parsed.cleanTitle;

        // Find best match — prefer exact title match, then contains
        QJsonObject item;
        bool found = false;
        for (const auto &v : results) {
            QJsonObject obj = v.toObject();
            QString resultName = obj.value(QLatin1String("name")).toString();
            if (resultName.compare(parsed.cleanTitle, Qt::CaseInsensitive) == 0) {
                item = obj; found = true; break;
            }
        }
        if (!found) {
            for (const auto &v : results) {
                QJsonObject obj = v.toObject();
                QString resultName = obj.value(QLatin1String("name")).toString();
                if (resultName.contains(parsed.cleanTitle, Qt::CaseInsensitive)
                    || parsed.cleanTitle.contains(resultName, Qt::CaseInsensitive)) {
                    item = obj; found = true; break;
                }
            }
        }
        // Short titles need exact match to avoid false positives
        if (!found && parsed.cleanTitle.length() > 4 && !results.isEmpty()) {
            item = results[0].toObject();
            found = true;
        }

        if (!found) {
            qDebug() << "[metadata] IGDB: no confident match for" << parsed.cleanTitle;
            if (parsed.contentType == ContentType::Unknown)
                queryTmdbMovie(infoHash, parsed);
            else
                m_rateLimiter.start();
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
            m_rateLimiter.start();
        }
    });
}

void MetadataResolver::downloadPoster(const QString &infoHash, const QString &url,
                                       const MetadataResult &partial)
{
    QDir().mkpath(posterDir());

    QUrl posterUrl(url);
    QNetworkRequest req{posterUrl};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("BATorrent/") + QLatin1String(APP_VERSION));
    req.setTransferTimeout(10000);

    QNetworkReply *reply = m_nam->get(req);

    connect(reply, &QNetworkReply::finished, this, [this, reply, infoHash, partial]() {
        reply->deleteLater();

        MetadataResult result = partial;

        if (reply->error() == QNetworkReply::NoError) {
            const QString filePath = posterDir() + QLatin1Char('/') + infoHash + QStringLiteral(".jpg");
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

        m_cache.insert(infoHash, result);
        saveToDisk(infoHash, result);
        emit metadataReady(infoHash, result);
        m_rateLimiter.start();
    });
}

void MetadataResolver::loadFromDisk(const QString &infoHash)
{
    QFile file(cacheDir() + QLatin1Char('/') + infoHash + QStringLiteral(".json"));
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
    result.contentType = contentTypeFromString(obj.value(QLatin1String("contentType")).toString());

    const QJsonArray genresArr = obj.value(QLatin1String("genres")).toArray();
    for (const QJsonValue &v : genresArr)
        result.genres.append(v.toString());

    const QJsonArray platformsArr = obj.value(QLatin1String("platforms")).toArray();
    for (const QJsonValue &v : platformsArr)
        result.platforms.append(v.toString());

    const QString posterFile = obj.value(QLatin1String("posterFile")).toString();
    if (!posterFile.isEmpty() && QFile::exists(posterFile))
        result.posterPath = posterFile;

    m_cache.insert(infoHash, result);
}

void MetadataResolver::saveToDisk(const QString &infoHash, const MetadataResult &result)
{
    QDir().mkpath(cacheDir());

    QJsonObject obj;
    obj.insert(QLatin1String("title"), result.title);
    obj.insert(QLatin1String("description"), result.description);
    obj.insert(QLatin1String("rating"), result.rating);
    obj.insert(QLatin1String("year"), result.year);
    obj.insert(QLatin1String("genres"), QJsonArray::fromStringList(result.genres));
    obj.insert(QLatin1String("platforms"), QJsonArray::fromStringList(result.platforms));
    obj.insert(QLatin1String("posterFile"), result.posterPath);
    obj.insert(QLatin1String("contentType"), contentTypeToString(result.contentType));
    obj.insert(QLatin1String("resolvedAt"), QDateTime::currentDateTimeUtc().toString(Qt::ISODate));

    QString path = cacheDir() + QLatin1Char('/') + infoHash + QStringLiteral(".json");
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
