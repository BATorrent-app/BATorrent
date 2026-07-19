// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "services/discovery/gamesourcemanager.h"

#include <QSettings>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCryptographicHash>
#include <QDateTime>

static constexpr qint64 kCacheTtlSecs = 12 * 60 * 60;   // catalogs change ~daily

// Several popular catalogs (FitGirl, DODI, …) 403 a non-browser User-Agent —
// this is the wall that makes the feature look "impossible" without it.
static const char *kBrowserUA =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";

GameSourceManager &GameSourceManager::instance()
{
    static GameSourceManager m;
    return m;
}

GameSourceManager::GameSourceManager() : QObject(nullptr)
{
    loadSources();
}

void GameSourceManager::loadSources()
{
    QSettings s;
    const int n = s.beginReadArray(QStringLiteral("gameSources"));
    for (int i = 0; i < n; ++i) {
        s.setArrayIndex(i);
        m_sources.append({s.value("name").toString(), s.value("url").toString()});
    }
    s.endArray();
}

void GameSourceManager::saveSources()
{
    QSettings s;
    s.beginWriteArray(QStringLiteral("gameSources"));
    for (int i = 0; i < m_sources.size(); ++i) {
        s.setArrayIndex(i);
        s.setValue("name", m_sources[i].first);
        s.setValue("url", m_sources[i].second);
    }
    s.endArray();
}

void GameSourceManager::addSource(const QString &name, const QString &url)
{
    if (url.isEmpty()) return;
    for (const auto &s : m_sources) if (s.second == url) return;   // de-dup by URL
    m_sources.append({name.isEmpty() ? url : name, url});
    saveSources();
}

void GameSourceManager::removeSource(const QString &url)
{
    for (int i = 0; i < m_sources.size(); ++i)
        if (m_sources[i].second == url) { m_sources.removeAt(i); saveSources(); return; }
}

QList<QPair<QString, QString>> GameSourceManager::sources() const { return m_sources; }

QString GameSourceManager::cleanGameTitle(const QString &title)
{
    QString out = title;
    // Strip leading bracketed tags first (RuTracker-style: "[DL] ", "[Антология] ").
    static const QRegularExpression lead(QStringLiteral("^\\s*(?:[\\[(][^\\])]*[\\])]\\s*)+"));
    out.remove(lead);
    // Then cut at the first repacker / version / language / year marker, e.g.
    //   "Cyberpunk 2077 v2.1 [FitGirl Repack] (MULTi)" -> "Cyberpunk 2077"
    static const QRegularExpression cut(
        QStringLiteral("[\\(\\[]|\\bv\\d|\\bRepack\\b|\\bUpdate\\b|\\bMULTi|\\bGOG\\b|\\bRUNE\\b"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = cut.match(out);
    if (m.hasMatch()) out.truncate(m.capturedStart());
    out.remove(QRegularExpression(QStringLiteral("[\\s\\-.]+$")));   // collapse trailing separators
    out = out.trimmed();
    return out.isEmpty() ? title.trimmed() : out;   // never return empty
}

int GameSourceManager::indexCatalog(const QString &sourceName, const QByteArray &json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) return 0;
    const QJsonArray downloads = doc.object().value(QStringLiteral("downloads")).toArray();
    int added = 0;
    for (const QJsonValue &v : downloads) {
        const QJsonObject o = v.toObject();
        const QString title = o.value(QStringLiteral("title")).toString();
        if (title.isEmpty()) continue;
        // Prefer a magnet (a swarm beats a single host); keep the first http(s)
        // link as a fallback so file-host-only entries are downloadable too.
        QString magnet, httpUrl;
        for (const QJsonValue &u : o.value(QStringLiteral("uris")).toArray()) {
            const QString s = u.toString();
            if (s.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) { magnet = s; break; }
            if (httpUrl.isEmpty()
                && (s.startsWith(QStringLiteral("http://"), Qt::CaseInsensitive)
                    || s.startsWith(QStringLiteral("https://"), Qt::CaseInsensitive)))
                httpUrl = s;
        }
        if (magnet.isEmpty() && httpUrl.isEmpty()) continue;   // nothing fetchable
        GameDownload g;
        g.title      = title;
        g.cleanTitle = cleanGameTitle(title);
        g.magnet     = magnet;
        g.httpUrl    = httpUrl;
        g.fileSize   = o.value(QStringLiteral("fileSize")).toString();
        g.uploadDate = o.value(QStringLiteral("uploadDate")).toString();
        g.source     = sourceName;
        m_games.append(g);
        ++added;
    }
    return added;
}

QList<GameDownload> GameSourceManager::search(const QString &query, int limit) const
{
    QList<GameDownload> out;
    const QString q = query.trimmed().toLower();
    if (q.isEmpty()) return out;
    // Match all whitespace-separated tokens (any order) against the raw title, so
    // "god of war fitgirl" hits "God of War [FitGirl Repack]" and typing just
    // "fitgirl" returns every FitGirl entry.
    const auto tokens = q.split(QChar(' '), Qt::SkipEmptyParts);
    for (const GameDownload &g : m_games) {
        const QString hay = g.title.toLower();
        bool all = true;
        for (const QString &t : tokens)
            if (!hay.contains(t)) { all = false; break; }
        if (all) {
            out.append(g);
            if (out.size() >= limit) break;
        }
    }
    return out;
}

QString GameSourceManager::cachePath(const QString &url)
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                        + QStringLiteral("/gamecatalogs");
    const QString key = QString::fromLatin1(
        QCryptographicHash::hash(url.toUtf8(), QCryptographicHash::Md5).toHex());
    return dir + QLatin1Char('/') + key + QStringLiteral(".json");
}

QByteArray GameSourceManager::readCache(const QString &url, bool freshOnly)
{
    QFileInfo fi(cachePath(url));
    if (!fi.exists()) return {};
    if (freshOnly && fi.lastModified().secsTo(QDateTime::currentDateTime()) > kCacheTtlSecs) return {};
    QFile f(fi.filePath());
    if (!f.open(QIODevice::ReadOnly)) return {};
    return f.readAll();
}

void GameSourceManager::writeCache(const QString &url, const QByteArray &data)
{
    const QString path = cachePath(url);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) f.write(data);
}

void GameSourceManager::fetchSource(const QString &name, const QString &url)
{
    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kBrowserUA));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(30000);
    QNetworkReply *reply = m_net.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, name, url]() {
        if (reply->error() == QNetworkReply::NoError) {
            const QByteArray body = reply->readAll();
            if (indexCatalog(name, body) > 0) writeCache(url, body);
        } else {
            const QByteArray stale = readCache(url, false);   // network down → use stale if any
            if (!stale.isEmpty()) indexCatalog(name, stale);
            else emit sourceError(name, reply->errorString());
        }
        reply->deleteLater();
        if (--m_pending == 0) emit refreshed(m_games.size());
    });
}

void GameSourceManager::refresh(bool forceNetwork)
{
    m_games.clear();
    QList<QPair<QString, QString>> toFetch;
    for (const auto &src : m_sources) {
        const QByteArray cached = forceNetwork ? QByteArray() : readCache(src.second, true);
        if (!cached.isEmpty()) indexCatalog(src.first, cached);
        else toFetch.append(src);
    }

    m_pending = toFetch.size();
    if (m_pending == 0) { emit refreshed(m_games.size()); return; }
    for (const auto &src : toFetch) fetchSource(src.first, src.second);
}
