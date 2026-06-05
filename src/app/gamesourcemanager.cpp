// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#include "gamesourcemanager.h"

#include <QSettings>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

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
        QString magnet;
        for (const QJsonValue &u : o.value(QStringLiteral("uris")).toArray()) {
            const QString s = u.toString();
            if (s.startsWith(QStringLiteral("magnet:"), Qt::CaseInsensitive)) { magnet = s; break; }
        }
        if (magnet.isEmpty()) continue;   // no magnet → not downloadable, skip
        GameDownload g;
        g.title      = title;
        g.cleanTitle = cleanGameTitle(title);
        g.magnet     = magnet;
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
    for (const GameDownload &g : m_games) {
        if (g.title.toLower().contains(q)) {
            out.append(g);
            if (out.size() >= limit) break;
        }
    }
    return out;
}

void GameSourceManager::refresh()
{
    m_games.clear();
    m_pending = m_sources.size();
    if (m_pending == 0) { emit refreshed(0); return; }

    for (const auto &src : m_sources) {
        const QString name = src.first;
        QNetworkRequest req{QUrl(src.second)};
        req.setHeader(QNetworkRequest::UserAgentHeader, QString::fromLatin1(kBrowserUA));
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setTransferTimeout(30000);
        QNetworkReply *reply = m_net.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply, name]() {
            if (reply->error() == QNetworkReply::NoError)
                indexCatalog(name, reply->readAll());
            else
                emit sourceError(name, reply->errorString());
            reply->deleteLater();
            if (--m_pending == 0) emit refreshed(m_games.size());
        });
    }
}
