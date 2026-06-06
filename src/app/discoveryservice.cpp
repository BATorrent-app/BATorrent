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

const qint64 CacheTtlSecs = 12 * 60 * 60;
const int CacheVersion = 3;   // bump when the row schema/order changes (invalidates stale cache)

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

    if (haveTmdb) {
        fetchTmdb(0, QStringLiteral("/trending/movie/week"), tr_("discover_trending_movies"), QStringLiteral("movie"));
        fetchTmdb(2, QStringLiteral("/trending/tv/week"),    tr_("discover_trending_series"), QStringLiteral("series"));
        fetchTmdb(4, QStringLiteral("/movie/popular"),       tr_("discover_popular_movies"),  QStringLiteral("movie"));
        fetchTmdb(5, QStringLiteral("/tv/popular"),          tr_("discover_popular_series"),  QStringLiteral("series"));
    }
    if (haveIgdb) {
        fetchIgdbTrending(1, tr_("discover_trending_games"));   // games of the moment — kept high
        fetchIgdbRecent(3, tr_("discover_new_games"));
    }
}

void DiscoveryService::fetchTmdb(int order, const QString &path, const QString &label, const QString &type)
{
    ++m_pending;

    QUrl url(TmdbBaseUrl + path);
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("api_key"), tmdbApiKey());
    q.addQueryItem(QStringLiteral("language"), tmdbLang());
    q.addQueryItem(QStringLiteral("page"), QStringLiteral("1"));
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
                items.append(m);
            }
        }
        QVariantMap row;
        row.insert(QStringLiteral("label"), label);
        row.insert(QStringLiteral("items"), items);
        if (!items.isEmpty()) m_accum.insert(order, row);
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
        seen.append(name);
        const qint64 rel = qint64(o.value(QLatin1String("first_release_date")).toDouble());
        QVariantMap m;
        m.insert(QStringLiteral("title"), name);
        m.insert(QStringLiteral("poster"),
                 QStringLiteral("https://images.igdb.com/igdb/image/upload/t_cover_big/%1.jpg").arg(imageId));
        m.insert(QStringLiteral("backdrop"), QString());
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
        QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/popularity_primitives"))};
        setIgdbHeaders(req);
        const QByteArray body = "fields game_id,value; sort value desc; limit 60;";
        QNetworkReply *reply = m_nam->post(req, body);
        connect(reply, &QNetworkReply::finished, this, [this, reply, order, label]() {
            reply->deleteLater();
            QList<qint64> ids;
            if (reply->error() == QNetworkReply::NoError) {
                const QJsonArray arr = QJsonDocument::fromJson(reply->readAll()).array();
                for (const QJsonValue &v : arr) {
                    const qint64 gid = qint64(v.toObject().value(QLatin1String("game_id")).toDouble());
                    if (gid > 0 && !ids.contains(gid)) ids.append(gid);
                    if (ids.size() >= 40) break;
                }
            } else {
                qDebug() << "[discover] IGDB popularity error:" << reply->errorString();
            }
            qDebug() << "[discover] IGDB trending ids:" << ids.size();
            if (ids.isEmpty()) { maybeFinish(); return; }
            fetchIgdbGamesByIds(order, label, ids);
        });
    });
}

void DiscoveryService::fetchIgdbGamesByIds(int order, const QString &label, const QList<qint64> &ids)
{
    QStringList idStrs;
    for (qint64 id : ids) idStrs << QString::number(id);

    QNetworkRequest req{QUrl(QStringLiteral("https://api.igdb.com/v4/games"))};
    setIgdbHeaders(req);
    const QByteArray body = QStringLiteral(
        "fields id,name,summary,rating,first_release_date,cover.image_id;"
        " where id = (%1) & cover != null; limit %2;")
        .arg(idStrs.join(QLatin1Char(','))).arg(ids.size()).toUtf8();

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
            "fields name,summary,rating,first_release_date,cover.image_id;"
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
    for (auto it = m_accum.constBegin(); it != m_accum.constEnd(); ++it)
        m_rows.append(it.value());

    // hero: top items that carry a backdrop + overview (TMDB trending), de-duped.
    m_hero.clear();
    QStringList seen;
    for (auto it = m_accum.constBegin(); it != m_accum.constEnd() && m_hero.size() < 6; ++it) {
        const QVariantList items = it.value().value(QStringLiteral("items")).toList();
        for (const QVariant &v : items) {
            const QVariantMap m = v.toMap();
            const QString backdrop = m.value(QStringLiteral("backdrop")).toString();
            const QString title = m.value(QStringLiteral("title")).toString();
            if (backdrop.isEmpty() || m.value(QStringLiteral("overview")).toString().isEmpty()) continue;
            if (seen.contains(title)) continue;
            seen.append(title);
            m_hero.append(m);
            if (m_hero.size() >= 6) break;
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
