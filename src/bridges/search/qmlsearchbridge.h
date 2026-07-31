// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 Mateus Cruz
// See LICENSE file for details

#ifndef QMLSEARCHBRIDGE_H
#define QMLSEARCHBRIDGE_H

#include "bridges/bridgecommon.h"

class HttpDownloadManager;

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
    // The picked title behind the current drill-down ("movie"|"series"|"game", ""
    // when the list isn't a single work) and its identity + backdrops/screenshots.
    Q_PROPERTY(QString workType READ workType NOTIFY workChanged)
    Q_PROPERTY(QString workTitle READ workTitle NOTIFY workChanged)
    Q_PROPERTY(QString workPoster READ workPoster NOTIFY workChanged)
    Q_PROPERTY(QString workYear READ workYear NOTIFY workChanged)
    Q_PROPERTY(QStringList workStills READ workStills NOTIFY workStillsChanged)
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
    QString workType() const { return m_workType; }
    QString workTitle() const { return m_workTitle; }
    QString workPoster() const { return m_workPoster; }
    QString workYear() const { return m_workYear; }
    QStringList workStills() const { return m_workStills; }
    QString getFlowType() const { return m_gwActive ? m_gwType : QString(); }
    Q_INVOKABLE void fetchWorkStills();   // lazy TMDB backdrops for the picked title

    Q_INVOKABLE void refreshSources();
    Q_INVOKABLE void search(const QString &sourceKey, const QString &query, int categoryCode = 0);
    Q_INVOKABLE void activateResult(int index, bool force = false);   // catalog→streams; else add magnet (force skips the disk-fit guard)
    Q_INVOKABLE void back();                       // streams → catalog

    // Relevance ranking (backed by the tested SearchRanker service) so the view
    // sorts by relevance without reimplementing the scoring in QML/JS.
    Q_INVOKABLE QStringList queryWords() const;                       // significant words of the active query
    Q_INVOKABLE int relevance(const QString &name, const QStringList &words) const;
    // Multi-title ranking: a drill-down is searched under the localised name AND
    // the original one, so a result may legitimately match either. Scoring only
    // against the typed query would sink every release named in the other
    // language to the bottom — found, then hidden.
    Q_INVOKABLE QVariantList queryWordSets() const;                   // one word list per searched title
    Q_INVOKABLE int relevanceMulti(const QString &name, const QVariantList &sets) const;
    Q_INVOKABLE int bestResultIndex() const { return pickBestResult(); }   // best release in the current list, or -1
    // Game list sort: >0 means a ranks above b (catalog/version/date/seeds).
    Q_INVOKABLE int compareGameReleases(const QVariantMap &a, const QVariantMap &b) const;
    // Build-version order alone, for the explicit "newest version" sort. Shares
    // GameReleasePick's parser so the list can't disagree with "Get the best".
    Q_INVOKABLE int compareBuildVersions(const QString &a, const QString &b) const;

    // Hydra-format game catalogs the user adds (neutral infra — nothing bundled).
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
    // scrolls into view — mirrors the Downloads grid's on-demand resolution.
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
    void sourcesChanged();
    void resultsChanged();
    void modeChanged();
    void searchingChanged();
    void statusChanged();
    void workChanged();
    void workStillsChanged();
    void gameSourcesChanged();
    void coverReady(const QString &infoHash, const QString &posterPath);
    void addedTorrent(const QString &infoHash);   // a magnet was added from Search
    void addWontFit(int index, const QString &name, qint64 needed, qint64 freeBytes);   // result too big for the save volume; QML confirms before force-add
    void watchSearching(const QString &title);    // Get&Watch / Get&Install: started looking
    void watchNoRelease(const QString &title);    // nothing usable found
    void prepareAndWatch(const QString &infoHash, const QString &title);   // movie → buffer & open
    void prepareAndInstall(const QString &infoHash, const QString &title); // game → download & install
    void sourceSummary(const QString &title, int count, qint64 bestSize, int maxSeeds);
    void getFlowChanged();

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
    // Pre-download trust verdict — reads the attrs fillMediaAttrs just wrote, so
    // it has to run after it (and after seedsN/sizeBytes are set).
    static void fillTrust(QVariantMap &m, const QString &name);
    // Flat aggregate search over every enabled source (the old "Tudo" behavior).
    void rawAggregateSearch(const QString &q, int categoryCode);
    // Type-scoped drill-down for a picked title: games hit game catalogs + the
    // games category; movies/series hit video torrents only (no game catalogs).
    // originalTitle: the work's name in its own language. A work is hunted under
    // BOTH names when they differ — see the .cpp for why one is never enough.
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
    QString m_workTitle, m_workPoster, m_workYear;
    int m_workTmdbId = 0;
    QStringList m_workStills;
    bool m_workStillsRequested = false;

    QVariantList m_episodeCache;      // series episode rows (videoId per row) for mode "episodes"
    QString m_epType, m_epId;         // the catalog item behind the episode list
    bool m_fromEpisodes = false;      // current streams view was entered from an episode row

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
