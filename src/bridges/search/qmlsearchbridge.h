// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLSEARCHBRIDGE_H
#define QMLSEARCHBRIDGE_H

#include "services/discovery/cinemeta.h"
#include "bridges/bridgecommon.h"

class HttpDownloadManager;

class QNetworkAccessManager;

class QmlSearchBridge : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList sources READ sources NOTIFY sourcesChanged)
    Q_PROPERTY(QVariantList categories READ categories CONSTANT)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString activeQuery READ activeQuery NOTIFY resultsChanged) // for client-side relevance ranking
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)        // titles|catalog|streams|torrent|games
    Q_PROPERTY(bool inStreams READ inStreams NOTIFY modeChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY modeChanged) // sources view reachable from a title/catalog
    Q_PROPERTY(bool singleTitleView READ singleTitleView NOTIFY modeChanged) // list is one picked title → drop per-row covers
    Q_PROPERTY(bool searching READ searching NOTIFY searchingChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusChanged)
    // a title search that failed instead of finding nothing: "offline" or "catalog" ("" otherwise)
    Q_PROPERTY(QString failure READ failure NOTIFY statusChanged)
    // the nearest real title to a query that found none (a typo), {} when none is close
    Q_PROPERTY(QVariantMap suggestion READ suggestion NOTIFY suggestionChanged)
    // the system's word on the connection; true when it has none to give
    Q_PROPERTY(bool online READ online NOTIFY onlineChanged)
    // The picked title behind the current drill-down ("movie"|"series"|"game", ""
    // when the list isn't a single work) and its identity + backdrops/screenshots.
    Q_PROPERTY(QString workType READ workType NOTIFY workChanged)
    Q_PROPERTY(QString workTitle READ workTitle NOTIFY workChanged)
    Q_PROPERTY(QString workPoster READ workPoster NOTIFY workChanged)
    Q_PROPERTY(QString workYear READ workYear NOTIFY workChanged)
    Q_PROPERTY(QStringList workStills READ workStills NOTIFY workStillsChanged)
    // Already in the row TMDB answered with; the header simply never read
    // them, so a drill-down showed a title and a year where it could have
    // shown the work.
    Q_PROPERTY(QString workBackdrop READ workBackdrop NOTIFY workChanged)
    Q_PROPERTY(QString workOverview READ workOverview NOTIFY workChanged)
    Q_PROPERTY(double workRating READ workRating NOTIFY workChanged)
    Q_PROPERTY(int workTmdbId READ workTmdbId NOTIFY workChanged)
    Q_PROPERTY(QString workLogo READ workLogo NOTIFY workLogoChanged)
    Q_PROPERTY(QStringList workGenres READ workGenres NOTIFY workDetailsChanged)
    Q_PROPERTY(QStringList workCast READ workCast NOTIFY workDetailsChanged)
    Q_PROPERTY(int workSeasonCount READ workSeasonCount NOTIFY workDetailsChanged)
    Q_PROPERTY(QString workMaker READ workMaker NOTIFY workDetailsChanged)   // a game's developer
    // The other films of the open film's TMDB collection, in release order:
    // empty for a film of none, and for anything that is not a film.
    Q_PROPERTY(QVariantList workCollection READ workCollection NOTIFY workCollectionChanged)
    Q_PROPERTY(QString workCollectionName READ workCollectionName NOTIFY workCollectionChanged)
    // "movie"|"series"|"game" while a Get & Watch / Install flow is active (else "").
    Q_PROPERTY(QString getFlowType READ getFlowType NOTIFY getFlowChanged)
public:
    explicit QmlSearchBridge(IEngine *session, QObject *parent = nullptr);

    QVariantList sources() const;
    QVariantList categories() const;
    QVariantList results() const;
    QString activeQuery() const;
    QString mode() const;
    bool inStreams() const;
    bool canGoBack() const;
    bool singleTitleView() const;
    bool searching() const;
    QString statusText() const;
    QString failure() const { return m_failure; }
    QVariantMap suggestion() const { return m_suggestion; }
    bool online() const;
    QString workType() const { return m_workType; }
    QString workTitle() const { return m_workTitle; }
    QString workPoster() const { return m_workPoster; }
    QString workYear() const { return m_workYear; }
    QStringList workStills() const { return m_workStills; }
    QString workBackdrop() const { return m_workBackdrop; }
    QString workOverview() const { return m_workOverview; }
    double workRating() const { return m_workRating; }
    int workTmdbId() const { return m_workTmdbId; }
    QString workLogo() const { return m_workLogo; }
    QStringList workGenres() const { return m_workGenres; }
    QStringList workCast() const { return m_workCast; }
    int workSeasonCount() const { return m_workSeasonCount; }
    QString workMaker() const { return m_workMaker; }
    QVariantList workCollection() const { return m_workCollection; }
    QString workCollectionName() const { return m_workCollectionName; }
    QString getFlowType() const { return m_gwActive ? m_gwType : QString(); }
    Q_INVOKABLE void fetchWorkStills();   // lazy TMDB backdrops, logo and details for the picked title

    Q_INVOKABLE void refreshSources();
    Q_INVOKABLE void search(const QString &sourceKey, const QString &query, int categoryCode = 0);
    Q_INVOKABLE void activateResult(int index, bool force = false);
    // The same add, then the player once enough of it is here (the Get & Watch overlay).
    Q_INVOKABLE void addAndWatch(int index, bool force = false);   // catalog→streams; else add magnet (force skips the disk-fit guard)
    // The same from an episode's sources: a pack then plays that episode's file.
    Q_INVOKABLE void addAndWatchEpisode(int index, int season, int episode);
    // Ask the stream addons (Torrentio) for one episode of the picked series and
    // add their answers to the release list, tagged with that episode.
    // airDate (TMDB's, "YYYY-MM-DD") carries the episode over to IMDb's
    // numbering, which is what the stream addons are asked in.
    Q_INVOKABLE void fetchEpisodeStreams(int season, int episode, const QString &airDate = QString());
    // The episode after the one a player is showing, when it is not in that
    // torrent: answered by nextEpisodeFound with a release from the stream
    // addons. playFoundEpisode adds it and watches it like a pick on the page.
    Q_INVOKABLE void findNextEpisode(const QString &infoHash, int fileIndex);
    // The film after tmdbId in its TMDB collection, for a player showing it.
    // Answers with nextFilmFound when there is one (released or not).
    Q_INVOKABLE void findNextFilm(const QString &infoHash, int collectionId, int tmdbId);
    Q_INVOKABLE void playFoundEpisode(const QVariantMap &next);
    Q_INVOKABLE void back();                       // streams → catalog
    // A title browse already knows (a tile, the billboard): straight to its releases.
    Q_INVOKABLE void openTitle(const QVariantMap &work);

    // Relevance ranking (backed by the tested SearchRanker service) so the view
    // sorts by relevance without reimplementing the scoring in QML/JS.
    Q_INVOKABLE QStringList queryWords() const;                       // significant words of the active query
    Q_INVOKABLE int relevance(const QString &name, const QStringList &words) const;
    // Multi-title ranking: a drill-down is searched under the localised name AND
    // the original one, so a result may legitimately match either. Scoring only
    // against the typed query would sink every release named in the other
    // language to the bottom: found, then hidden.
    Q_INVOKABLE QVariantList queryWordSets() const;                   // one word list per searched title
    Q_INVOKABLE int relevanceMulti(const QString &name, const QVariantList &sets) const;
    Q_INVOKABLE int bestResultIndex() const { return pickBestResult(); }   // best release in the current list, or -1
    // A group from the known table (FitGirl, RUNE…), not just any name tail.
    Q_INVOKABLE bool isKnownGroup(const QString &group) const;
    // Game list sort: >0 means a ranks above b (catalog/version/date/seeds).
    Q_INVOKABLE int compareGameReleases(const QVariantMap &a, const QVariantMap &b) const;
    // Build-version order alone, for the explicit "newest version" sort. Shares
    // GameReleasePick's parser so the list can't disagree with "Get the best".
    Q_INVOKABLE int compareBuildVersions(const QString &a, const QString &b) const;
    // A game's newer build in the game catalogs: {version, size, name}, {}
    // when the installed release has no version to compare or none is newer.
    Q_INVOKABLE QVariantMap newerBuild(const QString &title, const QString &installedRelease) const;

    // Hydra-format game catalogs the user adds (neutral infra: nothing bundled).
    Q_INVOKABLE QVariantList gameSources() const;          // [{name, url}]
    Q_INVOKABLE void addGameSource(const QString &name, const QString &url);
    Q_INVOKABLE void removeGameSource(const QString &url);
    Q_INVOKABLE void refreshGames();
    // Find → Games catalog browse (paginated, optional release-group tab).
    Q_INVOKABLE void ensureGamesIndexed();                 // refresh from cache if index empty
    Q_INVOKABLE void browseGames(const QString &group, int page, int pageSize = 48);
    Q_INVOKABLE int gameBrowseTotal(const QString &group) const;
    Q_INVOKABLE QVariantList gameRepackTabs() const;       // [{name, count}]

    // Lazily resolve a TMDB cover for a torrent row (by its info hash) as it
    // scrolls into view: mirrors the Downloads grid's on-demand resolution.
    void setResolver(MetadataResolver *r);
    void setHttpDownloads(HttpDownloadManager *mgr) { m_httpDownloads = mgr; }
    Q_INVOKABLE void resolveCover(int index);

    // Title-first search (default "Tudo"): resolve the query to real works via
    // DiscoveryService, then drill into a picked title's torrents.
    void setDiscovery(DiscoveryService *d);
    Q_INVOKABLE void searchRaw();   // escape hatch: flat aggregate over every source
    Q_INVOKABLE void copyMagnet(int index);   // copy a flat result's magnet to the clipboard
    Q_INVOKABLE QString magnetAt(int index) const;   // a flat result's magnet (for Real-Debrid)
    // One-click Get & Watch (movie/series) or Get & Install (game): search the
    // title, auto-pick the best release, add it, then hand the hash off.
    Q_INVOKABLE void getAndWatch(const QString &title, const QString &year, const QString &type);
    Q_INVOKABLE void cancelGetAndWatch();   // user cancelled before a release was added
    // Discover hero: one cached source lookup per featured title → emits sourceSummary.
    Q_INVOKABLE void summarizeSources(const QString &title);

signals:
    void nextEpisodeFound(const QString &infoHash, int fileIndex, const QVariantMap &next);
    void nextFilmFound(const QString &infoHash, const QVariantMap &film);
    void sourcesChanged();
    void resultsChanged();
    void modeChanged();
    void searchingChanged();
    void statusChanged();
    void workChanged();
    void workStillsChanged();
    void workLogoChanged();
    void workDetailsChanged();
    void workCollectionChanged();
    void gameSourcesChanged();
    void coverReady(const QString &infoHash, const QString &posterPath);
    void addedTorrent(const QString &infoHash);   // a magnet was added from Search
    void addWontFit(int index, const QString &name, qint64 needed, qint64 freeBytes);   // result too big for the save volume; QML confirms before force-add
    void watchSearching(const QString &title);    // Get&Watch / Get&Install: started looking
    void watchNoRelease(const QString &title);    // nothing usable found
    // movie → buffer & open. target: {fileIdx, fileName, season, episode}, the
    // file to play when the torrent holds more than one ({} = the largest video)
    void prepareAndWatch(const QString &infoHash, const QString &title, const QVariantMap &target);
    // a plain add of one episode out of a pack: download only that file
    void pickFileWhenReady(const QString &infoHash, const QVariantMap &target);
    void prepareAndInstall(const QString &infoHash, const QString &title); // game → download & install
    void sourceSummary(const QString &title, int count, qint64 bestSize, int maxSeeds);
    void getFlowChanged();
    void suggestionChanged();
    void onlineChanged();

private:
    bool fitsOnSaveVolume(qint64 needed) const;   // false ⇒ won't fit on the save disk
    void setSearching(bool on);
    void setStatus(const QString &s);
    void setMode(const QString &m);
    void runGameSearch(const QString &query);
    QSet<QString> currentResultKeys() const;
    void appendGameRows(const QList<GameDownload> &games);
    void appendTorrentRows(const QList<TorrentSearchResult> &results);
    void finishAggregateSource();
    // Copy seed/leech from indexer rows onto catalog magnets with the same
    // infohash, and drop the duplicate indexer rows. Catalog JSON has no swarm.
    void mergeCatalogSwarms();
    // When indexers didn't return the catalog hash, ask BitSearch once for the
    // active title and patch matching catalog rows.
    void requestCatalogSeedEnrichment();
    void applySeedHits(const QHash<QString, QPair<int, int>> &byHash);   // hash → (seeds, leech)
    static QString infoHashFromMagnet(const QString &magnet);
    static qint64 parseSizeToBytes(const QString &s);
    static QString detectReleaseGroup(const QString &name);
    // Parse quality/source/codec/hdr/lang tokens out of a release name for filtering.
    static void fillMediaAttrs(QVariantMap &m, const QString &name);
    // Pre-download trust verdict: reads the attrs fillMediaAttrs just wrote, so
    // it has to run after it (and after seedsN/sizeBytes are set).
    static void fillTrust(QVariantMap &m, const QString &name);
    // Flat aggregate search over every enabled source (the old "Tudo" behavior).
    void rawAggregateSearch(const QString &q, int categoryCode);
    // Type-scoped drill-down for a picked title: games hit game catalogs + the
    // games category; movies/series hit video torrents only (no game catalogs).
    // originalTitle: the work's name in its own language. A work is hunted under
    // BOTH names when they differ: see the .cpp for why one is never enough.
    void searchSourcesForWork(const QString &title, const QString &year, const QString &type,
                              const QString &originalTitle = QString());
    // the names the current drill-down was searched under (localised + original)
    QStringList m_activeTitles;

    int pickBestResult() const;       // index of the best release in m_results, or -1
    void gwResolve();                 // Get&Watch: pick + add once the search settles
    void setWorkContext(const QVariantMap &work);   // a titles-grid row (title/type/year/poster/tmdbId/stills)
    void clearWorkContext();
    void showEpisodeRows();           // m_episodeCache → m_results (mode "episodes")
    void rebuildCatalogRows();        // m_catalogCache → m_results (mode "catalog")

    QString m_workType;               // picked title's type; "" outside a drill-down
    QString m_workTitle, m_workPoster, m_workYear, m_workBackdrop, m_workOverview;
    double m_workRating = 0.0;
    int m_workTmdbId = 0;
    QStringList m_workStills;
    QString m_workLogo;
    QStringList m_workGenres;
    QStringList m_workCast;
    int m_workSeasonCount = 0;
    QString m_workMaker;
    QHash<int, QPair<QString, int>> m_nextFilmAsks;   // collectionId -> (player's hash, its film's tmdbId)
    QVariantList m_workCollection;
    QString m_workCollectionName;
    int m_workCollectionId = 0;   // the one asked for, so a late reply for a former film is dropped
    bool m_workStillsRequested = false;

    QVariantList m_episodeCache;      // series episode rows (videoId per row) for mode "episodes"
    QString m_epType, m_epId;         // the catalog item behind the episode list
    bool m_fromEpisodes = false;      // current streams view was entered from an episode row

    bool m_watchRequest = false;      // the current activateResult came from addAndWatch
    int m_watchSeason = -1;           // the episode page's episode behind a watch
    int m_watchEpisode = -1;
    QString m_workImdbId;             // the picked series' IMDb id, for stream addons
    QSet<QString> m_addonEpisodesAsked;   // "season:episode" already asked this drill-down
    QList<QPair<int, int>> m_addonEpisodesWaiting;   // asked before the IMDb id (or its episode list) arrived
    QHash<QString, QString> m_episodeAirDates;        // "season:episode" (TMDB) → air date
    QHash<QString, QList<Cinemeta::Video>> m_cinemetaVideos;   // IMDb id → IMDb's episode list
    QSet<QString> m_cinemetaLoading;
    QHash<QString, QPair<int, int>> m_imdbToPage;     // "season:episode" asked of the addons → the page's
    QNetworkAccessManager *m_cinemetaNet = nullptr;
    void fetchCinemeta(const QString &imdbId);
    // findNextEpisode's state: one ask per playing file, kept until the
    // addons are asked; then held by the IMDb video id they answer for.
    struct NextAsk {
        QString infoHash;
        int fileIndex = -1;
        int tmdbId = 0;
        QString show;
        int season = -1, episode = -1;          // the episode playing, as its file numbers it
        int nextSeason = -1, nextEpisode = -1;  // IMDb's
        QString quality, group;                 // of the release playing, to stay close to it
        bool asked = false;
    };
    QList<NextAsk> m_nextAsks;
    QHash<QString, NextAsk> m_nextByVideoId;
    QHash<int, QString> m_imdbByTmdb;
    void resumeNextAsks();
    void takeNextStreams(const QString &videoId, const QList<StreamResult> &streams);
    void appendAddonEpisodeRows(const QString &videoId, const QList<StreamResult> &streams);
    // Six addons answer one episode over a few seconds, and every resultsChanged
    // re-derives the whole page: replies landing close together share one.
    QTimer *m_addonRowsFlush = nullptr;
    // resultsChanged with the time QML's bindings took to follow it (they run
    // inside the emit), at debug level: the page's cost per batch of releases.
    void notifyResults(const char *why);
    QVariantMap fileTargetFor(const QVariantMap &row) const;
    int m_watchIdx = -1;              // a watch held across the disk-fit prompt
    bool m_gwActive = false;          // a Get&Watch search is in flight
    bool m_gwCancelled = false;       // user cancelled mid-search → don't add
    QString m_gwTitle, m_gwType;

    MetadataResolver *m_resolver = nullptr;
    DiscoveryService *m_discovery = nullptr;
    QString m_streamHintPoster;         // activated catalog item's poster, for stream rows
    QVariantList m_titleCache;          // works grid, restored when leaving the sources view
    bool m_fromTitles = false;          // sources view was opened by picking a title
    bool m_titleSources = false;        // current flat list is one picked title's torrents
    QString m_titleQuery;               // the free-text query behind the current titles
    QString m_activeQuery;              // effective query of the current flat list (for relevance)

    IEngine *m_session;
    HttpDownloadManager *m_httpDownloads = nullptr;   // set in main.cpp; direct-HTTP add target
    QString m_mode;
    QString m_savePath;
    QString m_lastQuery;
    bool m_searching = false;
    bool m_isGameSearch = false;
    bool m_aggregate = false;          // "Tudo": merge every source into one list
    int m_pendingSources = 0;          // async sources still running in aggregate
    QString m_status;
    QString m_failure;
    QVariantMap m_suggestion;
    QVariantList m_results;
    QStringList m_resultMagnets;        // magnet per flat result row (add target)
    QStringList m_resultHttp;           // direct/file-host http URL per row, "" when it's a magnet row
    QStringList m_resultTitles;         // clean game title per row (cover hint), "" for torrents
    QList<CatalogItem> m_catalogCache;
    QList<StreamResult> m_streamCache;
    QList<TorrentSearchResult> m_torrentCache;
    QHash<QString, QVariantList> m_srcSummaryCache;   // lc(title) → [count, bestSize, maxSeeds]
    QSet<QString> m_srcSummaryInFlight;               // titles being summarized (dedup requests)
    QList<GameDownload> m_gameCache;
    QString m_pendingGameQuery;
    QString m_streamHintTitle;          // parent catalog title for a Stremio stream add
    int m_streamHintType = -1;          // its ContentType (Movie/Series) as int
    QVariantList m_catalogResultsSnapshot;
    int m_seedEnrichGen = 0;              // invalidate in-flight BitSearch seed lookups
};

#endif // QMLSEARCHBRIDGE_H
