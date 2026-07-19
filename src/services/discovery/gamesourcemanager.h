// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef GAMESOURCEMANAGER_H
#define GAMESOURCEMANAGER_H

// Consumes Hydra-Launcher-format game "sources": JSON catalogs of
//   { "name": "...", "downloads": [ { "title", "uris" (magnets), "fileSize", "uploadDate" } ] }
// The app never bundles the pirated content — the user adds a source URL, we
// fetch + cache the catalog and search it locally. There is no game-search API;
// these community JSON catalogs are the de-facto standard.

#include <QObject>
#include <QString>
#include <QList>
#include <QPair>
#include <QNetworkAccessManager>

struct GameDownload {
    QString title;        // raw catalog title (with repacker/version tags)
    QString cleanTitle;   // stripped for IGDB cover matching
    QString magnet;       // first magnet: URI ("" if the entry is http-only)
    QString httpUrl;      // first direct/file-host http(s) URI (fallback when no magnet)
    QString fileSize;     // e.g. "14.97 GB"
    QString uploadDate;   // ISO-8601
    QString source;       // catalog the entry came from
};

class GameSourceManager : public QObject
{
    Q_OBJECT
public:
    static GameSourceManager &instance();

    void addSource(const QString &name, const QString &url);
    void removeSource(const QString &url);
    QList<QPair<QString, QString>> sources() const;   // (name, url)

    void refresh(bool forceNetwork = false);          // re-index; uses disk cache unless forced
    QList<GameDownload> search(const QString &query, int limit = 100) const;
    int gameCount() const { return m_games.size(); }

    // Parse one catalog into the index; returns how many entries were added.
    // Exposed (and pure) so it can be unit-tested without the network.
    int indexCatalog(const QString &sourceName, const QByteArray &json);

    // Strip repacker / version / language tags so the title matches IGDB.
    static QString cleanGameTitle(const QString &title);

signals:
    void refreshed(int gameCount);
    void sourceError(const QString &name, const QString &error);

private:
    GameSourceManager();
    void loadSources();
    void saveSources();
    void fetchSource(const QString &name, const QString &url);
    static QString cachePath(const QString &url);
    static QByteArray readCache(const QString &url, bool freshOnly);   // freshOnly → honor TTL
    static void writeCache(const QString &url, const QByteArray &data);

    QList<GameDownload> m_games;
    QList<QPair<QString, QString>> m_sources;
    QNetworkAccessManager m_net;
    int m_pending = 0;
};

#endif // GAMESOURCEMANAGER_H
