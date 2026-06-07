// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef DISCOVERYSERVICE_H
#define DISCOVERYSERVICE_H

#include <QObject>
#include <QVariantList>
#include <QMap>
#include <functional>

class QNetworkAccessManager;
class QNetworkRequest;

// Feeds the Discover page: TMDB trending/popular + IGDB popular, assembled into
// horizontal rows + a hero list. Posters are remote URLs (QML Image caches them);
// only the assembled lists are disk-cached (12h) so the page opens instantly.
class DiscoveryService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList rows READ rows NOTIFY rowsChanged)
    Q_PROPERTY(QVariantList hero READ hero NOTIFY heroChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
public:
    explicit DiscoveryService(QObject *parent = nullptr);

    QVariantList rows() const { return m_rows; }
    QVariantList hero() const { return m_hero; }
    bool loading() const { return m_loading; }
    QString statusText() const { return m_status; }

    Q_INVOKABLE void load();      // serve fresh cache if any, else fetch
    Q_INVOKABLE void refresh();   // force a network fetch

    // Title-first search: resolve a free-text query to real works (movies,
    // series, games) before any torrent lookup. Emits titleResults().
    Q_INVOKABLE void searchTitles(const QString &query);
    bool hasMetadataKeys() const;   // false → caller should skip the title step

signals:
    void rowsChanged();
    void heroChanged();
    void loadingChanged();
    void statusChanged();
    // works: [{ title, year, type: movie|series|game, poster, rating }]
    void titleResults(const QString &query, const QVariantList &works);

private:
    void searchTmdbTitles(const QString &query);
    void searchIgdbTitles(const QString &query);
    void maybeFinishSearch();

    void fetchTmdb(int order, const QString &path, const QString &label, const QString &type);
    void fetchIgdbTrending(int order, const QString &label);   // IGDB popularity = games of the moment
    void fetchIgdbRecent(int order, const QString &label);     // recently released games
    void fetchIgdbGamesByIds(int order, const QString &label, const QList<qint64> &ids);
    void setIgdbHeaders(QNetworkRequest &req) const;
    void ensureIgdbToken(std::function<void()> then);
    void maybeFinish();
    void assembleAndEmit();
    bool loadFromCache();
    void saveToCache();

    void setLoading(bool on);
    void setStatus(const QString &s);

    QNetworkAccessManager *m_nam;
    bool m_loading = false;
    QString m_status;
    QVariantList m_rows;
    QVariantList m_hero;

    QMap<int, QVariantMap> m_accum;   // order index → {label, type, items}
    int m_pending = 0;

    QString m_igdbToken;
    qint64 m_igdbTokenExpiry = 0;

    // title-search state (independent of the trending rows above)
    int m_searchPending = 0;
    QString m_searchQuery;
    QVariantList m_searchWorks;
};

#endif
