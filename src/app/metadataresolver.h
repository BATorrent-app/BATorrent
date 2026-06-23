// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef METADATARESOLVER_H
#define METADATARESOLVER_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QPair>
#include <QTimer>
#include <functional>
#include "nameparser.h"

class QNetworkAccessManager;

struct MetadataResult {
    QString title;
    QString posterPath;
    QString description;
    double rating = 0.0;
    int year = 0;
    QStringList genres;
    QStringList platforms;
    ContentType contentType = ContentType::Unknown;
    int tmdbId = 0;          // TMDB movie/series id (0 if unknown) — for episode lookups
    bool valid = false;
};

class MetadataResolver : public QObject
{
    Q_OBJECT
public:
    explicit MetadataResolver(QObject *parent = nullptr);

    // fileNames (optional): the torrent's file list. The payload is the strongest
    // type signal, so it settles game-vs-movie when the name is ambiguous.
    void resolve(const QString &infoHash, const QString &torrentName,
                 const QStringList &fileNames = QStringList());
    MetadataResult cached(const QString &infoHash) const;
    bool hasCached(const QString &infoHash) const;
    void batchResolve(const QStringList &infoHashes, const QStringList &torrentNames);
    // Manual override when the auto match was wrong: re-link the cover/title
    // with a user-supplied query + explicit type, or clear it to no cover.
    void resolveManual(const QString &infoHash, const QString &query, ContentType type);
    void clearMetadata(const QString &infoHash);

signals:
    void metadataReady(const QString &infoHash, const MetadataResult &result);

private:
    void processQueue();
    void queryTmdbMovie(const QString &infoHash, const ParsedName &parsed);
    void queryTmdbTv(const QString &infoHash, const ParsedName &parsed);
    void queryIgdb(const QString &infoHash, const ParsedName &parsed);
    void ensureIgdbToken();
    // Re-fetch the English overview when the localized TMDB result had none,
    // then continue via `done`. kind = "movie" | "tv".
    void fetchTmdbOverviewEn(const QString &kind, int id, MetadataResult result,
                             std::function<void(MetadataResult)> done);
    void downloadPoster(const QString &infoHash, const QString &url, const MetadataResult &partial);
    void loadFromDisk(const QString &infoHash);
    void saveToDisk(const QString &infoHash, const MetadataResult &result);
    QString cacheDir() const;
    QString posterDir() const;

    QNetworkAccessManager *m_nam;
    QHash<QString, MetadataResult> m_cache;
    QQueue<QPair<QString, ParsedName>> m_queue;
    QTimer m_rateLimiter;
    bool m_requestInFlight = false;

    QString m_igdbAccessToken;
    qint64 m_igdbTokenExpiry = 0;
    QQueue<QPair<QString, ParsedName>> m_igdbPending;
};

#endif
