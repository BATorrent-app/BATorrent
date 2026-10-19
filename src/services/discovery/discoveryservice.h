// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef DISCOVERYSERVICE_H
#define DISCOVERYSERVICE_H

#include <QObject>
#include <QVariantList>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QPair>
#include <QVariantMap>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;
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
    // The content language changed: titles, overviews, logos and episode names
    // are all per language, so drop what was fetched and fetch it again.
    // Pages showing a title hear languageReloaded and ask for theirs.
    Q_INVOKABLE void reloadForLanguage();

    // Title-first search: resolve a free-text query to real works (movies,
    // series, games) before any torrent lookup. Emits titleResults().
    Q_INVOKABLE void searchTitles(const QString &query);
    // Why the last title search came back empty, when it failed rather than
    // found nothing: no internet, or the catalogs not answering.
    enum SearchFailure { NoFailure = 0, Offline = 1, CatalogDown = 2 };
    SearchFailure lastSearchFailure() const { return m_searchFailure; }
    // A query that found nothing, likely misspelt: look for the nearest real
    // title (answers titleSuggestion, with an empty map when none is close).
    Q_INVOKABLE void suggestTitle(const QString &query);
    bool hasMetadataKeys() const;   // false → caller should skip the title step

    // On-demand YouTube trailer key for a TMDB title (list endpoints don't carry
    // videos, so the Discover hero fetches it lazily). Emits trailerReady.
    Q_INVOKABLE void fetchTrailer(int tmdbId, const QString &type);
    // "Because you watched X": TMDB recommendations for a seed title. Emits recommendationsReady.
    Q_INVOKABLE void fetchRecommendations(int tmdbId, const QString &type);
    // "Because you played X": IGDB similar games for a seed game (by name).
    Q_INVOKABLE void fetchGameRecommendations(const QString &gameName);
    // Art, stills, maker and Steam appid for an owned game, by name. Cached per
    // session. Emits gameDetailsReady ({} when IGDB has no match).
    // first: ahead of the queue (a page's banner, before the tiles under it).
    Q_INVOKABLE void fetchGameDetails(const QString &gameName, bool first = false);
    // Episode list (number, name, air date) for a TV season. Emits episodesReady.
    Q_INVOKABLE void fetchEpisodes(int tmdbId, int season);
    // Backdrop/still URLs for a title (text-free ones first). Emits backdropsReady.
    Q_INVOKABLE void fetchBackdrops(int tmdbId, const QString &type);
    // Genres, billed cast and season count for a picked title. Emits workDetailsReady.
    Q_INVOKABLE void fetchWorkDetails(int tmdbId, const QString &type);
    // The title's lettering for a browse tile. Cached per session, one request
    // per title however many tiles ask. Emits titleLogoReady ("" = none), and
    // titleBackdropReady with the first textless backdrop from the same reply.
    Q_INVOKABLE void fetchLogo(int tmdbId, const QString &type);
    // Whether a logo is dark enough to vanish on dark art. Cached per url.
    // Emits logoToneReady.
    Q_INVOKABLE void fetchLogoTone(const QString &url);

    // Hub library helpers (pure; also covered by test_hublogic).
    Q_INVOKABLE QString genreKey(const QString &name) const;
    Q_INVOKABLE QString topGenreFromNames(const QStringList &genreNames) const;
    Q_INVOKABLE QVariantList applyLibraryView(const QVariantList &list,
                                             const QString &search,
                                             const QString &sort) const;
    Q_INVOKABLE QVariantList excludeOwned(const QVariantList &candidates,
                                         const QStringList &ownedTitles,
                                         int limit = 12) const;

signals:
    void languageReloaded();
    void rowsChanged();
    void heroChanged();
    void loadingChanged();
    void statusChanged();
    // works: [{ title, year, type: movie|series|game, poster, rating }]
    void titleResults(const QString &query, const QVariantList &works);
    void titleSuggestion(const QString &query, const QVariantMap &work);
    void trailerReady(int tmdbId, const QString &youtubeKey);   // "" if none
    void recommendationsReady(int tmdbId, const QVariantList &items);
    void gameRecommendationsReady(const QString &gameName, const QVariantList &items);
    void gameDetailsReady(const QString &gameName, const QVariantMap &details);
    void episodesReady(int tmdbId, int season, const QVariantList &episodes);   // [{episode,name,air_date}]
    void backdropsReady(int tmdbId, const QStringList &urls);
    void logoReady(int tmdbId, const QString &url);   // the title's own lettering, same reply
    void workDetailsReady(int tmdbId, const QVariantMap &details);   // {genres, cast, seasons, imdbId}
    void seasonCastReady(int tmdbId, int season, const QStringList &cast);
    void titleLogoReady(int tmdbId, const QString &type, const QString &url);
    void titleBackdropReady(int tmdbId, const QString &type, const QString &url);
    void logoToneReady(const QString &url, bool dark);

private:
    void searchTmdbTitles(const QString &query);
    void searchIgdbTitles(const QString &query);
    void maybeFinishSearch();

    QNetworkReply *tmdbGet(const QString &path, const QList<QPair<QString, QString>> &extra = {},
                           int timeoutMs = 10000);
    void fetchTmdb(int order, const QString &path, const QString &label, const QString &type,
                   const QList<QPair<QString, QString>> &extra = {}, int page = 1);
    void fetchIgdbTrending(int order, const QString &label);   // hot + recent games (hype-typed)
    void fetchIgdbHypeIds(int order, const QString &label);    // popularity primitives for the hype type
    void fetchIgdbRecent(int order, const QString &label);     // recently released games
    void fetchIgdbGames(int order, const QString &label, const QString &whereClause, const QString &sort);  // generic games shelf
    void fetchIgdbGamesByIds(int order, const QString &label, const QList<qint64> &ids);
    void setIgdbHeaders(QNetworkRequest &req) const;
    void ensureIgdbToken(std::function<void()> then);
    void maybeFinish();
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
    int m_hypeTypeId = 0;   // IGDB "Want to Play" popularity_type, resolved once per session

    // title-search state (independent of the trending rows above)
    int m_searchPending = 0;
    QString m_searchQuery;
    QVariantList m_searchWorks;
    SearchFailure m_searchFailure = NoFailure;
    void noteSearchError(int networkError);

    QHash<QString, QString> m_logoCache;   // "type:id" → logo url, "" when the title has none
    QHash<QString, QString> m_backdropCache;   // same key → first textless backdrop, "" when none
    QHash<QString, bool> m_logoTone;            // logo url → dark
    QSet<QString> m_logoTonePending;
    QSet<QString> m_logoPending;
    QHash<QString, QVariantMap> m_gameDetails;   // game name → IGDB details, {} when unmatched
    QHash<QString, qint64> m_gameDetailsFailedAt;   // name → when IGDB last couldn't be reached
    QStringList m_gameDetailsQueue;   // names waiting their turn: IGDB allows ~4 requests/s
    bool m_gameDetailsBusy = false;
    void runGameDetailsQueue();
};

#endif
